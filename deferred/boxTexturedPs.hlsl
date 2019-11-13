Texture2D boxTexture : register(t0);
SamplerState samLinear : register(s0);

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float3 Normal      : NORMAL;
    float2 Tex         : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}