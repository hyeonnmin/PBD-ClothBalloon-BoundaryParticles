#include "Common.hlsli" 

cbuffer BasicGeometryConstantBuffer : register(b0)
{
    matrix model;
    matrix invTranspose;
    matrix view;
    matrix projection;
    float3 gCamRightWS;
    float3 gCamUpWS;
    float scaling;
};

[maxvertexcount(4)]
void main(point GeometryShaderInput input[1], inout TriangleStream<PixelShaderInput> triStream)
{
    float3x3 R_model = (float3x3)model;
    float3x3 R_view = (float3x3)view;

    float3x3 S_local = float3x3(
        0.03f, 0, 0,
        0, 0.03f, 0,
        0, 0, 0.03f
    );

    // scale 포함 최종 변환
    float3x3 M_final = mul(mul(S_local, R_model), R_view);

    // row-vector 기준에서 ex*M_final = M_final의 row0 (축 벡터)
    float2 a0 = M_final[0].xy; // local X axis (scaled) projected to view XY
    float2 a1 = M_final[1].xy; // local Y axis
    float2 a2 = M_final[2].xy; // local Z axis

    float l0 = dot(a0, a0);
    float l1 = dot(a1, a1);
    float l2 = dot(a2, a2);

    // 가장 짧은 축 index 찾기
    int k = 0;
    float lk = l0;
    if (l1 < lk) { k = 1; lk = l1; }
    if (l2 < lk) { k = 2; lk = l2; }

    // 남길 두 축 선택
    float2 u, v;
    if (k == 0) { u = a1; v = a2; }
    else if (k == 1) { u = a0; v = a2; }
    else { u = a0; v = a1; }

    // u를 right로
    float uLen = length(u);
    float2 right = (uLen > 1e-8) ? (u / uLen) : float2(1, 0);
    float rightLen = uLen;

    // v에서 right 성분 제거해서 up 만들기 (직교화)
    float2 v_ortho = v - dot(v, right) * right;
    float vLen = length(v_ortho);

    // v가 거의 right와 평행이면(perfect edge-on 같은 상황) fallback
    float2 up = (vLen > 1e-8) ? (v_ortho / vLen) : float2(-right.y, right.x);
    float upLen = (vLen > 1e-8) ? vLen : length(v);

    // "길이 포함" 축 벡터(뷰 XY 평면 위)
    float3 axisRight = float3(right.x, right.y, 0) * rightLen;
    float3 axisUp = float3(up.x, up.y, 0) * upLen;

    // 1. 중심 위치(model → world)
    float3 center = mul(float4(input[0].posModel, 1.0f), model).xyz;

    // 2. 중심 위치(world -> view)
    center = mul(float4(center, 1.0f), view).xyz;


    float3 cornersWS[4] = {
        center + (-axisRight * scaling + axisUp * scaling),
        center + (axisRight * scaling + axisUp * scaling),
        center + (-axisRight * scaling - axisUp * scaling),
        center + (axisRight * scaling - axisUp * scaling),
    };

    float2 uvs[4] = { float2(0,0), float2(1,0), float2(0,1), float2(1,1) };

    PixelShaderInput output;


    for (int i = 0; i < 4; ++i)
    {
        float3 posView = cornersWS[i];
        float4 posProj = mul(float4(posView, 1.0f), projection);

        output.posModel = float4(input[0].posModel, 1.0f);
        output.posWorld = mul(float4(input[0].posModel, 1.0f), model);
        output.posProj = posProj;
        output.texcoord = uvs[i];
        output.color = float3(0.0f, 0.0f, 0.0f);

        float4 normal = float4(input[0].normalModel, 0.0f);
        output.normalWorld = mul(normal, invTranspose).xyz;
        output.normalWorld = normalize(output.normalWorld);

        triStream.Append(output);
    }
    triStream.RestartStrip();

}