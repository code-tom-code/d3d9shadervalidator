sampler2D tex : register(s0);

float4 main() : COLOR0
{
	return tex2D(tex, float2(0.5f, 0.5f) );
}
