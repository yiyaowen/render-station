/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#define ADD 0
#define MULTIPLY 1
#define LERP 2

cbuffer cbMixConfigs : register(b0)
{
	int gMixType;

	float gBkgnWeight;
};

Texture2D gInputA : register(t0);
Texture2D gInputB : register(t1);

RWTexture2D<float4> gOutput : register(u0);

[numthreads(32, 32, 1)]
void MixerCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	int3 dtid = dispatchThreadID;

	float4 a = gInputA[dtid.xy];
	float4 b = gInputB[dtid.xy];

	float luminanceA = saturate(a.r * 0.299f + a.g * 0.587f + a.b * 0.114f);
	float luminanceB = saturate(b.r * 0.299f + b.g * 0.587f + b.b * 0.114f);

	float4 result = (float4)0.0f;

	switch (gMixType)
	{
	case ADD: // Not use gBkgnWeight.
		//result = a + b;
		//float luminanceR1 = saturate(result.r * 0.299f + result.g * 0.587f + result.b * 0.114f);
		//result = saturate(result * luminanceA / luminanceR1);

		result = saturate(a + b);

		break;

	case MULTIPLY: // Not use gBkgnWeight.
		//result = a * b;
		//float luminanceR2 = saturate(result.r * 0.299f + result.g * 0.587f + result.b * 0.114f);
		//result = result * luminanceA / luminanceR2;

		result = a * b;

		//result = saturate(a * b * 1.8f);

		break;

	case LERP:
		result = gInputB[dtid.xy] * gBkgnWeight + gInputA[dtid.xy] * (1.0f - gBkgnWeight);
		break;

	default: // Keep the same with background plate.
		result = gInputB[dtid.xy];
		break;
	}

	gOutput[dtid.xy] = result;
}