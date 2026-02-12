#pragma once

// vma config (before #include <vk_mem_alloc.h>)
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_VULKAN_VERSION 1002000  // target Vulkan 1.2 on Android
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "resources/vk_debug.h"

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