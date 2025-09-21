#pragma once
#include <gfx/vk_loader.h>
#include <gfx/vk_types.h>
#include <gfx/vk_images.h>
#include <gfx/vk_initializers.h>
#include <gfx/vk_types.h>
#include <gfx/vk_pipelines.h>

#include <resources/vk_descriptors.h>

#include <chrono>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "tools/ShaderCompiler.h"
#include "resources/DeletionQueue.h"
#include "tools/FilePathManager.h"
#include "resources/vk_swapchain.h"

struct FrameData 
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	VkSemaphore swapchainSemaphore;
	VkFence renderFence;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
	VulkanEngine() = default;
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
	GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
private:

	void InitVulkan();
	bool CheckValidationLayerSupport();

	//void InitSwapchain(bool VsyncEnabled);
	//void RecreateSwapchain(bool VsyncEnabled);
	//void CreateSwapchain(uint32_t width, uint32_t height, bool Vsync);
	//void DestroySwapchain();
	
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	//void InitRenderPasses();
	void InitPipelines();
	void InitGlobalPipelines();
	//void InitFramebuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void InitDefaultMesh();

	// immediate submit structures
	//submit a function to be executed immediately
	void ImmediateSubmitQueued(std::function<void(VkCommandBuffer cmd)>&& function);
	VkQueue immediateQueue;
	uint32_t immediateQueueIndex;

	VkFence immFence;
	VkCommandBuffer immCommandBuffer;
	VkCommandPool immCommandPool;
	// immediate submit structures
	
	//Utility functions
	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void DestroyBuffer(const AllocatedBuffer& buffer);
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

	Swapchain swapchain;

	//VkSwapchainKHR swapchain;														//	   d
	//VkFormat swapchainImageFormat;											  //	  d
	//std::vector<VkImage> swapchainImages;									  //  d
	//std::vector<VkImageView> swapchainImageViews;					 //	  d
	//std::vector<VkFramebuffer> swapchainFramebuffers;				 //	  d
	//VkExtent2D swapchainExtent;														//	  d
	//AllocatedImage drawImage;															  //  d
	//VkExtent2D drawExtent;																  //	  disable

	bool isInitialized{ false };
	int frameNumber{ 0 };
	bool framebufferResized = false;
	bool stopRendering{ false };
	bool VsyncEnabled{ true };

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;

	//VkRenderPass renderPass; //disable
	GraphicsPipeline graphicsPipeline;

	//std::vector<VkSemaphore> imagePresentSemaphores; //disable

	//Mesh Stuff
	std::vector<std::shared_ptr<MeshAsset>> testMeshes;
private:
	FrameData frames[FRAME_OVERLAP];

	//DeletionQueue swapchainDestroy;   // size/format-dependent stuff     //disable
	DeletionQueue mainDeletionQueue;      // allocator, long-lived objects
};

