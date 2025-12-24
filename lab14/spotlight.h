#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include <glm/glm.hpp>
#include <GL/glew.h>

class Spotlight {
public:
    SpotLight()
        : position(0.0f, 0.0f, 0.0f),
        direction(0.0f, -1.0f, 0.0f),
        color(1.0f, 1.0f, 1.0f),
        cutOff(glm::cos(glm::radians(12.5f))),
        outerCutOff(glm::cos(glm::radians(17.5f))),
        constant(1.0f),
        linear(0.09f),
        quadratic(0.032f),
        intensity(1.5f),
        isEnabled(true) {
    }

    SpotLight(glm::vec3 pos, glm::vec3 dir, glm::vec3 col = glm::vec3(1.0f))
        : position(pos),
        direction(dir),
        color(col),
        cutOff(glm::cos(glm::radians(12.5f))),
        outerCutOff(glm::cos(glm::radians(17.5f))),
        constant(1.0f),
        linear(0.09f),
        quadratic(0.032f),
        intensity(1.5f),
        isEnabled(true) {
    }

    void Update(glm::vec3 newPos) {
        position = newPos;
    }

    void Toggle() {
        isEnabled = !isEnabled;
    }
private:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;

    float cutOff;         
    float outerCutOff;    

    float constant;
    float linear;
    float quadratic;

    float intensity;
    bool isEnabled;
};
#endif