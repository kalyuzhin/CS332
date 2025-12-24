#ifndef CS332_TRIANGLE_H
#define CS332_TRIANGLE_H

#include <SFML/Window.hpp>
#include <optional>
#include <iostream>
#include <vector>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION

#include <OpenGL/gl3.h>

#else
#include <GL/glew.h>
#endif

namespace greenTriangle {
    GLuint Program = 0;
    GLuint VBO = 0;
    GLuint VAO = 0;
    GLint Attrib_vertex = -1;

    struct Vertex {
        GLfloat x;
        GLfloat y;
    };

    const char* VertexShaderSource = R"(
#version 150 core
in vec2 coord;

void main()
{
    gl_Position = vec4(coord.x,coord.y, 0.0, 1.0);
}
)";

    const char* FragShaderSource = R"(
#version 150 core
out vec4 color;

void main()
{
    color = vec4(0.0, 1.0, 0.0, 1.0);
}
)";

    void checkOpenGLerror() {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: 0x" << std::hex << err << std::dec << '\n';
        }
    }

    void ShaderLog(GLuint shader) {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 1) {
            std::vector<char> infoLog(infoLogLen);
            GLsizei charsWritten = 0;
            glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog.data());
            std::cout << "InfoLog:\n" << infoLog.data() << std::endl;
        }
    }

    void InitShader() {
        GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vShader, 1, &VertexShaderSource, nullptr);
        glCompileShader(vShader);
        ShaderLog(vShader);

        GLint status = GL_FALSE;
        glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
        if (!status) {
            std::cerr << "Vertex shader compilation failed\n";
            return;
        }

        GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fShader, 1, &FragShaderSource, nullptr);
        glCompileShader(fShader);
        ShaderLog(fShader);

        glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
        if (!status) {
            std::cerr << "Fragment shader compilation failed\n";
            return;
        }

        Program = glCreateProgram();
        glAttachShader(Program, vShader);
        glAttachShader(Program, fShader);
        glLinkProgram(Program);

        GLint link_ok = GL_FALSE;
        glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cerr << "Program link failed\n";
            return;
        }

        const char* attr_name = "coord";
        Attrib_vertex = glGetAttribLocation(Program, attr_name);
        if (Attrib_vertex == -1) {
            std::cerr << "Could not bind attribute " << attr_name << '\n';
            return;
        }

        glDeleteShader(vShader);
        glDeleteShader(fShader);

        checkOpenGLerror();
    }

    void InitBuffers() {
        Vertex triangle[3] = {
                {-1.0f, -1.0f},
                {0.0f,  1.0f},
                {1.0f,  -1.0f}
        };

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);

        glEnableVertexAttribArray(Attrib_vertex);
        glVertexAttribPointer(
            Attrib_vertex,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(0)
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        checkOpenGLerror();
    }

    void Init() {
        glClearColor(0.f, 0.f, 0.f, 1.f);
        InitShader();
        InitBuffers();
    }

    void Draw() {
        glUseProgram(Program);
        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glUseProgram(0);

        checkOpenGLerror();
    }

    void Release() {
        if (Program != 0) {
            glDeleteProgram(Program);
            Program = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
    }

    int run_green_triangle() {
        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.majorVersion = 3;
        settings.minorVersion = 2;
        settings.attributeFlags = sf::ContextSettings::Core;

        sf::Window window(
            sf::VideoMode({ 600u, 600u }),
            "Green Triangle (macOS)",
            sf::Style::Default,
            sf::State::Windowed,
            settings
        );

        window.setVerticalSyncEnabled(true);
        window.setActive(true);

        glViewport(0, 0, window.getSize().x, window.getSize().y);

        Init();

        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
                else if (const auto* resized =
                    event->getIf<sf::Event::Resized>()) {
                    glViewport(0, 0, resized->size.x, resized->size.y);
                }
            }

            glClear(GL_COLOR_BUFFER_BIT);
            Draw();
            window.display();
        }

        Release();
        return 0;
    }
}

#endif //CS332_TRIANGLE_H
