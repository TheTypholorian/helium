#define NANOVG_GL3_IMPLEMENTATION
#include <iostream>
#include <argon.hpp>
#include "starship.hpp"
#include "stackTrace.hpp"
#include "nanovg_gl.h"
#include <sstream>
#include "particles.hpp"

using namespace std;
using namespace Ar;
using namespace He;
using namespace H;

static vector<Light> lights;
static LinkedList<Particle> particles;

const static int aliasW = 2, aliasH = 2;

class HFrame : public GLFrame {
public:
	float zoom = 0.05;
	GLuint fbo, tex;

	HFrame(int width, int height, const char* title = "", GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) : GLFrame(width, height, title, monitor, share) {
		glfwMakeContextCurrent(handle);
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

			bool* print = new bool(false);

			getStackTrace([print](hushort id, char* name, char* file, huint line) {
				if (id > 1) {
					print[0] = true;
				}

				if (print[0]) {
					cout << "\t" << id << ": " << name << " (" << file << ") @ " << line << endl;
				}
				});

			delete[] print;
			}, nullptr);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width * aliasW, height * aliasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			cerr << "Framebuffer not complete, status: 0x" << toHex(status) << endl;
			exit(-1);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void scroll(GLFWwindow* win, double x, double y) {
		zoom *= 1 + (float)(y * 0.1);

		if (zoom < 0.001) {
			zoom = 0.001;
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

static HFrame* hFrame;

class FPSCounter : public GLComponent {
public:
	double last = 0, fps = 60;

	void renderThis(NVGcontext* vg, Rect2D<hushort> rect) {
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

class EngineComponent : public Component {
public:
	GLuint texOff, texOn;
	GLuint64 bindlessOff, bindlessOn;

	EngineComponent() {
		glCreateTextures(GL_TEXTURE_2D, 1, &texOff);

		glBindTexture(GL_TEXTURE_2D, texOff);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &texOn);

		glBindTexture(GL_TEXTURE_2D, texOn);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void uploadOff(const void* data, GLenum format, GLenum type) {
		//glMakeTextureHandleNonResidentARB(bindless);
		glBindTexture(GL_TEXTURE_2D, texOff);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindlessOff = glGetTextureHandleARB(texOff);
		glMakeTextureHandleResidentARB(bindlessOff);
	}

	void uploadOn(const void* data, GLenum format, GLenum type) {
		//glMakeTextureHandleNonResidentARB(bindless);
		glBindTexture(GL_TEXTURE_2D, texOn);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindlessOn = glGetTextureHandleARB(texOn);
		glMakeTextureHandleResidentARB(bindlessOn);
	}

	void uploadOff(string img) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		if (w != 16 || h != 16) {
			cerr << "Component texture must be 16x16" << endl;
			exit(-1);
		}

		uploadOff(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
	}

	void uploadOn(string img) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		if (w != 16 || h != 16) {
			cerr << "Component texture must be 16x16" << endl;
			exit(-1);
		}

		uploadOn(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
	}

	void frame(GLFWwindow* window, huint x, huint y, huint i, Starship* ship, GLuint64* textures, mat4 mat, double delta) {
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			textures[i] = bindlessOn;

			vec4 vec = mat * vec4(x + 0.5, y + 0.25, 0, 1);
			float tx = vec.x / 2 - 0.5, ty = vec.y / 2 - 0.5;

			lights.push_back(Light(hFrame->zoom / 2, (float)239 / 255, (float)217 / 255, (float)105 / 255, 1, tx, ty));

			static random_device rd;
			static mt19937 gen(rd());
			static uniform_real_distribution<float> check(0, 0.1);
			static uniform_real_distribution<float> size(0.1, 0.25);
			static uniform_real_distribution<float> color(0, 1);
			static uniform_real_distribution<float> alpha(2, 5);
			static uniform_real_distribution<float> vel(-0.01, 0.01);
			static uniform_real_distribution<float> life(0.25, 1);

			if (check(gen) <= delta) {
				float col = color(gen);
				particles.addFirst(Particle(
					size(gen),
					((float)206 / 255) * col + ((float)255 / 255) * (1 - col),
					((float)175 / 255) * col + ((float)247 / 255) * (1 - col),
					((float)0 / 255) * col + ((float)216 / 255) * (1 - col),
					alpha(gen),
					tx,
					ty,
					vel(gen),
					vel(gen),
					life(gen)
				));
			}
		} else {
			textures[i] = bindlessOff;
		}
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
	hFrame = new HFrame(1280, 800, "helium :3");
	hFrame->children.addFirst(new FPSCounter());

	NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_DEBUG);

	if (!vg) {
		exit(-1);
	}

	string font = loadRes(L"times.ttf", RT_RCDATA);
	nvgCreateFontMem(vg, "Times New Roman", (unsigned char*)font.data(), font.length(), true);

	Starship ship = Starship(3, 8);

	stbi_set_flip_vertically_on_load(true);

	BasicComponent plating = BasicComponent();

	plating.upload(loadRes(L"structure/plating.png", RT_RCDATA));

	EngineComponent engine = EngineComponent();

	engine.uploadOn(loadRes(L"engine/small/on.png", RT_RCDATA));
	engine.uploadOff(loadRes(L"engine/small/off.png", RT_RCDATA));

	ship.comps[0] = &engine;
	ship.comps[1] = &plating;
	ship.comps[2] = &plating;
	ship.comps[3] = &plating;
	ship.comps[4] = &plating;
	//ship.comps[5] = &plating;
	//ship.comps[6] = &plating;
	//ship.comps[7] = &plating;

	//ship.comps[8] = &engine;
	//ship.comps[9] = &plating;
	ship.comps[10] = &plating;
	ship.comps[11] = &plating;
	ship.comps[12] = &plating;
	ship.comps[13] = &plating;
	ship.comps[14] = &plating;
	ship.comps[15] = &plating;

	ship.comps[16] = &engine;
	ship.comps[17] = &plating;
	ship.comps[18] = &plating;
	ship.comps[19] = &plating;
	ship.comps[20] = &plating;
	//ship.comps[21] = &plating;
	//ship.comps[22] = &plating;
	//ship.comps[23] = &plating;

	StarshipShader shader;

	Shader post;
	post.attach(GL_VERTEX_SHADER, loadRes(L"post.vert", RT_RCDATA));
	post.attach(GL_FRAGMENT_SHADER, loadRes(L"post.frag", RT_RCDATA));
	post.link();

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLint), nullptr, GL_STATIC_DRAW);
	GLint* buf = (GLint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	buf[0] = -1;
	buf[1] = -1;

	buf[2] = 1;
	buf[3] = -1;

	buf[4] = 1;
	buf[5] = 1;

	buf[6] = 1;
	buf[7] = 1;

	buf[8] = -1;
	buf[9] = 1;

	buf[10] = -1;
	buf[11] = -1;
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 2, GL_INT, false, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	GLuint ssbo;
	glGenBuffers(1, &ssbo);

	double last = 0;

	int width = 0, height = 0;
	glfwGetFramebufferSize(hFrame->handle, &width, &height);
	hFrame->framebufferSize(hFrame->handle, width, height);

	while (!glfwWindowShouldClose(hFrame->handle)) {
		double now = glfwGetTime();
		double delta = now - last;
		last = now;

		glfwPollEvents();

		lights.clear();

		width = 0, height = 0;
		glfwGetFramebufferSize(hFrame->handle, &width, &height);
		float ar = (float)width / height;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glClearColor(0.05, 0.05, 0.05, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, hFrame->fbo);

		glBindTexture(GL_TEXTURE_2D, hFrame->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width * aliasW, height * aliasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		glViewport(0, 0, width * aliasW, height * aliasH);

		glClearColor(0, 0, 0, 0.1);
		glClear(GL_COLOR_BUFFER_BIT);

		ship.render(shader, hFrame->zoom, hFrame->handle, delta, ar);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		for (auto node = particles.first; node != particles.last; node = node->next) {
			if (!node->t.frame(hFrame->zoom, delta, ar, &lights)) {
				node->unlink();
			}
		}

		glBindVertexArray(vao);
		glUseProgram(post.id);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hFrame->tex);

		glUniform1i(glGetUniformLocation(post.id, "uTex"), 0);
		glUniform1ui(glGetUniformLocation(post.id, "uNumLights"), lights.size());
		glUniform2fv(glGetUniformLocation(post.id, "uAspectRatio"), 1, (GLfloat*)value_ptr(ar > 1 ? vec2(1, ar) : ar < 1 ? vec2(ar, 1) : vec2(1, 1)));
		//glUniform2ui(glGetUniformLocation(post.id, "uAliasSize"), aliasW, aliasH);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(Light), lights.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(0);
		glBindVertexArray(0);

		glViewport(0, 0, width, height);

		nvgBeginFrame(vg, width, height, 1);

		hFrame->render(vg);

		nvgEndFrame(vg);

		glfwSwapBuffers(hFrame->handle);
	}

	glfwDestroyWindow(hFrame->handle);

	glfwTerminate();

	return 0;
}