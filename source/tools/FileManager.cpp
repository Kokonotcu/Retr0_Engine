#include "FileManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

			std::vector<std::shared_ptr<retro::Mesh>> LoadGltfMeshes(fs::path filePath) 
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

                std::vector<std::shared_ptr<retro::Mesh>> meshes;

                for (fastgltf::Mesh& gltfMesh : gltf.meshes) 
				{

					meshes.push_back(std::make_shared<retro::Mesh>());
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

						for (int i = 0; i < meshes.back()->vertices.size(); i++)
						{
							meshes.back()->vertices[i].color = { 255,255,255,255 };
						}

						// load vertex colors
						if (auto colors = p.findAttribute("COLOR_0")) { // Fixed attribute name
							auto& colAccessor = gltf.accessors[colors->accessorIndex];

							// Branch based on whether the GLTF file stores RGB or RGBA colors
							if (colAccessor.type == fastgltf::AccessorType::Vec4) {
								fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colAccessor,
									[&](glm::vec4 v, size_t index) {
										auto& vert = meshes.back()->vertices[initial_vtx + index];
										vert.color = glm::u8vec4(
											glm::clamp(int(v.r * 255.f + 0.5f), 0, 255),
											glm::clamp(int(v.g * 255.f + 0.5f), 0, 255),
											glm::clamp(int(v.b * 255.f + 0.5f), 0, 255),
											glm::clamp(int(v.a * 255.f + 0.5f), 0, 255));
									});
							}
							else if (colAccessor.type == fastgltf::AccessorType::Vec3) {
								fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, colAccessor,
									[&](glm::vec3 v, size_t index) {
										auto& vert = meshes.back()->vertices[initial_vtx + index];
										vert.color = glm::u8vec4(
											glm::clamp(int(v.r * 255.f + 0.5f), 0, 255),
											glm::clamp(int(v.g * 255.f + 0.5f), 0, 255),
											glm::clamp(int(v.b * 255.f + 0.5f), 0, 255),
											255); // Hardcode Alpha to fully opaque
									});
							}
						}
                    }

                    //// display the vertex normals
                    //constexpr bool OverrideColors = false;
                    //if (OverrideColors) 
					//{
                    //    for (retro::Vertex& vtx : meshes.back()->vertices) 
					//	{
					//		glm::vec3 n01 = vtx.normal * 0.5f;
					//		vtx.color = glm::u8vec4(
					//			(uint8_t)glm::clamp(int(n01.r * 255.0f + 0.5f), 0, 255),
					//			(uint8_t)glm::clamp(int(n01.g * 255.0f + 0.5f), 0, 255),
					//			(uint8_t)glm::clamp(int(n01.b * 255.0f + 0.5f), 0, 255),
					//			255
					//		);
                    //    }
                    //}
                }

                return meshes;
            }
		}
	    
		std::vector<std::shared_ptr<retro::Mesh>> LoadMeshFromFile(fs::path filePath)
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

	namespace TextureLoader
	{
		bool LoadTexture(std::string filePath, retro::Texture& outTexture,
			VkDevice device, VmaAllocator allocator,
			VkQueue copyQueue, VkCommandPool commandPool) 
		{
			// 1. Load Image from Disk (CPU)
			int texWidth, texHeight, texChannels;

			// Force 4 channels (RGBA) for consistency
			stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels) 
			{
				retro::print("Failed to load texture file: " + filePath + "\n");
				return false;
			}

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			// 2. Create Staging Buffer (Upload Buffer)
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = imageSize;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VmaAllocationCreateInfo vmaAllocInfo = {};
			vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

			VkBuffer stagingBuffer;
			VmaAllocation stagingAllocation;
			vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);

			// 3. Copy Pixel Data to Staging Buffer
			void* data;
			vmaMapMemory(allocator, stagingAllocation, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vmaUnmapMemory(allocator, stagingAllocation);

			stbi_image_free(pixels); // Free CPU copy

			// 4. Create GPU Image
			retro::Image& newImage = outTexture.image;
			newImage.imageExtent = { (uint32_t)texWidth, (uint32_t)texHeight, 1 };
			newImage.imageFormat = VK_FORMAT_R8G8B8A8_SRGB; // Standard texture format

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent = newImage.imageExtent;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = newImage.imageFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo imgAllocInfo = {};
			imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &newImage.image, &newImage.allocation, nullptr);

			// 5. Execute Command to Copy Buffer -> Image
			// We manually allocate a temporary command buffer here to avoid dependency on Engine helper functions
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer cmd;
			vkAllocateCommandBuffers(device, &allocInfo, &cmd);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(cmd, &beginInfo);

			// Transition: Undefined -> Transfer DST
			VkImageMemoryBarrier barrierToTransfer = {};
			barrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrierToTransfer.srcAccessMask = 0;
			barrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrierToTransfer.image = newImage.image;
			barrierToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrierToTransfer.subresourceRange.baseMipLevel = 0;
			barrierToTransfer.subresourceRange.levelCount = 1;
			barrierToTransfer.subresourceRange.baseArrayLayer = 0;
			barrierToTransfer.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToTransfer);

			// Copy
			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = newImage.imageExtent;

			vkCmdCopyBufferToImage(cmd, stagingBuffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition: Transfer DST -> Shader Read Only
			VkImageMemoryBarrier barrierToShader = barrierToTransfer;
			barrierToShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrierToShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrierToShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrierToShader.dstAccessMask = 0;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToShader);

			vkEndCommandBuffer(cmd);

			// Submit and Wait
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmd;

			vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(copyQueue); // Block until upload is done

			// Cleanup temporary resources
			vkFreeCommandBuffers(device, commandPool, 1, &cmd);
			vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

			// 6. Create ImageView
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = newImage.image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = newImage.imageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &viewInfo, nullptr, &newImage.imageView) != VK_SUCCESS) {
				retro::print("Failed to create texture image view!\n");
				return false;
			}

			// 7. Create Sampler
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_NEAREST; // Retro style
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			// Note: Ideally you query physical device properties for maxAnisotropy, but 16.0f is standard for modern GPUs
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy = 1.0f;

			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			if (vkCreateSampler(device, &samplerInfo, nullptr, &outTexture.sampler) != VK_SUCCESS) 
			{
				retro::print("Failed to create texture sampler!\n");
				return false;
			}

			retro::print("Texture loaded successfully: " + filePath + "\n");
			return true;
		}
	}
}
