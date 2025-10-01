//> includes
#include "vk_engine.h"
#include <VkBootstrap.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
 };

#ifdef DEBUG
    const bool enableValidationLayers = true;
 #else
    const bool enableValidationLayers = false;
 #endif

void VulkanEngine::Init()
{
    // We initialize SDL and create a window with it. 
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(
        "Retr0 Engine",
        windowExtent.width,
        windowExtent.height,
        window_flags
    );
   fmt::print("{}",SDL_GetError());
   SDL_SetWindowResizable(window, true);

    InitVulkan();

    MeshManager::Init(this, 
       64 * 1024 * 1024,   // 64MB vertex pool, 
       64 * 1024 * 1024 );   // 64MB index  pool

	swapchain.SetProperties(allocator, chosenGPU, device, surface, windowExtent);
	swapchain.Build(VsyncEnabled);

	InitDescriptors();

	InitPipelines();

    InitCommands();

    InitSyncStructures();

    InitDefaultMesh();

    //everything went fine
    isInitialized = true;

	frameTimer = Time::RequestTracker(1.0f);
}

bool VulkanEngine::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) 
        {
            return false;
        }
    }
    return true;
}

void VulkanEngine::InitVulkan()
{
    if (enableValidationLayers && !CheckValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vkb::InstanceBuilder builder;

    //make the vulkan instance, with basic debug features 
    auto inst_ret = builder.set_app_name("Retr0 Engine") //Take a look at this later, might need to change for older devices
        .request_validation_layers(enableValidationLayers)
        .use_default_debug_messenger()
        .desire_api_version(1, 3, 0)
        .build();

#ifdef DEBUG
    if (!inst_ret)
    {
		fmt::print("failed to create instance: {}\n", inst_ret.error().message());
        return;
    }
#endif // DEBUG

	 vkb::Instance vkbInst = inst_ret.value();

    //grab the instance 
    instance = vkbInst.instance;
    debugMessenger = vkbInst.debug_messenger;

    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
    {
		fmt::print("failed to create surface: {}\n", SDL_GetError());
        return;
    }

    uint32_t instanceVersion = 0;
    vkEnumerateInstanceVersion(&instanceVersion);

    uint32_t major = VK_VERSION_MAJOR(instanceVersion);
    uint32_t minor = VK_VERSION_MINOR(instanceVersion);
    uint32_t patch = VK_VERSION_PATCH(instanceVersion);

    vkb::PhysicalDeviceSelector selector{ vkbInst };

    vkb::Result<vkb::PhysicalDevice>  phy_ret{ std::error_code()};
    if ((major > 1) || (major ==1 && minor >= 2))
    {
        //vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES }; //Take a look at this later, might need to change for older devices
        features12.bufferDeviceAddress = true; // make this false if the device doesnt support it
        features12.descriptorIndexing = true; // make this false if the device doesnt support it

		bufferDeviceAddress = features12.bufferDeviceAddress;

        phy_ret = selector
            .set_minimum_version(1, 2)
            .set_required_features_12(features12)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.set_surface(surface)
            .select();
    }
    else
    {
		bufferDeviceAddress = false;

        phy_ret = selector
            .set_minimum_version(1, 0)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .set_surface(surface)
            .select();
    }

    
#ifdef DEBUG
    if (!phy_ret)
    {
        fmt::print("failed to select physical device: {}\n", phy_ret.error().message());
        return;
	}
#endif // DEBUG

	vkb::PhysicalDevice physicalDevice = phy_ret.value();

    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    auto dev_ret = deviceBuilder.build();

#ifdef DEBUG
    if (!dev_ret)
    {
        fmt::print("failed to create logical device: {}\n", dev_ret.error().message());
		return;
	}
#endif // DEBUG

    vkb::Device vkbDevice = dev_ret.value();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkbDevice.device;
    chosenGPU = physicalDevice.physical_device;

	auto gq_ret = vkbDevice.get_queue(vkb::QueueType::graphics);
	auto iq_ret = vkbDevice.get_queue(vkb::QueueType::transfer);

    auto gqi_ret = vkbDevice.get_queue_index(vkb::QueueType::graphics);
	auto iqi_ret = vkbDevice.get_queue_index(vkb::QueueType::transfer);
#ifdef DEBUG
    if (!gq_ret)
    {
        fmt::print("failed to get graphics queue: {}\n", gq_ret.error().message());
		return;
    }
#endif // DEBUG

    if (!iq_ret)
    {
		iq_ret = gq_ret;
		iqi_ret = gqi_ret;
	}

	graphicsQueue = gq_ret.value();
	immediateQueue = iq_ret.value();

#ifdef DEBUG
    if ( !gqi_ret)
    {
		fmt::print("failed to get graphics queue index: {}\n", gqi_ret.error().message());
		return;
    }
    if (!iqi_ret)
	{
        fmt::print("failed to attach self to graphics queue index: {}\n", iqi_ret.error().message());
        return;
	}
#endif // DEBUG

    graphicsQueueIndex = gqi_ret.value();
	immediateQueueIndex = iqi_ret.value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGPU;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    mainDeletionQueue.init(device, allocator);

#ifdef DEBUG
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(chosenGPU, &props);

    fmt::print("Vulkan Instance Version: {}.{}.{}\nSelected GPU: {}\nType: {}  (1=integrated, 2=discrete)\n", major, minor, patch, props.deviceName, (int)props.deviceType);
#endif // DEBUG
}

void VulkanEngine::InitCommands()
{
    //create a command pool for commands submitted to the graphics queue.
     //we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VkCommandPoolCreateInfo immCommandPoolInfo = vkinit::command_pool_create_info(immediateQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) 
    {
        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1);
        
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));
    
		mainDeletionQueue.addCommandPool(frames[i].commandPool);
    }
    VK_CHECK(vkCreateCommandPool(device, &immCommandPoolInfo, nullptr, &immCommandPool));
	VkCommandBufferAllocateInfo immAllocInfo = vkinit::command_buffer_allocate_info(immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &immAllocInfo, &immCommandBuffer));

    mainDeletionQueue.addCommandPool(immCommandPool);

}

void VulkanEngine::InitSyncStructures()
{
    //create syncronization structures
    //one fence to control when the gpu has finished rendering the frame,
    //and 2 semaphores to syncronize rendering with swapchain
    //we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));

		mainDeletionQueue.addFence(frames[i].renderFence);
		mainDeletionQueue.addSemaphore(frames[i].renderSemaphore);
    }
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
	mainDeletionQueue.addFence(immFence);

}

void VulkanEngine::InitDescriptors()
{
    ////create a descriptor pool that will hold 10 sets with 1 image each
    //std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
    //{
    //    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    //};
    //
    //globalDescriptorAllocator.InitPool(device, 10, sizes);
    //
    ////make the descriptor set layout for our compute draw
    //{
    //    DescriptorLayoutBuilder builder;
    //    builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    //    drawImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_ALL_GRAPHICS);
    //}
    //
    ////allocate a descriptor set for our draw image
    //drawImageDescriptors = globalDescriptorAllocator.Allocate(device, drawImageDescriptorLayout);
    //
    //VkDescriptorImageInfo imgInfo{};
    //imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    //imgInfo.imageView = drawImage.imageView;
    //
    //VkWriteDescriptorSet drawImageWrite = {};
    //drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //drawImageWrite.pNext = nullptr;
    //
    //drawImageWrite.dstBinding = 0;
    //drawImageWrite.dstSet = drawImageDescriptors;
    //drawImageWrite.descriptorCount = 1;
    //drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    //drawImageWrite.pImageInfo = &imgInfo;
    //
    //vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, nullptr);
    //
    ////make sure both the descriptor allocator and the new layout get cleaned up properly
    //
    //mainDeletionQueue.addPool(globalDescriptorAllocator.pool);
    //mainDeletionQueue.addSetLayout(drawImageDescriptorLayout);
}

void VulkanEngine::InitPipelines()
{
	InitGlobalPipelines();
}

void VulkanEngine::InitGlobalPipelines()
{
	graphicsPipeline.SetDevice(device);

    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//
    graphicsPipeline.CreateVertexShaderModule<retro::GPUDrawPushConstants>("pushed_triangle.vert.spv");
    graphicsPipeline.CreateFragmentShaderModule("hardcoded_triangle_red.frag.spv");
    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//
	graphicsPipeline.CreateDynamicState();
    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateVertexInput();
    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateInputAssembly();
    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateRasterizer();
    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//

     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateMultisampling();
     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//
	///graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE, VK_COMPARE_OP_LESS);
    //graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_COMPARE_OP_LESS);
    graphicsPipeline.CreateBlending(69, VK_COMPARE_OP_LESS);
    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//

	// -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//
	graphicsPipeline.CreatePipelineLayout();
    // -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//
    graphicsPipeline.CreateGraphicsPipeline(swapchain.GetRenderPass());
    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//
	mainDeletionQueue.addPipeline(graphicsPipeline.GetPipeline());
    mainDeletionQueue.addPipelineLayout(graphicsPipeline.GetPipelineLayout());
}

void VulkanEngine::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

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

    retro::GPUDrawPushConstants pushConstants;

    // View: camera at (0,0,5) looking at origin
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f,3.f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Projection: correct near/far order
    float aspect = float(swapchain.GetExtent().width) / float(swapchain.GetExtent().height);
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 10000.0f);

    // Vulkan Y-flip (keep this if you’re not using a negative viewport height)
    proj[1][1] *= -1.0f;

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians((float)frameTimer->GetTimePassed() * 80.0f), glm::vec3(0.1f, 1.0f, 0.0f));
    // If your push constants hold MVP, name it that:
    pushConstants.worldMatrix = proj * view * rotation;

	graphicsPipeline.UpdateDynamicState(commandBuffer, swapchain.GetExtent());

    pushConstants.vertexBuffer = testMesh.vertexBufferAddress;
    vkCmdPushConstants(commandBuffer, graphicsPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(retro::GPUDrawPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(commandBuffer, MeshManager::GetGlobalIndexBuffer(), testMesh.indexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, testMesh.submeshes[0].count, 1, testMesh.submeshes[0].startIndex, 0, 0);

    rotation = glm::rotate(glm::mat4(1.0f), glm::radians((float)frameTimer->GetTimePassed() * 80.0f), glm::vec3(1.0f, 0.2f, 0.0f));
    pushConstants.worldMatrix = proj * view * rotation;

    pushConstants.vertexBuffer = testMesh2.vertexBufferAddress;
    vkCmdPushConstants(commandBuffer, graphicsPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(retro::GPUDrawPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(commandBuffer, MeshManager::GetGlobalIndexBuffer(), testMesh2.indexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, testMesh2.submeshes[0].count, 1, testMesh2.submeshes[0].startIndex, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanEngine::InitDefaultMesh()
{
	testMesh = MeshManager::LoadMeshGPU(FileManager::path::GetAssetPath("basicmesh.glb"),1);
    testMesh2 = MeshManager::LoadMeshGPU(FileManager::path::GetAssetPath("basicmesh.glb"), 2);
}

void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer _cmd)>&& function)
{
    VK_CHECK(vkResetFences(device, 1, &immFence));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

    VkCommandBuffer cmd = immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vkinit::submit_info(&cmd, nullptr, nullptr); //Take a look at this later, might need to change for older devices

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(immediateQueue, 1, &submit, immFence));   //Take a look at this later, might need to change for older devices

    VK_CHECK(vkWaitForFences(device, 1, &immFence, true, UINT64_MAX));
}

void VulkanEngine::Cleanup()
{
    if (isInitialized) 
    {
        //make sure the gpu has stopped doing its things
        vkDeviceWaitIdle(device);
        
		MeshManager::ClearBuffers();
		mainDeletionQueue.flush();
        swapchain.Destroy();

		vmaDestroyAllocator(allocator);

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);


        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::Draw()
{
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().renderFence, true, UINT64_MAX));

    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain.GetSwapchain(), UINT64_MAX, get_current_frame().renderSemaphore, nullptr, &swapchainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        int w, h;
        //SDL_GetRenderOutputSize(SDL_GetRenderer(window),&w,&h);
        SDL_GetWindowSize(window, &w, &h);
        windowExtent.width = w;
        windowExtent.height = h;
        swapchain.Recreate(windowExtent,VsyncEnabled);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VK_CHECK(vkResetFences(device, 1, &get_current_frame().renderFence));

    VkCommandBuffer cmd = get_current_frame().mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    
	RecordCommandBuffer(cmd, swapchainImageIndex);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { get_current_frame().renderSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { swapchain.GetSemaphore(swapchainImageIndex) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, get_current_frame().renderFence));

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

    VkResult resultP = vkQueuePresentKHR(graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR  || framebufferResized)
    {
		framebufferResized = false;
        int w, h;
        //SDL_GetRenderOutputSize(SDL_GetRenderer(window),&w,&h);
        SDL_GetWindowSize(window, &w, &h);
        windowExtent.width = w;
        windowExtent.height = h;
        swapchain.Recreate(windowExtent,VsyncEnabled);
        return;
    }
    else if (resultP != VK_SUCCESS && resultP != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    //increase the number of frames drawn
    frameNumber = (frameNumber + 1) % FRAME_OVERLAP;
}

void VulkanEngine::Run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
                
            switch (e.type)
            {
            case SDL_EVENT_WINDOW_MINIMIZED:
                stopRendering = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
				framebufferResized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                stopRendering = false;
                break;
            case SDL_EVENT_QUIT:
                bQuit = true;
                break;
            }
        }

        // do not draw if we are minimized
        if (stopRendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

		Time::CalculateDeltaTime();
        Draw();
    }
}

