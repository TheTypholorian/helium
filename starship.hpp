#pragma once

#define NANOVG_GL3_IMPLEMENTATION
#include "argon.hpp"
#include "io.hpp"
#include "bitMath.hpp"
#include <random>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"
#include "nanovg_gl.h"

using namespace std;
using namespace Ar;
using namespace H;
using namespace glm;

namespace He {
	static const uint32_t compSize = 16, aliasW = 2, aliasH = 2;

	class Starship;

	struct Light;

	struct LineLight;

	struct Particle;

	class StarshipShader : public Shader {
	public:
		StarshipShader() : Shader() {
			attach(GL_VERTEX_SHADER, loadRes(L"starship.vert", RT_RCDATA));
			attach(GL_FRAGMENT_SHADER, loadRes(L"starship.frag", RT_RCDATA));

			link();
		}
	};

	struct Universe : public GLComponent {
	public:
		GLFrame* frame;
		NVGcontext* vg;
		double last = 0, delta = 0;
		float aRatio = 1, zoom = 0.05;
		vec2 uRatio = vec2(1);
		vector<Light> lights;
		vector<LineLight> lineLights;
		LinkedList<Particle> particles = LinkedList<Particle>();

		Shader* postShader;
		StarshipShader* shipShader;
		GLuint fbo, tex, vao, vbo, lightBuf, lineBuf;

		Universe(GLFrame* frame) : frame(frame) {
			glfwMakeContextCurrent(frame->handle);
			vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_DEBUG);

			string font = loadRes(L"times.ttf", RT_RCDATA);
			nvgCreateFontMem(vg, "Times New Roman", (unsigned char*)font.data(), font.length(), true);

			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			int width, height;
			glfwGetFramebufferSize(frame->handle, &width, &height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width * aliasW, height * aliasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				cerr << "Framebuffer incomplete, status: 0x" << toHex(status) << endl;
				exit(-1);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			postShader = new Shader();
			postShader->attach(GL_VERTEX_SHADER, loadRes(L"post.vert", RT_RCDATA));
			postShader->attach(GL_FRAGMENT_SHADER, loadRes(L"post.frag", RT_RCDATA));
			postShader->link();

			shipShader = new StarshipShader();

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

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

			glGenBuffers(1, &lightBuf);
			glGenBuffers(1, &lineBuf);
		}

		void startFrame() {
			double now = glfwGetTime();
			delta = now - last;
			last = now;

			lights.clear();
			lineLights.clear();

			int width, height;
			glfwGetFramebufferSize(frame->handle, &width, &height);

			aRatio = (float)width / height;
			uRatio = aRatio > 1 ? vec2(1, aRatio) : vec2(aRatio, 1);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);

			glClearColor(0.05, 0.05, 0.05, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width * aliasW, height * aliasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);

			glViewport(0, 0, width * aliasW, height * aliasH);

			glClearColor(0, 0, 0, 0.1);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		void postFrame();

		void scroll(GLFWwindow* win, double x, double y) {
			zoom *= 1 + (float)(y * 0.1);

			if (zoom < 0.001) {
				zoom = 0.001;
			} else if (zoom > 0.5) {
				zoom = 0.5;
			}

			GLComponent::scroll(win, x, y);
		}

		void key(GLFWwindow* win, int key, int scancode, int action, int mods) {
			switch (key) {
			case GLFW_KEY_EQUAL:
			{
				zoom = 0.05;
			}
			};

			GLComponent::key(win, key, scancode, action, mods);
		}
	};

	struct Light {
	public:
		mat4 mat;
		GLfloat r, g, b, a;

		Light(mat4 mat, GLfloat r, GLfloat g, GLfloat b, GLfloat a) : mat(mat), r(r), g(g), b(b), a(a) {}

		void render() {
			static GLuint vao = 0, vbo = 0, ebo = 0;
			static Shader shader;

			if (vao == 0) {
				glGenVertexArrays(1, &vao);

				glBindVertexArray(vao);

				glGenBuffers(1, &vbo);

				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);
				GLubyte* buf = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

				buf[0] = 0;
				buf[1] = 0;
				buf[2] = 1;
				buf[3] = 0;
				buf[4] = 1;
				buf[5] = 1;
				buf[6] = 0;
				buf[7] = 1;

				glUnmapBuffer(GL_ARRAY_BUFFER);
				glVertexAttribPointer(0, 2, GL_UNSIGNED_BYTE, false, 0, 0);
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glGenBuffers(1, &ebo);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), nullptr, GL_STATIC_DRAW);
				buf = (GLubyte*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

				buf[0] = 0;
				buf[1] = 1;
				buf[2] = 2;
				buf[3] = 2;
				buf[4] = 3;
				buf[5] = 0;

				glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				glBindVertexArray(0);

				shader.attach(GL_VERTEX_SHADER, loadRes(L"light.vert", RT_RCDATA));
				shader.attach(GL_FRAGMENT_SHADER, loadRes(L"light.frag", RT_RCDATA));
				shader.link();
			}

			glBindVertexArray(vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glUseProgram(shader.id);

			glUniformMatrix4fv(glGetUniformLocation(shader.id, "uMat"), 1, false, value_ptr(mat));
			glUniform4f(glGetUniformLocation(shader.id, "uCol"), r, g, b, a);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

			glUseProgram(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
	};

	struct LineLight {
	public:
		GLfloat rad, r, g, b, a, x1, y1, x2, y2;

		LineLight(GLfloat rad, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) : rad(rad), r(r), g(g), b(b), a(a), x1(x1), y1(y1), x2(x2), y2(y2) {}
	};

	struct Particle {
	public:
		const float maxLife;
		float rad, r, g, b, a, x, y, vx, vy, life;

		Particle(float rad, float r, float g, float b, float a, float x, float y, float vx = 0, float vy = 0, float life = 0) : rad(rad), r(r), g(g), b(b), a(a), x(x), y(y), vx(vx), vy(vy), life(life), maxLife(life) {}

		bool frame(Universe* universe) {
			if (maxLife != 0) {
				life -= universe->delta;

				if (life <= 0) {
					return false;
				}
			}

			x += vx * universe->delta;
			y += vy * universe->delta;

			float f = maxLife == 0 ? 1 : life / maxLife;

			//universe->lights.push_back(Light(universe->zoom * rad * f, r * f, g * f, b * f, a * f, x, y));

			return true;
		}
	};

	class Component;

	class Starship {
	public:
		uint32_t width, height, len;
		float rot = 0, x = 0, y = 0, vx = 0, vy = 0, speed = 5;
		GLuint vao, vPos, ebo, uTex;
		GLuint64* textures;
		Component** comps;

		Starship(const uint32_t width, const uint32_t height) : width(width), height(height), comps(new Component* [width * height]()), len(width* height * 6) {
			glGenVertexArrays(1, &vao);

			glBindVertexArray(vao);

			glGenBuffers(1, &vPos);
			glBindBuffer(GL_ARRAY_BUFFER, vPos);
			glBufferData(GL_ARRAY_BUFFER, width * height * 8 * sizeof(GLushort), nullptr, GL_STATIC_DRAW);

			GLushort* vBuf = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			uint32_t i = 0;

			for (GLushort x = 0; x < width; x++) {
				for (GLushort y = 0; y < height; y++) {
					vBuf[i++] = x;
					vBuf[i++] = y;

					vBuf[i++] = x;
					vBuf[i++] = y + 1;

					vBuf[i++] = x + 1;
					vBuf[i++] = y + 1;

					vBuf[i++] = x + 1;
					vBuf[i++] = y;
				}
			}

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, false, 0, 0);
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, len * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

			GLuint* eBuf = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

			i = 0;
			uint32_t j = 0;

			for (GLuint x = 0; x < width; x++) {
				for (GLuint y = 0; y < height; y++) {
					eBuf[i++] = j + 0;
					eBuf[i++] = j + 1;

					eBuf[i++] = j + 2;
					eBuf[i++] = j + 2;

					eBuf[i++] = j + 3;
					eBuf[i++] = j + 0;

					j += 4;
				}
			}

			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glBindVertexArray(0);

			glGenBuffers(1, &uTex);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
			GLuint size = width * height * sizeof(GLuint64);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			textures = (GLuint64*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		virtual void resize(uint32_t width, uint32_t height) {
			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vPos);
			glBufferData(GL_ARRAY_BUFFER, width * height * 8 * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

			GLuint* vBuf = (GLuint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			uint32_t i = 0;

			for (uint32_t x = 0; x < width; x++) {
				for (uint32_t y = 0; y < height; y++) {
					vBuf[i++] = x;
					vBuf[i++] = y;

					vBuf[i++] = x;
					vBuf[i++] = y + 1;

					vBuf[i++] = x + 1;
					vBuf[i++] = y + 1;

					vBuf[i++] = x + 1;
					vBuf[i++] = y;
				}
			}

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, false, 0, 0);
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, len * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

			GLuint* eBuf = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

			i = 0;
			uint32_t j = 0;

			for (uint32_t x = 0; x < width; x++) {
				for (uint32_t y = 0; y < height; y++) {
					eBuf[i++] = j + 0;
					eBuf[i++] = j + 1;

					eBuf[i++] = j + 2;
					eBuf[i++] = j + 2;

					eBuf[i++] = j + 3;
					eBuf[i++] = j + 0;

					j += 4;
				}
			}

			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glBindVertexArray(0);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			glDeleteBuffers(1, &uTex);
			glGenBuffers(1, &uTex);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
			GLuint size = width * height * sizeof(GLuint64);
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			textures = (GLuint64*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			Component** nComps = new Component * [width * height]();
			uint32_t oldW = this->width;
			uint32_t oldH = this->height;
			uint32_t minW = (oldW < width ? oldW : width);
			uint32_t minH = (oldH < height ? oldH : height);

			for (uint32_t y = 0, i = 0, j = 0; y < minH; y++, i += width, j += oldW) {
				for (uint32_t x = 0; x < minW; x++) {
					nComps[i + x] = comps[j + x];
				}
			}

			for (uint32_t i = 0, x = 0; x < width; x++) {
				for (uint32_t y = 0; y < height; y++, i++) {
					cout << x << " " << y << ": " << nComps[i] << endl;
				}
			}

			this->width = width;
			this->height = height;
			delete[] comps;
			comps = nComps;
		}

		virtual void render(Universe* universe);
	};

	class Component {
	public:
		virtual void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) = 0;
	};

	void Starship::render(Universe* universe) {
		if (glfwGetKey(universe->frame->handle, GLFW_KEY_A) == GLFW_PRESS) {
			rot += universe->delta * 90;
		}

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_D) == GLFW_PRESS) {
			rot -= universe->delta * 90;
		}

		float acc = 0;

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_W) == GLFW_PRESS) {
			acc += speed;
		}

		if (glfwGetKey(universe->frame->handle, GLFW_KEY_S) == GLFW_PRESS) {
			acc -= speed;
		}

		float rad = radians(rot), rad90 = radians(rot + 90);

		vx += cos(rad90) * acc * universe->delta;
		vy += sin(rad90) * acc * universe->delta;
		x += vx * universe->delta;
		y += vy * universe->delta;

		const float max = 50;

		if (x > max) {
			x = max;
			vx = 0;
		}

		if (x < -max) {
			x = -max;
			vx = 0;
		}

		if (y > max) {
			y = max;
			vy = 0;
		}

		if (y < -max) {
			y = -max;
			vy = 0;
		}

		mat4 mat = mat4(1);

		mat = scale(mat, vec3(universe->uRatio, 1));

		mat = scale(mat, vec3(universe->zoom));

		mat = translate(mat, vec3(x, y, 0));

		mat = rotate(mat, rad, vec3(0, 0, 1));

		mat = translate(mat, vec3(-(float)width / 2, -(float)height / 2, 0));

		uint32_t i = 0;

		for (uint32_t x = 0, dx = 0; x < width; x++, dx += compSize) {
			for (uint32_t y = 0, dy = 0; y < height; y++, dy += compSize) {
				Component* c = comps[i];

				if (c != nullptr) {
					c->frame(universe, x, y, i, this, mat);
				} else {
					textures[i] = 0;
				}

				i++;
			}
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glUseProgram(universe->shipShader->id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

		glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uMat"), 1, GL_FALSE, value_ptr(mat));

		glDrawElements(GL_TRIANGLES, len, GL_UNSIGNED_INT, 0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glUseProgram(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	class BasicComponent : public Component {
	public:
		GLuint tex;
		GLuint64 bindless = 0;

		BasicComponent() {
			glCreateTextures(GL_TEXTURE_2D, 1, &tex);

			glBindTexture(GL_TEXTURE_2D, tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		virtual void upload(const void* data, GLenum format, GLenum type) {
			//glMakeTextureHandleNonResidentARB(bindless);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
			glBindTexture(GL_TEXTURE_2D, 0);
			bindless = glGetTextureHandleARB(tex);
			glMakeTextureHandleResidentARB(bindless);
		}

		virtual void upload(string img) {
			int w = 0, h = 0, c = 0;

			stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

			if (w != 16 || h != 16) {
				cerr << "Component texture must be 16x16" << endl;
				exit(-1);
			}

			upload(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
		}

		virtual void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
			ship->textures[i] = bindless;
		}
	};

	class PlatingComponent : public BasicComponent {
	public:
		PlatingComponent() {
			upload(loadRes(L"structure/plating.png", RT_RCDATA));
		}
	};

	class MultiComponent : public Component {
	public:
		GLuint* tex;
		GLuint64* bindless;

		MultiComponent(int i) {
			tex = new GLuint[i];
			bindless = new GLuint64[i];
			glCreateTextures(GL_TEXTURE_2D, i, tex);
		}

		virtual void upload(const void* data, GLenum format, GLenum type, int i = 0) {
			glBindTexture(GL_TEXTURE_2D, tex[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);

			glMakeTextureHandleResidentARB(bindless[i] = glGetTextureHandleARB(tex[i]));
		}

		virtual void upload(string img, int i = 0) {
			int w = 0, h = 0, c = 0;

			stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

			if (w != 16 || h != 16) {
				cerr << "Component texture must be 16x16" << endl;
				exit(-1);
			}

			upload(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, i);
		}
	};

	class EngineComponent : public MultiComponent {
	public:
		EngineComponent() : MultiComponent(2) {
			upload(loadRes(L"engine/small/off.png", RT_RCDATA), false);
			upload(loadRes(L"engine/small/on.png", RT_RCDATA), true);
		}

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
			if (glfwGetKey(universe->frame->handle, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(universe->frame->handle, GLFW_KEY_S) == GLFW_PRESS) {
				ship->textures[i] = bindless[true];

				mat = translate(mat, vec3(x, y, 0));

				universe->lights.push_back(Light(mat, (float)239 / 255, (float)217 / 255, (float)105 / 255, 1));

				/*
				static random_device rd;
				static mt19937 gen(rd());
				static uniform_real_distribution<float> check(0, 0.05), color(0, 1), size(0.1, 0.25), alpha(2, 5), life(0.25, 1);

				if (check(gen) <= universe->delta) {
					float col = color(gen);
					universe->particles.addFirst(Particle(
						size(gen),
						((float)206 / 255) * col + ((float)255 / 255) * (1 - col),
						((float)175 / 255) * col + ((float)247 / 255) * (1 - col),
						((float)0 / 255) * col + ((float)216 / 255) * (1 - col),
						alpha(gen),
						tx,
						ty,
						-ship->vx * universe->zoom / ship->speed,
						-ship->vy * universe->zoom / ship->speed,
						life(gen)
					));
				}
				*/
			} else {
				ship->textures[i] = bindless[false];
			}
		}
	};

	class TurretComponent : public MultiComponent {
	public:
		GLuint vao, vPos, ebo, uTex;

		TurretComponent() : MultiComponent(2) {
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

			MultiComponent::upload(loadRes(L"weapons/pds/turret/base.png", RT_RCDATA), 0);
			MultiComponent::upload(loadRes(L"weapons/pds/turret/gun.png", RT_RCDATA), 1);
		}

		void upload(const void* data, GLenum format, GLenum type, int i = 0) {
			MultiComponent::upload(data, format, type, i);

			if (i == 1) {
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64), &bindless[1], GL_STATIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}
		}

		void frame(Universe* universe, uint32_t x, uint32_t y, uint32_t i, Starship* ship, mat4 mat) {
			ship->textures[i] = bindless[0];

			mat = translate(mat, vec3(x + 0.5, y - (float)1 / 16, 0));

			double cx, cy;
			glfwGetCursorPos(universe->frame->handle, &cx, &cy);

			int w, h;
			glfwGetFramebufferSize(universe->frame->handle, &w, &h);

			vec4 pos = mat * vec4(0, 0, 0, 1);

			float px = (pos.x / 2 + 0.5) * w, py = (pos.y / 2 + 0.5) * h;

			float ang = (float)atan2(-(cx - px), (h - cy) - py) - radians(ship->rot);
			constexpr float max = radians((float)70);
			bool canShoot = true;

			if (ang > max) {
				ang = max;
				canShoot = false;
			}

			if (ang < -max) {
				ang = -max;
				canShoot = false;
			}

			mat = rotate(mat, ang, vec3(0, 0, 1));

			mat = translate(mat, vec3(-0.5, 0, 0));

			glBindVertexArray(vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glUseProgram(universe->shipShader->id);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

			glUniformMatrix4fv(glGetUniformLocation(universe->shipShader->id, "uMat"), 1, GL_FALSE, value_ptr(mat));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

			glUseProgram(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			if (canShoot && glfwGetMouseButton(universe->frame->handle, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
				mat = translate(mat, vec3(0, (float)1 / 16, 0));

				universe->lights.push_back(Light(mat, 1, 0.1, 0.2, 1));

				//mat = scale(mat, vec3(1, 100000, 1));

				//vec3 end = mat * vec4(0.5, (float)7 / 16, 0, 1);

				//universe->lineLights.push_back(LineLight(universe->zoom / 20, 1, 0.1, 0.2, 1.5, barrel.x / 2 - 0.5, barrel.y / 2 - 0.5, end.x / 2 - 0.5, end.y / 2 - 0.5));
			}
		}
	};

	void Universe::postFrame() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		for (auto node = particles.first; node != particles.last; node = node->next) {
			if (!node->t.frame(this)) {
				node->unlink();
			}
		}

		glBindVertexArray(vao);
		glUseProgram(postShader->id);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);

		glUniform1i(glGetUniformLocation(postShader->id, "uTex"), 0);

		/*
		glUniform2fv(glGetUniformLocation(postShader->id, "uAspectRatio"), 1, (GLfloat*)value_ptr(uRatio));

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuf);
		glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(Light), lights.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuf);
		glUniform1ui(glGetUniformLocation(postShader->id, "uNumLights"), lights.size());

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineBuf);
		glBufferData(GL_SHADER_STORAGE_BUFFER, lineLights.size() * sizeof(LineLight), lineLights.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lineBuf);
		glUniform1ui(glGetUniformLocation(postShader->id, "uNumLineLights"), lineLights.size());
		*/

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(0);
		glBindVertexArray(0);

		int width, height;
		glfwGetFramebufferSize(frame->handle, &width, &height);
		glViewport(0, 0, width, height);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		for (Light light : lights) {
			light.render();
		}
	}
}