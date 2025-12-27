#pragma once

#ifndef OBJECT_H
#define OBJECT_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "model.h"
#include "shader.h"

namespace indiv3 {

    class GameObject {
    public:
        glm::vec3 position;
        glm::vec3 rotation; // В градусах (x, y, z)
        glm::vec3 scale;
        Model* model; // Указатель на общие данные модели [6]

        GameObject(Model* m, float scl = 1.0f, glm::vec3 pos = glm::vec3(0.0f))
            : model(m), position(pos), rotation(0.0f), scale(scl) {
        }

        // Метод для получения матрицы модели [7, 8]
        glm::mat4 GetTransform() const {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position);
            m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            m = glm::scale(m, scale);
            return m;
        }

        virtual void Draw(Shader& shader) {
            if (model) {
                shader.setMat4("model", GetTransform());
                model->Draw(shader); // Вызов отрисовки загруженного меша [1]
            }
        }
    };

    class Sleigh : public GameObject {
    public:
        float angle = 0.0f;
        float orbitRadius = 15.0f;
        float speed = 1.0f;

        Sleigh(Model* m, float scl = 1.0f) : GameObject(m, scl) {}

        void Update(float dt) {
            angle += speed * dt;
            // Параметрическое уравнение окружности 
            position.x = orbitRadius * glm::cos(angle);
            position.z = orbitRadius * glm::sin(angle);

            // Поворот саней по направлению движения (тангенс к траектории) [10]
            rotation.y = -glm::degrees(angle) + 90.0f;
        }
    };
}

#endif