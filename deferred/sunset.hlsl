struct PS_INPUT
{
	float4 pos         : SV_POSITION;
	float2 tex         : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float aspectRatio = 720.0 / 1280.0f;
	float yAspect = input.tex.y *= aspectRatio;
	float rad = 0.15f;
	float2 ctr = float2(0.5f, 0.4f);
	float4 output;
	float2 diff = float2(input.tex.x, yAspect) - ctr;
	if (dot(diff, diff) < (rad * rad) && input.tex.y > 0.3f)
	{
		output = float4(0.97f, 0.46f, 0.3f, 1.0f);
	}
	else
	{
		output = float4(0.60f, 0.18f, 0.996f, 1.0f);
	}

	return output;
}