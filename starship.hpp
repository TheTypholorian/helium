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
		GLuint tex;
		GLuint64 bindless;

		Component() {
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

		virtual void frame(huint x, huint y, huint i, Starship* ship, GLuint64* textures) {
			textures[i] = bindless;
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

		void render(StarshipShader shader, mat4 mat) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, uTex);

			GLuint64* textures = (GLuint64*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

			huint i = 0;

			for (huint x = 0, dx = 0; x < width; x++, dx += compSize) {
				for (huint y = 0, dy = 0; y < height; y++, dy += compSize) {
					Component* c = comps[i];

					if (c != nullptr) {
						c->frame(x, y, i, this, textures);
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