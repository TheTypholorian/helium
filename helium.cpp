#include "bitMath.hpp"
#include "universe.hpp"
#include "starship.hpp"
#include "tiles.hpp"
#include "io.hpp"

#include "stb_image.h"
#include "stackTrace.hpp"
#include <sstream>
#include <filesystem>
#include <iostream>

using namespace std;
using namespace Ar;
using namespace He;
using namespace H;

class FPSCounter : public GLComponent {
public:
	double last = 0, fps = 60;
	Universe* universe;

	FPSCounter(Universe* universe) : universe(universe) {}

	void renderThis(NVGcontext* vg, Rect2D<uint16_t> rect) {
		double now = glfwGetTime();
		double delta = now - last;
		last = now;

		double current = 1 / delta;
		double a = 2 / fps;
		fps = (a * current) + ((1 - a) * fps);

		nvgFontSize(vg, 24);
		nvgFontFace(vg, "times");
		nvgTextAlign(vg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
		nvgFillColor(vg, gold3);
		stringstream text;
		text << "FPS: " << (int)fps;
		nvgText(vg, 0, 0, text.str().data(), nullptr);
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
	GLFrame* frame = new GLFrame(1280, 800, "helium");
	glfwSwapInterval(1);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		cerr << "Unable to init glew" << endl;
		exit(-1);
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		cout << message << endl;

		getStackTrace([](uint16_t id, char* name, char* file, uint32_t line) {
			if (file != "unknown" && id > 1) {
				filesystem::path p(file);
				string fName = p.filename().string();

				if (fName != "exe_common.inl") {
					cout << "\t at " << fName << "." << name << " line " << line << endl;
				}
			}
			});
		}, nullptr);

	Universe universe = Universe(frame);

	string font = loadRes(L"times.ttf", RT_RCDATA);
	nvgCreateFontMem(universe.vg, "times", (unsigned char*)font.data(), font.length(), false);

	//universe.frame->children.addFirst(new FPSCounter(&universe));
	//universe.masses.push_back(new OrbitalMass(0, 0, 0, 0, 5.97219e24));

	Starship ship(5, 8);

	universe.objects.push_back(&ship.phys);

	ship.phys.y = 0;
	ship.phys.vx = 0;
	ship.phys.mass = 5;

	stbi_set_flip_vertically_on_load(true);

	PlatingTile plating;

	EngineTile engine;

	TurretTile turret;

	ship.set(0, 0, &engine);
	ship.set(0, 1, &plating);
	ship.set(0, 2, &plating);
	ship.set(0, 3, &plating);
	ship.set(0, 4, &plating);
	ship.set(0, 5, &turret);

	ship.set(1, 1, &engine);
	ship.set(1, 2, &plating);
	ship.set(1, 3, &plating);
	ship.set(1, 4, &plating);
	ship.set(1, 5, &plating);
	ship.set(1, 6, &turret);

	ship.set(2, 1, &engine);
	ship.set(2, 2, &plating);
	ship.set(2, 3, &plating);
	ship.set(2, 4, &plating);
	ship.set(2, 5, &plating);
	ship.set(2, 6, &plating);
	ship.set(2, 7, &turret);

	ship.set(3, 1, &engine);
	ship.set(3, 2, &plating);
	ship.set(3, 3, &plating);
	ship.set(3, 4, &plating);
	ship.set(3, 5, &plating);
	ship.set(3, 6, &turret);

	ship.set(4, 0, &engine);
	ship.set(4, 1, &plating);
	ship.set(4, 2, &plating);
	ship.set(4, 3, &plating);
	ship.set(4, 4, &plating);
	ship.set(4, 5, &turret);

	while (!glfwWindowShouldClose(universe.frame->handle)) {
		glfwPollEvents();

		universe.startFrame();

		ship.render(&universe);

		/*
		for (OrbitalMass* mass : universe.masses) {
			mat4 mat = mat4(1);

			if (mass->mass > 5000000) {
				mat = scale(mat, vec3(6378 * 2));
			} else {
				mat = scale(mat, vec3(500 * 2));
			}

			mat = translate(mat, vec3(mass->x - 1, mass->y - 1, 0));

			mat = scale(mat, vec3(2));

			universe.lights.push_back(Light(mat, 0.5, 0.7, 0.3, 10));
		}

		cout << ship.mass.x << " " << ship.mass.y << endl;
		*/

		universe.postFrame();

		int width, height, winWidth, winHeight;
		glfwGetFramebufferSize(universe.frame->handle, &width, &height);
		glfwGetWindowSize(universe.frame->handle, &winWidth, &winHeight);

		glViewport(0, 0, width, height);

		nvgBeginFrame(universe.vg, width, height, (float)width / winWidth);

		universe.frame->render(universe.vg);

		nvgEndFrame(universe.vg);

		glfwSwapBuffers(universe.frame->handle);
	}

	glfwDestroyWindow(universe.frame->handle);

	glfwTerminate();

	return 0;
}
