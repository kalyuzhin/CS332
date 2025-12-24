#ifndef AIRSHIP_H
#define AIRSHIP_H

#include "spotlight.h"
#include <vector>

class Airship {
public:
    Airship()
        : position(0.0f, 10.0f, 0.0f),
        speed(0.5f),
        rotation(0.0f) {

        searchlight = SpotLight(
            position + glm::vec3(0.0f, -1.5f, -1.0f), 
            glm::vec3(0.0f, -1.0f, 0.0f),            
            glm::vec3(1.0f, 1.0f, 0.9f)              
        );

        searchlight.SetIntensity(3.0f);
        searchlight.SetAngles(15.0f, 25.0f);
    }
};