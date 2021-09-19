/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

// This file is very similar to rs.hlsl (default shader). An important difference
// is that a geometry shader is configured to achieve some interesting effects.
// Note the definitions of some structs are changed to serve the geometry shader.

#include "light-utils.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gInvTrWorld;
	float4x4 gTexTrans;
};

cbuffer cbGlobalProc : register(b1)
{
	float4x4 gView;
	float4x4 gProj;

	float3 gEyePosW;
	float gElapsedSecs;

	float4 gAmbientLight;
	Light gLights[MAX_LIGHTS];

	float4 gFogColor;
	float gFogFallOffStart;
	float gFogFallOffEnd;

	float4x4 gReflectTrans;
	float4x4 gInvTrReflectTrans;
}

cbuffer cbMaterial : register(b2)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMatTrans;
}

Texture2D gDiffuseMap : register(t0);

// gsam: global sampler
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct VertexIn
{
	float3 posL  : POSITION;
	float3 normalL : NORMAL;
	float2 uv : TEXCOORD;
	float2 size : SIZE;
};

// VertexOut is exactly the same with VertexIn. The works originally done by VS
// are moved into GS. (transform vertices from local-coordinate to world-coordinate etc.)
struct VertexOut
{
	float3 posL  : POSITION;
	float3 normalL : NORMAL;
	float2 uv : TEXCOORD;
	float2 size : SIZE;
};

struct GeoOut
{
	float4 posH  : SV_POSITION;
	float3 posW : POSITION;
	float3 normalW : NORMAL;
	float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.posL = vin.posL;
	vout.normalL = vin.normalL;
	vout.uv = vin.uv;
	vout.size = vin.size;

	return vout;
}

// To realize a simple explosion animation, we move each triangle along its normal over time.
[maxvertexcount(3)]
void GS(triangle VertexOut gin[3], uint primID : SV_PrimitiveID, inout TriangleStream<GeoOut> triStream)
{
	GeoOut gout[3];

	// Firstly, move each triangle along its normal over time.
	[unroll]
	for (int i = 0; i < 3; ++i)
	{
		// We simply give it a hard coded velocity factor here.
		gin[i].posL = gin[i].posL + gin[i].normalL * gElapsedSecs * (primID % 3 + 1) * 7.0f;
	}

	// Secondly, finish the work in VS originally.
	[unroll]
	for (int j = 0; j < 3; ++j)
	{
		float4 posW = mul(mul(float4(gin[j].posL, 1.0f), gWorld), gReflectTrans);
		gout[j].posH = mul(mul(posW, gView), gProj);
		gout[j].posW = posW.xyz;
		gout[j].normalW = mul(mul(gin[j].normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
		float4 uv = mul(float4(gin[j].uv, 0.0f, 1.0f), gTexTrans);
		gout[j].uv = mul(uv, gMatTrans).xy;
	}

	[unroll]
	for (int k = 0; k < 3; ++k)
	{
		triStream.Append(gout[k]);
	}
	triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
	pin.normalW = normalize(pin.normalW);
	float3 eyeVecW = gEyePosW - pin.posW;
	float distToEye = length(eyeVecW);
	eyeVecW /= distToEye;
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap.Sample(gsamAnisotropicWrap, pin.uv);

	clip(diffuseAlbedo.a - 0.1f);

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	float4 litColor = { calcAllLights(gLights, pin.posW, pin.normalW, eyeVecW, mat), 0.0f };
	litColor += ambient;

	// Simulate the effect of fog.
	float fogAmount = saturate((distToEye - gFogFallOffStart) / (gFogFallOffEnd - gFogFallOffStart));
	litColor = lerp(litColor, gFogColor, fogAmount);

	// It is a common means to get the alpha value from diffuse albedo.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}