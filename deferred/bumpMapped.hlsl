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
    float3 Normal  : NORMAL;
    float3 Pos     : POSITION;
    float4 Tangent : TANGENT0;
    float2 Tex     : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos       : SV_POSITION;
    float2 Tex       : TEXCOORD0;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BINORMAL;
};

Texture2D boxTexture    : register(t0);
Texture2D normalTexture : register(t1);
SamplerState samLinear  : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(MVP, float4(input.Pos, 1.0));
    output.Normal = mul(WorldMat, float4(input.Normal, 1.0)).xyz;
    output.Tex = input.Tex;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    //float2 coords = float2(0.25f, 0.22f);
    //float3 color = boxTexture.Sample(samLinear, coords).rgb;
    //float3 color = boxTexture.Sample(samLinear, input.Tex.xy).rgb;
    //color.x = input.Tex.x;
    //color.y = input.Tex.y;
    //return float4(color, 1.0f);

    float3 light = float3(0.0, 1.0, 1.0);
    float3 color = boxTexture.Sample(samLinear, input.Tex.xy).rgb;
    float3 normal = normalize(input.Normal);
    light = normalize(light);

    float3 diffuse = saturate(dot(normal, light)) * color;
    return float4(diffuse, 1.0f);
}