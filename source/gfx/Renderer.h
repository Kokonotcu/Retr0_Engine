#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <gfx/Window.h>
#include <gfx/vk_swapchain.h>
#include <gfx/vk_pipelines.h>

#include <resources/DeletionQueue.h>
#include <resources/vk_mesh.h>
#include <resources/vk_descriptors.h>
#include <gfx/vk_mesh_manager.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <tools/Time.h>


// A simple container for the core Vulkan handles that almost every system needs
struct VulkanContext 
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	VmaAllocator allocator;
};

struct FrameData
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	VkSemaphore renderFinished;
	VkFence renderFence;

	retro::DescriptorAllocator frameDescriptorAllocator;
};

constexpr unsigned int FRAME_OVERLAP = 3;

class Renderer 
{
public:
    Renderer() = default;

    // Initialize the renderer resources (Swapchain, Pipelines, Commands)
    void Init(Window& window, const VulkanContext& context);

    // Cleanup renderer resources
    void Shutdown();

    // The main draw command
    void Draw();

    // Handle window resizing
    void Resize(uint32_t width, uint32_t height);

    void AddRenderable(const std::shared_ptr<retro::Mesh>& renderable) 
    {
        renderables.push_back(renderable);
	}

    void RemoveRenderable(const std::shared_ptr<retro::Mesh>& renderable)
    {
        renderables.erase(std::remove(renderables.begin(), renderables.end(), renderable), renderables.end());
    }

private:
    void InitCommands();
    void InitSyncStructures();
    void InitPipelines();
	void InitDescriptors();
    
private:
    void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);

private:
    // References to external objects (We don't own these)
    Window* window{ nullptr };
    VulkanContext vkContext;

    // Renderer-owned resources
    Swapchain swapchain;
    GraphicsPipeline graphicsPipeline; // Could be a map/manager later
    DeletionQueue rendererDeletionQueue;

    VkDescriptorSetLayout bindlessLayout;

    // Frame management
    FrameData frames[FRAME_OVERLAP];

    //Timers
    std::shared_ptr<Time::TimeTracker> frameTimer;
    std::shared_ptr<Time::TimeTracker> printCheckerTimer;

    //Actual Data
    std::vector<std::shared_ptr<retro::Mesh>> renderables;
private:
    int currentFrameIndex = 0;
    bool isInitialized{ false };
    bool vsyncEnabled{ false };
    int selectedFrameBuf = 0;
    bool gpuAvailable = false;
};