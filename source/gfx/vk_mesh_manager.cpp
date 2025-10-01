#include <gfx/vk_mesh_manager.h>
#include "stb_image.h"

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_debug.h"
#include <glm/gtx/quaternion.hpp>

#define SIMDJSON_NO_INLINE
#undef simdjson_inline
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>


namespace MeshManager
{
    namespace 
    {
		VulkanEngine* engine = nullptr;

        // the big buffers:
		VkDeviceAddress globalVertexAddress = 0;
        retro::Buffer globalVertexBuffer;
        retro::Buffer globalIndexBuffer;

        // very dumb bump allocators (start at 0, move forward):
        VkDeviceSize vertexHead = 0; 
        VkDeviceSize indexHead = 0;

        // helper to copy CPU→staging→bigBuffer
        void Upload(VkBuffer dst, VkDeviceSize dstOffset,
            const void* src, size_t sizeBytes);


    }


    void Init(VulkanEngine* _engine, size_t maxVertexBytes, size_t maxIndexBytes)
    {
        engine = _engine;
        // create the big buffers:

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
    }
}


std::optional<std::vector<std::shared_ptr<retro::GPUMeshHandle>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath)
{
#ifdef DEBUG
	fmt::print("Loading GLTF: {}", filePath.string() + "\n");
#endif // DEBUG


    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
    if (load) {
        gltf = std::move(load.get());
    }
    else {
        fmt::print("Failed to load glTF: {}", fastgltf::to_underlying(load.error()) + "\n");
        return {};
    }

    std::vector<std::shared_ptr<retro::GPUMeshHandle>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<retro::Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        retro::GPUMeshHandle newmesh;

        newmesh.name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            retro::Submesh newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        retro::Vertex newvtx;
                        newvtx.position = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4{ 1.f };
                        newvtx.uv_x = 0;
                        newvtx.uv_y = 0;
                        vertices[initial_vtx + index] = newvtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].color = v;
                    });
            }
            newmesh.submeshes.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors) {
            for (retro::Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        //
        newmesh.meshBuffer = engine->UploadMesh(indices, vertices);
        //
        meshes.emplace_back(std::make_shared<retro::GPUMeshHandle>(std::move(newmesh)));
    }

    return meshes;
}
