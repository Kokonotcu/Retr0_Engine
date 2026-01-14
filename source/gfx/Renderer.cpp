#include <gfx/Renderer.h>

void  Renderer::Init(Window& window, const VulkanContext& context) 
{
	this->window = &window;
	this->vkContext = context;

	frameTimer = Time::RequestTracker(1.0f);
	printCheckerTimer = Time::RequestTracker(1.0f);

	// Initialize the main deletion queue with the device and allocator
	rendererDeletionQueue.init(context.device, context.allocator);

	swapchain.SetProperties(context.allocator, context.physicalDevice, context.device, window.GetVulkanSurface(), { window.GetWindowExtent().width, window.GetWindowExtent().height });

	retro::print("Renderer: Building swapchain\n");
	swapchain.Build(vsyncEnabled);

	retro::print("Renderer: Initializing Pipelines\n");
	InitPipelines();

	retro::print("Renderer: Initializing Commands\n");
	InitCommands();

	retro::print("Renderer: Initializing Sync Structures\n");
	InitSyncStructures();

	isInitialized = true;
	retro::print("Renderer: Initialized\n");
}

void Renderer::Shutdown()
{
	if (isInitialized)
	{
		vkDeviceWaitIdle(vkContext.device);

		rendererDeletionQueue.flush();
		swapchain.Destroy();

		isInitialized = false;
	}
}

void Renderer::InitCommands()
{
	// Create a command pool for commands submitted to the graphics queue.
	// We also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(vkContext.graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(vkContext.device, &commandPoolInfo, nullptr, &frames[i].commandPool));

		// Allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(vkContext.device, &cmdAllocInfo, &frames[i].mainCommandBuffer));

		rendererDeletionQueue.addCommandPool(frames[i].commandPool);
	}
}

void Renderer::InitSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(vkContext.device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

		// This semaphore is used to signal when the Swapchain Image is available
		VK_CHECK(vkCreateSemaphore(vkContext.device, &semaphoreCreateInfo, nullptr, &frames[i].renderFinished));

		rendererDeletionQueue.addFence(frames[i].renderFence);
		rendererDeletionQueue.addSemaphore(frames[i].renderFinished);
	}
}

void Renderer::InitPipelines()
{
	graphicsPipeline.SetDevice(vkContext.device);

	// 1. Shaders
	//graphicsPipeline.CreateVertexShaderModule<retro::GPUPushConstant>("default_vertex_BDA.vert.spv");
	graphicsPipeline.CreateVertexShaderModule<retro::CPUPushConstant>("light_shaded.vert.spv");
	graphicsPipeline.CreateFragmentShaderModule("default_fragment.frag.spv");

	// 2. Dynamic State
	graphicsPipeline.CreateDynamicState();

	// 3. Vertex Input
	//graphicsPipeline.CreateBDAVertexInput();
	graphicsPipeline.CreateClassicVertexInput();

	// 4. Input Assembly
	graphicsPipeline.CreateInputAssembly();

	// 5. Rasterizer
	graphicsPipeline.CreateRasterizer();

	// 6. Multisampling
	graphicsPipeline.CreateMultisampling();

	// 7. Blending (No blending, Depth Less)
	//graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE, VK_COMPARE_OP_LESS); Transparency Additive
	//graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_COMPARE_OP_LESS); Transparency Alpha Blending
	graphicsPipeline.CreateBlending(69, VK_COMPARE_OP_LESS);

	// 8. Pipeline Layout
	graphicsPipeline.CreatePipelineLayout();

	// 9. Build Pipeline
	graphicsPipeline.CreateGraphicsPipeline(swapchain.GetRenderPass());

	rendererDeletionQueue.addPipeline(graphicsPipeline.GetPipeline());
	rendererDeletionQueue.addPipelineLayout(graphicsPipeline.GetPipelineLayout());
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(vkContext.device);
	swapchain.Recreate({ width, height }, vsyncEnabled);
}

void Renderer::Draw()
{
	// Instead of a fixed round-robin, this searches for the first available fence
	selectedFrameBuf = 0;
	gpuAvailable = false;

	// Try to find a frame that is ready to be used (fence is signaled)
	//for (int i = 0; i < 240; i++)
	while (true)
	{
		if (vkGetFenceStatus(vkContext.device, frames[selectedFrameBuf].renderFence) == VK_SUCCESS)
		{
			gpuAvailable = true;
			break;
		}

		selectedFrameBuf = (selectedFrameBuf + 1) % FRAME_OVERLAP;
	}

	if (!gpuAvailable) return; // GPU is completely full, skip this frame

	// Check swapchain status
	if (!swapchain.IsGood())
	{
		swapchain.Recreate({ window->GetWindowExtent().width, window->GetWindowExtent().height }, vsyncEnabled);
		retro::print("Renderer: Swapchain not good, recreating\n");
		return;
	}

	// Acquire next image
	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(vkContext.device, swapchain.GetSwapchain(), UINT64_MAX, frames[selectedFrameBuf].renderFinished, VK_NULL_HANDLE, &swapchainImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		swapchain.Recreate({ window->GetWindowExtent().width, window->GetWindowExtent().height }, vsyncEnabled);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		retro::print("vkAcquireNextImageKHR failed:\n");
		throw std::runtime_error(": failed to acquire swap chain image!");
	}

	// Prepare command buffer
	VkCommandBuffer cmd = frames[selectedFrameBuf].mainCommandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// Pass renderables to the recording function
	RecordCommandBuffer(cmd, swapchainImageIndex);

	// Submit Frame
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { frames[selectedFrameBuf].renderFinished };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkSemaphore signalSemaphores[] = { swapchain.GetSemaphore(swapchainImageIndex) };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkResetFences(vkContext.device, 1, &frames[selectedFrameBuf].renderFence));
	VK_CHECK(vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, frames[selectedFrameBuf].renderFence));

	// Present Frame
	VkPresentInfoKHR presentInfo = {};
	presentInfo.pNext = nullptr;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	VkSwapchainKHR swapchains[] = { swapchain.GetSwapchain() };
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &swapchainImageIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult resultP = vkQueuePresentKHR(vkContext.graphicsQueue, &presentInfo);

	if (resultP == VK_ERROR_OUT_OF_DATE_KHR || window->IsResizePending() || result == VK_ERROR_SURFACE_LOST_KHR)
	{
		window->ResizeHandled();
		swapchain.Recreate({ window->GetWindowExtent().width, window->GetWindowExtent().height }, vsyncEnabled);
		return;
	}
	else if (resultP != VK_SUCCESS && resultP != VK_SUBOPTIMAL_KHR)
	{
		retro::print("vkQueuePresentKHR failed:\n");
		throw std::runtime_error(": failed to present the graphics queue!");
	}

	if (printCheckerTimer->Check())
	{
		retro::print("Frame Time: ");
		retro::print((Time::GetDeltatime() * 1000.0));
		retro::print("ms \n");
	}
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// Clear color (Deep Blue)
	VkClearValue clearValues[2];
	clearValues[0].color = { {0.0f, 0.0f, 0.32f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapchain.GetRenderPass();
	renderPassInfo.framebuffer = swapchain.GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchain.GetExtent();
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipeline());

	// Dynamic Viewport/Scissor
	graphicsPipeline.UpdateDynamicState(commandBuffer, swapchain.GetExtent());

	// --- Camera Setup (Basic fixed camera for now) ---
	glm::vec3 camPos = { 0.f, 0.f, -5.f };
	glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);

	float aspect = (float)swapchain.GetExtent().width / (float)swapchain.GetExtent().height;
	glm::mat4 projection = glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
	projection[1][1] *= -1; // Flip Y for Vulkan

	// --- Draw Objects ---
	// Just a simple rotation effect based on time for demonstration
	float rotationTimer = frameTimer->GetTimePassed();

	// Assume we are using the global buffers managed by MeshManager for now
	// This matches the logic from the previous vk_engine.cpp
	if (!renderables.empty())
	{
		int i = 0;
		for (const auto& mesh : renderables)
		{
			if (!mesh) continue;

			glm::mat4 model = glm::mat4(1.f);
			model = glm::rotate(model, glm::radians(rotationTimer * 40.f * (i + 1)), glm::vec3(-0.5f, 0, 1.0f));

			// Calculate final MVP matrix
			glm::mat4 meshMatrix = projection * view * model;

			retro::CPUPushConstant cpuPushConstant{};

			cpuPushConstant.worldMatrix = meshMatrix;
			cpuPushConstant.model = model;

			mesh->Draw(commandBuffer, graphicsPipeline.GetPipelineLayout(), MeshManager::GetGlobalIndexBuffer(), MeshManager::GetGlobalVertexBuffer(), cpuPushConstant);
			i++;
		}
	}

	vkCmdEndRenderPass(commandBuffer);
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}