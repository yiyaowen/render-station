/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
cbuffer uniqueObjectData : register(b0) {
    float4x4 worldMatrix;
};

cbuffer commonObjectData : register(b1) {
    float4x4 viewMatrix;
    float4x4 inverseViewMatrix;
    float4x4 projectionMatrix;
    float4x4 inverseProjectionMatrix;
    float4x4 viewProjectionMatrix;
    float4x4 inverseViewProjectionMatrix;
    float3 eyeWorldPosition;
    float nearZ;
    float farZ;
};

struct VertexIn {
    float4 color : COLOR;
    float3 localPosition : POSITION;
};

struct VertexOut {
    float4 homogeneousPosition : SV_POSITION;
    float4 color : COLOR;
};

VertexOut VS(VertexIn vin) {
    VertexOut vout;

    float4 worldPosition = mul(float4(vin.localPosition, 1.0f), worldMatrix);
    vout.homogeneousPosition = mul(worldPosition, viewProjectionMatrix);

    vout.color = vin.color;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    return pin.color;
}