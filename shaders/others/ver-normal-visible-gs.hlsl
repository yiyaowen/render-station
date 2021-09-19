/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

// This file is very similar to rs.hlsl (default shader). An important difference
// is that a geometry shader is configured to achieve some interesting effects.
// Note the definitions of some structs are changed to serve the geometry shader.

#include "../basic/light-utils.hlsl"

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

// Handle every vertex separately and expand them to visible normal lines.
[maxvertexcount(2)]
void GS(point VertexOut gin[1], inout LineStream<GeoOut> lineStream)
{
	VertexOut gtmp[2];
	GeoOut gout[2];

	// Firstly, expand single vertex to visible normal line.
	gtmp[0] = gin[0];

	gtmp[1] = gin[0];
	gtmp[1].posL += gtmp[1].normalL; // Unit length

	// Secondly, finish the work in VS originally.
	[unroll]
	for (int i = 0; i < 2; ++i)
	{
		float4 posW = mul(mul(float4(gtmp[i].posL, 1.0f), gWorld), gReflectTrans);
		gout[i].posH = mul(mul(posW, gView), gProj);
		gout[i].posW = posW.xyz;
		gout[i].normalW = mul(mul(gtmp[i].normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
		float4 uv = mul(float4(gtmp[i].uv, 0.0f, 1.0f), gTexTrans);
		gout[i].uv = mul(uv, gMatTrans).xy;
	}

	lineStream.Append(gout[0]);
	lineStream.Append(gout[1]);
	lineStream.RestartStrip();
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
	float4 litColor = { calcAllLightsPhysicsBased(gLights, pin.posW, pin.normalW, eyeVecW, mat), 0.0f };
	litColor += ambient;

	// Simulate the effect of fog.
	float fogAmount = saturate((distToEye - gFogFallOffStart) / (gFogFallOffEnd - gFogFallOffStart));
	litColor = lerp(litColor, gFogColor, fogAmount);

	// It is a common means to get the alpha value from diffuse albedo.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}