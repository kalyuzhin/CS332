//
// Created by Марк Калюжин on 29.11.2025.
//

#ifndef CS332_LAB11_H
#define CS332_LAB11_H

#include <SFML/Window.hpp>
#include <optional>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION

#include <OpenGL/gl3.h>

#else
#include <GL/glew.h>
#endif

namespace lab11 {
    struct Vertex {
        GLfloat x;
        GLfloat y;
        GLfloat r;
        GLfloat g;
        GLfloat b;
    };

    struct Shape {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLenum primitive = GL_TRIANGLES;
        GLsizei vertexCount = 0;
    };

    GLuint gProgramFlatConst = 0;
    GLuint gProgramFlatUniform = 0;
    GLuint gProgramGradient = 0;
    GLint gUniformColorLocation = -1;

    Shape gQuad;
    Shape gFan;
    Shape gPentagon;

    enum class FillMode {
        FlatConst = 0,
        FlatUniform = 1,
        Gradient = 2
    };

    void checkOpenGLerror() {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: 0x" << std::hex << err << std::dec << '\n';
        }
    }

    void shaderLog(GLuint shader) {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 1) {
            std::vector<char> infoLog(infoLogLen);
            GLsizei charsWritten = 0;
            glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog.data());
            std::cerr << "Shader log:\n" << infoLog.data() << '\n';
        }
    }

    GLuint compileShader(GLenum type, const char *src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            std::cerr << "Shader compilation failed\n";
            shaderLog(shader);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint createProgram(GLuint vertexShader, const char *fragSource) {
        GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource);
        if (!fragShader) {
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);

        glAttachShader(program, fragShader);

        glBindAttribLocation(program, 0, "coord");
        glBindAttribLocation(program, 1, "vertexColor");

        glLinkProgram(program);

        GLint link_ok = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cerr << "Program link failed\n";
            GLint infoLogLen = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
            if (infoLogLen > 1) {
                std::vector<char> infoLog(infoLogLen);
                GLsizei charsWritten = 0;
                glGetProgramInfoLog(program, infoLogLen, &charsWritten, infoLog.data());
                std::cerr << "Program log:\n" << infoLog.data() << '\n';
            }
            glDeleteProgram(program);
            glDeleteShader(fragShader);
            return 0;
        }

        glDetachShader(program, fragShader);
        glDeleteShader(fragShader);

        return program;
    }

    void initShaders() {
        const char *vertexShaderSource = R"(
#version 150 core
in vec2 coord;
in vec3 vertexColor;
out vec3 vertColor;

void main()
{
    gl_Position = vec4(coord, 0.0, 1.0);
    vertColor = vertexColor;
}
)";

        const char *fragFlatConst = R"(
#version 150 core
out vec4 color;

void main()
{
    color = vec4(0.0, 0.8, 0.2, 1.0);
}
)";

        const char *fragFlatUniform = R"(
#version 150 core
uniform vec4 uColor;
out vec4 color;

void main()
{
    color = uColor;
}
)";

        const char *fragGradient = R"(
#version 150 core
in vec3 vertColor;
out vec4 color;

void main()
{
    color = vec4(vertColor, 1.0);
}
)";

        GLuint vShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        if (!vShader) {
            std::cerr << "Vertex shader compilation failed\n";
            return;
        }

        gProgramFlatConst = createProgram(vShader, fragFlatConst);
        gProgramFlatUniform = createProgram(vShader, fragFlatUniform);
        gProgramGradient = createProgram(vShader, fragGradient);

        glDeleteShader(vShader);

        if (!gProgramFlatConst || !gProgramFlatUniform || !gProgramGradient) {
            std::cerr << "Program creation failed\n";
            return;
        }

        glUseProgram(gProgramFlatUniform);
        gUniformColorLocation = glGetUniformLocation(gProgramFlatUniform, "uColor");
        glUseProgram(0);

        checkOpenGLerror();
    }

    void initShape(Shape &shape, const std::vector<Vertex> &vertices, GLenum primitive) {
        shape.primitive = primitive;
        shape.vertexCount = static_cast<GLsizei>(vertices.size());

        glGenVertexArrays(1, &shape.vao);
        glBindVertexArray(shape.vao);

        glGenBuffers(1, &shape.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                     vertices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
                0,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void *>(0)
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
                1,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void *>(2 * sizeof(GLfloat))
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        checkOpenGLerror();
    }

    void initShapes() {
        constexpr float PI = 3.1415926535f;

        std::vector<Vertex> quadVertices = {
                {-0.9f, -0.1f, 1.0f, 0.0f, 0.0f},
                {-0.4f, -0.1f, 0.0f, 1.0f, 0.0f},
                {-0.4f, 0.4f,  0.0f, 0.0f, 1.0f},
                {-0.9f, 0.4f,  1.0f, 1.0f, 0.0f}
        };

        initShape(gQuad, quadVertices, GL_TRIANGLE_FAN);

        std::vector<Vertex> fanVertices;
        float cx = 0.5f;
        float cy = 0.3f;
        float radiusFan = 0.5f;

        fanVertices.push_back({cx, cy, 1.0f, 1.0f, 1.0f});

        const int segments = 10;
        for (int i = 0; i <= segments; ++i) {
            float angleDeg = -60.0f + 120.0f * static_cast<float>(i) / static_cast<float>(segments);
            float a = angleDeg * PI / 180.0f;
            float x = cx + radiusFan * std::cos(a);
            float y = cy + radiusFan * std::sin(a);

            float t = static_cast<float>(i) / static_cast<float>(segments);
            float r = 1.0f - t;
            float g = t;
            float b = 0.5f + 0.5f * std::sin(a);

            fanVertices.push_back({x, y, r, g, b});
        }

        initShape(gFan, fanVertices, GL_TRIANGLE_FAN);

        std::array<Vertex, 5> basePentagon{};
        float centerX = 0.0f;
        float centerY = -0.4f;
        float radiusPentagon = 0.35f;

        for (int i = 0; i < 5; ++i) {
            float angleDeg = 90.0f + 72.0f * static_cast<float>(i);
            float a = angleDeg * PI / 180.0f;
            float x = centerX + radiusPentagon * std::cos(a);
            float y = centerY + radiusPentagon * std::sin(a);

            float r = (i == 0 || i == 1) ? 1.0f : 0.0f;
            float g = (i == 1 || i == 2 || i == 4) ? 1.0f : 0.0f;
            float b = (i == 2 || i == 3) ? 1.0f : 0.0f;

            basePentagon[i] = {x, y, r, g, b};
        }

        std::vector<Vertex> pentagonVertices;
        pentagonVertices.reserve(9);
        pentagonVertices.push_back(basePentagon[0]);
        pentagonVertices.push_back(basePentagon[1]);
        pentagonVertices.push_back(basePentagon[2]);

        pentagonVertices.push_back(basePentagon[0]);
        pentagonVertices.push_back(basePentagon[2]);
        pentagonVertices.push_back(basePentagon[3]);

        pentagonVertices.push_back(basePentagon[0]);
        pentagonVertices.push_back(basePentagon[3]);
        pentagonVertices.push_back(basePentagon[4]);

        initShape(gPentagon, pentagonVertices, GL_TRIANGLES);

        checkOpenGLerror();
    }

    void drawShape(const Shape &shape) {
        glBindVertexArray(shape.vao);
        glDrawArrays(shape.primitive, 0, shape.vertexCount);
        glBindVertexArray(0);
    }

    void releaseShapes() {
        auto destroyShape = [](Shape &s) {
            if (s.vbo != 0) {
                glDeleteBuffers(1, &s.vbo);
                s.vbo = 0;
            }
            if (s.vao != 0) {
                glDeleteVertexArrays(1, &s.vao);
                s.vao = 0;
            }
        };

        destroyShape(gQuad);
        destroyShape(gFan);
        destroyShape(gPentagon);
    }

    int run_lab11() {
        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.majorVersion = 3;
        settings.minorVersion = 2;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Core;

        sf::Window window(
                sf::VideoMode({800u, 600u}),
                "OpenGL + SFML: quad, fan, pentagon",
                sf::Style::Default,
                sf::State::Windowed,
                settings
        );

        window.setVerticalSyncEnabled(true);
        window.setActive(true);

#ifndef __APPLE__
        glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed\n";
        return 1;
    }
#endif

        glViewport(0, 0, window.getSize().x, window.getSize().y);

        initShaders();
        initShapes();

        glClearColor(0.f, 0.f, 0.f, 1.f);

        FillMode mode = FillMode::FlatConst;

        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto *resized = event->getIf<sf::Event::Resized>()) {
                    glViewport(0, 0, resized->size.x, resized->size.y);
                } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    using Scan = sf::Keyboard::Scan;
                    if (keyPressed->scancode == Scan::Num1) {
                        mode = FillMode::FlatConst;
                    } else if (keyPressed->scancode == Scan::Num2) {
                        mode = FillMode::FlatUniform;
                    } else if (keyPressed->scancode == Scan::Num3) {
                        mode = FillMode::Gradient;
                    }
                }
            }

            glClear(GL_COLOR_BUFFER_BIT);

            switch (mode) {
                case FillMode::FlatConst:
                    glUseProgram(gProgramFlatConst);
                    drawShape(gQuad);
                    drawShape(gFan);
                    drawShape(gPentagon);
                    break;
                case FillMode::FlatUniform:
                    glUseProgram(gProgramFlatUniform);
                    if (gUniformColorLocation >= 0) {
                        glUniform4f(gUniformColorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
                        drawShape(gQuad);

                        glUniform4f(gUniformColorLocation, 0.0f, 1.0f, 0.0f, 1.0f);
                        drawShape(gFan);

                        glUniform4f(gUniformColorLocation, 0.0f, 0.4f, 1.0f, 1.0f);
                        drawShape(gPentagon);
                    }
                    break;
                case FillMode::Gradient:
                    glUseProgram(gProgramGradient);
                    drawShape(gQuad);
                    drawShape(gFan);
                    drawShape(gPentagon);
                    break;
            }

            glUseProgram(0);
            window.display();
        }

        releaseShapes();

        if (gProgramFlatConst) glDeleteProgram(gProgramFlatConst);
        if (gProgramFlatUniform) glDeleteProgram(gProgramFlatUniform);
        if (gProgramGradient) glDeleteProgram(gProgramGradient);

        return 0;
    }

}

#endif //CS332_LAB11_H
