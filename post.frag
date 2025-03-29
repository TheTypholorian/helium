#version 460

in vec2 fPos;

out vec4 oCol;

uniform sampler2D uTex;

struct Light {
	float rad, r, g, b, x, y;
};

layout(std430, binding = 0) buffer Lights {
    Light uLights[];
};

uniform uint uNumLights;

uniform vec2 uAspectRatio = vec2(1);

void main() {
	ivec2 size = textureSize(uTex, 0);

	vec4 col = vec4(0);

	ivec2 o = ivec2(gl_FragCoord.xy) * 2;

	col += texelFetch(uTex, o, 0);
	col += texelFetch(uTex, ivec2(o.x + 1, o.y), 0);
	col += texelFetch(uTex, ivec2(o.x + 1, o.y + 1), 0);
	col += texelFetch(uTex, ivec2(o.x, o.y + 1), 0);

	col /= 4;

	for (uint i = 0; i < uNumLights; i++) {
		Light light = uLights[i];
		vec2 d = (vec2(light.x, light.y) - fPos) / uAspectRatio;
		float dist = sqrt(d.x * d.x + d.y * d.y);

		if (dist <= light.rad) {
			col.xyz += vec3(light.r, light.g, light.b) * (1 - dist / light.rad);
		}
	}

	oCol = col;
}