#pragma once

#include "starship.hpp"
#include "tiles.hpp"
#include "universe.hpp"
#include "shaders.hpp"
#include "sfx.hpp"

#include <random>
#include "stb_image.h"
#include "io.hpp"

namespace He {
	BasicTile::BasicTile() {
		glGenTextures(1, &tex);
	}

	void BasicTile::upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type) {
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, type, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindless = glGetTextureHandleARB(tex);
		glMakeTextureHandleResidentARB(bindless);
	}

	void BasicTile::upload(string img) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		upload(data, w, h, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
	}

	void BasicTile::frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
		ship->textures[i] = bindless;
	}

	PlatingTile::PlatingTile() {
		upload(loadRes(L"structure/plating.png", RT_RCDATA));
	}

	MultiTile::MultiTile(int i) {
		tex = new GLuint[i];
		bindless = new GLuint64[i];
		glGenTextures(i, tex);
	}

	void MultiTile::upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type, int i) {
		glBindTexture(GL_TEXTURE_2D, tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, type, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindless[i] = glGetTextureHandleARB(tex[i]);
		glMakeTextureHandleResidentARB(bindless[i]);
	}

	void MultiTile::upload(string img, int i) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		upload(data, w, h, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, i);
	}

	void MultiTile::frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
		ship->textures[i] = bindless[0];
	}

	EngineTile::EngineTile() : MultiTile(2) {
		upload(loadRes(L"engine/small/off.png", RT_RCDATA), false);
		upload(loadRes(L"engine/small/on.png", RT_RCDATA), true);
	}

	void EngineTile::frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
		if (glfwGetKey(universe->frame->handle, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(universe->frame->handle, GLFW_KEY_S) == GLFW_PRESS) {
			ship->textures[i] = bindless[true];

			mat = translate(mat, vec3(x, y, 0));

			universe->lights.push_back(Light(mat, (float)239 / 255, (float)217 / 255, (float)105 / 255, 1));

			static random_device rd;
			static mt19937 gen(rd());
			static uniform_real_distribution<float> check(0, 0.05), color(0, 1), size(0.1, 0.5), alpha(2, 5), life(0.25, 1);

			if (check(gen) <= universe->delta) {
				float col = color(gen),
					r = ((float)206 / 255) * col + ((float)255 / 255) * (1 - col),
					g = ((float)175 / 255) * col + ((float)247 / 255) * (1 - col),
					b = ((float)0 / 255) * col + ((float)216 / 255) * (1 - col),
					a = alpha(gen),
					vx = -ship->phys.vx,
					vy = -ship->phys.vy,
					s = size(gen);

				mat = translate(mat, vec3(0.5 - s / 2, 0.5 - s / 2, 0));

				mat = scale(mat, vec3(s));

				vec4 pos = mat * vec4(0, 0, 0, 1);

				universe->particles.addFirst(Particle(
					vec2(pos.x, pos.y),
					vec4(r, g, b, a),
					10,
					[r, g, b, a, vx, vy](Particle* p, Universe* universe) {
						p->pos += vec2(vx * universe->delta, vy * universe->delta);
						float f = p->life / p->maxLife;
						p->col = vec4(r, g, b, a) * f;
						p->size = 10 * f;
					},
					life(gen)
				));
			}
		} else {
			ship->textures[i] = bindless[false];
		}
	}

	TurretTile::TurretTile() : MultiTile(2) {
		glCreateVertexArrays(1, &vao);

		glBindVertexArray(vao);

		glGenBuffers(1, &vPos);
		glBindBuffer(GL_ARRAY_BUFFER, vPos);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);

		GLubyte* vBuf = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		vBuf[0] = 0;
		vBuf[1] = 0;
		vBuf[2] = 0;
		vBuf[3] = 1;
		vBuf[4] = 1;
		vBuf[5] = 1;
		vBuf[6] = 1;
		vBuf[7] = 0;

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_BYTE, false, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);

		GLubyte* eBuf = (GLubyte*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		eBuf[0] = 0;
		eBuf[1] = 1;
		eBuf[2] = 2;
		eBuf[3] = 2;
		eBuf[4] = 3;
		eBuf[5] = 0;

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		glGenBuffers(1, &uTex);

		MultiTile::upload(loadRes(L"weapons/pds/turret/base.png", RT_RCDATA), 0);
		MultiTile::upload(loadRes(L"weapons/pds/turret/gun.png", RT_RCDATA), 1);
	}

	void TurretTile::upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type, int i) {
		MultiTile::upload(data, width, height, format, type, i);

		if (i == 1) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64), &bindless[1], GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	void TurretTile::frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
		ship->textures[i] = bindless[0];

		mat = translate(mat, vec3(x + 0.5, y - (float)1 / 16, 0));

		double cx, cy;
		glfwGetCursorPos(universe->frame->handle, &cx, &cy);

		int w, h;
		glfwGetFramebufferSize(universe->frame->handle, &w, &h);

		vec4 pos = universe->viewMat * mat * vec4(0, 0, 0, 1);

		float px = (pos.x / 2 + 0.5) * w, py = (pos.y / 2 + 0.5) * h;

		float ang = (float)atan2(-(cx - px), (h - cy) - py) - radians(ship->rot);
		constexpr float max = radians((float)70);
		bool canShoot = true;

		if (ang > max) {
			ang = max;
			canShoot = false;
		}

		if (ang < -max) {
			ang = -max;
			canShoot = false;
		}

		mat = rotate(mat, ang, vec3(0, 0, 1));

		mat = translate(mat, vec3(-0.5, 0, 0));

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glUseProgram(universe->shipShader->id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

		glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uViewMat"), 1, GL_FALSE, value_ptr(universe->viewMat));
		glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uMat"), 1, GL_FALSE, value_ptr(mat));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glUseProgram(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		if (canShoot && glfwGetMouseButton(universe->frame->handle, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
			mat = translate(mat, vec3(0, (float)1 / 16, 0));

			universe->lights.push_back(Light(mat, 1, 0.1, 0.2, 1));

			//mat = scale(mat, vec3(1, 100000, 1));

			//vec4 end = universe->viewMat * mat * vec4(0.5, (float)7 / 16, 0, 1);

			//universe->lineLights.push_back(LineLight(universe->zoom / 20, 1, 0.1, 0.2, 1.5, barrel.x / 2 - 0.5, barrel.y / 2 - 0.5, end.x / 2 - 0.5, end.y / 2 - 0.5));
		}
	}
}
