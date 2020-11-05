struct PS_INPUT
{
    float4 pos         : SV_POSITION;
    float2 tex         : TEXCOORD0;
};

// based on "Vertex Shader Tricks by Bill Bilodeau" GDC14
// https://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
PS_INPUT main( uint id : SV_VERTEXID )
{
    PS_INPUT output;
    output.pos.x = (float)(id / 2) * 4.0 - 1.0;
    output.pos.y = (float)(id % 2) * 4.0 - 1.0;
    output.pos.z = 0.0;
    output.pos.w = 1.0;

    output.tex.x = (float)(id / 2) * 2.0;
    output.tex.y = (float)(id % 2) * 2.0;

	return output;
}