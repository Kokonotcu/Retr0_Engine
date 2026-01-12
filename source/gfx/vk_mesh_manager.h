#pragma once
#include "resources/vk_buffer.h"
#include "resources/vk_mesh.h"
#include "resources/DeletionQueue.h"

#include <vector>
#include <unordered_map>
#include <filesystem>


//forward declaration
class VulkanEngine;

namespace MeshManager 
{
    // create the big buffers once
    void Init(VulkanEngine* engine, size_t maxVertexBytes, size_t maxIndexBytes);
	void ClearBuffers();

    VkBuffer GetGlobalVertexBuffer();//
    VkBuffer GetGlobalIndexBuffer(); // 
    
	// Load a mesh from a file, return create a CPU mesh or a GPU mesh handle
    std::shared_ptr<retro::CPUMesh> LoadMeshCPU(std::filesystem::path filePath, int modelIndex);
    retro::GPUMeshHandle LoadMeshGPU(std::filesystem::path filePath, int modelIndex);

    // Load multiple meshes from the same file efficiently (avoids duplicate file parsing)
    std::vector<std::shared_ptr<retro::CPUMesh>> LoadMultipleMeshesCPU(std::filesystem::path filePath, const std::vector<int>& modelIndices);

	// take a CPUMesh and transfer it to the GPU, returning a handle
    retro::GPUMeshHandle MakeGPU(const retro::CPUMesh& mesh);

    // free the region for a handle (basic version; no fences shown here)
    void FreeGPU(const retro::GPUMeshHandle& handle);

}