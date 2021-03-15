
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
    float4 WorldPos  : POSITION0;
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
    output.WorldPos = mul(WorldMat, float4(input.Pos, 1.0));
    output.Normal = mul(WorldMat, float4(input.Normal, 1.0)).xyz;
#ifdef BASECOLOR_TEX
    output.Tex = input.Tex;
#endif

    return output;
}

// http://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=35A58EDADCAC2BF3A46AC602C7122A06?doi=10.1.1.50.2297&rep=rep1&type=pdf
// Schlick approximation of the Cook-Torrence Fresnel
// Factor which defines the ratio of light reflected by each microfacet
float3 fresnel(float3 f0, float VdotH)
{
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// Geometric Occlusion
// Smith Joint GGX
// Geometrical attenuation coefficient which expresses the ration of light that is not self-obstructed by the surface
// https://github.com/KhronosGroup/glTF-Sample-Viewer/commit/21aca11d51ef41c377adf3d23f8e9b38d9396e30#diff-b335630551682c19a781afebcf4d07bf978fb1f8ac04c6bf87428ed5106870f5
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;

    float returnVal = 0.0;

    if (GGX > 0.0)
    {
        returnVal = 0.5 / GGX;
    }

    return returnVal;
}

// Normal Distribution
// Trowbridge-Reitz GGX
// Distribution of facets oriented in H direction
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (PI * f * f);
}

// Lambertian Diffuse
float3 lambertian(float3 diffuseColor)
{
    return diffuseColor / PI;
}

float3 calcPbrBRDF(float3 diffuseColor, float alphaRoughness, float VdotH, float NdotL, float NdotV, float NdotH)
{
    float3 f0 = float3(0.04, 0.04, 0.04);
    // calculate specular terms
    float3 F = fresnel(f0, VdotH);
    float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    float3 diffuse = (1.0 - F)* lambertian(diffuseColor);
    float3 specular = F* Vis* D;

    return NdotL * (diffuse + specular);
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 light = float3(0.0, 1.0, 1.0);
    float roughness = 0.0;
    float3 color = float3(0.0, 0.0, 0.0);
    float lightIntensity = PI;

#if BASECOLOR_TEX
    color = baseColorTex.Sample(samLinear, input.Tex.xy).rgb;
#else
    color = baseColor.rgb;
#endif

#if NORMAL_TEX
    // todo: sample bump map
    float3 normal = normalize(input.Normal);
#else
    float3 normal = normalize(input.Normal);
#endif

    light = normalize(light);

#ifdef ROUGHNESSMETALLIC_TEX
    // roghness is in the g channel
    float4 matSample = roughnessTex.Sample(samLinear, input.Tex.xy);
    roughness = matSample.g * roughnessFactor;
    float metallic = matSample.b * metallicFactor;
#if HAS_OCCLUSION
    color = color * matSample.r;
#endif
#else
    roughness = roughnessFactor;
    float metallic = metallicFactor;
#endif 

#ifdef HAS_EMISSIVE
    color += emissiveFactor;
#endif

    float3 cameraPos = float3(0.0, 0.0, 0.0);
    float3 v = normalize(cameraPos - input.WorldPos.xyz);
    float3 n = normalize(input.Normal);
    float3 l = normalize(light);
    float3 h = normalize(l + v);

    float NdotL = clamp(dot(n, l), 0, 1);
    float VdotH = clamp(dot(v, h), 0, 1);
    float NdotV = clamp(dot(n, v), 0, 1);
    float NdotH = clamp(dot(h, h), 0, 1);

    float3 pbrColor = lightIntensity * calcPbrBRDF(color, roughness, VdotH, NdotL, NdotV, NdotH);
    return float4(pbrColor, 1.0f);
    //float3 diffuse = saturate(dot(normal, light)) * color;
    //return float4(diffuse, 1.0f);
    //return float4(color, 1.0f);
}