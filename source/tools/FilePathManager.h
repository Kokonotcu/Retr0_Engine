#pragma once
#include <filesystem>

namespace fs = std::filesystem;
namespace FilePathManager {
	//inline fs::path GetExecutablePath() { return ""; };
	//inline fs::path GetExecutableDirectory() { return ""; };
	inline fs::path GetAssetsDirectory() { return "assets/"; };
	inline fs::path GetAssetPath(std::string filename) { return GetAssetsDirectory().string() + filename; };
	inline fs::path GetShadersDirectory() { return "shaders/"; };
	inline fs::path GetShaderPath(std::string filename) { return GetShadersDirectory().string() + filename; };
	//inline fs::path CalculateShaderPath(const std::string& shaderName) { return ""; };
	//inline fs::path CalculateAssetPath(const std::string& assetName) { return ""; };
};