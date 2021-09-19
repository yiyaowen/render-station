/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 posL  : POSITION;
	float4 color : COLOR;
};

struct VertexOut
{
	float4 posH  : SV_POSITION;
	float4 color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.posH = mul(float4(vin.posL, 1.0f), gWorldViewProj);

	// Just pass vertex color into the pixel shader.
	vout.color = vin.color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.color;
}