#pragma once

#include "argon.hpp"
#include "io.hpp"
#include "bitMath.hpp"
#include <random>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

using namespace std;
using namespace Ar;
using namespace H;
using namespace glm;

namespace He {
	static const huint compSize = 16;

	class Starship;

	class Component {
	public:
		virtual void frame(GLFWwindow* window, huint x, huint y, huint i, Starship* ship, GLuint64* textures, mat4 mat) = 0;
	};

	class BasicComponent : public Component {
	public:
		GLuint tex;
		GLuint64 bindless;

		BasicComponent() {
			glCreateTextures(GL_TEXTURE_2D, 1, &tex);

			glBindTexture(GL_TEXTURE_2D, tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void upload(const void* data, GLenum format, GLenum type) {
			//glMakeTextureHandleNonResidentARB(bindless);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, compSize, compSize, 0, format, type, data);
			glBindTexture(GL_TEXTURE_2D, 0);
			bindless = glGetTextureHandleARB(tex);
			glMakeTextureHandleResidentARB(bindless);
		}

		void upload(string img) {
			int w = 0, h = 0, c = 0;

			stbi_uc* data = stbi_load_from_memory((stbi_uc*)img.data(), img.length(), &w, &h, &c, 0);

			if (w != 16 || h != 16) {
				cerr << "Component texture must be 16x16" << endl;
				exit(-1);
			}

			upload(data, c == 1 ? GL_RED : c == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE);
		}

		void frame(GLFWwindow* window, huint x, huint y, huint i, Starship* ship, GLuint64* textures, mat4 mat) {
			textures[i] = bindless;
		}
	};

	struct Light {
	public:
		GLfloat rad, r, g, b, x, y;

		Light(GLfloat rad, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y) : rad(rad), r(r), g(g), b(b), x(x), y(y) {
		}
	};

	class StarshipShader : public Shader {
	public:
		StarshipShader() : Shader() {
			attach(GL_VERTEX_SHADER, loadRes(L"starship.vert", RT_RCDATA));
			attach(GL_FRAGMENT_SHADER, loadRes(L"starship.frag", RT_RCDATA));

			link();
		}
	};

	class Starship {
	public:
		const huint width, height, len;
		float rot = 0, x = 0, y = 0, vx = 0, vy = 0, speed = 10;
		GLuint vao, vPos, ebo, uTex;
		Component** comps;

		Starship(const huint width, const huint height) : width(width), height(height), comps(new Component*[width * height]()), len(width* height * 6) {
			glCreateVertexArrays(1, &vao);

			glBindVertexArray(vao);

			glGenBuffers(1, &vPos);
			glBindBuffer(GL_ARRAY_BUFFER, vPos);
			glBufferData(GL_ARRAY_BUFFER, width * height * 8 * sizeof(GLushort), nullptr, GL_STATIC_DRAW);

			GLushort* vBuf = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			huint i = 0;

			for (huint x = 0; x < width; x++) {
				for (huint y = 0; y < height; y++) {
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
			huint j = 0;

			for (huint x = 0; x < width; x++) {
				for (huint y = 0; y < height; y++) {
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
			glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(GLuint64), nullptr, GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		virtual void render(StarshipShader shader, float zoom, GLFWwindow* window, double delta, float ar) {
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
				rot += delta * 90;
			}
			
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
				rot -= delta * 90;
			}

			float acc = 0;

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				acc += speed;
			}
			
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				acc -= speed;
			}

			float rad = radians(rot), rad90 = radians(rot + 90);

			vx += cos(rad90) * acc * delta;
			vy += sin(rad90) * acc * delta;
			x += vx * delta;
			y += vy * delta;

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

			mat = scale(mat, ar > 1 ? vec3(1, ar, 1) : ar < 1 ? vec3(ar, 1, 1) : vec3(1, 1, 1));

			mat = scale(mat, vec3(zoom));

			mat = translate(mat, vec3(x, y, 0));

			mat = rotate(mat, rad, vec3(0, 0, 1));

			mat = translate(mat, vec3(-(float)width / 2, -(float)height / 2, 0));

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);

			GLuint64* textures = (GLuint64*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

			huint i = 0;

			for (huint x = 0, dx = 0; x < width; x++, dx += compSize) {
				for (huint y = 0, dy = 0; y < height; y++, dy += compSize) {
					Component* c = comps[i];

					if (c != nullptr) {
						c->frame(window, x, y, i, this, textures, mat);
					} else {
						textures[i] = 0;
					}

					i++;
				}
			}

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			glBindVertexArray(vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glUseProgram(shader.id);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, uTex);

			glUniform1i(glGetUniformLocation(shader.id, "uTex"), 0);
			glUniformMatrix4fv(glGetUniformLocation(shader.id, "uMat"), 1, GL_FALSE, value_ptr(mat));

			glDrawElements(GL_TRIANGLES, len, GL_UNSIGNED_INT, 0);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

			glUseProgram(0);
			glBindVertexArray(0);
		}
	};
}