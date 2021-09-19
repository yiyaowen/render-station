/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

struct VecAddDemoCSData
{
    float2 v1;
    float3 v2;
};

// 't#' for shader resource view.
// In general, normal shader resource is read only.
StructuredBuffer<VecAddDemoCSData> gInputA : register(t0);
StructuredBuffer<VecAddDemoCSData> gInputB : register(t1);
// 'u#' for unordered access view.
// To enable the computer shader to write to the specific shader resource, the resource
// must be bound to a UAV, i.e. Unordered Access View. Note the prefix 'RW' means 'Read and Write'.
RWStructuredBuffer<VecAddDemoCSData> gOutput : register(u0);

// One thread group contains many threads, which will be actually divided into many wraps.
// Note the max thread number in one wrap can be different between different brands of GPUs.
// For example, that number of NVIDIA GPUs is usually 32, while for AMD GPUs it may be 64.
// It is recommended to create a thread group that holds 256 threads initially for compatibility.
// However, this setting may be changed in the future. Anyway, it is fine to use 32 in this little demo.
[numthreads(32, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID) // dtid: DispaTch thread ID
{
    gOutput[dtid.x].v1 = gInputA[dtid.x].v1 + gInputB[dtid.x].v1;
    gOutput[dtid.x].v2 = gInputA[dtid.x].v2 + gInputB[dtid.x].v2;
}