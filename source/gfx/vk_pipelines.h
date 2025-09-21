#pragma once 
#include <gfx/vk_initializers.h>
#include <gfx/vk_types.h>

#include <tools/FilePathManager.h>
#include <tools/ShaderCompiler.h>

class GraphicsPipeline
{
public:
	GraphicsPipeline(VkDevice dev);
	~GraphicsPipeline();
private:
	//internal functions

	template<typename vertexBufferType>
	void CreateVertexShaderModule(const char* fileName)
	{
		if (ShaderCompiler::LoadShaderModule(FilePathManager::GetShaderPath(fileName).string(), device, &vertexShader))
		{
			fmt::print("Error when building the vertex shader \n");
		}
		VkPipelineShaderStageCreateInfo vertexStageInfo{};
		vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStageInfo.pNext = nullptr;
		vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStageInfo.module = vertexShader;
		vertexStageInfo.pName = "main";

		pushConstants.push_back(VkPushConstantRange{});
		pushConstants[0].offset = 0;
		pushConstants[0].size = sizeof(vertexBufferType);
		pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}
	template<typename fragmentBufferType>
	void CreateFragmentShaderModule(const char* filePath)
	{
		if (!ShaderCompiler::LoadShaderModule(FilePathManager::GetShaderPath("hardcoded_triangle_red.frag.spv").string(), device, &fragmentShader)) 
		{
			fmt::print("Error when building the fragment shader \n");
		}
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.pNext = nullptr;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShader;
		fragmentShaderStageInfo.pName = "main";
	}


public:
	//Create infos here
	
	//Shaders//
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	//Shaders//
	//Dynamic State//
	VkPipelineDynamicStateCreateInfo dynamicState{};
	//Dynamic State//
private:
	VkDevice device; //logical device


	//Internal handles
	//----------------------------------Shader Stages----------------------------------//
	VkShaderModule vertexShader;
	VkShaderModule fragmentShader;
	std::vector<VkPushConstantRange> pushConstants;
	//----------------------------------Shader Stages----------------------------------//
	//----------------------------------Dynamic State----------------------------------//
	VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
	//----------------------------------Dynamic State----------------------------------//

};
