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
				retro::print("FileManager : Loading GLTF " + filePath.string() + "\n");
#endif // DEBUG
#ifdef __ANDROID__
				auto bytes = ReadAssetCrossPlatform(filePath.string());
				if (bytes.empty()) {
					retro::print("FileManager : Failed to read {}\n", filePath.string());
					return {};
				}
				constexpr size_t HARD_CAP = 256u * 1024u * 1024u;
				if (bytes.size() > HARD_CAP) {
					retro::print("FileManager : Refusing to parse {} ({} bytes > cap)\n",
						filePath.string(), bytes.size());
					return{};
				}

				auto data = fastgltf::GltfDataBuffer::FromBytes(
					reinterpret_cast<const std::byte*>(bytes.data()), bytes.size());
				if (data.error() != fastgltf::Error::None) {
					retro::print("FileManager : GltfDataBuffer::FromBytes failed for {}\n", filePath.string());
					return{};
				}
#else
				auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
#endif // __ANDROID__

                constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

				
                fastgltf::Asset gltf;
                fastgltf::Parser parser{};


				const auto ext = filePath.extension().string();
				auto load = parser.loadGltf(data.get(), ".", gltfOptions, fastgltf::Category::All);
				

				
				if (load) {
					gltf = std::move(load.get());
				}
				else {
					retro::print("FileManager : Failed to load glTF ");
					retro::print(filePath.string().c_str());
					//retro::print(std::to_string(fastgltf::to_underlying(load.error())).c_str());
					return {};
				}

                std::vector<std::shared_ptr<retro::CPUMesh>> meshes;

                for (fastgltf::Mesh& gltfMesh : gltf.meshes) 
				{

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
							if (auto pos = p.findAttribute("POSITION")) {
								auto& posAccessor = gltf.accessors[pos->accessorIndex];
								meshes.back()->vertices.resize(meshes.back()->vertices.size() + posAccessor.count);

								fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
									[&](glm::vec3 v, size_t index) {
										retro::Vertex newvtx;
										newvtx.position = v;
										newvtx.normal = { 1, 0, 0 };
										newvtx.color = glm::u8vec4(255);
										newvtx.uv.x = 0;
										newvtx.uv.y = 0;
										meshes.back()->vertices[initial_vtx + index] = newvtx;
									});
							}
                        }

						if (auto normals = p.findAttribute("NORMAL")) {
							auto& nrmAccessor = gltf.accessors[normals->accessorIndex];
							fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, nrmAccessor,
								[&](glm::vec3 v, size_t index) {
									meshes.back()->vertices[initial_vtx + index].normal = v;
								});
						}

						// load UVs (float2 -> two 16-bit halfs)
						if (auto uv = p.findAttribute("TEXCOORD_0")) {
							auto& uvAccessor = gltf.accessors[uv->accessorIndex];
							fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, uvAccessor,
								[&](glm::vec2 v, size_t index) {
									auto& vert = meshes.back()->vertices[initial_vtx + index];
									vert.uv.x = glm::packHalf1x16(v.x);
									vert.uv.y = glm::packHalf1x16(v.y);
								});
						}

						// load vertex colors (float4 0..1 -> RGBA8 UNORM)
						//if (auto colors = p.findAttribute("COLOR_")) {
						//	auto& colAccessor = gltf.accessors[colors->accessorIndex];
						//	fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colAccessor,
						//		[&](glm::vec4 v, size_t index) {
						//			auto& vert = meshes.back()->vertices[initial_vtx + index];
						//			vert.color = glm::u8vec4(
						//				glm::clamp(int(v.r * 255.f + 0.5f), 0, 255),
						//				glm::clamp(int(v.g * 255.f + 0.5f), 0, 255),
						//				glm::clamp(int(v.b * 255.f + 0.5f), 0, 255),
						//				glm::clamp(int(v.a * 255.f + 0.5f), 0, 255));
						//		});
						//}
                    }

                    // display the vertex normals
                    constexpr bool OverrideColors = true;
                    if (OverrideColors) 
					{
                        for (retro::Vertex& vtx : meshes.back()->vertices) 
						{
							glm::vec3 n01 = vtx.normal * 0.5f;
							vtx.color = glm::u8vec4(
								(uint8_t)glm::clamp(int(n01.r * 255.0f + 0.5f), 0, 255),
								(uint8_t)glm::clamp(int(n01.g * 255.0f + 0.5f), 0, 255),
								(uint8_t)glm::clamp(int(n01.b * 255.0f + 0.5f), 0, 255),
								255
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
				retro::print("Shader compilation error: ");
				retro::print(e.what());
				retro::print("\n");
			}

		}

		bool LoadShaderModule(std::string filePath,
			VkDevice device,
			VkShaderModule* outShaderModule)
		{
			// filePath should be relative, e.g. "shaders/triangle.vert.spv"
			auto bytes = FileManager::ReadAssetCrossPlatform(filePath);
			if (bytes.empty()) {
				retro::print("LoadShaderModule: cannot read ");
				retro::print(filePath);
				return false;
			}

			// SPIR-V is uint32_t-aligned; copy into u32 buffer
			if (bytes.size() % 4 != 0) {
				retro::print("LoadShaderModule: " +  filePath + "size{} not multiple of 4" +  std::to_string(bytes.size()));
				return false;
			}
			std::vector<uint32_t> code(bytes.size() / 4);
			std::memcpy(code.data(), bytes.data(), bytes.size());

			VkShaderModuleCreateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			ci.codeSize = bytes.size();
			ci.pCode = code.data();

			return vkCreateShaderModule(device, &ci, nullptr, outShaderModule) == VK_SUCCESS;
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
	std::vector<char> ReadAssetCrossPlatform(const std::string& relPath)
	{
		
#if defined(__ANDROID__)
		retro::print("reading from: ");
		retro::print(relPath.c_str());
		size_t sz = 0;
		void* mem = SDL_LoadFile(relPath.c_str(), &sz);   // relPath like "shaders/triangle.vert.spv"
		if (!mem) {
			retro::print("ReadAssetAll failed: " + relPath + SDL_GetError());
			return {};
		}
		std::vector<char> out(sz);
		std::memcpy(out.data(), mem, sz);
		SDL_free(mem);
		return out;
#else
		std::ifstream f(relPath, std::ios::binary | std::ios::ate);
		if (!f.is_open()) {
			retro::print("ReadAssetAll failed (desktop): " +  relPath);
			return {};
		}
		auto sz = (size_t)f.tellg();
		std::vector<char> out(sz);
		f.seekg(0);
		f.read(out.data(), sz);
		return out;
#endif
	}
}