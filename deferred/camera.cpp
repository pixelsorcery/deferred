#include "camera.h"
#include "glm/gtc/matrix_transform.hpp"

void Camera::init(glm::vec3 inPos, glm::vec3 inAt, glm::vec3 inUp)
{
    position = inPos;
    at = inAt;
    up = inUp;
    viewMat = glm::lookAt(position, at, up);
}