#pragma once
#include "resources/vk_buffer.h"
#include "resources/vk_mesh.h"
#include "resources/DeletionQueue.h"

#include <unordered_map>
#include <filesystem>


//forward declaration
class Engine;

namespace MeshManager 
{
    // create the big buffers once
    void Init(Engine* engine, size_t maxVertexBytes, size_t maxIndexBytes);
	void ClearBuffers();

    VkBuffer GetGlobalVertexBuffer();//
    VkBuffer GetGlobalIndexBuffer(); // 
    
	// Load a mesh from a file, return create a CPU mesh or a GPU mesh handle
    std::shared_ptr<retro::Mesh> LoadMeshCPU(std::filesystem::path filePath, int modelIndex);
    retro::GPUMeshHandle LoadMeshGPU(std::filesystem::path filePath, int modelIndex);

	// take a CPUMesh and transfer it to the GPU, returning a handle
    retro::GPUMeshHandle MakeGPU(const retro::Mesh& mesh);

    // free the region for a handle (basic version; no fences shown here)
    void FreeGPU(const retro::GPUMeshHandle& handle);

}