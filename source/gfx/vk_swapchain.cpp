#include "gfx/vk_swapchain.h"

void Swapchain::CreateSwapchain(uint32_t width, uint32_t height, bool Vsync)
{
    vkb::SwapchainBuilder swapchainBuilder{ chosenGPU,device,surface };
    vkb::Swapchain vkbSwapchain;

    vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_B8G8R8A8_SRGB, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(Vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(width, height)
        .set_desired_min_image_count(3) // Sets the size of buffering (either triple or double is recommended and also change FRAME_OVERLAP in engine.h)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    imageFormat = vkbSwapchain.image_format;
    images = vkbSwapchain.get_images().value();
    imageViews = vkbSwapchain.get_image_views().value();
    extent = vkbSwapchain.extent;
    swapchain = vkbSwapchain.swapchain;
}

void Swapchain::CreateRenderPass()
{
    // Color
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat;
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

    // Depth
    VkAttachmentDescription depth{};
    depth.format = depthFormat;                       // pick_depth_format()
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = dep.srcStageMask;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = { colorAttachment, depth };

    VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpci.attachmentCount = 2;
    rpci.pAttachments = attachments;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;

    VK_CHECK(vkCreateRenderPass(device, &rpci, nullptr, &renderPass));
	//destroyQueue.addRenderPass(renderPass);  DO NOT
}

void Swapchain::CreateFramebuffers(VkRenderPass renderPass)
{
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {
            imageViews[i],

            depthImages.empty() ? VK_NULL_HANDLE : depthImages[i].imageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));
        destroyQueue.addFramebuffer(framebuffers[i]);
    }
}

void Swapchain::Build(bool vsync)
{
	depthFormat = pick_depth_format();
    CreateSwapchain(extent.width, extent.height, vsync);
    //draw image size will match the window
    VkExtent3D drawImageExtent = {
        extent.width,
        extent.height,
        1
    };

    //hardcoding the draw format to 32 bit float
    drawImage.imageFormat = imageFormat;
    drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = {};
    rimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    rimg_info.pNext = nullptr;
    rimg_info.imageType = VK_IMAGE_TYPE_2D;
    rimg_info.format = drawImage.imageFormat;
    rimg_info.extent = drawImageExtent;
    rimg_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    rimg_info.mipLevels = 1;
    rimg_info.arrayLayers = 1;
    rimg_info.samples = VK_SAMPLE_COUNT_1_BIT; //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    rimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;  //optimal tiling, which means the image is stored on the best gpu format
    rimg_info.usage = drawImageUsages; //for the draw image, we want to allocate it from gpu local memory

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo,
        &drawImage.image, &drawImage.allocation, nullptr));

    VkImageViewCreateInfo rview_info = {};
    rview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    rview_info.pNext = nullptr;
    rview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    rview_info.image = drawImage.image;
    rview_info.format = drawImage.imageFormat;
    rview_info.subresourceRange.baseMipLevel = 0;
    rview_info.subresourceRange.levelCount = 1;
    rview_info.subresourceRange.baseArrayLayer = 0;
    rview_info.subresourceRange.layerCount = 1;
    rview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &drawImage.imageView));
	createDepthResources();

    destroyQueue.addImageView(drawImage.imageView);
    destroyQueue.addImage(drawImage.image, drawImage.allocation);

    imagePresentSemaphores.resize(images.size());

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semInfo.pNext = nullptr;
    semInfo.flags = 0;

    for (size_t i = 0; i < imagePresentSemaphores.size(); ++i)
    {
        VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &imagePresentSemaphores[i]));
        destroyQueue.addSemaphore(imagePresentSemaphores[i]);
    }

    if (renderPass == VK_NULL_HANDLE)
        CreateRenderPass();
    CreateFramebuffers(renderPass);
}

void Swapchain::Recreate(VkExtent2D dim, bool vsync)
{
    vkDeviceWaitIdle(device);
    Clear();

    UpdateDimensions(dim);
    Build(vsync);
}

void Swapchain::Clear()
{
	destroyQueue.flush();

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < imageViews.size(); i++)
        vkDestroyImageView(device, imageViews[i], nullptr);
}

VkFormat Swapchain::pick_depth_format()
{
    return find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Swapchain::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties p{};
        vkGetPhysicalDeviceFormatProperties(chosenGPU, format, &p);
        if (tiling == VK_IMAGE_TILING_LINEAR && (p.linearTilingFeatures & features) == features) return format;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (p.optimalTilingFeatures & features) == features) return format;
    }
    return VK_FORMAT_UNDEFINED;
}

void Swapchain::createDepthResources()
{
    depthImages.resize(images.size());
    for (size_t i = 0; i < images.size(); ++i)
    {
        retro::Image& di = depthImages[i];
		di.imageFormat = depthFormat;
        di.imageExtent = { extent.width, extent.height, 1 };

        VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.format = di.imageFormat;
        ici.extent = di.imageExtent;
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo aci{};
        aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VK_CHECK(vmaCreateImage(allocator, &ici, &aci, &di.image, &di.allocation, nullptr));

        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (di.imageFormat == VK_FORMAT_D24_UNORM_S8_UINT /* or any *S8* */) {
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        VkImageViewCreateInfo ivci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.image = di.image;
        ivci.format = di.imageFormat;
        ivci.subresourceRange = { aspect, 0, 1, 0, 1 };
        VK_CHECK(vkCreateImageView(device, &ivci, nullptr, &di.imageView));

        destroyQueue.addImageView(di.imageView);
        destroyQueue.addImage(di.image, di.allocation);
    }
}

void Swapchain::Destroy()
{
    vkDeviceWaitIdle(device);

    Clear();
    vkDestroyRenderPass(device, renderPass, nullptr);
}

