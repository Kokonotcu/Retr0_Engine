#pragma once
#include <filesystem>
#include "resources/vk_mesh.h"

#define SIMDJSON_NO_INLINE
#undef simdjson_inline
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>


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
	namespace modelLoader
	{
		
		
	}
}