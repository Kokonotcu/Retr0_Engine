#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "resources/Texture.h"

namespace retro 
{
	struct Material 
	{
		// The texture loaded via FileManager
		std::shared_ptr<Texture> albedoTexture;

		// The descriptor set specific to this material (Set 1)
		VkDescriptorSet materialSet{ VK_NULL_HANDLE };

		// Later on, can add:
		// VkPipeline pipeline;
		// glm::vec4 baseColorFactor;
		// float roughness;
	};
}