#version 460

in vec2 fPos;

out vec4 oCol;

uniform vec4 uCol;

void main() {
	float d = sqrt(fPos.x * fPos.x + fPos.y * fPos.y);

	if (uCol.a != 1) {
		d = pow(d, uCol.a);
	}

	oCol = vec4(uCol.rgb * (1 - d), 1);
	//gl_FragDepth = 2;
}