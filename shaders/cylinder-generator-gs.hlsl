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

/*     v2 (2)     v3 (0)
*       *---------*
*       |\        |
*       | \       |  (*) point from screen to you
*       |  \      |  (x) point from you to screen
*       |   \     |
*       |	 \    |
*       |     \   |         /|\ z
*       |      \  |          |
*       |       \ |          |
*       |        \|   (*) y  *-----> x
*       *---------*
*      v0 (3)     v1 (1)
*/

// Expand one line to generate one side face (rectangle, i.e. 2 triangles) of cylinder.
[maxvertexcount(4)]
void GS(line VertexOut gin[2], inout TriangleStream<GeoOut> triStream)
{
	VertexOut gtmp[4];
	GeoOut gout[4];

	// Firstly, expand line to rectangle.
	gtmp[0] = gin[0];
	gtmp[0].uv = float2(0.0f, 1.0f);

	gtmp[1] = gin[1];
	gtmp[1].uv = float2(1.0f, 1.0f);

	// Note the normal (positive direction) may not be along with Z-axis.
	// However, it is assumed as so for the sake of convenience and simplification.
	gtmp[2] = gin[0];
	gtmp[2].posL.z += 2.0f; // Default height 2
	gtmp[2].uv = float2(0.0f, 0.0f);

	gtmp[3] = gin[1];
	gtmp[3].posL.z += 2.0f; // Default height 2
	gtmp[3].uv = float2(1.0f, 0.0f);

	// Secondly, finish the work in VS originally.
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		float4 posW = mul(mul(float4(gtmp[i].posL, 1.0f), gWorld), gReflectTrans);
		gout[i].posH = mul(mul(posW, gView), gProj);
		gout[i].posW = posW.xyz;
		gout[i].normalW = mul(mul(gtmp[i].normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
		float4 uv = mul(float4(gtmp[i].uv, 0.0f, 1.0f), gTexTrans);
		gout[i].uv = mul(uv, gMatTrans).xy;
	}

	triStream.Append(gout[3]);
	triStream.Append(gout[1]);
	triStream.Append(gout[2]);
	triStream.Append(gout[0]);
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