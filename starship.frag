#version 460
#extension GL_ARB_bindless_texture : require

in flat uint fTex;
in vec2 fTexCoord;

out vec4 oCol;

layout(std430, binding = 0) buffer Textures {
    uvec2 uTex[];
};

void main() {
	oCol = texture(sampler2D(uTex[fTex]), fTexCoord);
	//gl_FragDepth = 1;

	if (oCol.a == 0) {
		discard;
	}
}