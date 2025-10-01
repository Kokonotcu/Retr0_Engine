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

    struct MeshBuffer 
    {
        Buffer indexBuffer;          // Get rid of these per mesh buffers and put all meshes into mega global buffer
        Buffer vertexBuffer;         // Get rid of these per mesh buffers and put all meshes into mega global buffer
    };

    // holds the resources needed for a mesh
    struct GPUMeshBuffer : public MeshBuffer
    {
        VkDeviceAddress vertexBufferAddress;
    };


    retro::Buffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void DestroyBuffer(VmaAllocator allocator, const retro::Buffer& buffer);
}