#pragma once

#include <vk_types.h>
#include <vk_images.h>
#include <vk_descriptors.h>
#include <vk_initializers.h>
#include <vk_types.h>
#include <vk_pipelines.h>
#include <chrono>
#include <thread>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Utilities/ShaderCompiler.h"
#include "Utilities/DeletionQueue.h"

struct FrameData 
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	VkSemaphore swapchainSemaphore;
	VkFence renderFence;

	DeletionQueue deletionQueue;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:

	static VulkanEngine& Get();

	//initializes everything in the engine
	void Init();
	//shuts down the engine
	void Cleanup();
	//draw loop
	void Draw();
	//run main loop
	void Run();

	FrameData& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };

private:

	void InitVulkan();
	bool CheckValidationLayerSupport();

	void InitSwapchain(bool VsyncEnabled);
	void CreateSwapchain(uint32_t width, uint32_t height, bool Vsync);
	void DestroySwapchain();
	
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitBackgroundPipelines();

	void DrawBackground(VkCommandBuffer cmd);

public:
	struct SDL_Window* window{ nullptr };

	VkInstance instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT debugMessenger;// Vulkan debug output handle

	VkPhysicalDevice chosenGPU;// GPU chosen as the default device
	VkDevice device; // Vulkan device for commands

	VkQueue graphicsQueue;
	uint32_t graphicsQueueIndex;

	VmaAllocator allocator;

	VkSurfaceKHR surface;// Vulkan window surface
	VkExtent2D windowExtent{ 800 ,600 };

	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;

	bool isInitialized{ false };
	int frameNumber{ 0 };
	bool stopRendering{ false };

	AllocatedImage drawImage;
	VkExtent2D drawExtent;

	DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;

	VkPipeline gradientPipeline;
	VkPipelineLayout gradientPipelineLayout;		 //Imgui stuff

	std::vector<VkSemaphore> imagePresentSemaphores;

private:

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 0 };

	FrameData frames[FRAME_OVERLAP];

	DeletionQueue swapchainDestroy;   // size/format-dependent stuff
	DeletionQueue mainDeletionQueue;      // allocator, long-lived objects
};

