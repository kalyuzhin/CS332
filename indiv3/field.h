#pragma once

#ifndef FIELD_H
#define FIELD_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include "mesh.h"

namespace indiv3 {

    float getTerrainHeight(float x, float z) {
        return 1.5f * sin(x * 0.1f) + 1.5f * cos(z * 0.1f);
    }

    Mesh generateUnevenField(float size, int resolution) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        float step = size / resolution;
        float offset = size / 2.0f;

        // 1. Генерируем вершины с перепадом высот
        for (int z = 0; z <= resolution; ++z) {
            for (int x = 0; x <= resolution; ++x) {
                Vertex vertex;
                float posX = x * step - offset;
                float posZ = z * step - offset;

                float posY = 1.5f * sin(posX * 0.1f) + 1.5f * cos(posZ * 0.1f);

                vertex.Position = glm::vec3(posX, posY, posZ);
                vertex.TexCoords = glm::vec2((float)x / resolution * 10.0f, (float)z / resolution * 10.0f);
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
                for (int i = 0; i < 4; i++) { vertex.m_BoneIDs[i] = -1; vertex.m_Weights[i] = 0.0f; }

                vertices.push_back(vertex);
            }
        }

        // 2. Расчет сглаженных нормалей для освещения
        for (int z = 0; z <= resolution; ++z) {
            for (int x = 0; x <= resolution; ++x) {
                int idx = z * (resolution + 1) + x;
                float posX = x * step - offset;
                float posZ = z * step - offset;

                // Вычисляем высоты соседних точек (с проверкой границ)
                float centerY = vertices[idx].Position.y;
                float leftY = (x > 0) ? vertices[idx - 1].Position.y : centerY;
                float rightY = (x < resolution) ? vertices[idx + 1].Position.y : centerY;
                float downY = (z > 0) ? vertices[idx - (resolution + 1)].Position.y : centerY;
                float upY = (z < resolution) ? vertices[idx + (resolution + 1)].Position.y : centerY;

                // Вычисляем нормаль через среднее градиентов
                glm::vec3 normal(0.0f);

                // Горизонтальный градиент (dx)
                if (x > 0 && x < resolution) {
                    glm::vec3 dx = glm::vec3(2 * step, rightY - leftY, 0);
                    normal += glm::cross(glm::vec3(0, 0, 1), dx);
                }

                // Вертикальный градиент (dz)
                if (z > 0 && z < resolution) {
                    glm::vec3 dz = glm::vec3(0, upY - downY, 2 * step);
                    normal += glm::cross(dz, glm::vec3(1, 0, 0));
                }

                // Если normal нулевой (угловая вершина), используем вертикальную нормаль
                if (glm::length(normal) < 0.001f) {
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                vertices[idx].Normal = glm::normalize(normal);
            }
        }

        // 3. Индексы треугольников
        for (int z = 0; z < resolution; ++z) {
            for (int x = 0; x < resolution; ++x) {
                int start = z * (resolution + 1) + x;
                indices.push_back(start);
                indices.push_back(start + (resolution + 1));
                indices.push_back(start + 1);
                indices.push_back(start + 1);
                indices.push_back(start + (resolution + 1));
                indices.push_back(start + (resolution + 1) + 1);
            }
        }

        return Mesh(vertices, indices, {});
    }

}
#endif