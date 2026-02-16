#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <gfx/Window.h>
#include <gfx/vk_swapchain.h>
#include <gfx/vk_pipelines.h>
#include <gfx/vk_mesh_manager.h>

#include <resources/DeletionQueue.h>
#include <resources/vk_mesh.h>
#include <resources/vk_descriptors.h>

#include <components/Renderable.h>

#include <tools/Time.h>

#include <core/camera.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


//GPU-Driven Rendering
//If you want to achieve the maximum possible performance in your engine, the next logical step after Bindless Materials is Multi - Draw Indirect(MDI) combined with Compute Culling.
//
//Here is how the ultimate pipeline works :
// 
//Materials: Bindless Textures + Material buffer SSBOs
//
//The Object Buffer(SSBO) : Just like you made an SSBO for Materials, you make one massive SSBO for Objects.It holds the Transform Matrix and Material ID for every object in your entire game world.
//
//The Indirect Buffer : You create a special Vulkan buffer that holds the raw hardware instructions for drawing(Index Count, First Index, Vertex Offset, etc.).
//
//Compute Shader Culling(The Magic) : At the start of the frame, you run a Compute Shader.The GPU looks at every object in the Object SSBO, checks if it is inside the camera's view frustum, and if it is, the GPU writes a draw command into the Indirect Buffer.
//
//One Draw Call : On the CPU, you no longer have a for loop.You make exactly one API call for the entire scene :
//vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer, ...)


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
	retro::Buffer cameraBuffer;
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

    void AddRenderable(const retro::Renderable& renderable)
    {
        renderables.push_back(renderable);
	}

    void RemoveRenderable(const retro::Renderable& renderable)
    {
        renderables.erase(std::remove(renderables.begin(), renderables.end(), renderable), renderables.end());
    }

    std::shared_ptr<retro::Material> CreateMaterial(std::shared_ptr<retro::Texture> texture);

	Camera& GetMainCamera() { return mainCamera; }
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

	// Bindless descriptor set layout (for global resources like camera buffers, texture arrays, etc.)
    VkDescriptorSetLayout bindlessLayout;
    VkDescriptorSetLayout materialLayout;
    retro::DescriptorAllocator globalDescriptorAllocator;

    // Frame management
    FrameData frames[FRAME_OVERLAP];

    //Timers
    std::shared_ptr<Time::TimeTracker> frameTimer;
    std::shared_ptr<Time::TimeTracker> printCheckerTimer;

    //Actual Data
    std::vector<retro::Renderable> renderables;
    Camera mainCamera;
private:
    int currentFrameIndex = 0;
    bool isInitialized{ false };
    bool vsyncEnabled{ false };
    int selectedFrameBuf = 0;
    bool gpuAvailable = false;
};