#ifndef CS332_LAB12_H
#define CS332_LAB12_H

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
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

namespace lab12 {

    struct Vertex3D {
        GLfloat x;
        GLfloat y;
        GLfloat z;
        GLfloat r;
        GLfloat g;
        GLfloat b;
    };

    struct TextureVertex {
        GLfloat x;
        GLfloat y;
        GLfloat z;
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat u;
        GLfloat v;
    };

    struct TexCoordVertex {
        GLfloat x;
        GLfloat y;
        GLfloat z;
        GLfloat u;
        GLfloat v;
    };

    struct Shape3D {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLenum primitive = GL_TRIANGLES;
        GLsizei indexCount = 0;
    };

    struct ModelData {
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;
    };

    enum class FigureMode {
        Tetrahedron = 0,
        CubeColor = 1,
        CubeTexture = 2,
        Circle = 3
    };

    class Transform3D {
    public:
        float angleX = 0.0f;
        float angleY = 0.0f;
        float angleZ = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        float mixFactor = 1.0f;

        std::array<GLfloat, 16> matrix{};

        void createMatrix() {
            GLfloat rotationX[16] = {
                1, 0, 0, 0,
                0, cos(angleX), -sin(angleX), 0,
                0, sin(angleX), cos(angleX), 0,
                0, 0, 0, 1
            };

            GLfloat rotationY[16] = {
                cos(angleY), 0, sin(angleY), 0,
                0, 1, 0, 0,
                -sin(angleY), 0, cos(angleY), 0,
                0, 0, 0, 1
            };

            GLfloat rotationZ[16] = {
                cos(angleZ), -sin(angleZ), 0, 0,
                sin(angleZ), cos(angleZ), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };

            GLfloat scaleMatrix[16] = {
                scaleX, 0, 0, 0,
                0, scaleY, 0, 0,
                0, 0, scaleZ, 0,
                0, 0, 0, 1
            };

            GLfloat temp[16];
            for (int i = 0; i < 16; i++) {
                temp[i] =
                    rotationY[i % 4] * rotationX[i / 4 * 4] +
                    rotationY[i % 4 + 4] * rotationX[i / 4 * 4 + 1] +
                    rotationY[i % 4 + 8] * rotationX[i / 4 * 4 + 2] +
                    rotationY[i % 4 + 12] * rotationX[i / 4 * 4 + 3];
            }

            GLfloat temp1[16];
            for (int i = 0; i < 16; i++) {
                temp1[i] =
                    rotationZ[i % 4] * temp[i / 4 * 4] +
                    rotationZ[i % 4 + 4] * temp[i / 4 * 4 + 1] +
                    rotationZ[i % 4 + 8] * temp[i / 4 * 4 + 2] +
                    rotationZ[i % 4 + 12] * temp[i / 4 * 4 + 3];
            }

            for (int i = 0; i < 16; i++) {
                matrix[i] =
                    scaleMatrix[i % 4] * temp1[i / 4 * 4] +
                    scaleMatrix[i % 4 + 4] * temp1[i / 4 * 4 + 1] +
                    scaleMatrix[i % 4 + 8] * temp1[i / 4 * 4 + 2] +
                    scaleMatrix[i % 4 + 12] * temp1[i / 4 * 4 + 3];
            }
        }

        void reset(bool textureMode = false) {
            angleX = 0.0f;
            angleY = 0.0f;
            angleZ = 0.0f;
            scaleX = 1.0f;
            scaleY = 1.0f;
            scaleZ = 1.0f;
            mixFactor = textureMode ? 0.5f : 1.0f;
            createMatrix();
        }
    };

    class Renderer3D {
    private:
        GLuint program = 0;
        GLuint texture1 = 0;
        GLuint texture2 = 0;
        GLuint texture3 = 0;
        GLint modelLoc = -1;
        GLint mixFactorLoc = -1;
        Shape3D currentShape{};
        FigureMode currentMode = FigureMode::Tetrahedron;
        Transform3D transform;
        int circleSegments = 100;
        const float PI = 3.14159265358979323846f;

        const char* vertexShaderColor = R"(
            #version 330 core
            layout(location = 0) in vec3 coord;
            layout(location = 1) in vec3 color;

            uniform mat4 model;
            out vec3 fragColor;

            void main() {
                gl_Position = model * vec4(coord, 1.0);
                fragColor = color;
            }
        )";

        const char* fragmentShaderColor = R"(
            #version 330 core
            in vec3 fragColor;
            out vec4 color;

            void main() {
                color = vec4(fragColor, 1.0);
            }
        )";

        const char* vertexShaderColorTex = R"(
            #version 330 core
            layout(location = 0) in vec3 coord;
            layout(location = 1) in vec3 color;
            layout(location = 2) in vec2 texCoord;

            uniform mat4 model;
            out vec3 fragColor;
            out vec2 fragTexCoord;

            void main() {
                gl_Position = model * vec4(coord, 1.0);
                fragColor = color;
                fragTexCoord = texCoord;
            }
        )";

        const char* fragmentShaderColorTex = R"(
            #version 330 core
            in vec3 fragColor;
            in vec2 fragTexCoord;
            out vec4 color;

            uniform sampler2D texture1;
            uniform float mixFactor;

            void main() {
                vec4 texColor = texture(texture1, fragTexCoord);
                color = mix(texColor, vec4(fragColor, 1.0), mixFactor);
            }
        )";

        const char* vertexShaderTexTex = R"(
            #version 330 core
            layout(location = 0) in vec3 coord;
            layout(location = 1) in vec2 texCoord;

            uniform mat4 model;
            out vec2 fragTexCoord;

            void main() {
                gl_Position = model * vec4(coord, 1.0);
                fragTexCoord = texCoord;
            }
        )";

        const char* fragmentShaderTexTex = R"(
            #version 330 core
            in vec2 fragTexCoord;
            out vec4 color;

            uniform sampler2D texture1;
            uniform sampler2D texture2;
            uniform float mixFactor;

            void main() {
                vec4 texColor1 = texture(texture1, fragTexCoord);
                vec4 texColor2 = texture(texture2, fragTexCoord);
                color = mix(texColor1, texColor2, mixFactor);
            }
        )";

        ModelData tetrahedronData() {
            return {
                {
                    //Вершины тетраэдра
                    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  // Верхняя (красный)
                     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  // Нижняя (зеленый)
                     0.0f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  // Правая (синий)
                     0.0f,  0.0f,  0.5f,  1.0f, 1.0f, 1.0f   // Передняя (белый)
                },
                {
                    0, 1, 2,  // Основание (передняя грань)
                    0, 1, 3,  // Правая боковая грань
                    1, 2, 3,  // Левая боковая грань
                    0, 2, 3   // Задняя грань
                }
            };
        }

        ModelData cubeColorData() {
            return {
                {
                    // Задняя грань куба
                    -0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f, // Нижняя левая (черный)
                     0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Нижняя правая (синий)
                     0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 1.0f,  0.0f, 1.0f, // Верхняя правая (голубой)
                    -0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, // Верхняя левая (зеленый)

                    // Передняя грань куба
                    -0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Нижняя левая (красный)
                     0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f, // Нижняя правая (малиновый)
                     0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // Верхняя правая (белый)
                    -0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  // Верхняя левая (желтый)

                    // Левая грань куба
                    -0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Нижняя левая (черный)
                    -0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, // Нижняя левая (красный)
                    -0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f, // Верхняя левая (желтый)
                    -0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Верхняя левая (зеленый)

                    // Правая грань куба
                     0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Нижняя правая (малиновый)
                     0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, // Нижняя правая (синий)
                     0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f, // Верхняя правая (голубой)
                     0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f, // Верхняя правая (белый)

                     // Верхняя грань куба
                    -0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 0.0f,  0.0f, 0.0f, // Верхняя левая (желтый)
                     0.4f,  0.4f, -0.4f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // Верхняя правая (белый)
                     0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f, // Верхняя правая (голубой)
                    -0.4f,  0.4f,  0.4f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Верхняя левая (зеленый)

                    // Нижняя грань куба
                    -0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, // Нижняя левая (красный)
                    -0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, // Нижняя левая (черный)
                     0.4f, -0.4f,  0.4f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, // Нижняя правая (синий)
                     0.4f, -0.4f, -0.4f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f  // Нижняя правая (малиновый)
                },
                {
                    0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
                    8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12,
                    16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20
                }
            };
        }

        ModelData cubeTextureData() {
            return {
                {
                    // Задняя грань куба
                    -0.4f, -0.4f,  0.4f,  1.0f, 0.0f, // Нижняя левая (черный)
                     0.4f, -0.4f,  0.4f,  0.0f, 0.0f, // Нижняя правая (синий)
                     0.4f,  0.4f,  0.4f,  0.0f, 1.0f, // Верхняя правая (голубой)
                    -0.4f,  0.4f,  0.4f,  1.0f, 1.0f, // Верхняя левая (зеленый)

                    // Передняя грань куба
                    -0.4f, -0.4f, -0.4f,  0.0f, 0.0f, // Нижняя левая (красный)
                     0.4f, -0.4f, -0.4f,  1.0f, 0.0f, // Нижняя правая (малиновый)
                     0.4f,  0.4f, -0.4f,  1.0f, 1.0f, // Верхняя правая (белый)
                    -0.4f,  0.4f, -0.4f,  0.0f, 1.0f, // Верхняя левая (желтый)

                    // Левая грань куба
                    -0.4f, -0.4f,  0.4f,  0.0f, 0.0f, // Нижняя левая (черный)
                    -0.4f, -0.4f, -0.4f,  1.0f, 0.0f, // Нижняя левая (красный)
                    -0.4f,  0.4f, -0.4f,  1.0f, 1.0f, // Верхняя левая (желтый)
                    -0.4f,  0.4f,  0.4f,  0.0f, 1.0f, // Верхняя левая (зеленый)

                    // Правая грань куба
                     0.4f, -0.4f, -0.4f,  0.0f, 0.0f, // Нижняя правая (малиновый)
                     0.4f, -0.4f,  0.4f,  1.0f, 0.0f, // Нижняя правая (синий)
                     0.4f,  0.4f,  0.4f,  1.0f, 1.0f, // Верхняя правая (голубой)
                     0.4f,  0.4f, -0.4f,  0.0f, 1.0f, // Верхняя правая (белый)

                     // Верхняя грань куба
                    -0.4f,  0.4f, -0.4f,  0.0f, 0.0f, // Верхняя левая (желтый)
                     0.4f,  0.4f, -0.4f,  1.0f, 0.0f, // Верхняя правая (белый)
                     0.4f,  0.4f,  0.4f,  1.0f, 1.0f, // Верхняя правая (голубой)
                    -0.4f,  0.4f,  0.4f,  0.0f, 1.0f, // Верхняя левая (зеленый)

                    // Нижняя грань куба
                    -0.4f, -0.4f, -0.4f,  0.0f, 1.0f, // Нижняя левая (красный)
                    -0.4f, -0.4f,  0.4f,  0.0f, 0.0f, // Нижняя левая (черный)
                     0.4f, -0.4f,  0.4f,  1.0f, 0.0f, // Нижняя правая (синий)
                     0.4f, -0.4f, -0.4f,  1.0f, 1.0f  // Нижняя правая (малиновый)
                },
                {
                    0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
                    8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12,
                    16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20
                }
            };
        }

        ModelData circleData() {
            ModelData data;
            data.vertices.push_back(0.0f);
            data.vertices.push_back(0.0f);
            data.vertices.push_back(0.0f);
            data.vertices.push_back(1.0f);
            data.vertices.push_back(1.0f);
            data.vertices.push_back(1.0f);

            for (int i = 0; i <= circleSegments; ++i) {
                float angle = 2.0f * PI * i / circleSegments;
                float x = cos(angle) * 0.5f;
                float y = sin(angle) * 0.5f;

                float hue = static_cast<float>(i) / circleSegments;
                float r = fabs(hue * 6.0f - 3.0f) - 1.0f;
                float g = 2.0f - fabs(hue * 6.0f - 2.0f);
                float b = 2.0f - fabs(hue * 6.0f - 4.0f);
                r = std::clamp(r, 0.0f, 1.0f);
                g = std::clamp(g, 0.0f, 1.0f);
                b = std::clamp(b, 0.0f, 1.0f);

                data.vertices.push_back(x);
                data.vertices.push_back(y);
                data.vertices.push_back(0.0f);
                data.vertices.push_back(r);
                data.vertices.push_back(g);
                data.vertices.push_back(b);
            }

            for (int i = 1; i <= circleSegments; ++i) {
                data.indices.push_back(0);
                data.indices.push_back(i);
                data.indices.push_back(i + 1);
            }

            return data;
        }

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

        GLuint compileShader(GLenum type, const char* src) {
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

        GLuint createProgram(const char* vertSrc, const char* fragSrc) {
            GLuint vShader = compileShader(GL_VERTEX_SHADER, vertSrc);
            if (!vShader) return 0;

            GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);
            if (!fShader) {
                glDeleteShader(vShader);
                return 0;
            }

            GLuint program = glCreateProgram();
            glAttachShader(program, vShader);
            glAttachShader(program, fShader);
            glLinkProgram(program);

            GLint linkOk = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkOk);
            if (!linkOk) {
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
                glDeleteShader(vShader);
                glDeleteShader(fShader);
                return 0;
            }

            glDetachShader(program, vShader);
            glDetachShader(program, fShader);
            glDeleteShader(vShader);
            glDeleteShader(fShader);

            return program;
        }

        GLuint loadTexture() {
            sf::Image img, img2, img3;

            if (!img.loadFromFile("pic1.jpg")) {
                std::cerr << "Error loading texture file: pic1.jpg" << std::endl;
                return 0;
            }
            if (!img2.loadFromFile("pic2.jpg")) {
                std::cerr << "Error loading texture file: pic2.jpg" << std::endl;
                return 0;
            }
            if (!img3.loadFromFile("pic3.jpg")) {
                std::cerr << "Error loading texture file: pic3.jpg" << std::endl;
                return 0;
            }

            img.flipVertically();
            img2.flipVertically();
            img3.flipVertically();

            glGenTextures(1, &texture1);
            glBindTexture(GL_TEXTURE_2D, texture1);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());

            glGenerateMipmap(GL_TEXTURE_2D);

            glGenTextures(1, &texture2);
            glBindTexture(GL_TEXTURE_2D, texture2);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img2.getSize().x, img2.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img2.getPixelsPtr());

            glGenerateMipmap(GL_TEXTURE_2D);

            glGenTextures(1, &texture3);
            glBindTexture(GL_TEXTURE_2D, texture3);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img3.getSize().x, img3.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img3.getPixelsPtr());

            glGenerateMipmap(GL_TEXTURE_2D);

            checkOpenGLerror();
            return texture1;
        }

        void loadTextures() {
            loadTexture();
        }

        void setupVertexAttributes(int stride, int posOffset, int colorOffset, int texOffset = -1) {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(posOffset));
            glEnableVertexAttribArray(0);

            if (colorOffset >= 0) {
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(colorOffset));
                glEnableVertexAttribArray(1);
            }

            if (texOffset >= 0) {
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(texOffset));
                glEnableVertexAttribArray(2);
            }
        }

    public:
        bool init() {
            loadTextures();

            if (texture1 == 0 || texture2 == 0 || texture3 == 0) {
                return false;
            }

            switchMode(FigureMode::Tetrahedron);
            transform.reset();
            return true;
        }

        void switchMode(FigureMode mode) {
            currentMode = mode;

            if (mode == FigureMode::CubeColor) {
                transform.reset(true);
            }
            else if (mode == FigureMode::CubeTexture) {
                transform.reset(true);
                transform.mixFactor = 1.0f;
            }
            else {
                transform.reset(false);
            }

            if (program != 0) {
                glDeleteProgram(program);
                program = 0;
            }

            if (currentShape.vao != 0) {
                glDeleteVertexArrays(1, &currentShape.vao);
                currentShape.vao = 0;
            }
            if (currentShape.vbo != 0) {
                glDeleteBuffers(1, &currentShape.vbo);
                currentShape.vbo = 0;
            }
            if (currentShape.ebo != 0) {
                glDeleteBuffers(1, &currentShape.ebo);
                currentShape.ebo = 0;
            }

            const char* vertSrc = nullptr;
            const char* fragSrc = nullptr;
            ModelData data;
            int stride = 0;

            switch (mode) {
            case FigureMode::Tetrahedron:
                vertSrc = vertexShaderColor;
                fragSrc = fragmentShaderColor;
                data = tetrahedronData();
                stride = 6 * sizeof(GLfloat);
                break;
            case FigureMode::CubeColor:
                vertSrc = vertexShaderColorTex;
                fragSrc = fragmentShaderColorTex;
                data = cubeColorData();
                stride = 8 * sizeof(GLfloat);
                break;
            case FigureMode::CubeTexture:
                vertSrc = vertexShaderTexTex;
                fragSrc = fragmentShaderTexTex;
                data = cubeTextureData();
                stride = 5 * sizeof(GLfloat);
                break;
            case FigureMode::Circle:
                vertSrc = vertexShaderColor;
                fragSrc = fragmentShaderColor;
                data = circleData();
                stride = 6 * sizeof(GLfloat);
                break;
            }

            program = createProgram(vertSrc, fragSrc);
            if (program == 0) {
                std::cerr << "Failed to create shader program\n";
                return;
            }

            modelLoc = glGetUniformLocation(program, "model");
            mixFactorLoc = glGetUniformLocation(program, "mixFactor");

            glGenVertexArrays(1, &currentShape.vao);
            glBindVertexArray(currentShape.vao);

            glGenBuffers(1, &currentShape.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, currentShape.vbo);
            glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(GLfloat), data.vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &currentShape.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentShape.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(GLuint), data.indices.data(), GL_STATIC_DRAW);

            switch (mode) {
            case FigureMode::Tetrahedron:
            case FigureMode::Circle:
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                break;
            case FigureMode::CubeColor:
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(GLfloat)));
                glEnableVertexAttribArray(2);
                break;
            case FigureMode::CubeTexture:
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);
                break;
            }

            currentShape.indexCount = static_cast<GLsizei>(data.indices.size());
            glBindVertexArray(0);

            checkOpenGLerror();
        }

        void render() {
            glUseProgram(program);

            transform.createMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, transform.matrix.data());

            if (currentMode == FigureMode::CubeColor || currentMode == FigureMode::CubeTexture) {
                glUniform1f(mixFactorLoc, transform.mixFactor);

                if (currentMode == FigureMode::CubeColor) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture1);
                    glUniform1i(glGetUniformLocation(program, "texture1"), 0);
                }
                else if (currentMode == FigureMode::CubeTexture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture2);
                    glUniform1i(glGetUniformLocation(program, "texture1"), 0);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture3);
                    glUniform1i(glGetUniformLocation(program, "texture2"), 1);
                }
            }

            glBindVertexArray(currentShape.vao);
            glDrawElements(GL_TRIANGLES, currentShape.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            checkOpenGLerror();
        }

        void handleKey(sf::Keyboard::Scancode scancode) {
            using Scan = sf::Keyboard::Scan;

            if (scancode == Scan::Num1) {
                switchMode(FigureMode::Tetrahedron);
            }
            else if (scancode == Scan::Num2) {
                switchMode(FigureMode::CubeColor);
            }
            else if (scancode == Scan::Num3) {
                switchMode(FigureMode::CubeTexture);
            }
            else if (scancode == Scan::Num4) {
                switchMode(FigureMode::Circle);
            }
            else if (scancode == Scan::A) {
                if (currentMode != FigureMode::Circle) {
                    transform.angleY -= 0.03f;
                }
                else {
                    transform.scaleX -= 0.03f;
                }
            }
            else if (scancode == Scan::D) {
                if (currentMode != FigureMode::Circle) {
                    transform.angleY += 0.03f;
                }
                else {
                    transform.scaleX += 0.03f;
                }
            }
            else if (scancode == Scan::W) {
                if (currentMode != FigureMode::Circle) {
                    transform.angleX -= 0.03f;
                }
                else {
                    transform.scaleY += 0.03f;
                }
            }
            else if (scancode == Scan::S) {
                if (currentMode != FigureMode::Circle) {
                    transform.angleX += 0.03f;
                }
                else {
                    transform.scaleY -= 0.03f;
                }
            }
            else if (scancode == Scan::Q && currentMode != FigureMode::Circle) {
                transform.angleZ -= 0.03f;
            }
            else if (scancode == Scan::E && currentMode != FigureMode::Circle) {
                transform.angleZ += 0.03f;
            }
            else if ((scancode == Scan::Up || scancode == Scan::Down) &&
                (currentMode == FigureMode::CubeColor || currentMode == FigureMode::CubeTexture)) {
                float delta = (scancode == Scan::Up) ? 0.02f : -0.02f;
                transform.mixFactor = std::clamp(transform.mixFactor + delta, 0.0f, 1.0f);
            }
        }

        void cleanup() {
            if (currentShape.vao != 0) glDeleteVertexArrays(1, &currentShape.vao);
            if (currentShape.vbo != 0) glDeleteBuffers(1, &currentShape.vbo);
            if (currentShape.ebo != 0) glDeleteBuffers(1, &currentShape.ebo);
            if (program != 0) glDeleteProgram(program);
            if (texture1 != 0) glDeleteTextures(1, &texture1);
            if (texture2 != 0) glDeleteTextures(1, &texture2);
            if (texture3 != 0) glDeleteTextures(1, &texture3);
        }
    };

    int run_lab12() {
        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.majorVersion = 3;
        settings.minorVersion = 3;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Core;

        sf::Window window(
            sf::VideoMode({ 800u, 800u }),
            "3D Figures with Transformations",
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
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        Renderer3D renderer;
        if (!renderer.init()) {
            std::cerr << "Renderer initialization failed\n";
            return 1;
        }

        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
                else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    glViewport(0, 0, resized->size.x, resized->size.y);
                }
                else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    renderer.handleKey(keyPressed->scancode);
                }
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderer.render();
            window.display();
        }

        renderer.cleanup();
        return 0;
    }

}

#endif //CS332_LAB12_H