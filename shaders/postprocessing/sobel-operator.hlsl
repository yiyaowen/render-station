/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define BLACK_ON_WHITE 0
#define WHITE_ON_BLACK 1

cbuffer cbColorMode : register(b0)
{
	int gMode;
};

[numthreads(32, 32, 1)]
void SobelCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	int3 dtid = dispatchThreadID;

	// Collect adjacent pixel colors.
	float4 adjacents[3][3];
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			int2 xy = dtid.xy + int2(i - 1, j - 1);
			adjacents[i][j] = gInput[xy];
		}
	}

	// Calculate the derivative of RGB channels.
	float4 dx = -1.0f * adjacents[0][0] - 2.0f * adjacents[1][0] - 1.0f * adjacents[2][0] +
		1.0f * adjacents[0][2] + 2.0f * adjacents[1][2] + 1.0f * adjacents[2][2];

	float4 dy = -1.0f * adjacents[2][0] - 2.0f * adjacents[2][1] - 1.0f * adjacents[2][2] +
		1.0f * adjacents[0][0] + 2.0f * adjacents[0][1] + 1.0f * adjacents[0][2];

	float4 m = sqrt(dx * dx + dy * dy);

	float luminance = 0.0f;
	if (gMode == BLACK_ON_WHITE) {
		luminance = 1.0f - saturate(m.r * 0.299f + m.g * 0.587f + m.b * 0.114f);
	}
	else if (gMode == WHITE_ON_BLACK) {
		luminance = saturate(m.r * 0.299f + m.g * 0.587f + m.b * 0.114f);
	}

	gOutput[dtid.xy] = luminance;
}