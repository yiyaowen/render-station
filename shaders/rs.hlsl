/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

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
};

struct VertexOut
{
	float4 posH  : SV_POSITION;
	float3 posW : POSITION;
	float3 normalW : NORMAL;
	float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	float4 posW = mul(mul(float4(vin.posL, 1.0f), gWorld), gReflectTrans);
	vout.posH = mul(mul(posW, gView), gProj);
	vout.posW = posW.xyz;
	vout.normalW = mul(mul(vin.normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
	float4 uv = mul(float4(vin.uv, 0.0f, 1.0f), gTexTrans);
	vout.uv = mul(uv, gMatTrans).xy;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
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