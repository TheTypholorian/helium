#include <iostream>
#include <argon.hpp>
#include "starship.hpp"
#include "stackTrace.hpp"

using namespace std;
using namespace Ar;
using namespace He;
using namespace H;

class HFrame : public GLFrame {
public:
	float zoom = 0.05, rot = 0;

	HFrame(int width, int height, const char* title = "", GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) : GLFrame(width, height, title, monitor, share) {}

	void scroll(GLFWwindow* win, double x, double y) {
		zoom *= 1 + (float)(y * 0.1);

		if (zoom < 0.01) {
			zoom = 0.01;
		} else if (zoom > 0.5) {
			zoom = 0.5;
		}

		GLFrame::scroll(win, x, y);
	}

	void key(GLFWwindow* win, int key, int scancode, int action, int mods) {
		switch (key) {
		case GLFW_KEY_EQUAL:
		{
			zoom = 0.05;
		}
		};

		GLFrame::key(win, key, scancode, action, mods);
	}
};

int main() {
	glfwSetErrorCallback([](int code, const char* desc) {
		cout << "GLFW Error 0x" << toHex(code) << ": " << desc << endl;
		});

	if (!glfwInit()) {
		cerr << "Unable to init glfw" << endl;
		return -1;
	}

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	HFrame frame(1280, 800, "helium :3");

	glfwMakeContextCurrent(frame.handle);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		cerr << "Unable to init glew" << endl;
		return -1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		cout << message << endl;

		bool* print = new bool(false);

		getStackTrace([print](hushort id, char* name, char* file, huint line) {
			if (name[0] == 'g' && name[1] == 'l') {
				print[0] = true;
			}

			//if (print[0]) {
			cout << "\t" << id << ": " << name << " (" << file << ") @ " << line << endl;
			//}
			});

		delete[] print;
		}, nullptr);

	Starship ship = Starship(1, 2);

	stbi_set_flip_vertically_on_load(true);

	Component plating = Component();

	plating.upload(loadRes(L"structure/plating.png", RT_RCDATA));

	Component engine = Component();

	engine.upload(loadRes(L"engine/small/on.png", RT_RCDATA));

	ship.comps[0] = &engine;
	ship.comps[1] = &plating;

	StarshipShader shader;

	double last = 0;

	while (!glfwWindowShouldClose(frame.handle)) {
		glClearColor(0.05, 0.05, 0.05, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		double now = glfwGetTime();
		double delta = now - last;
		last = now;

		int width = 0, height = 0;
		glfwGetFramebufferSize(frame.handle, &width, &height);
		glViewport(0, 0, width, height);

		if (glfwGetKey(frame.handle, GLFW_KEY_LEFT) == GLFW_PRESS) {
			frame.rot += delta * 90;
		} else if (glfwGetKey(frame.handle, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			frame.rot -= delta * 90;
		}

		mat4 mat = mat4(1);

		float ar = (float)width / height;
		mat = scale(mat, ar > 1 ? vec3(1, ar, 1) : ar < 1 ? vec3(ar, 1, 1) : vec3(1, 1, 1));

		mat = rotate(mat, radians(frame.rot), vec3(0, 0, 1));

		mat = scale(mat, vec3(frame.zoom));

		mat = translate(mat, vec3(-(float)ship.width / 2, -(float)ship.height / 2, 0));

		ship.render(shader, mat);

		glfwSwapBuffers(frame.handle);
		glfwPollEvents();
	}

	glfwDestroyWindow(frame.handle);

	glfwTerminate();

	return 0;
}