#version 460

in vec2 fPos;

out vec4 oCol;

struct Light {
	float rad, r, g, b, a, x, y;
};

layout(std430, binding = 0) buffer Lights {
    Light uLights[];
};

uniform sampler2D uTex;
uniform uint uNumLights;
uniform vec2 uAspectRatio = vec2(1);
uniform ivec2 uAliasSize = ivec2(2);

void main() {
	ivec2 size = textureSize(uTex, 0);

	vec4 col = vec4(0);
	uint i = 0;

	ivec2 o = ivec2(gl_FragCoord.xy) * uAliasSize;

	for (uint x = 0; x < uAliasSize.x; x++) {
		for (uint y = 0; y < uAliasSize.y; y++) {
			col += texelFetch(uTex, ivec2(o.x + x, o.y + y), 0);
			i++;
		}
	}

	col /= i;

	for (uint i = 0; i < uNumLights; i++) {
		Light light = uLights[i];
		vec2 d = (vec2(light.x, light.y) - fPos) / uAspectRatio;
		float dist = sqrt(d.x * d.x + d.y * d.y);

		if (dist <= light.rad) {
			col += vec4(light.r, light.g, light.b, light.a) * (1 - dist / light.rad);
		}
	}

	col.xyz *= col.a;
	col.a = 1;

	oCol = col;
}