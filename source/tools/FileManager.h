#pragma once
#include <fstream>
#include <filesystem>

#define SIMDJSON_NO_INLINE
#undef simdjson_inline
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <shaderc/shaderc.hpp>
#include <fmt/core.h>

#include "gfx/vk_debug.h"
#include "resources/vk_mesh.h"

namespace fs = std::filesystem;
namespace FileManager 
{
	namespace path 
	{
		inline fs::path GetAssetsDirectory() { return "assets/"; };
		inline fs::path GetAssetPath(std::string filename) { return GetAssetsDirectory().string() + filename; };
		inline fs::path GetShadersDirectory() { return "shaders/"; };
		inline fs::path GetShaderPath(std::string filename) { return GetShadersDirectory().string() + filename; };
		//inline fs::path GetExecutablePath() { return ""; };
		//inline fs::path GetExecutableDirectory() { return ""; };
		//inline fs::path CalculateShaderPath(const std::string& shaderName) { return ""; };
		//inline fs::path CalculateAssetPath(const std::string& assetName) { return ""; };
	}
	namespace ShaderCompiler
	{
		void CompileFromDir(std::string dir);
		bool LoadShaderModule(std::string filePath, VkDevice device, VkShaderModule* outShaderModule);
	};
	namespace ModelLoader
	{
		std::vector<std::shared_ptr<retro::CPUMesh>> LoadMeshFromFile(fs::path filePath);
	}
}