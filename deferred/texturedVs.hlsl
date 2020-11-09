cbuffer WorldView : register(b0)
{
    matrix MVP;
};

cbuffer CB1 : register(b1)
{
    matrix WorldMat;
}

struct VS_INPUT
{
    float3 Pos         : POSITION;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos    = mul(MVP, float4(input.Pos, 1.0));
    output.Normal = mul(WorldMat, float4(input.Normal, 1.0)).xyz;
    output.Tex    = input.Tex;
    return output;
}