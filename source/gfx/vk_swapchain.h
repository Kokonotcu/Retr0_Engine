#pragma once
#include <resources/vk_images.h>
#include <resources/vk_debug.h>
#include <VkBootstrap.h>
#include <resources/DeletionQueue.h>

class Swapchain
{
public:
	Swapchain() = default;

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
	
	void Destroy(); //wipes everything
private:
	void CreateSwapchain(uint32_t width, uint32_t height,bool Vsync = true);
	void CreateRenderPass();
	void CreateFramebuffers(VkRenderPass renderPass);

	void Clear(); //does not destroy renderpass
public:
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
	bool IsGood()
	{
		return swapchainStatus;
	}

private:
	VkFormat pick_depth_format();
	VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void createDepthResources();

private:
	VkPhysicalDevice chosenGPU; 
	VkDevice device; 
	VkSurfaceKHR surface;
	VmaAllocator allocator;

	VkSwapchainKHR swapchain;

	VkFormat imageFormat;
	std::vector<VkImage> images = {};
	std::vector<VkImageView> imageViews = {};
	VkExtent2D extent;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	
	retro::Image drawImage;
	VkFormat depthFormat;
	std::vector<retro::Image> depthImages;
	VkExtent2D drawExtent;

	std::vector<VkSemaphore> imagePresentSemaphores;
	DeletionQueue destroyQueue;

private:
	bool swapchainStatus = false;
};