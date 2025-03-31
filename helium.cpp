#define NANOVG_GL3_IMPLEMENTATION
#include <argon.hpp>
#include "starship.hpp"
#include "stackTrace.hpp"
#include "nanovg_gl.h"
#include <sstream>
#include "particles.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace Ar;
using namespace He;
using namespace H;

static vector<Light> lights;
static vector<LineLight> lineLights;
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

			getStackTrace([](hushort id, char* name, char* file, huint line) {
				if (file != "unknown" && id > 1) {
					filesystem::path p(file);
					string fName = p.filename().string();

					if (fName != "exe_common.inl") {
						cout << "\t at " << fName << "." << name << ":" << line << endl;
					}
				}
				});
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

	void frame(StarshipShader* shader, GLFWwindow* window, huint x, huint y, huint i, Starship* ship, GLuint64* textures, mat4 mat, double delta, float zoom, float ar) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			textures[i] = bindlessOn;

			vec4 vec = mat * vec4(x + 0.5, y + 0.25, 0, 1);
			float tx = vec.x / 2 - 0.5, ty = vec.y / 2 - 0.5;

			lights.push_back(Light(hFrame->zoom / 2, (float)239 / 255, (float)217 / 255, (float)105 / 255, 1, tx, ty));

			static random_device rd;
			static mt19937 gen(rd());
			static uniform_real_distribution<float> check(0, 0.1), color(0, 1), size(0.1, 0.25), alpha(2, 5), life(0.25, 1);

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
					-ship->vx * hFrame->zoom / ship->speed,
					-ship->vy * hFrame->zoom / ship->speed,
					life(gen)
				));
			}
		} else {
			textures[i] = bindlessOff;
		}
	}
};

class ProjectileShader : public Shader {
public:
	GLuint vao, vPos;

	ProjectileShader() : Shader() {
		attach(GL_VERTEX_SHADER, loadRes(L"projectile.vert", RT_RCDATA));
		attach(GL_FRAGMENT_SHADER, loadRes(L"projectile.frag", RT_RCDATA));

		link();

		glCreateVertexArrays(1, &vao);

		glBindVertexArray(vao);

		glGenBuffers(1, &vPos);
		glBindBuffer(GL_ARRAY_BUFFER, vPos);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);

		GLubyte* vBuf = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		vBuf[0] = 0;
		vBuf[1] = 0;
		vBuf[2] = 0;
		vBuf[3] = 1;

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_BYTE, false, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class TurretComponent : public Component {
public:
	GLuint vao, vPos, ebo, uTex, texBase, texGun;
	GLuint64 bindlessBase;
	GLuint64* bindlessGun;

	TurretComponent() {
		glCreateVertexArrays(1, &vao);

		glBindVertexArray(vao);

		glGenBuffers(1, &vPos);
		glBindBuffer(GL_ARRAY_BUFFER, vPos);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);

		GLubyte* vBuf = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		vBuf[0] = 0;
		vBuf[1] = 0;
		vBuf[2] = 0;
		vBuf[3] = 1;
		vBuf[4] = 1;
		vBuf[5] = 1;
		vBuf[6] = 1;
		vBuf[7] = 0;

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glVertexAttribPointer(0, 2, GL_UNSIGNED_BYTE, false, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);

		GLubyte* eBuf = (GLubyte*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		eBuf[0] = 0;
		eBuf[1] = 1;
		eBuf[2] = 2;
		eBuf[3] = 2;
		eBuf[4] = 3;
		eBuf[5] = 0;

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		glGenBuffers(1, &uTex);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		bindlessGun = (GLuint64*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint64), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &texBase);

		glBindTexture(GL_TEXTURE_2D, texBase);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &texGun);

		glBindTexture(GL_TEXTURE_2D, texGun);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void uploadBase(const void* data, GLenum format, GLenum type) {
		//glMakeTextureHandleNonResidentARB(bindless);
		glBindTexture(GL_TEXTURE_2D, texBase);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindlessBase = glGetTextureHandleARB(texBase);
		glMakeTextureHandleResidentARB(bindlessBase);
	}

	void uploadGun(const void* data, GLenum format, GLenum type) {
		//glMakeTextureHandleNonResidentARB(bindless);
		glBindTexture(GL_TEXTURE_2D, texGun);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		bindlessGun[0] = glGetTextureHandleARB(texGun);
		glMakeTextureHandleResidentARB(bindlessGun[0]);
	}

	void uploadBase(string img) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		if (w != 16 || h != 16) {
			cerr << "Component texture must be 16x16" << endl;
			exit(-1);
		}

		uploadBase(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
	}

	void uploadGun(string img) {
		int w = 0, h = 0, c = 0;

		stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

		if (w != 16 || h != 16) {
			cerr << "Component texture must be 16x16" << endl;
			exit(-1);
		}

		uploadGun(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
	}

	void frame(StarshipShader* shader, GLFWwindow* window, huint x, huint y, huint i, Starship* ship, GLuint64* textures, mat4 mat, double delta, float zoom, float ar) {
		textures[i] = bindlessBase;

		mat = translate(mat, vec3(x + 0.5, y - (float)1 / 16, 0));

		double cx, cy;
		glfwGetCursorPos(window, &cx, &cy);

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);

		vec4 pos = mat * vec4(0, 0, 0, 1);

		float px = (pos.x / 2 + 0.5) * w, py = (pos.y / 2 + 0.5) * h;

		float ang = (float)atan2(-(cx - px), (h - cy) - py) - radians(ship->rot);

		mat = rotate(mat, ang, vec3(0, 0, 1));

		mat = translate(mat, vec3(-0.5, 0, 0));

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glUseProgram(shader->id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

		glUniform1i(glGetUniformLocation(shader->id, "uTex"), 0);
		glUniformMatrix4fv(glGetUniformLocation(shader->id, "uMat"), 1, GL_FALSE, value_ptr(mat));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glUseProgram(0);
		glBindVertexArray(0);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
			vec4 barrel = mat * vec4(0.5, (float)9 / 16, 0, 1);
			lights.push_back(Light(zoom / 4, 1, 0.1, 0.2, 1, barrel.x / 2 - 0.5, barrel.y / 2 - 0.5));

			mat = scale(mat, vec3(1, 100000, 1));

			vec3 end = mat * vec4(0.5, 1, 0, 1);

			lineLights.push_back(LineLight(zoom / 20, 1, 0.1, 0.2, 1.5, barrel.x / 2 - 0.5, barrel.y / 2 - 0.5, end.x / 2 - 0.5, end.y / 2 - 0.5));
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
	hFrame = new HFrame(1280, 800, "helium");
	hFrame->children.addFirst(new FPSCounter());

	NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_DEBUG);

	if (!vg) {
		exit(-1);
	}

	string font = loadRes(L"times.ttf", RT_RCDATA);
	nvgCreateFontMem(vg, "Times New Roman", (unsigned char*)font.data(), font.length(), true);

	Starship ship = Starship(5, 8);

	stbi_set_flip_vertically_on_load(true);

	BasicComponent plating;

	plating.upload(loadRes(L"structure/plating.png", RT_RCDATA));

	EngineComponent engine;

	engine.uploadOn(loadRes(L"engine/small/on.png", RT_RCDATA));
	engine.uploadOff(loadRes(L"engine/small/off.png", RT_RCDATA));

	TurretComponent turret;

	turret.uploadBase(loadRes(L"weapons/pds/turret/base.png", RT_RCDATA));
	turret.uploadGun(loadRes(L"weapons/pds/turret/gun.png", RT_RCDATA));

	uint i = 0;

	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	i += 3;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;

	i += 1;
	//ship.comps[i++] = &engine;
	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
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
	ship.comps[i++] = &plating;
	ship.comps[i++] = &turret;

	i += 1;
	//ship.comps[i++] = &engine;
	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	i += 2;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;

	ship.comps[i++] = &engine;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;
	//ship.comps[i++] = &plating;

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

	GLuint lightBuf;
	glGenBuffers(1, &lightBuf);
	GLuint lineBuf;
	glGenBuffers(1, &lineBuf);

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
		lineLights.clear();

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

		ship.render(&shader, hFrame->zoom, hFrame->handle, delta, ar);

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
		glUniform2fv(glGetUniformLocation(post.id, "uAspectRatio"), 1, (GLfloat*)value_ptr(ar > 1 ? vec2(1, ar) : ar < 1 ? vec2(ar, 1) : vec2(1, 1)));
		//glUniform2ui(glGetUniformLocation(post.id, "uAliasSize"), aliasW, aliasH);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuf);
		glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(Light), lights.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuf);
		glUniform1ui(glGetUniformLocation(post.id, "uNumLights"), lights.size());

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineBuf);
		glBufferData(GL_SHADER_STORAGE_BUFFER, lineLights.size() * sizeof(LineLight), lineLights.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lineBuf);
		glUniform1ui(glGetUniformLocation(post.id, "uNumLineLights"), lineLights.size());

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