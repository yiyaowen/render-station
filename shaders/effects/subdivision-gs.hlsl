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

/*               v0 (0)
 *                *
 *               / \
 *              /   \
 *      m2 (2) *-----* m0 (1)
 *            / \   / \
 *           /   \ /   \
 *   v2 (4) *-----*-----* v1 (5)
 *               m1 (3)
*/

// Also see 'subdivide' function defined in geometry-utils.cpp.
void Subdivide(VertexOut inVertices[3], out VertexOut outVertices[6])
{
	VertexOut m[3];

	m[0].posL = 0.5f * (inVertices[0].posL + inVertices[1].posL);
	m[1].posL = 0.5f * (inVertices[1].posL + inVertices[2].posL);
	m[2].posL = 0.5f * (inVertices[2].posL + inVertices[0].posL);

	m[0].normalL = normalize(0.5f * (inVertices[0].normalL + inVertices[1].normalL));
	m[1].normalL = normalize(0.5f * (inVertices[1].normalL + inVertices[2].normalL));
	m[2].normalL = normalize(0.5f * (inVertices[2].normalL + inVertices[0].normalL));

	m[0].uv = 0.5f * (inVertices[0].uv + inVertices[1].uv);
	m[1].uv = 0.5f * (inVertices[1].uv + inVertices[2].uv);
	m[2].uv = 0.5f * (inVertices[2].uv + inVertices[0].uv);

	// The order of output vertices is deliberately arranged to adapt to OutputSubdivision function.
	outVertices[0] = inVertices[0];
	outVertices[1] = m[0];
	outVertices[2] = m[2];
	outVertices[3] = m[1];
	outVertices[4] = inVertices[2];
	outVertices[5] = inVertices[1];
}

void OutputSubdivision(VertexOut v[6], inout TriangleStream<GeoOut> triStream)
{
	GeoOut gout[6];

	// Firstly, finish the work in VS originally.
	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		float4 posW = mul(mul(float4(v[i].posL, 1.0f), gWorld), gReflectTrans);
		gout[i].posH = mul(mul(posW, gView), gProj);
		gout[i].posW = posW.xyz;
		gout[i].normalW = mul(mul(v[i].normalL, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
		float4 uv = mul(float4(v[i].uv, 0.0f, 1.0f), gTexTrans);
		gout[i].uv = mul(uv, gMatTrans).xy;
	}

/*               v0 (0)
*                *
*               / \
*              /   \
*      m2 (2) *-----* m0 (1)
*            / \   / \
*           /   \ /   \
*   v2 (4) *-----*-----* v1 (5)
*               m1 (3)
*/

	// See the order of vertices in the subdivided triangle above.
	// Note we called RestartStrip out of FOR loop instead of in FOR loop. The first 5 vertices
	// are handled as a triangle strip, while the last 3 vertices are handled as a triangle list.

	// Triangles at left-top.
	[unroll]
	for (int j = 0; j < 5; ++j)
	{
		triStream.Append(gout[j]);
	}
	triStream.RestartStrip();
	// Triangle at right-bottom.
	triStream.Append(gout[1]);
	triStream.Append(gout[5]);
	triStream.Append(gout[3]);
	triStream.RestartStrip();
}

[maxvertexcount(8)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream)
{
	VertexOut v[6];
	Subdivide(gin, v);
	OutputSubdivision(v, triStream);
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