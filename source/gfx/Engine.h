#pragma once
#include <gfx/vk_mesh_manager.h>
#include <resources/vk_debug.h>
#include <gfx/vk_initializers.h>
#include <gfx/Renderer.h>

#include <resources/vk_images.h>
#include <resources/vk_buffer.h>
#include <resources/vk_push_constants.h>

#include "tools/FileManager.h"


class Engine 
{
public:
	Engine() = default;

	//initializes everything in the engine
	void Init();
	//shuts down the engine
	void Cleanup();
	//run main loop
	void Run();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);   
public:
	//Getters
	Engine& Get() { return *this; }
	VmaAllocator GetAllocator() { return allocator; }
	VkDevice GetDevice() { return device; }

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

	bool bufferDeviceAddress;
	bool isInitialized{ false };
	bool VsyncEnabled{ false };
	int selectedFrameBuf = 0;
	bool gpuAvailable = false;

	//Gpu Mesh Stuff
	//retro::GPUMeshHandle testMesh;
	//retro::GPUMeshHandle testMesh2;

	std::shared_ptr<retro::Mesh> testMesh;
	std::shared_ptr<retro::Mesh> testMesh2;
private:
	bool engineDebug = true;
	DeletionQueue mainDeletionQueue;
	Engine* loadedEngine = nullptr;
};

