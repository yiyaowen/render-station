/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

cbuffer cbBlurConfigs : register(b0)
{
    int gBlurRadius;

    // twoSigma2 = 2 * (gray blur grade)^2.
    // Note the difference between GRAY blur grade and DISTANCE blur grade.
    float twoSigma2;

    // Support max 11 blur weight values.
    // Thus blur radius must not be greater than 5.
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

#define MAX_BLUR_RADIUS 5

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

// The max memory space of groupshared cache is 32kB.
// To take full advantage of multicore CPU, we should make sure at lease 2 thread groups are
// working at the same time, which means the max memory space is only 16kB in practice.
// 
// 16kB = 2^14 Bytes = 2^12 float(int) = 64*64 float(int) = 1024 float4(int4)
// 
// What if we make N = 32: <DirectX warning X4714>
// sum of temp registers and indexable temp registers times 1024 threads
// exceeds the recommended total 16384.  Performance may be reduced.

#define N 16
#define CACHE_SIZE (N + MAX_BLUR_RADIUS * 2)
groupshared float4 gCache[CACHE_SIZE][CACHE_SIZE];

[numthreads(N, N, 1)]
void BlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    int3 gtid = groupThreadID;
    int3 dtid = dispatchThreadID;

    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

    // Handle the pixels outside the boundary.
    // Left
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[gtid.x][gtid.y + gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
        // Left Top
        if (groupThreadID.y < gBlurRadius)
        {
            int y = max(dispatchThreadID.y - gBlurRadius, 0);
            gCache[gtid.x][gtid.y] = gInput[int2(x, y)];
        }
    }
    // Right
    else if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
        gCache[gtid.x + gBlurRadius * 2][gtid.y + gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
        // Right Bottom
        if (groupThreadID.y >= N - gBlurRadius)
        {
            int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
            gCache[gtid.x + gBlurRadius * 2][gtid.y + gBlurRadius * 2] = gInput[int2(x, y)];
        }
    }
    // Top
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[gtid.x + gBlurRadius][gtid.y] = gInput[int2(dispatchThreadID.x, y)];
        // Right Top
        if (groupThreadID.x >= N - gBlurRadius)
        {
            int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
            gCache[gtid.x + gBlurRadius * 2][gtid.y] = gInput[int2(x, y)];
        }
    }
    // Bottom
    else if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
        gCache[gtid.x + gBlurRadius][gtid.y + gBlurRadius * 2] = gInput[int2(dispatchThreadID.x, y)];
        // Left Bottom
        if (groupThreadID.x < gBlurRadius)
        {
            int x = max(dispatchThreadID.x - gBlurRadius, 0);
            gCache[gtid.x][gtid.y + gBlurRadius * 2] = gInput[int2(x, y)];
        }
    }

    gCache[gtid.x + gBlurRadius][gtid.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Calculate gray-scale weight factors and multiply distance weights and gray weights.
    float e = 2.718f;
    float weightSum = 0.0f;

    float3 sourceRGB = (float3)gCache[gtid.x + gBlurRadius][gtid.y + gBlurRadius];
    float sourceGray = sourceRGB.x * 0.299f + sourceRGB.y * 0.587f + sourceRGB.z * 0.114f;

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        for (int j = -gBlurRadius; j <= gBlurRadius; ++j)
        {
            // Get target pixel gray scale.
            float4 targetColor = gCache[gtid.x + gBlurRadius + i][gtid.y + gBlurRadius + j];
            float targetGray = targetColor.x * 0.299f + targetColor.y * 0.587f + targetColor.z * 0.114f;

            // Compute mixed weight = distance weight * gray scale weight.
            float powerFactor = (sourceGray - targetGray) * (sourceGray - targetGray);
            float mixedWeight = weights[i + gBlurRadius] * weights[j + gBlurRadius] * pow(e, -powerFactor / twoSigma2);
           
            blurColor +=  mixedWeight * targetColor;

            weightSum += mixedWeight;
        }
    }

    // Normalize blur color.
    blurColor /= weightSum;

    gOutput[dispatchThreadID.xy] = blurColor;
}