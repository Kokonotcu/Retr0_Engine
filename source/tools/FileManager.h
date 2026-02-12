#pragma once
#include <fstream>
#include <filesystem>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>                 
#include <fastgltf/glm_element_traits.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/packing.hpp>

#include <vulkan/vulkan.h>

#include <shaderc/shaderc.hpp>

#include "resources/vk_mesh.h"
#include <tools/printer.h>
#include <SDL3/SDL.h> 

namespace fs = std::filesystem;
namespace FileManager 
{
	std::vector<char> ReadAssetCrossPlatform(const std::string& relPath);
	namespace Path 
	{
		inline fs::path GetAssetsDirectory() {return "assets/";}
		inline fs::path GetAssetPath(std::string filename) { return GetAssetsDirectory().string() + filename; };
		inline fs::path GetShadersDirectory() { return "shaders/"; };
		inline fs::path GetShaderPath(std::string filename) { return GetShadersDirectory().string() + filename; };
	}
	namespace ShaderCompiler
	{
		void CompileFromDir(std::string dir);
		bool LoadShaderModule(std::string filePath, VkDevice device, VkShaderModule* outShaderModule);
	};
	namespace ModelLoader
	{
		std::vector<std::shared_ptr<retro::Mesh>> LoadMeshFromFile(fs::path filePath);
	}
}