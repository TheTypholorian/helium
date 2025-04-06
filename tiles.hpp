#pragma once

#include "main.hpp"
#include "argon.hpp"
#include "glm/glm.hpp"

using namespace glm;

namespace He {
	class Tile {
	public:
		virtual void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) = 0;
	};

	class BasicTile : public Tile {
	public:
		GLuint tex;
		GLuint64 bindless = 0;

		BasicTile();

		virtual void upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type);

		virtual void upload(string img);

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat);
	};

	class PlatingTile : public BasicTile {
	public:
		PlatingTile();
	};

	class MultiTile : public Tile {
	public:
		GLuint* tex;
		GLuint64* bindless;

		MultiTile(int i);

		virtual void upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type, int i = 0);

		virtual void upload(string img, int i = 0);

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat);
	};

	class EngineTile : public MultiTile {
	public:
		EngineTile();

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat);
	};

	class TurretTile : public MultiTile {
	public:
		GLuint vao, vPos, ebo, uTex;

		TurretTile();

		void upload(const void* data, GLsizei width, GLsizei height, GLenum format, GLenum type, int i = 0);

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat);
	};
}
