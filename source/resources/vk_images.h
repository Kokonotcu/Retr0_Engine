#pragma once

#include <vulkan/vulkan.h>
#include "resources/vk_buffer.h"
namespace retro 
{
    struct Image
    {
        VkFormat imageFormat;
        VkExtent3D imageExtent;
        VkImage image;
        VkImageView imageView;

        VmaAllocation allocation;
    };
}