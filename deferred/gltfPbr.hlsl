
#define PI 3.1415926

cbuffer WorldView : register(b0)
{
    matrix MVP;
};

cbuffer CB1 : register(b1)
{
    matrix WorldMat;
    float4 baseColor;
    float3 emissiveFactor;
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

// PBR code from the glTF sample viewer https://github.com/KhronosGroup/glTF-Sample-Viewer#physically-based-materials-in-gltf-20
// Schlick Fresnel
float3 fresnel(float3 f0, float3 f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// Geometric Occlusion
// Smith Joint GGX
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// Normal Distribution
// Trowbridge-Reitz GGX
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (PI * f * f);
}

// Lambertian Diffuse
float3 lambertian(float3 f0, float3 f90, float3 diffuseColor, float VdotH)
{
    return (1.0 - fresnel(f0, f90, VdotH)) * (diffuseColor / PI);
}

float3 metallicBRDF(float3 f0, float3 f90, float alphaRoughness, float VdotH, float NdotL, float NdotV, float NdotH)
{
    float3 F = fresnel(f0, f90, VdotH);
    float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    return F * Vis * D;
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
    // todo: sample bump map
    float3 normal = normalize(input.Normal);
#else
    float3 normal = normalize(input.Normal);
#endif

    light = normalize(light);

#ifdef ROUGHNESSMETALLIC_TEX
    float3 roughness = roughnessTex.Sample(samLinear, input.Tex.xy).rgb;
#endif 
    
    //float3 diffuse = saturate(dot(normal, light)) * color;
    //return float4(diffuse, 1.0f);
    return float4(color, 1.0f);
}