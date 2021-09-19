/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
};

cbuffer cbGlobalProc : register(b1)
{
	float4x4 gView;
	float4x4 gProj;
}

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
	float4 posW = mul(float4(vin.posL, 1.0f), gWorld);
	vout.posH = mul(mul(posW, gView), gProj);

	// Just pass vertex color into the pixel shader.
	vout.color = vin.color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.color;
}