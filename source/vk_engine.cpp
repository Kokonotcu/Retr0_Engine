//> includes
#include "vk_engine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <VkBootstrap.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

VulkanEngine* loadedEngine = nullptr;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
 };

#ifdef DEBUG
    const bool enableValidationLayers = true;
 #else
    const bool enableValidationLayers = false;
 #endif

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
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

    InitVulkan();

    InitSwapchain(false);

    InitCommands();

    InitSyncStructures();

	InitDescriptors();

    InitRenderPasses();

	InitPipelines();

    //everything went fine
    isInitialized = true;
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
    vkb::Instance vkbInst = builder.set_app_name("Retr0 Engine")
        .request_validation_layers(enableValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build()
        .value();

    //grab the instance 
    instance = vkbInst.instance;
    debugMessenger = vkbInst.debug_messenger;

    SDL_Vulkan_CreateSurface(window, instance, NULL,&surface);

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;
    
    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    //use vkbootstrap to select a gpu. 
    //We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{ vkbInst };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(surface)
        .select()
        .value();


    //create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    device = vkbDevice.device;
    chosenGPU = physicalDevice.physical_device;

    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGPU;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    // Initialize the deletion queues
    for (int i = 0; i < FRAME_OVERLAP; ++i)
        frames[i].deletionQueue.init(device, allocator);
    swapchainDestroy.init(device, allocator);
    mainDeletionQueue.init(device, allocator);
}

void VulkanEngine::InitSwapchain(bool VsyncEnabled)
{
    CreateSwapchain(windowExtent.width, windowExtent.height, VsyncEnabled);

    //draw image size will match the window
    VkExtent3D drawImageExtent = {
        swapchainExtent.width,
        swapchainExtent.height,
        1
    };

    //hardcoding the draw format to 32 bit float
    drawImage.imageFormat = swapchainImageFormat;
    drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(drawImage.imageFormat, drawImageUsages, drawImageExtent);

    //for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //allocate and create the image
    vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo, &drawImage.image, &drawImage.allocation, nullptr);

    //build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &drawImage.imageView));

    swapchainDestroy.addImageView(drawImage.imageView);
    swapchainDestroy.addImage(drawImage.image, drawImage.allocation);

    imagePresentSemaphores.resize(swapchainImages.size());
    VkSemaphoreCreateInfo semInfo = vkinit::semaphore_create_info();
    for (size_t i = 0; i < imagePresentSemaphores.size(); ++i) 
    {
        VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &imagePresentSemaphores[i]));
        swapchainDestroy.addSemaphore(imagePresentSemaphores[i]);
    }
}

void VulkanEngine::CreateSwapchain(uint32_t width, uint32_t height, bool Vsync = true)
{
    vkb::SwapchainBuilder swapchainBuilder{ chosenGPU,device,surface };

    swapchainImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    vkb::Swapchain vkbSwapchain;

    if (Vsync)
        vkbSwapchain = swapchainBuilder
            //.use_default_format_selection()
            .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            //use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();
    
    else
        vkbSwapchain = swapchainBuilder
            //.use_default_format_selection()
            .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            //use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();
    
    

    swapchainExtent = vkbSwapchain.extent;
    //store swapchain and its related images
    swapchain = vkbSwapchain.swapchain;
    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::DestroySwapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < swapchainImageViews.size(); i++) 
    {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
}

void VulkanEngine::InitCommands()
{
    //create a command pool for commands submitted to the graphics queue.
     //we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) 
    {
        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

        // allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1);
        
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));
    
		mainDeletionQueue.addCommandPool(frames[i].commandPool);
    }
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

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
        //VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));

		mainDeletionQueue.addFence(frames[i].renderFence);
		mainDeletionQueue.addSemaphore(frames[i].swapchainSemaphore);
		//mainDeletionQueue.addSemaphore(frames[i].renderSemaphore);
    }
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
    //    drawImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_COMPUTE_BIT);
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

void VulkanEngine::InitRenderPasses()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	mainDeletionQueue.addRenderPass(renderPass);
}

void VulkanEngine::InitPipelines()
{
	InitGlobalPipelines();
}

void VulkanEngine::InitGlobalPipelines()
{
    //VK_CHECK(vkCreatePipelineLayout());
  

    // Pipeline layout :
	// Vertex and Fragment Shader Stage --> Dynamic State configuration --> Vertex Input --> Input Assembly --> Viewports and Scissors --> Rasterizer --> Multisampling --> Depth/Stencil --> Color Blending

    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//
    VkShaderModule vertexShader;
    if (!vkutil::load_shader_module("shaders/hardcoded_triangle.vert.spv", device, &vertexShader)) {
        fmt::print("Error when building the vertex shader \n");
    }

    VkShaderModule fragmentShader;
    if (!vkutil::load_shader_module("shaders/hardcoded_triangle_red.frag.spv", device, &fragmentShader)) {
        fmt::print("Error when building the fragment shader \n");
    }

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.pNext = nullptr;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertexShader;
    vertexStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStageInfo{};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.pNext = nullptr;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageInfo.module = fragmentShader;
    fragmentStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };
    // -------------------------------------------------------------------------Vertex and Fragment Shader Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//
    std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    // -------------------------------------------------------------------------Dynamic State configuration-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional helps define spacing between data and whether the data is per-vertex or per-instance
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional helps define how to extract a vertex attribute from a chunk of vertex data originating from a binding description
    // -------------------------------------------------------------------------Vertex Input Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//
    //Normally, the vertices are loaded from the vertex buffer by index in sequential order,
    // but with an element buffer you can specify the indices to use yourself. 
    // This allows you to perform optimizations like reusing vertices. 
    // If you set the primitiveRestartEnable member to VK_TRUE, 
    // then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    // -------------------------------------------------------------------------Input Assembly Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Viewports and scissors-------------------------------------------------------------------------//
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    // -------------------------------------------------------------------------Viewports and scissors-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE for wireframe // VK_POLYGON_MODE_POINT for points
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    // -------------------------------------------------------------------------Rasterizer Stage-------------------------------------------------------------------------//

     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
     // -------------------------------------------------------------------------Multisampling Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Depth/Stencil Stage-------------------------------------------------------------------------//
	// We are not using depth for now
    // -------------------------------------------------------------------------Depth/Stencil Stage-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//
    
	//Can be used for transparency
    //colorBlendAttachment.blendEnable = VK_TRUE;
    //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional
    // -------------------------------------------------------------------------Color Blending Stage-------------------------------------------------------------------------//

	// -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout));
    // -------------------------------------------------------------------------Pipeline Layout-------------------------------------------------------------------------//

    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = graphicsPipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    // -------------------------------------------------------------------------Pipeline Creation-------------------------------------------------------------------------//

    //VK_CHECK(vkCreateGraphicsPipelines());
    
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);

	mainDeletionQueue.addPipeline(graphicsPipeline);
    mainDeletionQueue.addPipelineLayout(graphicsPipelineLayout);
	//mainDeletionQueue.addPipeline(gradient.pipeline);
	//mainDeletionQueue.addPipeline(sky.pipeline);
}

void VulkanEngine::Cleanup()
{
    if (isInitialized) 
    {
        //make sure the gpu has stopped doing its things
        vkDeviceWaitIdle(device);

        for (int i = 0; i < FRAME_OVERLAP; i++) 
			frames[i].deletionQueue.flush();
        
		swapchainDestroy.flush();
		mainDeletionQueue.flush();

		vmaDestroyAllocator(allocator);

        DestroySwapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::Draw()
{
    // wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().renderFence, true, 1000000000));

    get_current_frame().deletionQueue.flush();

    VK_CHECK(vkResetFences(device, 1, &get_current_frame().renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, get_current_frame().swapchainSemaphore, nullptr, &swapchainImageIndex));

    //naming it cmd for shorter writing
    VkCommandBuffer cmd = get_current_frame().mainCommandBuffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    drawExtent.width = drawImage.imageExtent.width;
    drawExtent.height = drawImage.imageExtent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // transition our main draw image into general layout so we can write into it
    // we will overwrite it all so we dont care about what was the older layout
	vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); // CHANGE THIS TO VERTEX

    DrawBackground(cmd);

    //transition the draw image and the swapchain image into their correct transfer layouts
    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute a copy from the draw image into the swapchain
    vkutil::copy_image_to_image(cmd, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

    // set swapchain image layout to Present so we can show it on the screen
    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSemaphore presentSignal = imagePresentSemaphores[swapchainImageIndex];

    //prepare the submission to the queue. 
    //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    //we will signal the _renderSemaphore, to signal that rendering has finished
    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame().swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, presentSignal);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    //submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, get_current_frame().renderFence));

    //prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that, 
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &presentSignal;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    //increase the number of frames drawn
    frameNumber++;
}

void VulkanEngine::DrawBackground(VkCommandBuffer cmd)
{
    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    // bind the background compute pipeline
    //vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, PIPELINE);

    // bind the descriptor set containing the draw image for the compute pipeline
    //vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 0, 1, &drawImageDescriptors, 0, nullptr);

    //vkCmdPushConstants(cmd, gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    //vkCmdDispatch(cmd, std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);
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

        Draw();
    }
}

