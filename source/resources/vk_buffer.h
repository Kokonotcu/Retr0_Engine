#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "gfx/vk_debug.h"

namespace retro 
{
    struct Buffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    retro::Buffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void DestroyBuffer(VmaAllocator allocator, const retro::Buffer& buffer);
}