#include "FileManager.h"

namespace FileManager 
{
	namespace modelLoader 
	{
		namespace
		{
            enum class ModelExtension
            {
                NOTSUPPORTED,
                FBX,
                GLB,
                OBJ,
                _3DS
            };

            ModelExtension DetermineExtension(std::string extension)
            {
                if (extension == ".fbx")
                    return ModelExtension::FBX;
                else if (extension == ".glb")
                    return ModelExtension::GLB;
                else if (extension == ".obj")
                    return ModelExtension::OBJ;
                else if (extension == ".3ds")
                    return ModelExtension::_3DS;

                throw("File extension isn't supported.");
                return ModelExtension::NOTSUPPORTED;
            }

			std::vector<std::shared_ptr<retro::CPUMesh>> LoadGltfMeshes(fs::path filePath) 
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

                std::vector<std::shared_ptr<retro::CPUMesh>> meshes;

                for (fastgltf::Mesh& gltfMesh : gltf.meshes) {

					meshes.push_back(std::make_shared<retro::CPUMesh>());
					meshes.back()->name = gltfMesh.name;

                    for (auto&& p : gltfMesh.primitives) {
                        retro::Submesh newSurface;

                        newSurface.startIndex = (uint32_t)meshes.back()->indices.size();
                        newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

						meshes.back()->submeshes.push_back(newSurface);

                        size_t initial_vtx = meshes.back()->vertices.size();

                        // load indexes
                        {
                            fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                            meshes.back()->indices.reserve(meshes.back()->indices.size() + indexaccessor.count);

                            fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                                [&](std::uint32_t idx) {
                                    meshes.back()->indices.push_back(idx + initial_vtx);
                                });
                        }

                        // load vertex positions
                        {
                            fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                            meshes.back()->vertices.resize(meshes.back()->vertices.size() + posAccessor.count);

                            fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                [&](glm::vec3 v, size_t index) {
                                    retro::Vertex newvtx;
                                    newvtx.position = v;
                                    newvtx.normal = { 1, 0, 0 };
                                    newvtx.color = glm::vec4{ 1.f };
                                    newvtx.uv_x = 0;
                                    newvtx.uv_y = 0;
                                    meshes.back()->vertices[initial_vtx + index] = newvtx;
                                });
                        }

                        // load vertex normals
                        auto normals = p.findAttribute("NORMAL");
                        if (normals != p.attributes.end()) {

                            fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                                [&](glm::vec3 v, size_t index) {
                                    meshes.back()->vertices[initial_vtx + index].normal = v;
                                });
                        }

                        // load UVs
                        auto uv = p.findAttribute("TEXCOORD_0");
                        if (uv != p.attributes.end()) {

                            fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                                [&](glm::vec2 v, size_t index) {
                                    meshes.back()->vertices[initial_vtx + index].uv_x = v.x;
                                    meshes.back()->vertices[initial_vtx + index].uv_y = v.y;
                                });
                        }

                        // load vertex colors
                        auto colors = p.findAttribute("COLOR_0");
                        if (colors != p.attributes.end()) {

                            fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                                [&](glm::vec4 v, size_t index) {
                                    meshes.back()->vertices[initial_vtx + index].color = v;
                                });
                        }
                    }

                    // display the vertex normals
                    constexpr bool OverrideColors = true;
                    if (OverrideColors) {
                        for (retro::Vertex& vtx : meshes.back()->vertices) {
                            vtx.color = glm::vec4(vtx.normal, 1.f);
                        }
                    }
                }

                return meshes;
            }
		}
	    
    }
}