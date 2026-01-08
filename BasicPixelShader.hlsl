#include "Common.hlsli"

//object texture
Texture2D g_texture0 : register(t0);

// cube texture
TextureCube g_textureCube0 : register(t1);

SamplerState g_sampler : register(s0);

cbuffer BasicPixelConstantBuffer : register(b0)
{
    float3 eyeWorld;
    bool useTexture;
    Material material;
    Light light[MAX_LIGHTS];
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 toEye = normalize(eyeWorld - input.posWorld);
    float3 r = reflect(-toEye, input.normalWorld);
    float3 color = g_textureCube0.Sample(g_sampler, r.xyz).rgb;
    //float3 color = float3(0.0, 0.0, 0.0);

    // uv (0~1) → 중심 기준 (-1~1)
    float2 centered = input.texcoord * 2.0 - 1.0;
    float dist = dot(centered, centered);

    // 원 안쪽만 출력 (dist < 1)
    if (dist > 1.0)
        discard;

    if (dot(toEye, input.normalWorld) < 0.0f)
        discard;

    // Interpolate 대신 위치 기반 UV 좌표 계산 (Sphere Mapping)
    const float radius = length(input.posModel);
    const float theta = atan2(input.posModel.z, input.posModel.x);
    const float phi = acos(input.posModel.y / radius);
    const float PI = 3.141592654;
    input.texcoord.x = theta / (PI * 2);
    input.texcoord.y = phi / PI;

    int i = 0;

    [unroll] // warning X3557: loop only executes for 1 iteration(s), forcing loop to unroll
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        color += ComputeDirectionalLight(light[i], material, input.normalWorld, toEye);
    }

    [unroll]
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        color += ComputePointLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }

    [unroll]
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        color += ComputeSpotLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }

    return useTexture ? float4(color, 1.0) * g_texture0.Sample(g_sampler, input.texcoord) : float4(color, 1.0);
    //return useTexture ? g_texture0.Sample(g_sampler, input.texcoord) : float4(color, 1.0);
}
