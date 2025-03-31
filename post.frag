#version 460

in vec2 fPos;

out vec4 oCol;

struct Light {
	float rad, r, g, b, a, x, y;
};

layout(std430, binding = 0) buffer Lights {
    Light uLights[];
};

uniform uint uNumLights;

struct LineLight {
	float rad, r, g, b, a, x1, y1, x2, y2;
};

layout(std430, binding = 1) buffer LineLights {
    LineLight uLineLights[];
};

uniform uint uNumLineLights;

uniform sampler2D uTex;
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

	if (uNumLineLights != 0) {
		vec2 fp = fPos / uAspectRatio;

		for (uint i = 0; i < uNumLineLights; i++) {
			LineLight light = uLineLights[i];

			vec2 v = vec2(light.x1, light.y1) / uAspectRatio;
			vec2 w = vec2(light.x2, light.y2) / uAspectRatio;

			float dist = 0;
			vec2 d = w - v;
			float l2 = dot(d, d);

			if (l2 == 0) {
				dist = distance(fp, v);
			} else {
				dist = distance(fp, v + clamp(dot(fp - v, d) / l2, 0, 1) * d);
			}

			if (dist <= light.rad) {
				col += vec4(light.r, light.g, light.b, light.a) * (1 - dist / light.rad);
			}
		}
	}

	col.xyz *= col.a;
	col.a = 1;

	oCol = col;
}