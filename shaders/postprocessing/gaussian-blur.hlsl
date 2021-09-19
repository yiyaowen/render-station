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

#define N 256
#define CACHE_SIZE (N + MAX_BLUR_RADIUS * 2)
groupshared float4 gCache[CACHE_SIZE];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
        gCache[groupThreadID.x + gBlurRadius * 2] = gInput[int2(x, dispatchThreadID.y)];
    }

    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    // Block until all threads in current group finish working.
    GroupMemoryBarrierWithGroupSync();
 
    // Process blur pixel with cached data.
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + gBlurRadius + i;

        blurColor += weights[i + gBlurRadius] * gCache[k];
    }

    gOutput[dispatchThreadID.xy] = blurColor;
}

// Vertical Blur Computer Shader. See the comments of Horizontal Blur Computer Shader above.
[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
        gCache[groupThreadID.y + gBlurRadius * 2] = gInput[int2(dispatchThreadID.x, y)];
    }

    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.y + gBlurRadius + i;

        blurColor += weights[i + gBlurRadius] * gCache[k];
    }

    gOutput[dispatchThreadID.xy] = blurColor;
}