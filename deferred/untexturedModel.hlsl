
cbuffer WorldView : register(b0)
{
    matrix MVP;
};

struct VS_INPUT
{
    float3 Pos         : POSITION;
    float3 Normal      : NORMAL;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float3 Normal      : NORMAL;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(MVP, float4(input.Pos, 1.0));
    output.Normal = input.Normal;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float3 light = float3(15.0, -25.0, -200.0);
    float3 color = float3(0.8f, 0.8f, 0.8f);
    float3 normal = normalize(input.Normal);
    light = normalize(light);

    float3 diffuse = saturate(dot(normal, light)) * color;
    return float4(diffuse, 1.0f);
}

