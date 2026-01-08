#include "Common.hlsli" 

cbuffer BasicVertexConstantBuffer : register(b0)
{
    matrix model;
    matrix invTranspose;
    matrix view;
    matrix projection;
};

GeometryShaderInput main(VertexShaderInput input)
{
    GeometryShaderInput output;
    output.normalModel = input.normalModel;
    output.posModel = input.posModel;
    output.texcoord = input.texcoord;

    return output;
}