#pragma once
#include "glm/glm.hpp"

// Simple "look at" camera
struct Camera
{
    void init(const glm::vec3 position, const glm::vec3 at, const glm::vec3 up);
    const glm::mat4 lookAt() const { return viewMat; };

private:
    glm::vec3 position;
    glm::vec3 at;
    glm::vec3 up;
    glm::mat4 viewMat;
};