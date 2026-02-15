#include "Engine.h"
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

void Engine::Init()
{
    retro::print("Engine: Initializing Window\n");
	window.Init({ "Retr0 Engine", 800, 600, true });

#ifdef __ANDROID__
   VsyncEnabled = true;
#endif // __ANDROID_API__

    retro::print("Engine: Initializing Vulkan\n");
    InitVulkan();

    retro::print("Engine: Initializing MeshManager\n");
    MeshManager::Init(this, 
       64 * 1024 * 1024,   // 64MB vertex pool, ~2.000.000 vertices
       64 * 1024 * 1024 );   // 64MB index  pool

	renderer.Init(window, { instance, chosenGPU, device, graphicsQueue, graphicsQueueIndex, allocator });

    retro::print("Engine: Initializing commands\n");
    InitCommands();

	retro::print("Engine: Initializing sync structures\n");
    InitSyncStructures();
    
	retro::print("Engine: Initializing default mesh\n");
    InitDefaultMesh();
	//renderer.AddRenderable(testMesh2);
	renderer.AddRenderable(testMesh);
    
    //everything went fine
    isInitialized = true;
    retro::print("Engine: Initialized successfully\n");
}

bool Engine::CheckValidationLayerSupport()
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

void Engine::InitVulkan()
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
        .require_api_version(1, 3, 0).set_minimum_instance_version(1, 0, 0)
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

	window.CreateVulkanSurface(instance);
	

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
        .set_surface(window.GetVulkanSurface())
        .select();
    bufferDeviceAddress = false; 

#else
    if ((major > 1) || (major == 1 && minor >= 2))
    {
        //vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES }; //Take a look at this later, might need to change for older devices
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;
        features12.descriptorBindingPartiallyBound = VK_TRUE; // Important for Bindless!
        features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

        bufferDeviceAddress = features12.bufferDeviceAddress;

        phy_ret = selector
            .set_minimum_version(1, 2)
            .set_required_features_12(features12)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .set_surface(window.GetVulkanSurface())
            .select();
    }
    else
    {
        bufferDeviceAddress = false;

        phy_ret = selector
            .set_minimum_version(1, 0)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .set_surface(window.GetVulkanSurface())
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
#endif // DEBUG
}

void Engine::InitCommands()
{
	VkCommandPoolCreateInfo immCommandPoolInfo = vkinit::command_pool_create_info(immediateQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(device, &immCommandPoolInfo, nullptr, &immCommandPool));

	VkCommandBufferAllocateInfo immAllocInfo = vkinit::command_buffer_allocate_info(immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device, &immAllocInfo, &immCommandBuffer));

    mainDeletionQueue.addCommandPool(immCommandPool);
}

void Engine::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));

	mainDeletionQueue.addFence(immFence);
}

void Engine::InitDefaultMesh()
{
	testMesh.mesh = MeshManager::LoadMeshCPU(FileManager::Path::GetModelPath("smoothSphere.glb"),0);

	std::shared_ptr<retro::Texture> outTexture;
	outTexture = std::make_shared<retro::Texture>();
    FileManager::TextureLoader::LoadTexture(FileManager::Path::GetTexturePath("earth.jpg").string(), *outTexture, device, allocator, immediateQueue, immCommandPool);

	testMesh.material = renderer.CreateMaterial(outTexture);

	mainDeletionQueue.addSampler(outTexture->sampler);
	mainDeletionQueue.addImageView(outTexture->image.imageView);
	mainDeletionQueue.addImage(outTexture->image.image, outTexture->image.allocation);




    testMesh2.mesh = MeshManager::LoadMeshCPU(FileManager::Path::GetModelPath("basicmesh.glb"), 2);
}

void Engine::ImmediateSubmit(std::function<void(VkCommandBuffer _cmd)>&& function)
{
    VK_CHECK(vkResetFences(device, 1, &immFence));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

    VkCommandBuffer cmd = immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vkinit::submit_info(&cmd, nullptr, nullptr); //Take a look at this later, might need to change for older devices

	VK_CHECK(vkQueueSubmit(immediateQueue, 1, &submit, immFence));   //Take a look at this later, might need to change for older devices

    VK_CHECK(vkWaitForFences(device, 1, &immFence, true, UINT64_MAX));
}

void Engine::Cleanup()
{
    if (isInitialized) 
    {
        //make sure the gpu has stopped doing its things
        vkDeviceWaitIdle(device);
        
		MeshManager::ClearBuffers();
		mainDeletionQueue.flush();

		renderer.Shutdown();

		vmaDestroyAllocator(allocator);

        vkDestroySurfaceKHR(instance, window.GetVulkanSurface(), nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);

        window.Shutdown();
    }
}


void Engine::Run()
{
    bool bQuit = false;

    // main loop
    while (!bQuit) 
    {
		bQuit = !window.ProcessEvents();

		Time::CalculateDeltaTime();

        // do not draw if we are minimized
        if (window.IsMinimized())
        {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        else
        {
			//renderer.Draw({ testMesh, testMesh2 });
			renderer.Draw();
        }
    }
}

