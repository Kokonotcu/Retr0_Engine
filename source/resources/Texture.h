#pragma once
#include "resources/vk_images.h"
#include <vulkan/vulkan.h>

namespace retro 
{
    struct Texture 
    {
        Image image; // Contains VkImage, Allocation, Extent, Format
        VkSampler sampler;
    };
}