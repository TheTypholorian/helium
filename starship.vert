#version 460

layout(location = 0) in vec2 vPos;

out flat uint fTex;
out vec2 fTexCoord;

uniform mat4 uMat;

void main() {
	gl_Position = uMat * vec4(vPos, 0, 1);
	fTexCoord = vPos;
	fTex = gl_VertexID >> 2;
}