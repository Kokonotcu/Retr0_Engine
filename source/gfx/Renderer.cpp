#include <gfx/Renderer.h>

void  Renderer::Init(Window& window, const VulkanContext& context) 
{
	this->window = &window;
	this->vkContext = context;

	frameTimer = Time::RequestTracker(1.0f);
	printCheckerTimer = Time::RequestTracker(1.0f);

	rendererDeletionQueue.init(context.device, context.allocator);

	swapchain.SetProperties(context.allocator, context.physicalDevice, context.device, window.GetVulkanSurface(), { window.GetWindowExtent().width, window.GetWindowExtent().height });

	retro::print("Renderer: Building swapchain\n");
	swapchain.Build(vsyncEnabled);
	
	retro::print("Renderer: Initializing Descriptors\n");
	InitDescriptors();

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

std::shared_ptr<retro::Material> Renderer::CreateMaterial(std::shared_ptr<retro::Texture> texture)
{
	auto material = std::make_shared<retro::Material>();
	material->albedoTexture = texture;

	// Allocate a new descriptor set for this material
	// (Note: ensure 'globalDescriptorAllocator' matches the exact name of your allocator variable)
	material->materialSet = globalDescriptorAllocator.allocate(vkContext.device, materialLayout);

	// Write the texture data into the descriptor set
	retro::DescriptorWriter writer;
	writer.write_image(0, texture->image.imageView, texture->sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.update_set(vkContext.device, material->materialSet);

	return material;
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
	graphicsPipeline.CreateVertexShaderModule<retro::CPUPushConstant>("default_vertex.vert.spv");
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
	//graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE, VK_COMPARE_OP_LESS); //Transparency Additive
	//graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_COMPARE_OP_LESS); //Transparency Alpha Blending
	graphicsPipeline.CreateBlending(69, VK_COMPARE_OP_LESS);

	// 8. Pipeline Layout
	std::vector<VkDescriptorSetLayout> layouts = { bindlessLayout, materialLayout};
	graphicsPipeline.CreatePipelineLayout(layouts);

	// 9. Build Pipeline
	graphicsPipeline.CreateGraphicsPipeline(swapchain.GetRenderPass());

	rendererDeletionQueue.addPipeline(graphicsPipeline.GetPipeline());
	rendererDeletionQueue.addPipelineLayout(graphicsPipeline.GetPipelineLayout());
}

void Renderer::InitDescriptors()
{
	// 1. Create the Global Layout (Camera Data) -> SET 0
	retro::DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	bindlessLayout = layoutBuilder.build(vkContext.device, VK_SHADER_STAGE_VERTEX_BIT);

	// 2. Create the Material Layout (Textures) -> SET 1
	layoutBuilder.clear(); // <--- CRITICAL: Clear before building the next layout!
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	materialLayout = layoutBuilder.build(vkContext.device, VK_SHADER_STAGE_FRAGMENT_BIT);

	// 3. Define pool sizes for per-frame dynamic descriptors (UBOs)
	std::vector<retro::DescriptorAllocator::PoolSizeRatio> framePoolRatios =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1.0f }
	};

	// Initialize the allocators for each frame
	for (int i = 0; i < 3; i++)
	{
		frames[i].frameDescriptorAllocator.init_pool(vkContext.device, 1000, framePoolRatios);
		rendererDeletionQueue.addPool(frames[i].frameDescriptorAllocator.get_pool());
	}

	// 4. Initialize the GLOBAL allocator for persistent descriptors (Textures)
	std::vector<retro::DescriptorAllocator::PoolSizeRatio> globalPoolRatios =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f }
	};
	// maxSets: 100 is plenty for 100 different materials
	globalDescriptorAllocator.init_pool(vkContext.device, 100, globalPoolRatios);
	rendererDeletionQueue.addPool(globalDescriptorAllocator.get_pool());

	// Add layouts to deletion queue
	rendererDeletionQueue.addSetLayout(bindlessLayout);
	rendererDeletionQueue.addSetLayout(materialLayout); // Don't forget to queue this one too!

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		// Create a buffer large enough for CameraData, usable as a Uniform Buffer
		frames[i].cameraBuffer = retro::CreateBuffer(
			vkContext.allocator,
			sizeof(CameraData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Add to deletion queue
		rendererDeletionQueue.addBuffer(frames[i].cameraBuffer.buffer, frames[i].cameraBuffer.allocation);
	}
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

	// 1. Prepare the data from Camera class
	float aspect = (float)swapchain.GetExtent().width / (float)swapchain.GetExtent().height;
	mainCamera.SetStats(70.f, 0.1f, 200.f, aspect);
	glm::mat4 view = mainCamera.GetViewMatrix();
	glm::mat4 projection = mainCamera.GetProjectionMatrix();
	CameraData camData{};
	camData.view = view;
	camData.proj = projection;
	camData.viewproj = projection * view;

	// 2. Copy (Map) data to the current frame's GPU buffer
	void* data;
	vmaMapMemory(vkContext.allocator, frames[selectedFrameBuf].cameraBuffer.allocation, &data);
	memcpy(data, &camData, sizeof(CameraData));
	vmaUnmapMemory(vkContext.allocator, frames[selectedFrameBuf].cameraBuffer.allocation);

	// CRITICAL: Reset the pool for this frame so we can reuse memory
	frames[selectedFrameBuf].frameDescriptorAllocator.clear_descriptors(vkContext.device);
	VkDescriptorSet cameraSet = frames[selectedFrameBuf].frameDescriptorAllocator.allocate(vkContext.device, bindlessLayout);

	retro::DescriptorWriter writer;
	writer.write_buffer(0, frames[selectedFrameBuf].cameraBuffer.buffer, sizeof(CameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(vkContext.device, cameraSet);
	
	// Inside your Command Buffer recording:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 1, &cameraSet, 0, nullptr);

	// Clear color (Deep Blue)
	VkClearValue clearValues[2];
	clearValues[0].color = { {0.001f, 0.001f, 0.001f, 1.0f} };
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

	// --- Draw Objects ---
	// Just a simple rotation effect based on time for demonstration
	float rotationTimer = frameTimer->GetTimePassed();
	rotationTimer = (rotationTimer * 1020.f) * 0.0002777778 + 240.f;
	if (!renderables.empty())
	{
		int i = 0;
		for (auto& renderable : renderables)
		{
			if (!renderable.IsValid()) continue;

			// NEW: Bind Set 1 (The texture/material)
			// We assume the materialSet has already been allocated and written to before getting here
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 1, 1, &renderable.material->materialSet, 0, nullptr);

			glm::mat4 transform = glm::rotate(glm::mat4(1.f), glm::radians(rotationTimer * (i + 1)), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 normalMat = glm::transpose(glm::inverse(transform));


			retro::CPUPushConstant cpuPushConstant{};
			cpuPushConstant.transform = transform;
			cpuPushConstant.normalMatrix = normalMat;


			renderable.Draw(commandBuffer, graphicsPipeline.GetPipelineLayout(), MeshManager::GetGlobalIndexBuffer(), MeshManager::GetGlobalVertexBuffer(), cpuPushConstant);
			i++;
		}
	}

	vkCmdEndRenderPass(commandBuffer);
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}