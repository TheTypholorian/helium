#pragma once

#include "main.hpp"
#include "physics.hpp"
#include "argon.hpp"

namespace He {
	struct Starship {
	public:
		uint32_t width, height, len;
		PhysicsObject phys = PhysicsObject(5);
		float rot = 0, speed = 5;
		GLuint vao, vPos, ebo, uTex;
		GLuint64* textures;
		Tile** tiles;

		Starship(const uint32_t width, const uint32_t height);

		void resize(uint32_t width, uint32_t height);

		void render(Universe* universe);

		Tile* get(int x, int y);

		void set(int x, int y, Tile* comp);
	};
}
