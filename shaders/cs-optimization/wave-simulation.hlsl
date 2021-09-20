/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

cbuffer cbIntermidiates : register(b0)
{
	// Wave euqation coefficients.
	float a1;

	float a2;

	float a3;

	float d; // Distance between two adjacent nodes.

	float h; // Disturb height.

	int m; // Horizontal node count.
	int n; // Vertical node count.

	int x; // Disturb position x.
	int y; // Disturb position y.
};

RWTexture2D<float> gPrevCrestInput : register(u0);
RWTexture2D<float> gCurrCrestInput : register(u1);
// Note in GPU's compute shader the computation is processed parallelly, which means that
// the trick we used (write into _prevGeo directly) in CPU general computation does not work here.
RWTexture2D<float> gNextCrestOutput : register(u2);

RWTexture2D<float4> gDisplacementMap : register(u3);

[numthreads(1, 1, 1)]
void DisturbWave()
{
	float halfH = h * 0.5f;
	gCurrCrestInput[int2(x, y)] = h;
	gCurrCrestInput[int2(x - 1, y)] = halfH;
	gCurrCrestInput[int2(x + 1, y)] = halfH;
	gCurrCrestInput[int2(x, y - 1)] = halfH;
	gCurrCrestInput[int2(x, y + 1)] = halfH;
}

[numthreads(32, 32, 1)]
void CalcDisplacement(int3 dtid : SV_DispatchThreadID)
{
	// Do not modify the nodes on the boundary.
	if (dtid.x == 0 || dtid.x == m || dtid.y == 0 || dtid.y == n)
	{
		gNextCrestOutput[dtid.xy] = 0.0f;
	}
	else
	{
		float curr = gCurrCrestInput[dtid.xy];
		float prev = gPrevCrestInput[dtid.xy];

		float next = a1 * curr + a2 * prev
			+ a3 * (gCurrCrestInput[dtid.xy + int2(-1, 0)] + // Left
				gCurrCrestInput[dtid.xy + int2(+1, 0)] + // Right
				gCurrCrestInput[dtid.xy + int2(0, -1)] + // Above
				gCurrCrestInput[dtid.xy + int2(0, +1)]); // Below

		gNextCrestOutput[dtid.xy] = next;
	}

	gDisplacementMap[dtid.xy] = float4(0.0f, gNextCrestOutput[dtid.xy], 0.0f, 0.0f);
}

RWTexture2D<float4> gNormalMap : register(u4);

[numthreads(32, 32, 1)]
void CalcNormal(int3 dtid : SV_DispatchThreadID)
{
	// Do not modify the nodes on the boundary.
	if (dtid.x == 0 || dtid.x == m || dtid.y == 0 || dtid.y == n)
	{
		gNormalMap[dtid.xy] = float4(0.0f, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		gNormalMap[dtid.xy] = float4(normalize(float3
			(
				gNextCrestOutput[dtid.xy + int2(-1, 0)] - gNextCrestOutput[dtid.xy + int2(1, 0)],
				2 * d,
				gNextCrestOutput[dtid.xy + int2(0, -1)] - gNextCrestOutput[dtid.xy + int2(0, 1)]
			)),
			0.0f);
	}
}