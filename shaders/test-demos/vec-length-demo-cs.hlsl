/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

struct VecLengthDemoCSData
{
    float3 v;
};

struct VecLengthDemoCSResult
{
    float l;
};

// 't#' for shader resource view.
// In general, normal shader resource is read only.

StructuredBuffer<VecLengthDemoCSData> gInput : register(t0);
//Buffer<float3> gInput : register(t0);
//ConsumeStructuredBuffer<VecLengthDemoCSData> gInput : register(u0);

// 'u#' for unordered access view.
// To enable the computer shader to write to the specific shader resource, the resource
// must be bound to a UAV, i.e. Unordered Access View. Note the prefix 'RW' means 'Read and Write'.

RWStructuredBuffer<VecLengthDemoCSResult> gOutput : register(u0);
//RWBuffer<float> gOutput : register(u0);
//AppendStructuredBuffer<VecLengthDemoCSResult> gOutput : register(u1);

// One thread group contains many threads, which will be actually divided into many warps.
// Note the max thread number in one warp can be different between different brands of GPUs.
// For example, that number of NVIDIA GPUs is usually 32, while for AMD GPUs it may be 64.
// It is recommended to create a thread group that holds 256 threads initially for compatibility.
// However, this setting may be changed in the future. Anyway, it is fine to use 32 in this little demo.
[numthreads(64, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID) // dtid: DispaTch thread ID
{
    gOutput[dtid.x].l = length(gInput[dtid.x].v);

    //gOutput[dtid.x] = length(gInput[dtid.x]);
    // 
    //VecLengthDemoCSResult result;
    //result.l = length(gInput.Consume().v);
    //gOutput.Append(result);
}