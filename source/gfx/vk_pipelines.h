#pragma once 
#include <gfx/vk_initializers.h>
#include <resources/vk_debug.h>

#include <tools/FileManager.h>

class GraphicsPipeline
{
public:
	GraphicsPipeline() = default;
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
		if (!FileManager::ShaderCompiler::LoadShaderModule(FileManager::Path::GetShaderPath(fileName).string(), device, &vertexShader))
		{
			retro::print("Error when building the vertex shader \n");
		}
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.pNext = nullptr;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShader;
		vertexShaderStageInfo.pName = "main";

		pushConstants.push_back(VkPushConstantRange{});
		pushConstants.back().offset = 0;
		pushConstants.back().size = sizeof(vertexBufferType);
		pushConstants.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}
	void CreateVertexShaderModule(const char* fileName);
	template<typename fragmentBufferType>
	void CreateFragmentShaderModule(const char* filePath)
	{
		if (!FileManager::ShaderCompiler::LoadShaderModule(FileManager::Path::GetShaderPath(filePath).string(), device, &fragmentShader))
		{
			retro::print("Error when building the fragment shader \n");
		}
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.pNext = nullptr;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShader;
		fragmentShaderStageInfo.pName = "main";

		
		uint32_t siz = pushConstants.back().size;
		pushConstants.push_back(VkPushConstantRange{});
		pushConstants.back().offset = siz;
		pushConstants.back().size = sizeof(fragmentBufferType);
		pushConstants.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	void CreateFragmentShaderModule(const char* filePath);

	void CreateDynamicState();

	void CreateBDAVertexInput();
	void CreateClassicVertexInput();

	void CreateInputAssembly();

	void CreateRasterizer();

	void CreateMultisampling();

	void CreateBlending(uint32_t colorBlendFlag, VkCompareOp depthOperation);

	void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& layouts);

	void CreateGraphicsPipeline(VkRenderPass renderPass);
private:
	//Create infos 
	
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};

	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};

	VkPipelineRasterizationStateCreateInfo rasterizerInfo{};

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

	//-------------------------------Blending Stage--------------------------------//
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	//VkRenderingAttachmentInfo depthAttachment{};
	//-------------------------------Blending Stage--------------------------------//

	//-------------------------------Vertex Input--------------------------------//
	VkVertexInputBindingDescription bindingDescription{};
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
	//-------------------------------Vertex Input--------------------------------//

	//-------------------------------Pipeline Layout--------------------------------//
	VkPipelineLayout pipelineLayout{};
	VkPipeline pipeline{};
	//-------------------------------Pipeline Layout--------------------------------//
};
