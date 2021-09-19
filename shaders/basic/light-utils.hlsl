/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

/*
** Yiyaowen's Cartoon Style does not work very well. Basically, not satisfying.
*/

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif

#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

struct Light
{
    float3 strength;
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
};

struct Material
{
    float4 diffuseAlbedo;
    float3 fresnelR0;
    // Not this is different from the Material defined in cpp-header.
    // The conversion relation is: shiniess = 1 - roughness.
    float shininess;
};

float calcAttenuation(float d, float fallOffStart, float fallOffEnd)
{
    return saturate((fallOffEnd - d) / (fallOffEnd - fallOffStart));
}

float3 fresnelSchlickApprox(float3 R0, float3 lightVec, float3 normal)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float m = 1.0f - cosIncidentAngle;
    float3 reflectedPercent = R0 + (1.0f - R0) * pow(m, 5.0f);

    return reflectedPercent;
}

float3 blinnPhong(float3 strength, float3 lightVec, float3 normal, float3 eyeVec, Material mat)
{
    const float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(eyeVec + lightVec);

    float3 fresnelFactor = fresnelSchlickApprox(mat.fresnelR0, lightVec, normal);
    float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(halfVec, normal), 0.0f), m);

    float3 specularAlbedo = roughnessFactor * fresnelFactor;
    // Since m could be a big number, for example 256, the specular albedo could be more than 1.0,
    // which is not we want for LDR (Low Dynamic Range). We can simply clamp it between 0.0 ~ 1.0,
    // but the solution here is to scale down the value for the sake of the softness of specular light.
    specularAlbedo = specularAlbedo / (specularAlbedo + 1.0f);

    return (mat.diffuseAlbedo.rgb + specularAlbedo) * strength;
}

float3 yiyaowenCartoon(float3 strength, float3 lightVec, float3 normal, float3 eyeVec, Material mat)
{
    const float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(eyeVec + lightVec);

   // Calculate the fresnel factor of cartoon style.
    float kd = max(dot(lightVec, normal), 0.0f);

    float kdPrime = 0.0f;
    if (kd <= 0.0f)
    {
        kdPrime = 0.4f;
    }
    else if (kd <= 0.5f)
    {
        kdPrime = 0.6f;
    }
    else if (kd <= 1.0f)
    {
        kdPrime = 1.0f;
    }

    float mPrime = 1.0f - kdPrime;
    float3 fresnelFactor = mat.fresnelR0 + (1.0f - mat.fresnelR0) * pow(mPrime, 5.0f);

    // Calculate the roughness factor (shininess factor) of cartoon style.
    float ks = 0.0f;
    if (kd > 0)
    {
        ks = pow(max(dot(halfVec, normal), 0.0f), m);
    }

    float ksPrime = 0.0f;
    if (ks <= 0.1f)
    {
        ksPrime = 0.0f;
    }
    else if (ks <= 0.8f)
    {
        ksPrime = 0.5f;
    }
    else if (ks <= 1.0f)
    {
        ksPrime = 0.8f;
    }

    float roughnessFactor = (m + 8.0f) / 8.0f * ksPrime;

    float3 specularAlbedo = roughnessFactor * fresnelFactor;
    // Since m could be a big number, for example 256, the specular albedo could be more than 1.0,
    // which is not we want for LDR (Low Dynamic Range). We can simply clamp it between 0.0 ~ 1.0,
    // but the solution here is to scale down the value for the sake of the softness of specular light.
    specularAlbedo = specularAlbedo / (specularAlbedo + 1.0f);

    return (mat.diffuseAlbedo.rgb + specularAlbedo) * strength;
}

float3 calcDirectionalLightPhysicsBased(Light light, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = -light.direction;

    // Lambert's law
    float cosTheta = max(dot(lightVec, normal), 0.0f);
    float3 strength = light.strength * cosTheta;

    return blinnPhong(strength, lightVec, normal, eyeVec, mat);
}

float3 calcDirectionalLightYiyaowenCartoon(Light light, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = -light.direction;

    // Lambert's law
    float kd = max(dot(lightVec, normal), 0.0f);

    float kdPrime = 0.0f;
    if (kd <= 0.0f)
    {
        kdPrime = 0.4f;
    }
    else if (kd <= 0.5f)
    {
        kdPrime = 0.6f;
    }
    else if (kd <= 1.0f)
    {
        kdPrime = 1.0f;
    }
    float3 strength = light.strength * kdPrime;

    return yiyaowenCartoon(strength, lightVec, normal, eyeVec, mat);
}

float3 calcPointLightPhysicsBased(Light light, float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = light.position - pos;
    float d = length(lightVec);
    lightVec /= d;

    // Lambert's law
    float cosTheta = max(dot(lightVec, normal), 0.0f);
    float3 strength = light.strength * cosTheta;

    strength *= calcAttenuation(d, light.fallOffStart, light.fallOffEnd);

    return blinnPhong(strength, lightVec, normal, eyeVec, mat);
}

float3 calcPointLightYiyaowenCartoon(Light light, float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = light.position - pos;
    float d = length(lightVec);
    lightVec /= d;

    // Lambert's law
    float kd = max(dot(lightVec, normal), 0.0f);

    float kdPrime = 0.0f;
    if (kd <= 0.0f)
    {
        kdPrime = 0.4f;
    }
    else if (kd <= 0.5f)
    {
        kdPrime = 0.6f;
    }
    else if (kd <= 1.0f)
    {
        kdPrime = 1.0f;
    }
    float3 strength = light.strength * kdPrime;

    strength *= calcAttenuation(d, light.fallOffStart, light.fallOffEnd);

    return yiyaowenCartoon(strength, lightVec, normal, eyeVec, mat);
}

float3 calcSpotLightPhysicsBased(Light light, float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = light.position - pos;
    float d = length(lightVec);
    lightVec /= d;

    // Lambert's law
    float cosTheta = max(dot(lightVec, normal), 0.0f);
    float3 strength = light.strength * cosTheta;

    strength *= calcAttenuation(d, light.fallOffStart, light.fallOffEnd);

    strength *= pow(max(dot(-lightVec, light.direction), 0.0f), light.spotPower);

    return blinnPhong(strength, lightVec, normal, eyeVec, mat);
}

float3 calcSpotLightYiyaowenCartoon(Light light, float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 lightVec = light.position - pos;
    float d = length(lightVec);
    lightVec /= d;

    // Lambert's law
    float kd = max(dot(lightVec, normal), 0.0f);

    float kdPrime = 0.0f;
    if (kd <= 0.0f)
    {
        kdPrime = 0.4f;
    }
    else if (kd <= 0.5f)
    {
        kdPrime = 0.6f;
    }
    else if (kd <= 1.0f)
    {
        kdPrime = 1.0f;
    }
    float3 strength = light.strength * kdPrime;

    strength *= calcAttenuation(d, light.fallOffStart, light.fallOffEnd);

    strength *= pow(max(dot(-lightVec, light.direction), 0.0f), light.spotPower);

    return yiyaowenCartoon(strength, lightVec, normal, eyeVec, mat);
}

float3 calcAllLightsPhysicsBased(Light lights[MAX_LIGHTS], float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 result = 0.0f;
    int i = 0;

#ifdef NUM_DIR_LIGHTS
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += calcDirectionalLightPhysicsBased(lights[i], normal, eyeVec, mat);
    }
#endif

#ifdef NUM_POINT_LIGHTS
    for (i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        result += calcPointLightPhysicsBased(lights[i], pos, normal, eyeVec, mat);
    }
#endif

#ifdef NUM_SPOT_LIGHTS
    for (i = 0; i < NUM_SPOT_LIGHTS; ++i)
    {
        result += calcSpotLightPhysicsBased(lights[i], pos, normal, eyeVec, mat);
    }
#endif

    return result;
}

float3 calcAllLightsYiyaowenCartoon(Light lights[MAX_LIGHTS], float3 pos, float3 normal, float3 eyeVec, Material mat)
{
    float3 result = 0.0f;
    int i = 0;

#ifdef NUM_DIR_LIGHTS
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += calcDirectionalLightYiyaowenCartoon(lights[i], normal, eyeVec, mat);
    }
#endif

#ifdef NUM_POINT_LIGHTS
    for (i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        result += calcPointLightYiyaowenCartoon(lights[i], pos, normal, eyeVec, mat);
    }
#endif

#ifdef NUM_SPOT_LIGHTS
    for (i = 0; i < NUM_SPOT_LIGHTS; ++i)
    {
        result += calcSpotLightYiyaowenCartoon(lights[i], pos, normal, eyeVec, mat);
    }
#endif

    return result;
}