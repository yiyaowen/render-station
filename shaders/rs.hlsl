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
};

cbuffer cbGlobalProc : register(b1)
{
	float4x4 gView;
	float4x4 gProj;

	float3 gEyePosW;
	float _placeholder;

	float4 gAmbientLight;
	Light gLights[MAX_LIGHTS];
}

cbuffer cbMaterial : register(b2)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
}

struct VertexIn
{
	float3 posL  : POSITION;
	float4 color : COLOR;
	float3 normalL : NORMAL;
};

struct VertexOut
{
	float4 posH  : SV_POSITION;
	float4 color : COLOR;
	float3 posW : POSITION;
	float3 normalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	float4 posW = mul(float4(vin.posL, 1.0f), gWorld);
	vout.posH = mul(mul(posW, gView), gProj);
	vout.color = vin.color;
	vout.posW = posW.xyz;
	vout.normalW = mul(vin.normalL, (float3x3)gInvTrWorld);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	pin.normalW = normalize(pin.normalW);
	float3 eyeVecW = normalize(gEyePosW - pin.posW);

	float4 ambient = gAmbientLight * gDiffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
	float4 litColor = { calcAllLights(gLights, pin.posW, pin.normalW, eyeVecW, mat), 0.0f };
	litColor += ambient;
	// It is a common means to get the alpha value from diffuse albedo.
	litColor.a = gDiffuseAlbedo.a;

	return litColor;
}