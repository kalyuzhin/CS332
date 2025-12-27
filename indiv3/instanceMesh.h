#pragma once

#ifndef INSTANCE_MESH_H
#define INSTANCE_MESH_H
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.h"
#include "mesh.h"
#include <vector>
#include <string>
namespace indiv3 {
    class InstancedMesh {
    public:
        unsigned int VAO;
        unsigned int instanceVBO;
        unsigned int indexCount;
        unsigned int amount;

        // Конструктор принимает базовую геометрию и количество экземпляров
        InstancedMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, unsigned int instanceAmount)
            : amount(instanceAmount), indexCount(indices.size())
        {
            setupMesh(vertices, indices);
        }

        // Метод для загрузки/обновления матриц трансформации всех экземпляров
        void setInstanceMatrices(const std::vector<glm::mat4>& matrices) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &matrices, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        void Draw(Shader& shader) {
            // Устанавливаем текстуры (логика из обычного Mesh)
            //... (код привязки текстур)

            glBindVertexArray(VAO);
            // Вызов инстанс-отрисовки вместо обычной [1, 2]
            glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }

    private:
        void setupMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
            unsigned int VBO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            glGenBuffers(1, &instanceVBO);

            glBindVertexArray(VAO);

            // 1. Загрузка геометрии (VBO)
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices, GL_STATIC_DRAW);

            // Стандартные атрибуты (Position, Normal, TexCoords)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

            // 2. Настройка инстанс-буфера для mat4 (слоты 3, 4, 5, 6) [1, 3]
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            std::size_t vec4Size = sizeof(glm::vec4);

            for (int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(3 + i);
                glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
                // Делитель 1 говорит OpenGL обновлять атрибут раз в экземпляр [2, 4]
                glVertexAttribDivisor(3 + i, 1);
            }

            glBindVertexArray(0);
        }
    };
}
#endif