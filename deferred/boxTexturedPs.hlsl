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
    float3 color = boxTexture.Sample(samLinear, input.Tex.xy).rgb;
	return float4(color, 1.0f);
}