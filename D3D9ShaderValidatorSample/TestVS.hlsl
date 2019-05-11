const row_major float4x4 wvp : register(c0);

struct vsOut
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct vsIn
{
	float3 pos : POSITION0;
	float2 tex : TEXCOORD0;
};

vsOut main(vsIn input)
{
	vsOut ret;
	ret.pos = mul(float4(input.pos, 1.0f), wvp);
	ret.tex = input.tex;
	return ret;
}
