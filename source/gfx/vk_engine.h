#pragma once
#include <gfx/vk_mesh_manager.h>
#include <gfx/vk_debug.h>
#include <gfx/vk_initializers.h>
#include <gfx/vk_pipelines.h>

#include <resources/vk_descriptors.h>
#include <resources/vk_images.h>
#include <resources/vk_buffer.h>
#include <resources/vk_push_constants.h>
#include <resources/vk_mesh.h>

#include <chrono>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "resources/DeletionQueue.h"
#include "tools/FileManager.h"
#include "gfx/vk_swapchain.h"
#include "tools/Time.h"

struct FrameCommander 
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	VkSemaphore renderSemaphore;
	VkFence renderFence;
};

constexpr unsigned int FRAME_OVERLAP = 3;

class VulkanEngine {
public:
	VulkanEngine() = default;
	VulkanEngine& Get() { return *this; }
	VmaAllocator GetAllocator() { return allocator; }
	VkDevice GetDevice() { return device; }

	//initializes everything in the engine
	void Init();
	//shuts down the engine
	void Cleanup();
	//draw loop
	void Draw();
	//run main loop
	void Run();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);   
private:
	FrameCommander& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };

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

	bool bufferDeviceAddress;
	bool isInitialized{ false };
	bool framebufferResized = false;
	bool stopRendering{ false };
	bool VsyncEnabled{ false };
	int frameNumber{ 0 };

	//Mesh Stuff
	retro::GPUMeshHandle testMesh;
	retro::GPUMeshHandle testMesh2;
private:
	bool engineDebug = true;

	FrameCommander frames[FRAME_OVERLAP];
	DeletionQueue mainDeletionQueue;

	VulkanEngine* loadedEngine = nullptr;

	std::shared_ptr<Time::TimeTracker> frameTimer;
};

