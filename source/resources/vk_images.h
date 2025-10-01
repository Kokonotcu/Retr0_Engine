#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
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