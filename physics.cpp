#pragma once

#include "physics.hpp"
#include "universe.hpp"

namespace He {
	PhysicsObject::PhysicsObject(float x, float y, float vx, float vy, float mass) : x(x), y(y), vx(vx), vy(vy), mass(mass) {}

	PhysicsObject::PhysicsObject(float x, float y, float mass) : x(x), y(y), vx(0), vy(0), mass(mass) {}

	PhysicsObject::PhysicsObject(float mass) : x(0), y(0), vx(0), vy(0), mass(mass) {}

	void PhysicsObject::update(PhysicsObject* other, Universe* universe) {
		if (other->x != x && other->y != y) {
			const float G = (6.67430e-11);

			float diffX = other->x - x;
			float diffY = other->y - y;
			float distSq = diffX * diffX + diffY * diffY;
			float dDist = sqrt(distSq);

			float force = G * mass * other->mass / distSq;

			vx += force / mass * diffX / dDist * universe->delta;
			vy += force / mass * diffY / dDist * universe->delta;
		}
	}

	void PhysicsObject::frame(Universe* universe) {
		x += vx * universe->delta;
		y += vy * universe->delta;
	}
}
