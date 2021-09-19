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

// To use billboard technique to render trees, we maintain a texture array instead of a single texture.
// Note texture array is not necessary for billboard technique, single texture is also acceptable.
// It is cleaner using texture array rather than single texture to render various textures of trees.
//Texture2D gDiffuseMap : register(t0);
Texture2DArray gDiffuseMapArray : register(t0);

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
	uint primID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.posL = vin.posL;
	vout.size = vin.size;

	return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1], uint primID : SV_PrimitiveID, inout TriangleStream<GeoOut> triStream)
{
	// Transform from local coordinates to world coordinates.
	float3 centerW = mul(mul(float4(gin[0].posL, 1.0f), gWorld), gReflectTrans).xyz;

	// Calculate the arguments needed to make the billboard aligns with Y-axis and faces to viewer.
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - centerW;
	look.y = 0.0f;
	look = normalize(look);
	float3 right = cross(up, look);

	// Generate quadrangle from center point.
	float halfWidth = 0.5f * gin[0].size.x;
	float halfHeight = 0.5f * gin[0].size.y;

/*     v2         v0
*       *---------*
*       |\        |                          Top View: Y-axis points from screen to you
*       | \       |
*       |  \      |                                          look
*       |   \     |                                       _   /|\   _
*       |    % v  |                           half right |\    |    /| half left
*       |     \   |         /|\ y                          \   |   /
*       |      \  |          |                              \  |  /
*       |       \ |          |                               \ | /
*       |        \|  x <-----*-----> right     right <________\|/________> left (x)
*       *---------*
*      v3         v1                                       Figure (A)
*/

	float4 vRight[4];
	vRight[0] = float4(centerW - halfWidth * right + halfHeight * up, 1.0f);
	vRight[1] = float4(centerW - halfWidth * right - halfHeight * up, 1.0f);
	vRight[2] = float4(centerW + halfWidth * right + halfHeight * up, 1.0f);
	vRight[3] = float4(centerW + halfWidth * right - halfHeight * up, 1.0f);

	float2 uv[4] =
	{
		float2(1.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(0.0f, 1.0f)
	};

	GeoOut gout;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gout.posH = mul(mul(vRight[i], gView), gProj);
		gout.posW = vRight[i].xyz;
		gout.normalW = look;
		gout.uv = uv[i];
		gout.primID = primID;
		triStream.Append(gout);
	}
	triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
	pin.normalW = normalize(pin.normalW);
	float3 eyeVecW = gEyePosW - pin.posW;
	float distToEye = length(eyeVecW);
	eyeVecW /= distToEye;
	// An extra 3rd primID component is needed to get each texture in texture array.
	float3 uvw = float3(pin.uv, pin.primID % 3);
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMapArray.Sample(gsamAnisotropicWrap, uvw);

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