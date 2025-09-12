#pragma once

#include <vk_types.h>

struct DescriptorLayoutBuilder 
{
    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct DescriptorAllocator 
{
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void ClearDescriptors(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
    VkDescriptorPool pool;
};