#include "vk_engine.h"
#include <VkBootstrap.h>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
 };


#ifdef __ANDROID__
static constexpr bool enableValidationLayers = false;
#else
static constexpr bool enableValidationLayers =
#ifdef DEBUG
true
#else
false
#endif
;
#endif

void VulkanEngine::Init()
{
	
    // We initialize SDL and create a window with it. 
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
    window = SDL_CreateWindow(
        "Retr0 Engine",
        windowExtent.width,
        windowExtent.height,
        window_flags
    );
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "1");

   retro::print(SDL_GetError());
   SDL_SetWindowResizable(window, true);
   SDL_SetWindowFullscreenMode(window,NULL);
   SDL_SetWindowFullscreen(window, true);

#ifdef __ANDROID__
   VsyncEnabled = true;
#endif // __ANDROID_API__

    retro::print("Vulkan Engine: Initializing Vulkan\n");
    InitVulkan();

    retro::print("Vulkan Engine: Initializing MeshManager\n");
    MeshManager::Init(this, 
       64 * 1024 * 1024,   // 64MB vertex pool, ~2.000.000 vertices
       64 * 1024 * 1024 );   // 64MB index  pool


    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    windowExtent.width = w;
    windowExtent.height = h;

    retro::print("Vulkan Engine: Initializing swapchain\n");
	swapchain.SetProperties(allocator, chosenGPU, device, surface, windowExtent);
    retro::print("Vulkan Engine: building swapchain\n");
	swapchain.Build(VsyncEnabled);

    retro::print("Vulkan Engine: Initializing pipelines\n");
	InitPipelines();

    retro::print("Vulkan Engine: Initializing commands\n");
    InitCommands();

	retro::print("Vulkan Engine: Initializing sync structures\n");
    InitSyncStructures();
    
	retro::print("Vulkan Engine: Initializing default mesh\n");
    InitDefaultMesh();
    
    //everything went fine
    isInitialized = true;

	frameTimer = Time::RequestTracker(1.0f);
	printCheckerTimer = Time::RequestTracker(1.0f);
    retro::print("Vulkan Engine: initialized\n");
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
    auto inst_ret = builder.set_app_name("Retr0 Engine")
        .request_validation_layers(enableValidationLayers)
        .use_default_debug_messenger()
        .desire_api_version(1, 3, 0)
        .build();

#ifdef DEBUG
    if (!inst_ret)
    {
        retro::print("failed to create instance: " + inst_ret.error().message());
        return;
    }
#endif // DEBUG

	 vkb::Instance vkbInst = inst_ret.value();

    //grab the instance 
    instance = vkbInst.instance;
    debugMessenger = vkbInst.debug_messenger;

    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
    {
        retro::print("failed to create surface: " );
		retro::print(SDL_GetError());
        return;
    }

    uint32_t instanceVersion = 0;
    vkEnumerateInstanceVersion(&instanceVersion);

    uint32_t major = VK_VERSION_MAJOR(instanceVersion);
    uint32_t minor = VK_VERSION_MINOR(instanceVersion);
    uint32_t patch = VK_VERSION_PATCH(instanceVersion);

    vkb::PhysicalDeviceSelector selector{ vkbInst };

    vkb::Result<vkb::PhysicalDevice>  phy_ret{ std::error_code()};
    
#ifdef __ANDROID__
    phy_ret = selector
        .set_minimum_version(1, 0)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .set_surface(surface)
        .select();
    bufferDeviceAddress = false; 

#else
    if ((major > 1) || (major == 1 && minor >= 2))
    {
        //vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES }; //Take a look at this later, might need to change for older devices
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = false;

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
    bufferDeviceAddress = false;
#endif // __ANDROID__
    
#ifdef DEBUG
    if (!phy_ret)
    {
        retro::print("failed to select physical device: " + phy_ret.error().message());
        return;
	}
#endif // DEBUG

    vkb::PhysicalDevice physicalDevice;
   
     physicalDevice = phy_ret.value();
    
	

    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    auto dev_ret = deviceBuilder.build();

#ifdef DEBUG
    if (!dev_ret)
    {
        retro::print("failed to create logical device: " + dev_ret.error().message());
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
        retro::print("failed to get graphics queue: " + gq_ret.error().message());
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
        retro::print("failed to get graphics queue index: "+ gqi_ret.error().message());
		return;
    }
    if (!iqi_ret)
	{
        retro::print("failed to attach self to graphics queue index: " + iqi_ret.error().message());
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

    retro::print("Vulkan Instance Version: "+ std::to_string(major));
	retro::print("." + std::to_string(minor));
	retro::print("." + std::to_string(patch));
    retro::print("\nSelected GPU: ");
	retro::print(props.deviceName);
	retro::print("\nType: ");
	retro::print((int)props.deviceType);
	retro::print("  (1=integrated, 2=discrete)\n");

    //fmt::print("Vulkan Instance Version: {}.{}.{}\nSelected GPU: {}\nType: {}  (1=integrated, 2=discrete)\n", major, minor, patch, props.deviceName, (int)props.deviceType);
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
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderFinished));

		mainDeletionQueue.addFence(frames[i].renderFence);
		mainDeletionQueue.addSemaphore(frames[i].renderFinished);
    }
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
	mainDeletionQueue.addFence(immFence);

}



void VulkanEngine::InitPipelines()
{
	InitGlobalPipelines();
}

void VulkanEngine::InitGlobalPipelines()
{
	graphicsPipeline.SetDevice(device);
    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//
    //graphicsPipeline.CreateVertexShaderModule<retro::GPUPushConstant>("default_vertex_BDA.vert.spv");
    graphicsPipeline.CreateVertexShaderModule<retro::CPUPushConstant>("default_vertex.vert.spv");
    graphicsPipeline.CreateFragmentShaderModule("default_fragment.frag.spv");
    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//
	graphicsPipeline.CreateDynamicState(); retro::print("Vulkan Engine: Dynamic State Enabled: Viewport, Scissor\n");
    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//
	//graphicsPipeline.CreateBDAVertexInput();
	graphicsPipeline.CreateClassicVertexInput(); retro::print("Vulkan Engine: Classic Vertex Input Enabled: Pos, Normal, UV\n");
    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateInputAssembly(); retro::print("Vulkan Engine: Input Assembly: Triangle List, No Restart\n");
    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateRasterizer(); retro::print("Vulkan Engine: Rasterizer: Fill, Backface Cull, Clockwise Frontface\n");
    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//

     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//
	graphicsPipeline.CreateMultisampling(); retro::print("Vulkan Engine: Multisampling: No multisampling\n");
     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//
	//graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE, VK_COMPARE_OP_LESS);
    //graphicsPipeline.CreateBlending(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_COMPARE_OP_LESS);
	graphicsPipeline.CreateBlending(69, VK_COMPARE_OP_LESS); retro::print("Vulkan Engine: Color Blending: No blending, Depth Testing: Less\n");
    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//

	// -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//
	graphicsPipeline.CreatePipelineLayout(); retro::print("Vulkan Engine: Pipeline Layout: 1 Push Constant, Vertex Shader Stages\n");
    // -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//
	graphicsPipeline.CreateGraphicsPipeline(swapchain.GetRenderPass()); retro::print("Vulkan Engine: Graphics Pipeline: Created\n");
    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//
	mainDeletionQueue.addPipeline(graphicsPipeline.GetPipeline());
    mainDeletionQueue.addPipelineLayout(graphicsPipeline.GetPipelineLayout()); 

#ifdef __ANDROID__
    //auto ok = FileManager::ShaderCompiler::LoadShaderModule(FileManager::path::GetShaderPath("default_vertex.vert.spv").string(), device, &module);
    //SDL_Log("vert load: %s", ok ? "ok" : "fail");
#endif // __ANDROID__

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
	graphicsPipeline.UpdateDynamicState(commandBuffer, swapchain.GetExtent());

    // View: camera at (0,0,5) looking at origin
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f,5.f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    // Projection: correct near/far order
    float aspect = float(swapchain.GetExtent().width) / float(swapchain.GetExtent().height);
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 10000.0f);
    // Vulkan Y-flip (keep this if you’re not using a negative viewport height)
    proj[1][1] *= -1.0f;

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians((float)frameTimer->GetTimePassed() * 80.0f), glm::vec3(0.1f, 1.0f, 0.0f));
    //testMesh->Draw(commandBuffer, graphicsPipeline.GetPipelineLayout(), MeshManager::GetGlobalIndexBuffer(),MeshManager::GetGlobalVertexBuffer(), proj * view * rotation);

    rotation = glm::rotate(glm::mat4(1.0f), glm::radians((float)frameTimer->GetTimePassed() * 80.0f), glm::vec3(1.0f, 0.2f, 0.0f));
    testMesh2->Draw(commandBuffer, graphicsPipeline.GetPipelineLayout(), MeshManager::GetGlobalIndexBuffer(), MeshManager::GetGlobalVertexBuffer(), proj * view * rotation);

    vkCmdEndRenderPass(commandBuffer);
    
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanEngine::InitDefaultMesh()
{
	testMesh = MeshManager::LoadMeshCPU(FileManager::path::GetAssetPath("basicmesh.glb"),1);
    testMesh2 = MeshManager::LoadMeshCPU(FileManager::path::GetAssetPath("basicmesh.glb"), 2);
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
    selected = 0; bool flag =false;
    for (int i = 0; i < 300; i++)
    {
        if (vkGetFenceStatus(device, frames[selected].renderFence) == VK_SUCCESS) 
        {
            flag = true;
            break;
        }

        selected = (selected + 1) % FRAME_OVERLAP;
    }
    if (!flag)
    {
        return;
    }

    if (!swapchain.IsGood() )
    {
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        windowExtent.width = w;
        windowExtent.height = h;
        swapchain.Recreate(windowExtent, VsyncEnabled);
		retro::print("Swapchain not good, recreating\n");
        return;
    }

    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain.GetSwapchain(), UINT64_MAX, frames[selected].renderFinished, VK_NULL_HANDLE, &swapchainImageIndex);


    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        windowExtent.width = w;
        windowExtent.height = h;
        swapchain.Recreate(windowExtent,VsyncEnabled);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        retro::print("vkAcquireNextImageKHR failed:\n");
        throw std::runtime_error(": failed to acquire swap chain image!");
    }

    VkCommandBuffer cmd = frames[selected].mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    
	RecordCommandBuffer(cmd, swapchainImageIndex);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { frames[selected].renderFinished };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { swapchain.GetSemaphore(swapchainImageIndex)};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;


    VK_CHECK(vkResetFences(device, 1, &frames[selected].renderFence));
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frames[selected].renderFence));

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

    if (resultP == VK_ERROR_OUT_OF_DATE_KHR  || framebufferResized || result == VK_ERROR_SURFACE_LOST_KHR)
    {
		framebufferResized = false;
        int w, h;
        //SDL_GetRenderOutputSize(SDL_GetRenderer(window),&w,&h);
        //SDL_GetWindowSize(window, &w, &h);
        SDL_GetWindowSizeInPixels(window, &w, &h);
        windowExtent.width = w;
        windowExtent.height = h;
        swapchain.Recreate(windowExtent,VsyncEnabled);
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
            case SDL_EVENT_WILL_ENTER_BACKGROUND:
			case SDL_EVENT_WINDOW_HIDDEN:
			case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            case SDL_EVENT_WINDOW_MINIMIZED:
                stopRendering = true;
                //vkDeviceWaitIdle(device);
                //swapchain.Destroy();
                break;
            case SDL_EVENT_DISPLAY_ORIENTATION:
            case SDL_EVENT_WINDOW_RESIZED:
            case  SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				framebufferResized = true;
                break;
            case  SDL_EVENT_WILL_ENTER_FOREGROUND:
            case SDL_EVENT_WINDOW_RESTORED:
			case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
                stopRendering = false;
                break;
            case SDL_EVENT_QUIT:
                bQuit = true;
                break;
            }
        }

        // do not draw if we are minimized
        if (stopRendering) 
        {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
		Time::CalculateDeltaTime();
        if (!stopRendering) 
        {
            Draw();
        }
    }
}

