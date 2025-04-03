#include "starship.hpp"
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

	void renderThis(NVGcontext* vg, Rect2D<uint16_t> rect) {
		double now = glfwGetTime();
		double delta = now - last;
		last = now;

		double current = 1 / delta;
		double a = 2 / fps;
		fps = (a * current) + ((1 - a) * fps);

		nvgFontSize(vg, 24);
		nvgFontFace(vg, "Times New Roman");
		nvgFillColor(vg, gold3);
		nvgTextAlign(vg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
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

	//universe.frame->children.addFirst(new FPSCounter());
	universe.frame->children.addFirst(&universe);

	Starship ship(5, 8);

	stbi_set_flip_vertically_on_load(true);

	PlatingComponent plating;

	EngineComponent engine;

	TurretComponent turret;

	uint32_t i = 0;

	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;
	i += 2;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;

	i += 1;
	//ship.comps[i++] = &engine;
	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;
	i += 1;
	//ship.comps[i++] = &plating;

	i += 1;
	//ship.comps[i++] = &engine;
	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;

	i += 1;
	//ship.comps[i++] = &engine;
	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;
	i += 1;
	//ship.comps[i++] = &plating;

	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;

	//ship.resize(9, 13);

	while (!glfwWindowShouldClose(universe.frame->handle)) {
		glfwPollEvents();

		universe.startFrame();

		ship.render(&universe);

		universe.postFrame();

		int width, height;
		glfwGetFramebufferSize(universe.frame->handle, &width, &height);

		glViewport(0, 0, width, height);

		nvgBeginFrame(universe.vg, width, height, 1);

		universe.frame->render(universe.vg);

		nvgEndFrame(universe.vg);

		glfwSwapBuffers(universe.frame->handle);
	}

	glfwDestroyWindow(universe.frame->handle);

	glfwTerminate();

	return 0;
}