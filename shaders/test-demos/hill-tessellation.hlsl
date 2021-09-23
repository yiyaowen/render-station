/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "../basic/light-utils.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gInvTrWorld;
	float4x4 gTexTrans;
	
	int gHasDisplacementMap;
	int gHasNormalMap;
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

Texture2D gDisplacementMap : register(t1);

Texture2D gNormalMap : register(t2);

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

// Make VertexOut the same with VertexIn to use VS as a pass-through shader.
struct VertexOut
{
	float3 posL : POSITION;
	float3 normalL : NORMAL;
	float2 uv : TEXCOORD;
	float2 size : SIZE;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Apply displacement and normal map (If has).
	if (gHasDisplacementMap == 1)
	{
		vin.posL += gDisplacementMap.SampleLevel(gsamLinearWrap, vin.uv, 1.0f);
	}
	if (gHasNormalMap == 1)
	{
		vin.normalL = gNormalMap.SampleLevel(gsamLinearWrap, vin.uv, 1.0f);
	}

	vout.posL = vin.posL;
	vout.normalL = vin.normalL;
	vout.uv = vin.uv;
	vout.size = vin.size;

	return vout;
}

struct PatchTess
{
	float edgeTess[4] : SV_TessFactor;
	float insideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float3 centerL = 0.25f * (patch[0].posL + patch[1].posL + patch[2].posL + patch[3].posL);
	float3 centerW = mul(mul(float4(centerL, 1.0f), gWorld), gReflectTrans).xyz;
	float d = distance(centerW, gEyePosW);

	const float d0 = 20.0f;
	const float d1 = 100.0f;
	float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	pt.edgeTess[0] = tess;
	pt.edgeTess[1] = tess;
	pt.edgeTess[2] = tess;
	pt.edgeTess[3] = tess;

	pt.insideTess[0] = tess;
	pt.insideTess[1] = tess;

	return pt;
}

// Make HullOut the same with VertexOut to use HS as a pass-through shader.
struct HullOut
{
	float3 posL : POSITION;
	float3 normalL : NORMAL;
	float2 uv : TEXCOORD;
	float2 size : SIZE;
};

[domain("quad")]
//[partitioning("fractional_even")]
//[partitioning("fractional_odd")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")] // HLSL compiler will resolve the constant hull shader according to this field.
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
	HullOut hout;

	hout.posL = p[i].posL;
	hout.normalL = p[i].normalL;
	hout.uv = p[i].uv;
	hout.size = p[i].size;

	return hout;
}

struct DomainOut
{
	float4 posH : SV_POSITION;
	float3 posW : POSITION;
	float3 normalW : NORMAL;
	float2 uv : TEXCOORD;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HullOut, 4> quad)
{
	DomainOut dout;

	float3 v1 = lerp(quad[0].posL, quad[1].posL, uv.x);
	float3 v2 = lerp(quad[2].posL, quad[3].posL, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	float3 n1 = lerp(quad[0].normalL, quad[1].normalL, uv.x);
	float3 n2 = lerp(quad[2].normalL, quad[3].normalL, uv.x);
	float3 n = lerp(n1, n2, uv.y);

	float2 uv1 = lerp(quad[0].uv, quad[1].uv, uv.x);
	float2 uv2 = lerp(quad[2].uv, quad[3].uv, uv.x);
	float2 m_uv = lerp(uv1, uv2, uv.y); // m_uv: modified uv.

	// Simulate hill geometry.
	p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));

	// General VS works.
	float4 posW = mul(mul(float4(p, 1.0f), gWorld), gReflectTrans);
	dout.posH = mul(mul(posW, gView), gProj);
	dout.posW = posW.xyz;
	dout.normalW = mul(mul(n, (float3x3)gInvTrWorld), (float3x3)gInvTrReflectTrans);
	float4 f_uv = mul(float4(m_uv, 0.0f, 1.0f), gTexTrans); // f_uv: final uv.
	dout.uv = mul(f_uv, gMatTrans).xy;

	return dout;
}

float4 PS(DomainOut pin) : SV_Target
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