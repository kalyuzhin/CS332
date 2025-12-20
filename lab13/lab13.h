#ifndef CS332_LAB13_H
#define CS332_LAB13_H


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Window.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define DEG2RAD(x) ((x) * 3.14159265f / 180.0f)

namespace lab13 {
	struct Vertex {
		float position[3];
		float texCoord[2];
	};

	struct Model {
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint ebo = 0;
		GLuint texture = 0;
		GLsizei indexCount = 0;
	};

	struct Planet {
		Model* model;
		float orbitRadius;
		float orbitAngle;
		float orbitSpeed;
		float selfAngle;
		float selfSpeed;
		float scale;
	};

	struct Camera {
		float yaw = -90.0f;
		float pitch = 0.0f;
		float fov = 45.0f;


		float pos[3] = { 0.0f, 0.0f, 60.0f };
		float front[3] = { 0.0f, 0.0f, -1.0f };
		float up[3] = { 0.0f, 1.0f, 0.0f };
	};

	inline void updateCameraFront(Camera& cam) {
		float cy = cosf(DEG2RAD(cam.yaw)), sy = sinf(DEG2RAD(cam.yaw));
		float cp = cosf(DEG2RAD(cam.pitch)), sp = sinf(DEG2RAD(cam.pitch));
		cam.front[0] = cy * cp;
		cam.front[1] = sp;
		cam.front[2] = sy * cp;
	}

	inline void handleCameraMovement(Camera& cam, GLFWwindow* window, float speed) {
		float dx = 0, dy = 0, dz = 0;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dz += speed;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dz -= speed;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx += speed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx -= speed;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) dy += speed;
		if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) dy -= speed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) cam.yaw -= 0.1f;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.yaw += 0.1f;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) cam.pitch += 0.1f;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) cam.pitch -= 0.1f;

		if (cam.pitch > 89.0f) cam.pitch = 89.0f;
		if (cam.pitch < -89.0f) cam.pitch = -89.0f;

		updateCameraFront(cam);


		float right[3] = {
		cam.front[2] * cam.up[1] - cam.front[1] * cam.up[2],
		cam.front[0] * cam.up[2] - cam.front[2] * cam.up[0],
		cam.front[1] * cam.up[0] - cam.front[0] * cam.up[1]
		};
		float len = sqrtf(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
		for (int i = 0; i < 3; ++i) right[i] /= len;


		for (int i = 0; i < 3; ++i) {
			cam.pos[i] += cam.front[i] * dz;
			cam.pos[i] += right[i] * dx;
		}
		cam.pos[1] += dy;
	}

	bool loadOBJ(const std::string& path, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
		std::vector<float> temp_vertices;
		std::vector<float> temp_uvs;
		std::vector<unsigned int> vertexIndices, uvIndices;
		std::ifstream file(path);
		if (!file.is_open()) return false;
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string type;
			iss >> type;
			if (type == "v") {
				float x, y, z; iss >> x >> y >> z;
				temp_vertices.insert(temp_vertices.end(), { x, y, z });
			}
			else if (type == "vt") {
				float u, v; iss >> u >> v;
				temp_uvs.insert(temp_uvs.end(), { u, v });
			}
			else if (type == "f") {
				std::vector<std::string> faceVertices;
				std::string part;
				while (iss >> part) {
					faceVertices.push_back(part);
				}

				for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
					// Process triangle: v0, vi, vi+1
					for (int j = 0; j < 3; ++j) {
						std::string& vertex = faceVertices[(j == 0) ? 0 : ((j == 1) ? i : i + 1)];
						auto slash1 = vertex.find('/');
						auto slash2 = vertex.find('/', slash1 + 1);
						unsigned vi = std::stoi(vertex.substr(0, slash1));
						unsigned ti = (slash1 != std::string::npos && slash2 != slash1 + 1) ?
							std::stoi(vertex.substr(slash1 + 1, slash2 - slash1 - 1)) : 1;
						vertexIndices.push_back(vi);
						uvIndices.push_back(ti);
					}
				}
			}
		}
		for (size_t i = 0; i < vertexIndices.size(); ++i) {
			Vertex v;
			unsigned vi = vertexIndices[i], ti = uvIndices[i];
			for (int j = 0; j < 3; ++j)
				v.position[j] = temp_vertices[(vi - 1) * 3 + j];
			for (int j = 0; j < 2; ++j)
				v.texCoord[j] = temp_uvs[(ti - 1) * 2 + j];
			vertices.push_back(v);
			indices.push_back(static_cast<unsigned int>(indices.size()));
		}
		return true;
	}

	GLuint loadTexture(const std::string& path) {
		int w, h, ch;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
		if (!data) return 0;
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		GLenum format = (ch == 3) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);
		return tex;
	}

	GLuint compileShader(GLenum type, const char* src) {
		GLuint sh = glCreateShader(type);
		glShaderSource(sh, 1, &src, nullptr);
		glCompileShader(sh);
		return sh;
	}

	GLuint createProgram(const char* vs, const char* fs) {
		GLuint prog = glCreateProgram();
		GLuint v = compileShader(GL_VERTEX_SHADER, vs);
		GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
		glAttachShader(prog, v); glAttachShader(prog, f);
		glLinkProgram(prog);
		glDeleteShader(v); glDeleteShader(f);
		return prog;
	}

	int run_lab13() {
		if (!glfwInit()) return -1;
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
		GLFWwindow* window = glfwCreateWindow(1920, 1080, "Solar System", nullptr, nullptr);
		if (!window) { glfwTerminate(); return -1; }
		glfwMakeContextCurrent(window);
#ifndef __APPLE__
		if (glewInit() != GLEW_OK) return -1;
#endif
		glEnable(GL_DEPTH_TEST);

		const char* vs = R"(
			#version 330 core
			layout(location = 0) in vec3 aPos;
			layout(location = 1) in vec2 aTex;
			uniform mat4 uModel, uView, uProj;
			out vec2 TexCoord;
			void main() {
				gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
				TexCoord = aTex;
			}
		)";

		const char* fs = R"(
			#version 330 core
			in vec2 TexCoord;
			out vec4 FragColor;
			uniform sampler2D uTexture;
			void main() {
				FragColor = texture(uTexture, TexCoord);
			}
		)";
		GLuint program = createProgram(vs, fs);
		GLint locModel = glGetUniformLocation(program, "uModel");
		GLint locView = glGetUniformLocation(program, "uView");
		GLint locProj = glGetUniformLocation(program, "uProj");

		std::vector<Vertex> sunVerts;
		std::vector<unsigned int> sunInds;
		std::string absolutePathSun = "C:\\Users\\nikit\\Documents\\GitHub\\CS332\\models\\duck.obj";
		loadOBJ(absolutePathSun, sunVerts, sunInds);
		Model sunModel;
		glGenVertexArrays(1, &sunModel.vao);
		glGenBuffers(1, &sunModel.vbo);
		glGenBuffers(1, &sunModel.ebo);
		glBindVertexArray(sunModel.vao);
		glBindBuffer(GL_ARRAY_BUFFER, sunModel.vbo);
		glBufferData(GL_ARRAY_BUFFER, sunVerts.size() * sizeof(Vertex), sunVerts.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sunModel.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sunInds.size() * sizeof(unsigned int), sunInds.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
		sunModel.indexCount = sunInds.size();
		sunModel.texture = loadTexture("C:\\Users\\nikit\\Documents\\GitHub\\CS332\\models\\duck.jpg");

		//Model* model;
		//float orbitRadius;
		//float orbitAngle;
		//float orbitSpeed;
		//float selfAngle;
		//float selfSpeed;
		//float scale;
		Planet sun = { &sunModel, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 0.5f };

		std::vector<Vertex> verts;
		std::vector<unsigned int> inds;
		std::string absolutePathModel = "C:\\Users\\nikit\\Documents\\GitHub\\CS332\\models\\bird.obj";
		//loadOBJ("models/bird.obj", verts, inds);
		loadOBJ(absolutePathModel, verts, inds);
		Model m;
		glGenVertexArrays(1, &m.vao);
		glGenBuffers(1, &m.vbo);
		glGenBuffers(1, &m.ebo);
		glBindVertexArray(m.vao);
		glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		glEnableVertexAttribArray(1);
		m.indexCount = inds.size();
		std::string absolutePathText = "C:\\Users\\nikit\\Documents\\GitHub\\CS332\\models\\bird.jpg";
		m.texture = loadTexture(absolutePathText);
		//m.texture = loadTexture("models/bird.jpg");

		std::vector<Planet> planets;
		for (int i = 0; i < 5; ++i) {
			planets.push_back(Planet{ &m, 10.0f + 10 * i, 72.0f * i, 20.0f - 2 * i, 0, 30.0f + 5 * i, 1.0f - 0.1f * i });
		}
		Camera cam;
		auto last = std::chrono::high_resolution_clock::now();

		while (!glfwWindowShouldClose(window)) {
			auto now = std::chrono::high_resolution_clock::now();
			float dt = std::chrono::duration<float>(now - last).count();
			last = now;

			handleCameraMovement(cam, window, dt * 20.0f);
			sun.selfAngle += sun.selfSpeed * dt;
			for (auto& p : planets) {
				p.orbitAngle += p.orbitSpeed * dt;
				p.selfAngle += p.selfSpeed * dt;
			}

			glClearColor(0.3f, 0.3f, 0.3f, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(program);

			float aspect = 800.f / 600.f;
			float f = 1.0f / tanf(DEG2RAD(cam.fov / 2));
			float proj[16] = {
				f / aspect,0,0,0,
				0,f,0,0,
				0,0,-1.002,-1,
				0,0,-0.2002,0
			};

			float target[3] = {
				cam.pos[0] + cam.front[0],
				cam.pos[1] + cam.front[1],
				cam.pos[2] + cam.front[2]
			};
			float z[3] = { cam.pos[0] - target[0], cam.pos[1] - target[1], cam.pos[2] - target[2] };
			float lenZ = sqrtf(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
			for (int i = 0; i < 3; ++i) z[i] /= lenZ;
			float x[3] = {
				cam.up[1] * z[2] - cam.up[2] * z[1],
				cam.up[2] * z[0] - cam.up[0] * z[2],
				cam.up[0] * z[1] - cam.up[1] * z[0]
			};
			float lenX = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
			for (int i = 0; i < 3; ++i) x[i] /= lenX;
			float y[3] = {
				z[1] * x[2] - z[2] * x[1],
				z[2] * x[0] - z[0] * x[2],
				z[0] * x[1] - z[1] * x[0]
			};
			float view[16] = {
				x[0],y[0],z[0],0,
				x[1],y[1],z[1],0,
				x[2],y[2],z[2],0,
				-(x[0] * cam.pos[0] + x[1] * cam.pos[1] + x[2] * cam.pos[2]),
				-(y[0] * cam.pos[0] + y[1] * cam.pos[1] + y[2] * cam.pos[2]),
				-(z[0] * cam.pos[0] + z[1] * cam.pos[1] + z[2] * cam.pos[2]),
				1.0f
			};

			glUniformMatrix4fv(locProj, 1, GL_FALSE, proj);
			glUniformMatrix4fv(locView, 1, GL_FALSE, view);

			float angleY = DEG2RAD(sun.selfAngle);
			float cosY = cosf(angleY);
			float sinY = sinf(angleY);
			float s = sun.scale;

			float modelSun[16] = {
				s * cosY,  0, -s * sinY, 0,
				s * sinY,  0,  s * cosY, 0,
				0,         s,       0,   0,
				0,         0,       0,   1
			};
			glUniformMatrix4fv(locModel, 1, GL_FALSE, modelSun);
			glBindTexture(GL_TEXTURE_2D, sun.model->texture);
			glBindVertexArray(sun.model->vao);
			glDrawElements(GL_TRIANGLES, sun.model->indexCount, GL_UNSIGNED_INT, 0);

			for (auto& p : planets) {
				float x = cosf(DEG2RAD(p.orbitAngle)) * p.orbitRadius;
				float z = sinf(DEG2RAD(p.orbitAngle)) * p.orbitRadius;
				float a = DEG2RAD(p.selfAngle);
				float cosA = cosf(a);
				float sinA = sinf(a);
				float model[16] = {
					p.scale * cosA, 0, -p.scale * sinA, 0,
					p.scale * sinA, 0, p.scale * cosA, 0,
					0, p.scale, 0, 0,
					x, 0, z, 1
				};
				/*float model[16] = {
					p.scale,0,0,0,
					0,p.scale,0,0,
					0,0,p.scale,0,
					x,0,z,1
				};*/
				glUniformMatrix4fv(locModel, 1, GL_FALSE, model);
				glBindTexture(GL_TEXTURE_2D, m.texture);
				glBindVertexArray(m.vao);
				glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		glfwDestroyWindow(window);
		glfwTerminate();
		return 0;
	}
}

#endif