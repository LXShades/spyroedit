cbuffer VSView : register(b0) {
	row_major matrix matViewProj : packoffset(c0);
};

cbuffer VSObject : register(b1) {
	row_major matrix matObject : packoffset(c0);
};

cbuffer VSLights : register(b2) {
	// all lights
	bool lightsEnabled : packoffset(c0); // byte 0

	// individual lights
	struct {
		bool enabled; // byte 16
		int type; // byte 4?
		float3 color; // byte 8???
		float3 direction; // byte 8?? - 24?
	} lights[8] : packoffset(c1);
};

struct VertIn {
	float3 position : POSITION;
#ifdef VI_COLOR
	uint4 color     : COLOR;
#endif
#ifdef VI_TEXTURE
	float2 uv       : TEXCOORD0;
#endif
#ifdef VI_NORMAL
	float3 normal   : NORMAL;
#endif
};

struct VertOut {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv    : TEXCOORD0;
};

VertOut main(VertIn input) {
	VertOut output;

	// Position
	output.position = mul(mul(float4(input.position.x, input.position.y, input.position.z, 1.0f), matObject), matViewProj);

	// Colour (w or w/o lighting)
#ifdef LIGHTS
	if (lightsEnabled) {
		float3 srcClr = float3(input.color.r / 255.0f, input.color.g / 255.0f, input.color.b / 255.0f);
		output.color = float4(0.0f, 0.0f, 0.0f, input.color.a / 255.0f);

		for (int i = 0; i < 8; i++) {
			if (!lights[i].enabled)
				continue;

			if (lights[i].type == 1) { // ambient
				output.color.rgb += srcClr * lights[i].color;
			}
#ifdef VI_NORMAL
			else if (lights[i].type == 2) { // directional
				output.color.rgb += srcClr * mul(lights[i].color, saturate(-dot(input.normal, lights[i].direction)));
			}
#endif
		}

		saturate(output.color);
	} else
#endif // LIGHTS
	{
#ifdef VI_COLOR
		output.color = float4(input.color.r / 255.0f, input.color.g / 255.0f, input.color.b / 255.0f, input.color.a / 255.0f);
#else
		output.color = float4(1.0f, 1.0f, 1.0f, 1.0f); // we nerdy programmers apologise for our contribution to whitewashing
#endif
	}

	// Texture
#ifdef VI_TEXTURE
	output.uv = input.uv;
#else
	output.uv = float2(0.0f, 0.0f);
#endif

	return output;
}