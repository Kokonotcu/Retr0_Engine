#pragma once
#include <gfx/vk_types.h>
#include <VkBootstrap.h>
#include <resources/DeletionQueue.h>

class Swapchain
{
public:
	Swapchain() = default;
	//~Swapchain();

	void SetProperties(VmaAllocator _allocator, VkPhysicalDevice gpu, VkDevice _device, VkSurfaceKHR _surface, VkExtent2D dim)
	{
		allocator = _allocator;
		chosenGPU = gpu;
		device = _device;
		surface = _surface;
		extent = dim;
		destroyQueue.init(device, allocator);
		renderPass = VK_NULL_HANDLE;
	}
	void UpdateDimensions(VkExtent2D dim)
	{
		extent = dim;
	}

	void Build(bool vsync);
	void Recreate(VkExtent2D dim, bool vsync);
	
	void Destroy();
private:
	void CreateSwapchain(uint32_t width, uint32_t height,bool Vsync = true);
	void CreateRenderPass();
	void CreateFramebuffers(VkRenderPass renderPass);

public:
	//getters
	VkRenderPass GetRenderPass()
	{
		return renderPass;
	}
	VkFramebuffer GetFramebuffer(int imageIndex)
	{
		return framebuffers[imageIndex];
	}
	VkExtent2D GetExtent()
	{
		return extent;
	}
	VkSwapchainKHR GetSwapchain()
	{
		return swapchain;
	}
	VkSemaphore GetSemaphore(int imageIndex)
	{
		return imagePresentSemaphores[imageIndex];
	}

private:
	VkPhysicalDevice chosenGPU; 
	VkDevice device; 
	VkSurfaceKHR surface;
	VmaAllocator allocator;

	VkSwapchainKHR swapchain;

	VkFormat imageFormat;
	std::vector<VkImage> images = {};
	std::vector<VkImageView> imageViews = {};

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	
	VkExtent2D extent;

	AllocatedImage drawImage;
	VkExtent2D drawExtent;

	std::vector<VkSemaphore> imagePresentSemaphores;
	DeletionQueue destroyQueue;
};