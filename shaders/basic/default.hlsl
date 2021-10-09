/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

// This is the default shader for render station program.

#include "light-utils.hlsl"

#define SCENE_MATERIAL_COUNT 14

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gInvTrWorld;
	float4x4 gTexTrans;
	
	int gHasDisplacementMap;
	int gHasNormalMap;

	uint gMaterialIndex;
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

struct MaterialData
{
	float4 diffuseAlbedo;
	float3 fresnelR0;
	float roughness;
	float4x4 matTrans;
	uint diffuseMapIndex;
};

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

Texture2D gDiffuseMap[SCENE_MATERIAL_COUNT]: register(t0);

Texture2D gDisplacementMap : register(t0, space2);

Texture2D gNormalMap : register(t0, space3);

// gsam: global sampler
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct VertexIn
{
	float3 posL : POSITION;
	float3 normalL : NORMAL;
	float2 uv : TEXCOORD;
	float2 size : SIZE;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float3 posW : POSITION;
	float3 normalW : NORMAL;
	float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
	// Get material data.
	MaterialData matData = gMaterialData[gMaterialIndex];

	// Apply displacement and normal map (If has).
	if (gHasDisplacementMap == 1)
	{
		vin.posL += gDisplacementMap.SampleLevel(gsamLinearWrap, vin.uv, 1.0f);
	}
	if (gHasNormalMap == 1)
	{
		vin.normalL = gNormalMap.SampleLevel(gsamLinearWrap, vin.uv, 1.0f);
	}

	// General VS works.
	float4 posW = mul(mul(float4(vin.posL, 1.0f), gWorld), gReflectTrans);
	vout.posH = mul(mul(posW, gView), gProj);
	vout.posW = posW.xyz;
	vout.normalW = mul(mul(vin.normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
	float4 uv = mul(float4(vin.uv, 0.0f, 1.0f), gTexTrans);
	vout.uv = mul(uv, matData.matTrans).xy;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Get material data.
	MaterialData matData = gMaterialData[gMaterialIndex];

	pin.normalW = normalize(pin.normalW);
	float3 eyeVecW = gEyePosW - pin.posW;
	float distToEye = length(eyeVecW);
	eyeVecW /= distToEye;
	float4 diffuseAlbedo = matData.diffuseAlbedo *
		gDiffuseMap[matData.diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.uv);

	clip(diffuseAlbedo.a - 0.1f);

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.0f - matData.roughness;
	Material mat = { diffuseAlbedo, matData.fresnelR0, shininess };
	float4 litColor = { calcAllLightsPhysicsBased(gLights, pin.posW, pin.normalW, eyeVecW, mat), 0.0f };
	litColor += ambient;

	// Simulate the effect of fog.
	float fogAmount = saturate((distToEye - gFogFallOffStart) / (gFogFallOffEnd - gFogFallOffStart));
	litColor = lerp(litColor, gFogColor, fogAmount);

	// It is a common means to get the alpha value from diffuse albedo.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}