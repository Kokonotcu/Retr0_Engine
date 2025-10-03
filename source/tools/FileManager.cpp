#include "FileManager.h"

namespace FileManager 
{
	namespace ModelLoader 
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
                                    newvtx.uv.x = 0;
                                    newvtx.uv.y = 0;
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

						// load UVs (float2 -> two 16-bit halfs)
						auto uv = p.findAttribute("TEXCOORD_0");
						if (uv != p.attributes.end()) 
						{
							fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
								[&](glm::vec2 v, size_t index) 
								{
									auto& vert = meshes.back()->vertices[initial_vtx + index];
									vert.uv.x = glm::packHalf1x16(v.x); // uint16 half bits
									vert.uv.y = glm::packHalf1x16(v.y); // uint16 half bits
								});
						}

						// load vertex colors (float4 0..1 -> RGBA8 UNORM)
						auto colors = p.findAttribute("COLOR_0");
						if (colors != p.attributes.end()) 
						{
							fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
								[&](glm::vec4 v, size_t index) 
								{
									auto& vert = meshes.back()->vertices[initial_vtx + index];
									// Convert float[0..1] -> u8[0..255] (rounding & clamping)
									vert.color = glm::u8vec4(
										glm::clamp(int(v.r * 255.0f + 0.5f), 0, 255),
										glm::clamp(int(v.g * 255.0f + 0.5f), 0, 255),
										glm::clamp(int(v.b * 255.0f + 0.5f), 0, 255),
										glm::clamp(int(v.a * 255.0f + 0.5f), 0, 255)
									);
								});
						}
                    }

                    // display the vertex normals
                    constexpr bool OverrideColors = true;
                    if (OverrideColors) {
                        for (retro::Vertex& vtx : meshes.back()->vertices) {
                            vtx.color = glm::u8vec4(
								glm::clamp(int(vtx.normal.r * 255.0f + 0.5f), 0, 255),
								glm::clamp(int(vtx.normal.g * 255.0f + 0.5f), 0, 255),
								glm::clamp(int(vtx.normal.b * 255.0f + 0.5f), 0, 255),
								glm::clamp(255, 0, 255)
							);
                        }
                    }
                }

                return meshes;
            }
		}
	    
		std::vector<std::shared_ptr<retro::CPUMesh>> LoadMeshFromFile(fs::path filePath)
		{
			switch (DetermineExtension(filePath.extension().string()))
			{
			case ModelExtension::GLB:
				return LoadGltfMeshes(filePath);
				break;
			case ModelExtension::FBX:
				//return LoadFbxMeshes(filePath);
				break;
			case ModelExtension::OBJ:
				//return LoadObjMeshes(filePath);
				break;
			case ModelExtension::_3DS:
				//return Load3dsMeshes(filePath);
				break;
			default:
				break;
			}
			return {};
		}

}

	namespace ShaderCompiler
	{
		namespace
		{
			enum ShaderExtension
			{
				COMPUTE,
				VERTEX,
				FRAGMENT,
				SPIRV,
				INVALID
			};

			int determineExtension(std::string extension)
			{
				if (extension == ".comp")
					return ShaderExtension::COMPUTE;
				else if (extension == ".frag")
					return ShaderExtension::FRAGMENT;
				else if (extension == ".vert")
					return ShaderExtension::VERTEX;
				else if (extension == ".spv")
					return ShaderExtension::SPIRV;

				throw("File extension isn't supported.");
				return ShaderExtension::INVALID;
			}

			bool compileShader(const fs::path& sourcePath, const fs::path& spvPath, shaderc_shader_kind kind)
			{
				std::ifstream file(sourcePath);
				if (!file.is_open()) throw std::runtime_error("Cannot open shader file: " + sourcePath.string());

				std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

				shaderc::Compiler compiler;
				shaderc::CompileOptions options;

				auto result = compiler.CompileGlslToSpv(source, kind, sourcePath.string().c_str(), options);

				if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
					throw std::runtime_error("Error compiling " + sourcePath.string() + ": " + result.GetErrorMessage());
					return false;
				}

				std::ofstream out(spvPath, std::ios::binary);
				out.write(reinterpret_cast<const char*>(result.cbegin()), (result.cend() - result.cbegin()) * sizeof(uint32_t));
				return true;
			}

			bool needsRecompile(const fs::path& sourcePath, const fs::path& spvPath)
			{
				if (!fs::exists(spvPath)) return true;
				if (!fs::exists(sourcePath)) throw std::runtime_error("Shader source not found: " + sourcePath.string());
				return fs::last_write_time(sourcePath) > fs::last_write_time(spvPath);
			}

			void LoadShaderFromDir(fs::path path, shaderc_shader_kind kind)
			{
				fs::path spvPath = path;
				spvPath.replace_extension(spvPath.extension().string() + ".spv");

				if (needsRecompile(path, spvPath))
				{
					if (!compileShader(path, spvPath, kind))
						throw std::runtime_error("Failed to compile shader: " + path.string());
				}

				//auto data = readSPV(spvPath);

				// Here you can store the SPIR-V binary in a map if you want
				// shaders[path.string()] = std::make_shared<SPIRV>(data);
			}

			void LoadCompFromDir(fs::path path)
			{
				LoadShaderFromDir(path, shaderc_compute_shader);
			}

			void LoadFragFromDir(fs::path path)
			{
				LoadShaderFromDir(path, shaderc_fragment_shader);
			}
			void LoadVertFromDir(fs::path path)
			{
				LoadShaderFromDir(path, shaderc_vertex_shader);
			}
			void LoadSpvFromDir(fs::path path)
			{
				//auto data = readSPV(path);
			}

		}

		void CompileFromDir(std::string dir)
		{
			try
			{
				for (const auto& entry : fs::recursive_directory_iterator(dir))
				{
					if (entry.is_regular_file())
					{
						switch (determineExtension(entry.path().extension().string()))
						{
						case ShaderExtension::COMPUTE:
							LoadCompFromDir(entry.path());
							break;

						case ShaderExtension::FRAGMENT:
							LoadFragFromDir(entry.path());
							break;
						case ShaderExtension::VERTEX:
							LoadVertFromDir(entry.path());
							break;
						case ShaderExtension::SPIRV:
							LoadSpvFromDir(entry.path());
							break;


						default:
							break;
						}
					}
				}
			}
			catch (const std::exception& e)
			{
				fmt::print("Shader compilation error: {}\n", e.what());
			}

		}

		bool LoadShaderModule(std::string filePath,
			VkDevice device,
			VkShaderModule* outShaderModule)
		{
			// open the file. With cursor at the end
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				return false;
			}

			// find what the size of the file is by looking up the location of the cursor
			// because the cursor is at the end, it gives the size directly in bytes
			size_t fileSize = (size_t)file.tellg();

			// spirv expects the buffer to be on uint32, so make sure to reserve a int
			// vector big enough for the entire file
			std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

			// put file cursor at beginning
			file.seekg(0);

			// load the entire file into the buffer
			file.read((char*)buffer.data(), fileSize);

			// now that the file is loaded into the buffer, we can close it
			file.close();

			// create a new shader module, using the buffer we loaded
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;

			// codeSize has to be in bytes, so multply the ints in the buffer by size of
			// int to know the real size of the buffer
			createInfo.codeSize = buffer.size() * sizeof(uint32_t);
			createInfo.pCode = buffer.data();

			// check that the creation goes well.
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				return false;
			}
			*outShaderModule = shaderModule;
			return true;
		}

		std::vector<char> readSPV(const fs::path& spvPath)
		{
			std::ifstream file(spvPath, std::ios::binary | std::ios::ate);
			if (!file.is_open()) throw std::runtime_error("Cannot open SPV file: " + spvPath.string());

			std::vector<char> buffer(file.tellg());
			file.seekg(0);
			file.read(buffer.data(), buffer.size());
			return buffer;
		}

	}
}