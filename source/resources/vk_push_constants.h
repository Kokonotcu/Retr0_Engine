#pragma once

#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace retro 
{
    // push constants for our mesh object draws
    struct GPUPushConstant
    {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };
    struct CPUPushConstant
    {
        glm::mat4 worldMatrix;
        glm::mat4 model;
    };
}