#pragma once

#include "main.hpp"
#include "argon.hpp"
#include "glm/glm.hpp"

using namespace Ar;
using namespace glm;

namespace He {
	struct Light {
	public:
		mat4 mat;
		GLfloat r, g, b, a;

		Light(mat4 mat, GLfloat r, GLfloat g, GLfloat b, GLfloat a);

		void render(Universe* universe);
	};

	struct Particle {
	public:
		vec2 pos;
		vec4 col;
		GLfloat size;
		function<void(Particle*, Universe*)> updater;
		float maxLife, life;

		Particle(vec2 pos, vec4 col, GLfloat size, function<void(Particle*, Universe*)> updater, float life = 0);

		bool frame(Universe* universe);
	};
}
