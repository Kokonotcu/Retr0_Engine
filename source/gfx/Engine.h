#pragma once
#include <gfx/vk_mesh_manager.h>
#include <gfx/vk_debug.h>
#include <gfx/vk_initializers.h>
#include <gfx/Renderer.h>

#include <resources/vk_images.h>
#include <resources/vk_buffer.h>
#include <resources/vk_push_constants.h>

#include "tools/FileManager.h"

constexpr unsigned int FRAME_OVERLAP = 3;

class Engine 
{
public:
	Engine() = default;
	Engine& Get() { return *this; }
	VmaAllocator GetAllocator() { return allocator; }
	VkDevice GetDevice() { return device; }

	//initializes everything in the engine
	void Init();
	//shuts down the engine
	void Cleanup();
	//run main loop
	void Run();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);   
private:

	void InitVulkan();
	bool CheckValidationLayerSupport();
	
	void InitCommands();
	void InitSyncStructures();

	void InitDefaultMesh();

public:
	///////////////////////////////////////////////////////////////
	Window window;
	Renderer renderer;

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
	//Swapchain swapchain;
	//GraphicsPipeline graphicsPipeline;

	bool bufferDeviceAddress;
	bool isInitialized{ false };
	bool VsyncEnabled{ false };
	int selectedFrameBuf = 0;
	bool gpuAvailable = false;

	//Mesh Stuff
	//retro::GPUMeshHandle testMesh;
	//retro::GPUMeshHandle testMesh2;

	std::shared_ptr<retro::CPUMesh> testMesh;
	std::shared_ptr<retro::CPUMesh> testMesh2;
private:
	bool engineDebug = true;

	DeletionQueue mainDeletionQueue;

	Engine* loadedEngine = nullptr;
};

