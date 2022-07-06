struct PixelIn {
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD0;
};

struct PixelOut {
	float4 color : SV_TARGET;
	float depth  : SV_DEPTH;
};

#ifdef PI_TEXTURE
Texture2D testTexture : register(t0);
SamplerState testSampler : register(s0);
#endif

PixelOut main(PixelIn input) {
	PixelOut output;

#ifdef PI_TEXTURE
	output.color = input.color * 2.0f * testTexture.Sample(testSampler, input.uv);
#else
	output.color = input.color;
#endif

	if (output.color.r <= 1.0f/255.0f && output.color.g <= 1.0f/255.0f && output.color.b <= 1.0f/255.0f)
		discard; // Spyro treats black pixels as transparent

	if (output.color.a < 0.1f) // Discard transparent pixels from depth testing
		discard; //  (and errr rasterisation in general but they're transparent so who'll miss them?)

	output.depth = input.position.z;

	return output;
}