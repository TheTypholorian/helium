#pragma once

#include "universe.hpp"
#include "io.hpp"
#include "shaders.hpp"
#include "sfx.hpp"
#include "physics.hpp"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

namespace He {
	Universe::Universe(GLFrame* frame) : frame(frame) {
		frame->children.addFirst(this);

		glfwMakeContextCurrent(frame->handle);
		vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);

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

		GLuint rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width * aliasW, height * aliasH);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			cerr << "Framebuffer incomplete, status: " << status << endl;
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

	void Universe::startFrame() {
		double now = glfwGetTime();
		delta = now - last;
		last = now;

		lights.clear();
		//lineLights.clear();

		int width, height;
		glfwGetFramebufferSize(frame->handle, &width, &height);

		aRatio = (float)width / height;

		viewMat = scale(mat4(1), aRatio > 1 ? vec3(zoom, aRatio * zoom, zoom) : vec3(aRatio * zoom, zoom, zoom));

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		glClearColor(0.05, 0.05, 0.05, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width * aliasW, height * aliasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		glViewport(0, 0, width * aliasW, height * aliasH);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//for (OrbitalMass* mass : masses) {
		//	for (OrbitalMass* other : masses) {
		//		if (mass != other) {
		//			mass->update(other, this);
		//		}
		//	}
		//}

		for (PhysicsObject* phys : objects) {
			phys->frame(this);
		}
	}

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
			light.render(this);
		}
	}

	void Universe::scroll(GLFWwindow* win, double x, double y) {
		zoom *= 1 + (float)(y * 0.1);

		if (zoom < 0.001) {
			zoom = 0.001;
		} else if (zoom > 0.5) {
			zoom = 0.5;
		}

		GLComponent::scroll(win, x, y);
	}

	void Universe::key(GLFWwindow* win, int key, int scancode, int action, int mods) {
		switch (key) {
		case GLFW_KEY_EQUAL:
		{
			zoom = 0.05;
		}
		};

		GLComponent::key(win, key, scancode, action, mods);
	}
}
