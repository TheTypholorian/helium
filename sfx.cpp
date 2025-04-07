#pragma once

#include "sfx.hpp"
#include "universe.hpp"
#include "io.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace He {
	Light::Light(mat4 mat, GLfloat r, GLfloat g, GLfloat b, GLfloat a) : mat(mat), r(r), g(g), b(b), a(a) {}

	void Light::render(Universe* universe) {
		static GLuint vao = 0, vbo = 0, ebo = 0;
		static Shader shader;

		if (vao == 0) {
			glGenVertexArrays(1, &vao);

			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);
			GLubyte* buf = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			buf[0] = 0;
			buf[1] = 0;
			buf[2] = 1;
			buf[3] = 0;
			buf[4] = 1;
			buf[5] = 1;
			buf[6] = 0;
			buf[7] = 1;

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glVertexAttribPointer(0, 2, GL_UNSIGNED_BYTE, false, 0, 0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glGenBuffers(1, &ebo);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);
			buf = (GLubyte*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

			buf[0] = 0;
			buf[1] = 1;
			buf[2] = 2;
			buf[3] = 2;
			buf[4] = 3;
			buf[5] = 0;

			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glBindVertexArray(0);

			shader.attach(GL_VERTEX_SHADER, loadRes(L"light.vert", RT_RCDATA));
			shader.attach(GL_FRAGMENT_SHADER, loadRes(L"light.frag", RT_RCDATA));
			shader.link();
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glUseProgram(shader.id);

		glUniformMatrix4fv(glGetUniformLocation(shader.id, "uView"), 1, false, value_ptr(universe->viewMat));
		glUniformMatrix4fv(glGetUniformLocation(shader.id, "uMat"), 1, false, value_ptr(mat));
		glUniform4f(glGetUniformLocation(shader.id, "uCol"), r, g, b, a);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	Particle::Particle(vec2 pos, vec4 col, GLfloat size, function<void(Particle*, Universe*)> updater, float life) : pos(pos), col(col), size(size), updater(updater), life(life), maxLife(life) {}

	bool Particle::frame(Universe* universe) {
		if (maxLife != 0) {
			life -= universe->delta;

			if (life <= 0) {
				return false;
			}
		}

		updater(this, universe);

		return true;
	}
}
