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

struct FrameCommander 
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	VkSemaphore renderSemaphore;
	VkFence renderFence;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
	VulkanEngine() = default;
	VulkanEngine& Get() { return *this; }

	//initializes everything in the engine
	void Init();
	//shuts down the engine
	void Cleanup();
	//draw loop
	void Draw();
	//run main loop
	void Run();

	FrameCommander& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };
	GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices); //abstract this out
private:
	void InitVulkan();
	bool CheckValidationLayerSupport();
	
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitGlobalPipelines();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void InitDefaultMesh();

	//submit a function to be executed immediately
	void ImmediateSubmitQueued(std::function<void(VkCommandBuffer cmd)>&& function);   
																			 
	//Utility functions
	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage); //abstract this out
	void DestroyBuffer(const AllocatedBuffer& buffer); //abstract this out
public:
	///////////////////////////////////////////////////////////////
	struct SDL_Window* window{ nullptr };
	VkSurfaceKHR surface;// Vulkan window surface
	VkExtent2D windowExtent{ 800 ,600 };

	VkInstance instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT debugMessenger;// Vulkan debug output handle

	VkPhysicalDevice chosenGPU;// GPU chosen as the default device
	VkDevice device; // Vulkan device for commands

	///////////////////////////////////////////////////////////////
	VkQueue graphicsQueue;
	uint32_t graphicsQueueIndex;

	VkQueue immediateQueue;
	uint32_t immediateQueueIndex;
	VkFence immFence;
	VkCommandBuffer immCommandBuffer;
	VkCommandPool immCommandPool;
	///////////////////////////////////////////////////////////////

	VmaAllocator allocator;
	Swapchain swapchain;
	GraphicsPipeline graphicsPipeline;

	bool isInitialized{ false };
	int frameNumber{ 0 };
	bool framebufferResized = false;
	bool stopRendering{ false };
	bool VsyncEnabled{ true };

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;


	//Mesh Stuff
	std::vector<std::shared_ptr<MeshAsset>> testMeshes; //abstract this out
private:

	FrameCommander frames[FRAME_OVERLAP];
	DeletionQueue mainDeletionQueue;

	VulkanEngine* loadedEngine = nullptr;
};

