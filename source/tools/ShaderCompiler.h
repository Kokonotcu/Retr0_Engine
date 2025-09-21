#pragma once
#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <unordered_map>
#include <fmt/core.h>
#include <vulkan/vulkan.h>
#include <gfx/vk_types.h>

namespace ShaderCompiler
{	
	void CompileFromDir(std::string dir);
	bool LoadShaderModule(std::string filePath, VkDevice device, VkShaderModule* outShaderModule);
	
};