#pragma once

#ifndef FIELD_H
#define FIELD_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include "mesh.h"

namespace indiv3 {

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

                // Функция высоты: создаем "волны" на поле
                float posY = 1.5f * sin(posX * 0.1f) + 1.5f * cos(posZ * 0.1f);

                vertex.Position = glm::vec3(posX, posY, posZ);
                vertex.TexCoords = glm::vec2((float)x / resolution * 10.0f, (float)z / resolution * 10.0f);

                // Инициализация остальных полей
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

                // Берем соседние точки для вычисления наклона (Finite Difference)
                float hl = 1.5f * sin((x - 1) * step * 0.1f - offset * 0.1f) + 1.5f * cos(z * step * 0.1f - offset * 0.1f);
                float hr = 1.5f * sin((x + 1) * step * 0.1f - offset * 0.1f) + 1.5f * cos(z * step * 0.1f - offset * 0.1f);
                float hd = 1.5f * sin(x * step * 0.1f - offset * 0.1f) + 1.5f * cos((z - 1) * step * 0.1f - offset * 0.1f);
                float hu = 1.5f * sin(x * step * 0.1f - offset * 0.1f) + 1.5f * cos((z + 1) * step * 0.1f - offset * 0.1f);

                // Нормаль вычисляется как вектор, перпендикулярный поверхности
                vertices[idx].Normal = glm::normalize(glm::vec3(hl - hr, 2.0f, hd - hu));
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