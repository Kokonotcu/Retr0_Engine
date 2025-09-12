#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"


// -----------------------------------------------------------------WARNING-----------------------------------------------------------------//
//How to add a new type to the deletion queue:
// 1: Add a std::vector<T> member to the DeletionQueue struct
// 2: Add an inline addT(T) method to the DeletionQueue struct
// 3: Add a loop to the flush() method to destroy all Ts in the vector
// -----------------------------------------------------------------WARNING-----------------------------------------------------------------//


// Pair PODs for VMA-managed resources
struct ImageAlloc { VkImage image{}; VmaAllocation alloc{}; };
struct BufferAlloc { VkBuffer buffer{}; VmaAllocation alloc{}; };

// A blazing-fast, type-bucketed destroy list
struct DeletionQueue {
    VkDevice     device{ VK_NULL_HANDLE };
    VmaAllocator allocator{ VK_NULL_HANDLE };

    void init(VkDevice d, VmaAllocator a) {
        device = d; allocator = a;
    }

    // Reserve to avoid reallocations in hot paths
    void reserve(size_t n) {
        imageViews.reserve(n); images.reserve(n); samplers.reserve(n);
        framebuffers.reserve(n); pipelines.reserve(n); pipelineLayouts.reserve(n);
        descriptorSetLayouts.reserve(n); descriptorPools.reserve(n);
        shaderModules.reserve(n); renderPasses.reserve(n);
        commandPools.reserve(n); semaphores.reserve(n); fences.reserve(n);
        buffers.reserve(n);
    }

    // Enqueue helpers (inline, zero overhead beyond push_back)
    inline void addImageView(VkImageView v) { if (v) imageViews.push_back(v); }
    inline void addImage(VkImage img, VmaAllocation a) { if (img && a) images.push_back({ img,a }); }
    inline void addSampler(VkSampler s) { if (s) samplers.push_back(s); }
    inline void addFramebuffer(VkFramebuffer f) { if (f) framebuffers.push_back(f); }
    inline void addPipeline(VkPipeline p) { if (p) pipelines.push_back(p); }
    inline void addPipelineLayout(VkPipelineLayout l) { if (l) pipelineLayouts.push_back(l); }
    inline void addSetLayout(VkDescriptorSetLayout l) { if (l) descriptorSetLayouts.push_back(l); }
    inline void addPool(VkDescriptorPool p) { if (p) descriptorPools.push_back(p); }
    inline void addShader(VkShaderModule m) { if (m) shaderModules.push_back(m); }
    inline void addRenderPass(VkRenderPass r) { if (r) renderPasses.push_back(r); }
    inline void addCommandPool(VkCommandPool p) { if (p) commandPools.push_back(p); }
    inline void addSemaphore(VkSemaphore s) { if (s) semaphores.push_back(s); }
    inline void addFence(VkFence f) { if (f) fences.push_back(f); }
    inline void addBuffer(VkBuffer b, VmaAllocation a) { if (b && a) buffers.push_back({ b,a }); }

    // Destroy in reverse creation order, then clear
    void flush() {
        // GPU sync is *your* job (fences). This only destroys.

        for (auto it = pipelines.rbegin(); it != pipelines.rend(); ++it)
            vkDestroyPipeline(device, *it, nullptr);
        pipelines.clear();

        for (auto it = pipelineLayouts.rbegin(); it != pipelineLayouts.rend(); ++it)
            vkDestroyPipelineLayout(device, *it, nullptr);
        pipelineLayouts.clear();

        for (auto it = descriptorPools.rbegin(); it != descriptorPools.rend(); ++it)
            vkDestroyDescriptorPool(device, *it, nullptr);
        descriptorPools.clear();

        for (auto it = descriptorSetLayouts.rbegin(); it != descriptorSetLayouts.rend(); ++it)
            vkDestroyDescriptorSetLayout(device, *it, nullptr);
        descriptorSetLayouts.clear();

        for (auto it = shaderModules.rbegin(); it != shaderModules.rend(); ++it)
            vkDestroyShaderModule(device, *it, nullptr);
        shaderModules.clear();

        for (auto it = framebuffers.rbegin(); it != framebuffers.rend(); ++it)
            vkDestroyFramebuffer(device, *it, nullptr);
        framebuffers.clear();

        for (auto it = imageViews.rbegin(); it != imageViews.rend(); ++it)
            vkDestroyImageView(device, *it, nullptr);
        imageViews.clear();

        for (auto it = samplers.rbegin(); it != samplers.rend(); ++it)
            vkDestroySampler(device, *it, nullptr);
        samplers.clear();

        for (auto it = commandPools.rbegin(); it != commandPools.rend(); ++it)
            vkDestroyCommandPool(device, *it, nullptr);
        commandPools.clear();

        for (auto it = semaphores.rbegin(); it != semaphores.rend(); ++it)
            vkDestroySemaphore(device, *it, nullptr);
        semaphores.clear();

        for (auto it = fences.rbegin(); it != fences.rend(); ++it)
            vkDestroyFence(device, *it, nullptr);
        fences.clear();

        for (auto it = buffers.rbegin(); it != buffers.rend(); ++it)
            vmaDestroyBuffer(allocator, it->buffer, it->alloc);
        buffers.clear();

        for (auto it = images.rbegin(); it != images.rend(); ++it)
            vmaDestroyImage(allocator, it->image, it->alloc);
        images.clear();
    }

private :
    // Buckets (add more types if you use them)
    std::vector<VkImageView>          imageViews;
    std::vector<ImageAlloc>           images;
    std::vector<VkSampler>            samplers;
    std::vector<VkFramebuffer>        framebuffers;
    std::vector<VkPipeline>           pipelines;
    std::vector<VkPipelineLayout>     pipelineLayouts;
    std::vector<VkDescriptorSetLayout>descriptorSetLayouts;
    std::vector<VkDescriptorPool>     descriptorPools;
    std::vector<VkShaderModule>       shaderModules;
    std::vector<VkRenderPass>         renderPasses; // if you ever use legacy
    std::vector<VkCommandPool>        commandPools;
    std::vector<VkSemaphore>          semaphores;
    std::vector<VkFence>              fences;
    std::vector<BufferAlloc>          buffers;
};