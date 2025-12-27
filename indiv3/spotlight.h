#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include <glm/glm.hpp>
#include <GL/glew.h>

class Spotlight {
public:
    Spotlight()
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

    Spotlight(glm::vec3 pos, glm::vec3 dir, glm::vec3 col = glm::vec3(1.0f))
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

    void Update(glm::vec3 newPos, glm::vec3 newDir) {
        position = newPos;
        direction = newDir;
    }

    void Toggle() {
        isEnabled = !isEnabled;
    }

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getDirection() const { return direction; }
    glm::vec3 getColor() const { return color; }
    float getCutOff() const { return cutOff; }
    float getOuterCutOff() const { return outerCutOff; }
    float getConstant() const { return constant; }
    float getLinear() const { return linear; }
    float getQuadratic() const { return quadratic; }
    float getIntensity() const { return intensity; }
    bool getEnabled() const { return isEnabled; }

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