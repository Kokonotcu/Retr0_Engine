#include "camera.h"

glm::mat4 Camera::GetViewMatrix() const
{
    // Calculate the camera vectors
    glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.f), rotation.x, { 1.f, 0.f, 0.f });
    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.f), rotation.y, { 0.f, 1.f, 0.f });

    return glm::translate(glm::mat4(1.f), position) * yawRotation * pitchRotation;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1; // Standard Vulkan Y-flip
    return proj;
}

void Camera::SetStats(float newFov, float newNear, float newFar, float newAspect)
{
    fov = newFov;
    nearPlane = newNear;
    farPlane = newFar;
    aspectRatio = newAspect;
}