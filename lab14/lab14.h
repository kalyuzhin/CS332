#ifndef CS332_LAB14_H
#define CS332_LAB14_H

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <GL/glew.h>


#include <stb_image.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

#include "shader.h"
#include "camera.h"
#include "model.h"

namespace {

    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    float lastX = (float)SCR_WIDTH / 2.0;
    float lastY = (float)SCR_HEIGHT / 2.0;
    bool firstMouse = true;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    int lightType = 2;
    int newLightType = 2;

    int shadintTypeCount = 4;
    int shadingType = 0;
    int newShadingType = 0;
    bool isSpotlight = false;

    void checkOpenGLerror() {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << err << std::endl;
        }
    }

    void Init() {
        glEnable(GL_DEPTH_TEST);
    }

    void processInput(sf::Window& window) {

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
            window.close();

        float v;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            v *= 2.5f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) // Дирижабль вперед
            
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) // Дирижабль Назад
            
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) // Дирижабль влево
            
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) // Дирижабль вправо

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) // Вниз

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) // Вверх
            

    }

    unsigned int loadTexture(char const* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
}

namespace lab14 {

    int run_lab14() {
        setlocale(LC_ALL, "ru");

        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.majorVersion = 3;
        settings.minorVersion = 3;
        sf::VideoMode videoMode({ 1000, 800 });
        sf::Window window(videoMode, "Game", sf::State::Windowed, settings);
        window.setVerticalSyncEnabled(true);
        window.setMouseCursorVisible(false);

        GLenum glewInitResult = glewInit();
        if (glewInitResult != GLEW_OK) {
            std::cerr << "GLEW initialization error: " << glewGetErrorString(glewInitResult) << std::endl;
            return -1;
        }

        Init();
        Shader shaders[] = {
    Shader("../../../lab14/shaders/5.4.spot.vs", "../../../lab14/shaders/lighting/5.4.phong.fs"),
    Shader("../../../lab14/shaders/5.1.directional.vs", "../../../lab14/shaders/lighting/5.1.phong.fs"),
        };

        Shader lightingShader = shaders[2];

        std::vector<Model> models;

        try {
            
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading models: " << e.what() << std::endl;
            return -1;
        }


        lightingShader.use();
        lightingShader.setInt("material.diffuse", 0);
        lightingShader.setInt("material.specular", 1);

        sf::Clock clock;
        bool running = true;

        while (running) {
            float currentFrame = clock.getElapsedTime().asSeconds();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput(window);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (newLightType == 0) {
                if (newLightType != lightType || newShadingType != shadingType) {
                    lightType = newLightType;
                    shadingType = newShadingType;
                    lightingShader = shaders[lightType * shadintTypeCount + shadingType];
                    lightingShader.use();
                    lightingShader.setInt("material.diffuse", 0);
                    lightingShader.setInt("material.specular", 1);
                }

                lightingShader.use();
                lightingShader.setVec3("light.position", camera.Position);
                lightingShader.setVec3("light.direction", camera.Front);
                lightingShader.setFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
                lightingShader.setFloat("light.outerCutOff", glm::cos(glm::radians(17.5f)));
                lightingShader.setVec3("viewPos", camera.Position);

                lightingShader.setVec3("light.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
                lightingShader.setVec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
                lightingShader.setVec3("light.specular", glm::vec3(0.8f, 0.8f, 0.8f));
                lightingShader.setFloat("light.constant", 1.0f);
                lightingShader.setFloat("light.linear", 0.09f);
                lightingShader.setFloat("light.quadratic", 0.032f);

                lightingShader.setFloat("material.shininess", 32.0f);
            }
            else if (newLightType == 1) {
                if (newLightType != lightType || newShadingType != shadingType) {
                    lightType = newLightType;
                    shadingType = newShadingType;
                    lightingShader = shaders[lightType * shadintTypeCount + shadingType];
                    lightingShader.use();
                    lightingShader.setInt("material.diffuse", 0);
                    lightingShader.setInt("material.specular", 1);
                }

                lightingShader.use();
                lightingShader.setVec3("light.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
                lightingShader.setVec3("viewPos", camera.Position);

                lightingShader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
                lightingShader.setVec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
                lightingShader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

                lightingShader.setFloat("material.shininess", 32.0f);
            }
            else if (newLightType == 2) {
                if (newLightType != lightType || newShadingType != shadingType) {
                    lightType = newLightType;
                    shadingType = newShadingType;
                    lightingShader = shaders[lightType * shadintTypeCount + shadingType];
                    lightingShader.use();
                    lightingShader.setInt("material.diffuse", 0);
                    lightingShader.setInt("material.specular", 1);
                }

                lightingShader.use();
                lightingShader.setVec3("light.position", lightPos);
                lightingShader.setVec3("viewPos", camera.Position);

                lightingShader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
                lightingShader.setVec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
                lightingShader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));
                lightingShader.setFloat("light.constant", 1.0f);
                lightingShader.setFloat("light.linear", 0.09f);
                lightingShader.setFloat("light.quadratic", 0.032f);

                lightingShader.setFloat("material.shininess", 32.0f);
            }

            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            for (size_t i = 0; i < models.size(); i++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, objPosition[i]);
                if (i == 3) {
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                }
                model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
                lightingShader.setMat4("model", model);
                models[i].Draw(lightingShader);
            }

            window.display();
            checkOpenGLerror();


            while (const std::optional event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    window.close();

                if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                {
                    if (key->scancode == sf::Keyboard::Scancode::Escape)
                        window.close();
                }

                if (const auto* resize = event->getIf<sf::Event::Resized>())
                {
                    glViewport(0, 0, resize->size.x, resize->size.y);
                }
            }
        }

        window.close();
        return 0;
    }
}
#endif