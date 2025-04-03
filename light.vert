#version 460

layout(location = 0) in vec2 vPos;

out vec2 fPos;

uniform mat4 uMat;

void main() {
	gl_Position = uMat * vec4(vPos, 0, 1);
	fPos = vPos * 2 - 1;
}