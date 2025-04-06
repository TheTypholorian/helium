#pragma once

#include "main.hpp"
#include "sfx.hpp"
#include "argon.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define aliasW 2
#define aliasH 2

using namespace Ar;
using namespace glm;

namespace He {
	struct Universe : public GLComponent {
	public:
		GLFrame* frame;
		NVGcontext* vg;
		double last = 0, delta = 0;
		float aRatio = 1, zoom = 0.05;
		mat4 viewMat = mat4(1);
		vector<Light> lights;
		//vector<LineLight> lineLights;
		vector<PhysicsObject*> objects;
		LinkedList<Particle> particles = LinkedList<Particle>();

		Shader* postShader;
		StarshipShader* shipShader;
		GLuint fbo, tex, vao, vbo, lightBuf, lineBuf;

		Universe(GLFrame* frame);

		void startFrame();

		void postFrame();

		void scroll(GLFWwindow* win, double x, double y);

		void key(GLFWwindow* win, int key, int scancode, int action, int mods);
	};
}
