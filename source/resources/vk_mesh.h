#pragma once
#include <string>
#include <vector>
#include <span>

#include "resources/vk_buffer.h"
#include "resources/vk_push_constants.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/packing.hpp>

namespace retro 
{
    struct Vertex
    {
        glm::vec3 position;
        glm::u16vec2 uv;
        glm::vec3 normal;
        glm::u8vec4 color;
    };
    static_assert(sizeof(Vertex) == 32);

    struct Submesh 
    {
        uint32_t startIndex;
        uint32_t count;
    };

    struct Drawable
    {
        virtual void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, glm::mat4x4 worldMatrix) = 0;
    };

	struct Mesh : public Drawable
    {
        std::string name;
        std::vector<Submesh> submeshes;
    };

    struct GPUMeshHandle : public Mesh
    {
        // where this mesh lives inside the mega buffers:
        VkDeviceSize vertexOffset = 0; // bytes
        VkDeviceSize indexOffset = 0; // bytes
        uint32_t          indexCount = 0;

        VkDeviceAddress vertexBufferAddress;
		retro::GPUPushConstant gpuPushConstant;

        void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, glm::mat4x4 worldMatrix) override
        {
			gpuPushConstant.worldMatrix = worldMatrix;
			gpuPushConstant.vertexBuffer = vertexBufferAddress;
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(retro::GPUPushConstant), &gpuPushConstant);
            vkCmdBindIndexBuffer(cmd, indexBuffer, indexOffset, VK_INDEX_TYPE_UINT32);
            
            for (const auto& submesh : submeshes) {
                vkCmdDrawIndexed(cmd, submesh.count, 1, submesh.startIndex, 0, 0);
			}
		}
    };

    struct CPUMesh : public Mesh 
    {
        std::vector<retro::Vertex> vertices;
        std::vector<uint32_t> indices;

        ~CPUMesh()
        {
            vertices.clear();
            indices.clear();
            submeshes.clear();
            name = "";
		}

        void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, glm::mat4x4 worldMatrix) override 
        {

        }
    };
}