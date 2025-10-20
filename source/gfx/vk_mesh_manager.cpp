#include <gfx/vk_mesh_manager.h>
#include "stb_image.h"

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_debug.h"
#include <glm/gtx/quaternion.hpp>

#define SIMDJSON_NO_INLINE
#undef simdjson_inline

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"


namespace MeshManager
{
    namespace 
    {
		VulkanEngine* engine = nullptr;

        // the big buffers:
		VkDeviceAddress globalVertexAddress = 0; // If using buffer device address
        retro::Buffer globalVertexBuffer;
        retro::Buffer globalIndexBuffer;
		DeletionQueue deletionQueue;

        // very dumb bump allocators (start at 0, move forward):
        VkDeviceSize vertexHead = 0; 
        VkDeviceSize indexHead = 0;

        inline VkDeviceSize Align(VkDeviceSize v, VkDeviceSize a) {
            return (v + a - 1) & ~(a - 1);
        }
    }

    void Init(VulkanEngine* _engine, size_t maxVertexBytes, size_t maxIndexBytes)
    {
        engine = _engine;
		deletionQueue.init(engine->GetDevice(), engine->GetAllocator());

        if (engine == nullptr) 
        {
            retro::print("Error: MeshManager::Init() called with null engine pointer\n");
            return;
        }

        if (engine->bufferDeviceAddress)
        {
            // --- BDA path ---
            globalVertexBuffer = retro::CreateBuffer(engine->GetAllocator(), maxVertexBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            globalIndexBuffer = retro::CreateBuffer(engine->GetAllocator(), maxIndexBytes,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

            // Query device address
            VkBufferDeviceAddressInfo addrInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addrInfo.buffer = globalVertexBuffer.buffer;
            globalVertexAddress = vkGetBufferDeviceAddress(engine->GetDevice(), &addrInfo);
        }
        else
        {
            // --- Fallback path (no BDA) ---
            globalVertexBuffer = retro::CreateBuffer(engine->GetAllocator(), maxVertexBytes,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |  // classic vertex binding
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

            globalIndexBuffer = retro::CreateBuffer(engine->GetAllocator(), maxIndexBytes,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

            // No device address in this path
            globalVertexAddress = 0;
        }
        
        vertexHead = 0;
		indexHead = 0;

		deletionQueue.addBuffer(globalVertexBuffer.buffer, globalVertexBuffer.allocation);
		deletionQueue.addBuffer(globalIndexBuffer.buffer, globalIndexBuffer.allocation);
    }

    void ClearBuffers()
    {
        deletionQueue.flush();
        vertexHead = 0;
		indexHead = 0;
    }

    VkBuffer GetGlobalVertexBuffer()
    {
        return globalVertexBuffer.buffer;
    }

    VkBuffer GetGlobalIndexBuffer()
    {
        return globalIndexBuffer.buffer; 
    }

    std::shared_ptr<retro::CPUMesh> LoadMeshCPU(std::filesystem::path filePath, int modelIndex)
    {
        // 1) Load CPU-side mesh
        auto meshes = FileManager::ModelLoader::LoadMeshFromFile(filePath);

        if (meshes.size() <= 0 || modelIndex < 0 || modelIndex >= (int)meshes.size()) {
            retro::print("MeshManager: invalid modelIndex or load failed: " + filePath.string());
            return nullptr; // empty
        }

        retro::print("MeshManager: loaded mesh " + std::to_string(modelIndex));
        retro::print("index amount : " + std::to_string(meshes.at(modelIndex)->indices.size()));
		retro::print(" vertex amount : " + std::to_string(meshes.at(modelIndex)->vertices.size()));
		retro::print("\n");

        const auto& m = meshes.at(modelIndex);
        const size_t vertexBytes = m->vertices.size() * sizeof(retro::Vertex);
        const size_t indexBytes = m->indices.size() * sizeof(uint32_t);

        // Align heads before reserving space in the global buffers
        const VkDeviceSize alignedVertexHead = Align(vertexHead, 16); // 16B for vertex data
        const VkDeviceSize alignedIndexHead = Align(indexHead, 4);  // 4B  for uint32 indices

        // 2) Capacity check against global buffers
        const VkDeviceSize vCap = globalVertexBuffer.info.size;
        const VkDeviceSize iCap = globalIndexBuffer.info.size;
        if (alignedVertexHead + vertexBytes > vCap || alignedIndexHead + indexBytes > iCap) {
            retro::print("MeshManager: out of space ");
            return nullptr; // empty
        }

        // 3) Staging buffer (one-shot, merged for V+I)
        retro::Buffer staging = retro::CreateBuffer(
            engine->GetAllocator(),
            vertexBytes + indexBytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY
        );

        // 4) Fill staging directly
        uint8_t* mapped = static_cast<uint8_t*>(staging.allocation->GetMappedData());
        memcpy(mapped + 0, m->vertices.data(), vertexBytes);
        memcpy(mapped + vertexBytes, m->indices.data(), indexBytes);

        engine->ImmediateSubmit([&](VkCommandBuffer cmd) {
            // vertices → global vertex buffer
            VkBufferCopy vCopy{};
            vCopy.srcOffset = 0;
            vCopy.dstOffset = alignedVertexHead;
            vCopy.size = vertexBytes;
            vkCmdCopyBuffer(cmd, staging.buffer, globalVertexBuffer.buffer, 1, &vCopy);

            // indices → global index buffer
            VkBufferCopy iCopy{};
            iCopy.srcOffset = vertexBytes;
            iCopy.dstOffset = alignedIndexHead;
            iCopy.size = indexBytes;
            vkCmdCopyBuffer(cmd, staging.buffer, globalIndexBuffer.buffer, 1, &iCopy);
            });

        meshes.at(modelIndex)->vertexOffset = alignedVertexHead;
        meshes.at(modelIndex)->indexOffset = alignedIndexHead;
        meshes.at(modelIndex)->indexCount = static_cast<uint32_t>(m->indices.size());

        // 5) Advance heads
        vertexHead = alignedVertexHead + vertexBytes;
        indexHead = alignedIndexHead + indexBytes;

        // 6) Cleanup staging and CPU vectors
        retro::DestroyBuffer(engine->GetAllocator(), staging);

        return meshes.at(modelIndex);
    }

    retro::GPUMeshHandle LoadMeshGPU(std::filesystem::path filePath, int modelIndex)
    {
        retro::GPUMeshHandle handle{};

        // 1) Load CPU-side mesh
        auto meshes = FileManager::ModelLoader::LoadMeshFromFile(filePath);

        if (meshes.size() <= 0 || modelIndex < 0 || modelIndex >= (int)meshes.size()) {
            retro::print("MeshManager: invalid modelIndex or load failed: " + filePath.string());
            return handle; // empty
        }

        retro::print("MeshManager: loaded mesh " + std::to_string(modelIndex));
        retro::print("index amount : " + std::to_string(meshes.at(modelIndex)->indices.size()));
        retro::print(" vertex amount : " + std::to_string(meshes.at(modelIndex)->vertices.size()));
        retro::print("\n");

        const auto& m = meshes.at(modelIndex);
        const size_t vertexBytes = m->vertices.size() * sizeof(retro::Vertex);
        const size_t indexBytes = m->indices.size() * sizeof(uint32_t);

        // Align heads before reserving space in the global buffers
        const VkDeviceSize alignedVertexHead = Align(vertexHead, 16); // 16B for vertex data
        const VkDeviceSize alignedIndexHead = Align(indexHead, 4);  // 4B  for uint32 indices

        // 2) Capacity check against global buffers
        const VkDeviceSize vCap = globalVertexBuffer.info.size;
        const VkDeviceSize iCap = globalIndexBuffer.info.size;
        if (alignedVertexHead + vertexBytes > vCap || alignedIndexHead + indexBytes > iCap) {
            retro::print("MeshManager: out of space ");
            return handle; // empty
        }

        // 3) Staging buffer (one-shot, merged for V+I)
        retro::Buffer staging = retro::CreateBuffer(
            engine->GetAllocator(),
            vertexBytes + indexBytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY
        );

        // 4) Fill staging directly
        uint8_t* mapped = static_cast<uint8_t*>(staging.allocation->GetMappedData());
        memcpy(mapped + 0, m->vertices.data(), vertexBytes);
        memcpy(mapped + vertexBytes, m->indices.data(), indexBytes);

        engine->ImmediateSubmit([&](VkCommandBuffer cmd) {
            // vertices → global vertex buffer
            VkBufferCopy vCopy{};
            vCopy.srcOffset = 0;
            vCopy.dstOffset = alignedVertexHead;
            vCopy.size = vertexBytes;
            vkCmdCopyBuffer(cmd, staging.buffer, globalVertexBuffer.buffer, 1, &vCopy);

            // indices → global index buffer
            VkBufferCopy iCopy{};
            iCopy.srcOffset = vertexBytes;
            iCopy.dstOffset = alignedIndexHead;
            iCopy.size = indexBytes;
            vkCmdCopyBuffer(cmd, staging.buffer, globalIndexBuffer.buffer, 1, &iCopy);
            });

        // 6) Build the handle (offsets/counts)
        handle.name = m->name;
        handle.submeshes = m->submeshes;       // NOTE: these startIndex values are local to this mesh
        handle.vertexOffset = alignedVertexHead;
        handle.indexOffset = alignedIndexHead;
        handle.indexCount = static_cast<uint32_t>(m->indices.size());

        // BDA base address for shader fetch
        if (engine->bufferDeviceAddress != 0) 
        {
            handle.vertexBufferAddress = globalVertexAddress + handle.vertexOffset;
        }

        // 7) Advance heads
        vertexHead = alignedVertexHead  + vertexBytes;
        indexHead = alignedIndexHead + indexBytes;

        // 8) Cleanup staging and CPU vectors
        retro::DestroyBuffer(engine->GetAllocator(), staging);
		meshes.clear();

		return handle;
    }
}