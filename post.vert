#version 460

layout(location = 0) in vec2 vPos;

out vec2 fPos;

void main() {
	gl_Position = vec4(vPos, 0, 1);
	fPos = vPos;
}