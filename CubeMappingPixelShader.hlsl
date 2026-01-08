#include "Common.hlsli" 

TextureCube g_textureCube0 : register(t0);
SamplerState g_sampler : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(0, 0, 0, 1);
    return g_textureCube0.Sample(g_sampler, input.posWorld.xyz);
}