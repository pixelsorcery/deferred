cbuffer WorldView : register(b0)
{
    matrix MVP;
};

cbuffer CB1 : register(b1)
{
    matrix WorldMat;
    float4 baseColor;
    float3 emissiveFactor;
    float  pad;
    float  metallicFactor;
    float  roughnessFactor;
};

struct VS_INPUT
{
    float3 Normal  : NORMAL;
    float3 Pos     : POSITION;
#if HAS_TANGENT
    float4 Tangent : TANGENT0;
#endif
#ifdef BASECOLOR_TEX
    float2 Tex     : TEXCOORD0;
#endif
};

struct PS_INPUT
{
    float4 Pos       : SV_POSITION;
#ifdef BASECOLOR_TEX
    float2 Tex       : TEXCOORD0;
#endif

    float3 Normal    : NORMAL;

#ifdef HAS_TANGENT
    float3 Tangent   : TANGENT;
    float3 Bitangent : BINORMAL;
#endif
};

#ifdef BASECOLOR_TEX
Texture2D baseColorTex  : register(t0);
#endif

#ifdef ROUGHNESSMETALLIC_TEX
Texture2D roughnessTex  : register(t1);
#endif

#ifdef NORMAL_TEX
Texture2D normalTex     : register(t2);
#endif

#ifdef OCCLUSION_TEX
Texture2D occlusionTex  : register(t3);
#endif

#ifdef EMISSIVE_TEX
Texture2D emissiveTex   : register(t4);
#endif

#ifdef BASECOLOR_TEX
SamplerState samLinear  : register(s0);
#endif

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(MVP, float4(input.Pos, 1.0));
    output.Normal = mul(WorldMat, float4(input.Normal, 1.0)).xyz;
#ifdef BASECOLOR_TEX
    output.Tex = input.Tex;
#endif

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 light = float3(0.0, 1.0, 1.0);

#if BASECOLOR_TEX
    float3 color = baseColorTex.Sample(samLinear, input.Tex.xy).rgb;
#else
    float3 color = baseColor.rgb;
#endif

#if NORMAL_TEX
#else
    float3 normal = normalize(input.Normal);
#endif

    light = normalize(light);

#ifdef ROUGHNESSMETALLIC_TEX
    float3 roughness = roughnessTex.Sample(samLinear, input.Tex.xy).rgb;
#endif 
    
    return float4(color, 1.0f);
    //float3 diffuse = saturate(dot(normal, light)) * color;
    //return float4(diffuse, 1.0f);
}