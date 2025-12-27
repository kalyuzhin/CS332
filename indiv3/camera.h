#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Zoom;

    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
        : Position(position), Up(up), Zoom(45.0f) {
        // ¬ычисл€ем вектор направлени€ на цель [1, 2]
        Front = glm::normalize(target - position);
    }

    glm::mat4 GetViewMatrix() {
        //  амера не двигаетс€, так как Position и Front константны
        return glm::lookAt(Position, Position + Front, Up);
    }
};

#endif