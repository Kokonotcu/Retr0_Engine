#pragma once
#include <string>
#include <vector>
#include <span>

#include "resources/vk_buffer.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>



namespace retro 
{
    struct Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    struct Submesh 
    {
        uint32_t startIndex;
        uint32_t count;
    };

    struct Mesh 
    {
        std::string name;
        std::vector<Submesh> submeshes;
    };

    struct GPUMeshHandle : public Mesh
    {
        // where this mesh lives inside the mega buffers:
        VkDeviceSize vertexOffset = 0; // bytes
        VkDeviceSize indexOffset = 0; // bytes
        uint32_t     indexCount = 0;

        retro::GPUMeshBuffer meshBuffer;
    };

    struct CPUMesh : public Mesh 
    {
        std::vector<retro::Vertex> vertices;
        std::vector<uint32_t> indices;
    };
}