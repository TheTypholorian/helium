#version 460
#extension GL_ARB_bindless_texture : require

in vec2 fPos;

out vec4 oCol;

uniform vec4 uCol;

void main() {
	float d = sqrt(fPos.x * fPos.x + fPos.y * fPos.y);

	if (uCol.a != 1) {
		d = pow(d, uCol.a);
	}

	oCol = vec4(uCol.rgb * (1 - d), 1);
}