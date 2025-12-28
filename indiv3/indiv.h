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
        Shader instShader = Shader("../../../indiv3/shaders/instance.vs", "../../../indiv3/shaders/lighting/full.fs");

        Model shipModel = Model("../../../indiv3/resources/objs/ship/LS5JZOS0TE732E0JDGJZFNO76.obj");
        Airship ship(&shipModel, 2.0f, glm::vec3(0.4f, 0.4f, 0.4f));

        Model treeModel = Model("../../../indiv3/resources/objs/tree/TW6KD7FPMCV5JXQM7FXBBHZXW.obj");
        float treeY = getTerrainHeight(0.0f, 0.0f);
        GameObject tree(&treeModel, 5.0f, glm::vec3{0.0f, 9.0f, 0.0f});

        Model sleghtModel = Model("../../../indiv3/resources/objs/sleigh/SMDH4QR5ZND05JVLA9P8SZ56J.obj");
        Sleigh sleigh(&sleghtModel, 2.0f);

        unsigned int grassTexID = loadTexture("../../../indiv3/resources/soil.jpg");
        Texture fieldTexture = { grassTexID, "texture_diffuse", "soil.jpg" };
        std::vector<Texture> terrainTextures;
        terrainTextures.push_back(fieldTexture);

        Mesh field = generateUnevenField(400, 80);
        field.textures = terrainTextures;

        Model trashModel = Model("../../../indiv3/resources/objs/trash/AXRJ8DNQQ391GF0E9RY6P4CH3.obj");
        Model bochkaModel = Model("../../../indiv3/resources/objs/bochka/YIU9GYSZAN0DU9OFWL14RPMGO.obj");

        std::vector<glm::mat4> houseMatrices;
        for (int i = 0; i < 10; i++) {
            float x = (rand() % 200) - 100.0f;
            float z = (rand() % 200) - 100.0f;
            float y = getTerrainHeight(x, z);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, y, z));
            model = glm::rotate(model, glm::radians((float)(rand() % 360)), glm::vec3(0, 1, 0));
            houseMatrices.push_back(model);
        }
        unsigned int instanceVBO;
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, houseMatrices.size() * sizeof(glm::mat4), houseMatrices.data(), GL_STATIC_DRAW);
        for (unsigned int i = 0; i < trashModel.meshes.size(); i++) {
            unsigned int VAO = trashModel.meshes[i].VAO;
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

            std::size_t vec4Size = sizeof(glm::vec4);
            for (int j = 0; j < 4; j++) {
                glEnableVertexAttribArray(7 + j);
                glVertexAttribPointer(7 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(j * vec4Size));
                glVertexAttribDivisor(7 + j, 1);
            }
            glBindVertexArray(0);
        }

        
        std::vector<glm::mat4> bochkaMatrices;
        for (int i = 0; i < 15; i++) { // Например, 15 бочек
            float x = (rand() % 300) - 150.0f;
            float z = (rand() % 300) - 150.0f;
            float y = getTerrainHeight(x, z);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x, y, z));
            model = glm::rotate(model, glm::radians((float)(rand() % 360)), glm::vec3(0, 1, 0));
            float scale = 0.5f + (rand() % 100) / 200.0f; // Случайный масштаб
            model = glm::scale(model, glm::vec3(scale));
            bochkaMatrices.push_back(model);
        }
        unsigned int instanceVBO_bochka;
        glGenBuffers(1, &instanceVBO_bochka);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_bochka);
        glBufferData(GL_ARRAY_BUFFER, bochkaMatrices.size() * sizeof(glm::mat4), bochkaMatrices.data(), GL_STATIC_DRAW);

        for (unsigned int i = 0; i < bochkaModel.meshes.size(); i++) {
            unsigned int VAO = bochkaModel.meshes[i].VAO;
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_bochka);

            std::size_t vec4Size = sizeof(glm::vec4);
            for (int j = 0; j < 4; j++) {
                glEnableVertexAttribArray(7 + j);
                glVertexAttribPointer(7 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(j * vec4Size));
                glVertexAttribDivisor(7 + j, 1);
            }
            glBindVertexArray(0);
        }
        

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

            instShader.use();

            instShader.setMat4("projection", projection);
            instShader.setMat4("view", camera.GetViewMatrix());
            instShader.setVec3("viewPos", camera.Position);

            instShader.setVec3("dirLight.direction", 0.0f, -1.0f, -1.0f);
            instShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.4f);
            instShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.9f);
            instShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

            instShader.setVec3("spotLight.position", ship.searchlight.getPosition());
            instShader.setVec3("spotLight.direction", ship.searchlight.getDirection());
            instShader.setFloat("spotLight.cutOff", ship.searchlight.getCutOff());
            instShader.setFloat("spotLight.outerCutOff", ship.searchlight.getOuterCutOff());
            instShader.setBool("spotLight.enabled", ship.searchlight.getEnabled());
            instShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            instShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 0.8f);
            instShader.setVec3("spotLight.specular", 1.0f, 1.0f, 0.8f);
            instShader.setFloat("spotLight.constant", 1.0f);
            instShader.setFloat("spotLight.linear", 0.045f);
            instShader.setFloat("spotLight.quadratic", 0.0075f);

            for (unsigned int i = 0; i < trashModel.meshes.size(); i++) {
                // Установка текстур для данного меша
                trashModel.meshes[i].SetupTextures(instShader);

                glBindVertexArray(trashModel.meshes[i].VAO);
                glDrawElementsInstanced(GL_TRIANGLES,
                    trashModel.meshes[i].indices.size(),
                    GL_UNSIGNED_INT, 0,
                    houseMatrices.size());
                glBindVertexArray(0);
            }

            // Отрисовка инстансов bochkaModel с текстурами
            for (unsigned int i = 0; i < bochkaModel.meshes.size(); i++) {
                // Установка текстур для данного меша
                bochkaModel.meshes[i].SetupTextures(instShader);

                glBindVertexArray(bochkaModel.meshes[i].VAO);
                glDrawElementsInstanced(GL_TRIANGLES,
                    bochkaModel.meshes[i].indices.size(),
                    GL_UNSIGNED_INT, 0,
                    bochkaMatrices.size());
                glBindVertexArray(0);
            }

            glActiveTexture(GL_TEXTURE0);

            window.display();
            checkOpenGLerror();           
        }

        window.close();
        return 0;
    }
}
#endif