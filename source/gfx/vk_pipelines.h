#pragma once 
#include <gfx/vk_initializers.h>
#include <gfx/vk_types.h>

#include <tools/FilePathManager.h>
#include <tools/ShaderCompiler.h>

class GraphicsPipeline
{
public:
	GraphicsPipeline();
	//~GraphicsPipeline();
	void SetDevice(VkDevice dev)
	{
		device = dev;
	}

public:
	void UpdateDynamicState(VkCommandBuffer cmd, VkExtent2D extent);

	VkPipeline GetPipeline()
	{
		return pipeline;
	}
	VkPipelineLayout GetPipelineLayout()
	{
		return pipelineLayout;
	}

	void ClearPipeline();
public:
	//internal functions

	template<typename vertexBufferType>
	void CreateVertexShaderModule(const char* fileName)
	{
		if (ShaderCompiler::LoadShaderModule(FilePathManager::GetShaderPath(fileName).string(), device, &vertexShader))
		{
			fmt::print("Error when building the vertex shader \n");
		}
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.pNext = nullptr;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShader;
		vertexShaderStageInfo.pName = "main";

		pushConstants.push_back(VkPushConstantRange{});
		pushConstants[0].offset = 0;
		pushConstants[0].size = sizeof(vertexBufferType);
		pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}
	void CreateVertexShaderModule(const char* fileName);
	template<typename fragmentBufferType>
	void CreateFragmentShaderModule(const char* filePath)
	{
		if (!ShaderCompiler::LoadShaderModule(FilePathManager::GetShaderPath(filePath).string(), device, &fragmentShader))
		{
			fmt::print("Error when building the fragment shader \n");
		}
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.pNext = nullptr;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShader;
		fragmentShaderStageInfo.pName = "main";

		if (pushConstants.size() > 0)
		{
			pushConstants.push_back(VkPushConstantRange{});
			pushConstants[1].offset = pushConstants[0].size;
			pushConstants[1].size = sizeof(fragmentBufferType);
			pushConstants[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else
		{
			pushConstants.push_back(VkPushConstantRange{});
			pushConstants[0].offset = 0;
			pushConstants[0].size = sizeof(fragmentBufferType);
			pushConstants[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		
	}
	void CreateFragmentShaderModule(const char* filePath);

	void CreateDynamicState();

	void CreateVertexInput();

	void CreateInputAssembly();

	void CreateRasterizer();

	void CreateMultisampling();

	void CreateBlending(VkImageView view, bool depthWriteEnable, VkCompareOp op, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	void CreatePipelineLayout();

	void CreateGraphicsPipeline(VkRenderPass renderPass);
private:
	//Create infos 
	
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};

	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};

	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
private:
	//instance handles

	VkDevice device{}; //logical device

	//----------------------------------Shader Stages----------------------------------//
	VkShaderModule vertexShader{};
	VkShaderModule fragmentShader{};
	std::vector<VkPushConstantRange> pushConstants{};
	//----------------------------------Shader Stages----------------------------------//
	//----------------------------------Dynamic State----------------------------------//
	VkRect2D scissor{};
	VkViewport viewport{};
	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
	//----------------------------------Dynamic State----------------------------------//

	//-------------------------------Rasterizer Stage--------------------------------//
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	//-------------------------------Rasterizer Stage--------------------------------//

	//-------------------------------Blending Stage--------------------------------//
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	VkRenderingAttachmentInfo depthAttachment{};
	//-------------------------------Blending Stage--------------------------------//

	//-------------------------------Pipeline Layout--------------------------------//
	VkPipelineLayout pipelineLayout{};
	VkPipeline pipeline{};
	//-------------------------------Pipeline Layout--------------------------------//
};
