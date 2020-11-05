struct PS_INPUT
{
	float4 pos         : SV_POSITION;
	float2 tex         : TEXCOORD0;
};

static const float2 resolution = float2(1280.0f, 720.0f);

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 p = (2 * float2(input.pos.x, resolution.y - input.pos.y) - resolution) / resolution.y;
	if (p.y > 0.0) discard;

	float4 output = float4(0.0, p.y, 0.0f, 1.0f);
	return output;
}