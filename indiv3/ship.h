#ifndef AIRSHIP_H
#define AIRSHIP_H

#include "spotlight.h"
#include "model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace indiv3 {
    class Airship {
    public:
        glm::vec3 position;
        float speed;
        float yaw;
        glm::vec3 scale;
        Model* visualModel;
        Spotlight searchlight;

        Airship(Model* m, float scl = 1.0f, glm::vec3 pos = glm::vec3(0.0f))
            : position(pos),
            speed(10.0f),
            yaw(0.0f),
            visualModel(m),
            scale(scl)
        {

            glm::vec3 lightOffset(0.0f, -1.5f, 0.0f);
            glm::vec3 lightDir(0.0f, -1.0f, 0.0f);

            searchlight = Spotlight(
                position + lightOffset,
                lightDir,
                glm::vec3(1.0f, 1.0f, 0.9f)
            );
        }

        void Draw(Shader& shader) {
            glm::mat4 modelMatrix = GetModelMatrix();
            shader.setMat4("model", modelMatrix);

            if (visualModel) {
                visualModel->Draw(shader);
            }
        }

        glm::mat4 GetModelMatrix() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position);
            m = glm::rotate(m, glm::radians(yaw), glm::vec3(0, 1, 0));
            m = glm::scale(m, scale);
            return m;
        }

        void ProcessInput(float dt) {
            float moveStep = speed * dt;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) position.z -= moveStep;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) position.z += moveStep;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) position.x -= moveStep;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) position.x += moveStep;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))  position.y += moveStep;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) position.y -= moveStep;

            UpdateLight();
        }

        void UpdateLight() {
            searchlight.Update(position + glm::vec3(0.0f, -1.5f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        }
    };
}


#endif