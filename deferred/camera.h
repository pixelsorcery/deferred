#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
// Simple "look at" camera
struct Camera
{
    const glm::mat4 lookAt() const { return glm::lookAt(position, position + direction, up); };

    float dx = 0.f;
    float dy = 0.f;
    float speed = 12.0f;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;

private:
    //glm::mat4 viewMat;
};