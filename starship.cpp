#pragma once

#include "starship.hpp"
#include "universe.hpp"
#include "shaders.hpp"
#include "tiles.hpp"

namespace He {
	Starship::Starship(const uint32_t width, const uint32_t height) : width(width), height(height), tiles(new Tile* [width * height]()), len(width* height * 6) {
		glGenVertexArrays(1, &vao);

		glBindVertexArray(vao);

		glGenBuffers(1, &vPos);
		glBindBuffer(GL_ARRAY_BUFFER, vPos);
		glBufferData(GL_ARRAY_BUFFER, width * height * 8 * sizeof(GLushort), nullptr, GL_STATIC_DRAW);

		GLushort* vBuf = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		uint32_t i = 0;

		for (GLushort x = 0; x < width; x++) {
			for (GLushort y = 0; y < height; y++) {
				vBuf[i++] = x;
				vBuf[i++] = y;

				vBuf[i++] = x;
				vBuf[i++] = y + 1;

				vBuf[i++] = x + 1;
				vBuf[i++] = y + 1;

				vBuf[i++] = x + 1;
				vBuf[i++] = y;
			}
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, false, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, len * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

		GLuint* eBuf = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		i = 0;
		uint32_t j = 0;

		for (GLuint x = 0; x < width; x++) {
			for (GLuint y = 0; y < height; y++) {
				eBuf[i++] = j + 0;
				eBuf[i++] = j + 1;

				eBuf[i++] = j + 2;
				eBuf[i++] = j + 2;

				eBuf[i++] = j + 3;
				eBuf[i++] = j + 0;

				j += 4;
			}
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		glGenBuffers(1, &uTex);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
		GLuint size = width * height * sizeof(GLuint64);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		textures = (GLuint64*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void Starship::resize(uint32_t width, uint32_t height) {
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vPos);
		glBufferData(GL_ARRAY_BUFFER, width * height * 8 * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

		GLuint* vBuf = (GLuint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		uint32_t i = 0;

		for (uint32_t x = 0; x < width; x++) {
			for (uint32_t y = 0; y < height; y++) {
				vBuf[i++] = x;
				vBuf[i++] = y;

				vBuf[i++] = x;
				vBuf[i++] = y + 1;

				vBuf[i++] = x + 1;
				vBuf[i++] = y + 1;

				vBuf[i++] = x + 1;
				vBuf[i++] = y;
			}
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, false, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, len * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

		GLuint* eBuf = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		i = 0;
		uint32_t j = 0;

		for (uint32_t x = 0; x < width; x++) {
			for (uint32_t y = 0; y < height; y++) {
				eBuf[i++] = j + 0;
				eBuf[i++] = j + 1;

				eBuf[i++] = j + 2;
				eBuf[i++] = j + 2;

				eBuf[i++] = j + 3;
				eBuf[i++] = j + 0;

				j += 4;
			}
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDeleteBuffers(1, &uTex);
		glGenBuffers(1, &uTex);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
		GLuint size = width * height * sizeof(GLuint64);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		textures = (GLuint64*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		Tile** nTiles = new Tile * [width * height]();
		uint32_t oldW = this->width;
		uint32_t oldH = this->height;
		uint32_t minW = (oldW < width ? oldW : width);
		uint32_t minH = (oldH < height ? oldH : height);

		for (uint32_t y = 0, i = 0, j = 0; y < minH; y++, i += width, j += oldW) {
			for (uint32_t x = 0; x < minW; x++) {
				nTiles[i + x] = tiles[j + x];
			}
		}

		for (uint32_t i = 0, x = 0; x < width; x++) {
			for (uint32_t y = 0; y < height; y++, i++) {
				cout << x << " " << y << ": " << nTiles[i] << endl;
			}
		}

		this->width = width;
		this->height = height;
		delete[] tiles;
		tiles = nTiles;
	}

	void Starship::render(Universe* universe) {
		if (glfwGetKey(universe->frame->handle, GLFW_KEY_A) == GLFW_PRESS) {
			rot += universe->delta * 90;
		}

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_D) == GLFW_PRESS) {
			rot -= universe->delta * 90;
		}

		float acc = 0;

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_W) == GLFW_PRESS) {
			acc += speed;
		}

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_S) == GLFW_PRESS) {
			acc -= speed;
		}

		float rad = radians(rot), rad90 = radians(rot + 90);

		phys.vx += cos(rad90) * acc * universe->delta;
		phys.vy += sin(rad90) * acc * universe->delta;

		const float max = 500;

		if (phys.x > max) {
			phys.x = max;
			phys.vx = 0;
		}

		if (phys.x < -max) {
			phys.x = -max;
			phys.vx = 0;
		}

		if (phys.y > max) {
			phys.y = max;
			phys.vy = 0;
		}

		if (phys.y < -max) {
			phys.y = -max;
			phys.vy = 0;
		}

		mat4 mat = mat4(1);

		mat = translate(mat, vec3(phys.x, phys.y, 0));

		mat = rotate(mat, rad, vec3(0, 0, 1));

		mat = translate(mat, vec3(-(float)width / 2, -(float)height / 2, 0));

		uint32_t i = 0;

		for (uint32_t x = 0; x < width; x++) {
			for (uint32_t y = 0; y < height; y++) {
				Tile* t = tiles[i];

				if (t != nullptr) {
					t->frame(universe, x, y, i, this, mat);
				} else {
					textures[i] = 0;
				}

				i++;
			}
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glUseProgram(universe->shipShader->id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

		glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uView"), 1, GL_FALSE, value_ptr(universe->viewMat));
		glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uMat"), 1, GL_FALSE, value_ptr(mat));

		glDrawElements(GL_TRIANGLES, len, GL_UNSIGNED_INT, 0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glUseProgram(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	Tile* Starship::get(int x, int y) {
		return tiles[x * height + y];
	}

	void Starship::set(int x, int y, Tile* tile) {
		tiles[x * height + y] = tile;
	}
}
