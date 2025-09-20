#pragma once 
#include <gfx/vk_types.h>


namespace vkutil {
	bool load_shader_module(std::string filePath, VkDevice device, VkShaderModule* outShaderModule);
};