#version 460

layout(location = 0) in vec2 vPos;
layout(location = 1) in vec4 vCol;
layout(location = 2) in float vSize;

out flat vec4 fCol;

uniform mat4 uView;
uniform float uZoom;

void main() {
	gl_Position = uView * vec4(vPos, 0, 1);
	fCol = vCol;
	gl_PointSize = uZoom * vSize;
}