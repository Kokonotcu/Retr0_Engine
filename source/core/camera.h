#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraData 
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

class Camera 
{
public:
    glm::vec3 position{ 0.f, 0.f, -2.f };
	glm::vec3 rotation{ 0.f, 0.f, 0.f };

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

	void SetStats(float newFov, float newNear, float newFar, float newAspect);
private:
	float fov = 70.f;
	float aspectRatio = 16.f / 9.f;
	float nearPlane = 0.1f;
	float farPlane = 200.f;
};
