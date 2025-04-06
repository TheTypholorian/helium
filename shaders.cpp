#pragma once

#include "shaders.hpp"
#include "io.hpp"

namespace He {
	StarshipShader::StarshipShader() : Shader() {
		attach(GL_VERTEX_SHADER, loadRes(L"starship.vert", RT_RCDATA));
		attach(GL_FRAGMENT_SHADER, loadRes(L"starship.frag", RT_RCDATA));
		link();
	}
}
