#include "resources/vk_swapchain.h"

void Swapchain::CreateSwapchain(uint32_t width, uint32_t height, bool Vsync)
{
    vkb::SwapchainBuilder swapchainBuilder{ chosenGPU,device,surface };
    vkb::Swapchain vkbSwapchain;

    vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_R16G16B16A16_SFLOAT, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(Vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(width, height)
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	destroyQueue.addRenderPass(renderPass); // be sure to check validate priority
}

void Swapchain::CreateFramebuffers(VkRenderPass renderPass)
{
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {
            imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
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
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = {};
    rimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    rimg_info.pNext = nullptr;
    rimg_info.imageType = VK_IMAGE_TYPE_2D;
    rimg_info.format = drawImage.imageFormat;
    rimg_info.extent = drawImageExtent;
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

    destroyQueue.flush();
    Destroy();

    UpdateDimensions(dim);
    Build(vsync);
    
    CreateFramebuffers(renderPass);
}

void Swapchain::Destroy()
{
	destroyQueue.flush();

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < imageViews.size(); i++)
        vkDestroyImageView(device, imageViews[i], nullptr);
}

