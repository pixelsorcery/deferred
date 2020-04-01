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
    //float2 coords = float2(0.25f, 0.22f);
    //float3 color = boxTexture.Sample(samLinear, coords).rgb;
    float3 color = boxTexture.Sample(samLinear, input.Tex.xy).rgb;
    //color.x = input.Tex.x;
    //color.y = input.Tex.y;
	return float4(color, 1.0f);
}