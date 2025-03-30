#pragma once

#include "starship.hpp"

using namespace glm;

namespace He {
	struct Particle {
	public:
		const float maxLife;
		float rad, r, g, b, a, x, y, vx, vy, life;

		Particle(float rad, float r, float g, float b, float a, float x, float y, float vx = 0, float vy = 0, float life = 0) : rad(rad), r(r), g(g), b(b), a(a), x(x), y(y), vx(vx), vy(vy), life(life), maxLife(life) {}

		bool frame(float zoom, double delta, float ar, vector<Light>* lights) {
			if (maxLife != 0) {
				life -= delta;

				if (life <= 0) {
					return false;
				}
			}

			x += vx * delta;
			y += vy * delta;

			float f = maxLife == 0 ? 1 : life / maxLife;

			lights->push_back(Light(zoom * rad * f, r * f, g * f, b * f, a * f, x, y));

			return true;
		}
	};
}
