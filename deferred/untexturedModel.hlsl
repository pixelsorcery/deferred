
cbuffer CB0 : register(b0)
{
    matrix ProjMat;
};

cbuffer CB1 : register(b1)
{
    matrix WorldMat;
}

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

    output.Pos = mul(ProjMat, float4(input.Pos, 1.0));
    output.Normal = mul(WorldMat, float4(input.Normal, 1.0)).xyz;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float3 light = float3(0.0, 1.0, 1.0);
    float3 color = float3(0.7f, 0.7f, 0.7f);
    float3 normal = normalize(input.Normal).xyz;
    light = normalize(light);

    float3 diffuse = saturate(dot(normal, light)) * color;
    return float4(diffuse, 1.0f);
}

