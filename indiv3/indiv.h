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
#include "ship.h"
#include "gameobject.h"
#include "field.h"

namespace indiv3 {

    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

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

namespace indiv3 {

    int run_indiv3() {
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
        Shader shader = Shader("../../../indiv3/shaders/point.vs", "../../../indiv3/shaders/lighting/full.fs");

        Model shipModel = Model("../../../indiv3/resources/objs/ship/LS5JZOS0TE732E0JDGJZFNO76.obj");
        Airship ship(&shipModel, 2.0f, glm::vec3(0.4f, 0.4f, 0.4f));

        Model treeModel = Model("../../../indiv3/resources/objs/tree/TW6KD7FPMCV5JXQM7FXBBHZXW.obj");
        GameObject tree(&treeModel, 5.0f);

        Model sleghtModel = Model("../../../indiv3/resources/objs/sleigh/SMDH4QR5ZND05JVLA9P8SZ56J.obj");
        Sleigh sleigh(&sleghtModel, 2.0f);

        unsigned int grassTexID = loadTexture("../../../indiv3/resources/soil.jpg");
        Texture fieldTexture = { grassTexID, "texture_diffuse", "soil.jpg" };
        std::vector<Texture> terrainTextures;
        terrainTextures.push_back(fieldTexture);

        Mesh field = generateUnevenField(400, 20);
        field.textures = terrainTextures;

        /*Model trashModel = Model("../../../indiv3/resources/objs/trash/AXRJ8DNQQ391GF0E9RY6P4CH3.obj");
        Model bochkaModel = Model("../../../indiv3/resources/objs/bochka/YIU9GYSZAN0DU9OFWL14RPMGO.obj");*/

        glm::vec3 cameraPos = glm::vec3(0.0f, 40.0f, 60.0f);
        glm::vec3 cameraDir = glm::vec3(0.0f, 0.0f, 0.0f);
        Camera camera = Camera(cameraPos, cameraDir);


        sf::Clock clock;
        bool running = true;

        while (running) {
            float currentFrame = clock.getElapsedTime().asSeconds();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            while (const std::optional event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    window.close();

                if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                {
                    if (key->scancode == sf::Keyboard::Scancode::Escape)
                        window.close();
                    if (key->scancode == sf::Keyboard::Scancode::F)
                        ship.searchlight.Toggle();
                }

                if (const auto* resize = event->getIf<sf::Event::Resized>())
                {
                    glViewport(0, 0, resize->size.x, resize->size.y);
                }
            }

            ship.ProcessInput(deltaTime);
            sleigh.Update(deltaTime);

            glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();

            float aspect = (float)window.getSize().x / (float)window.getSize().y;
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 1000.0f);

            shader.setMat4("projection", projection);
            shader.setMat4("view", camera.GetViewMatrix());
            shader.setVec3("viewPos", camera.Position);

            shader.setVec3("dirLight.direction", 0.0f, -1.0f, -1.0f);
            shader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.4f);
            shader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.9f);
            shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

            shader.setVec3("spotLight.position", ship.searchlight.getPosition());
            shader.setVec3("spotLight.direction", ship.searchlight.getDirection());
            shader.setFloat("spotLight.cutOff", ship.searchlight.getCutOff());
            shader.setFloat("spotLight.outerCutOff", ship.searchlight.getOuterCutOff());
            shader.setBool("spotLight.enabled", ship.searchlight.getEnabled());
            shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 0.8f);
            shader.setVec3("spotLight.specular", 1.0f, 1.0f, 0.8f);
            shader.setFloat("spotLight.constant", 1.0f);
            shader.setFloat("spotLight.linear", 0.045f);
            shader.setFloat("spotLight.quadratic", 0.0075f);            

            shader.setMat4("model", glm::mat4(1.0f));
            field.Draw(shader);
            tree.Draw(shader);
            sleigh.Draw(shader);
            ship.Draw(shader);

            window.display();
            checkOpenGLerror();           
        }

        window.close();
        return 0;
    }
}
#endif