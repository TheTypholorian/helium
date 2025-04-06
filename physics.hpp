#pragma once

#include "main.hpp"

namespace He {
	class PhysicsObject {
	public:
		float x, y, vx, vy, mass;

		PhysicsObject(float x, float y, float vx, float vy, float mass);

		PhysicsObject(float x, float y, float mass);

		PhysicsObject(float mass);

		void update(PhysicsObject* other, Universe* universe);

		void frame(Universe* universe);
	};
}
