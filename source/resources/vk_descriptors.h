#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <deque>

namespace retro 
{
     // Handles the creation of VkDescriptorSetLayout handles.
    class DescriptorLayoutBuilder 
    {
    public:
        struct DescriptorBinding 
        {
            uint32_t binding;
            VkDescriptorType type;
            uint32_t count;
        };

        void add_binding(uint32_t binding, VkDescriptorType type, uint32_t count = 1);
        void clear();

        // Builds the layout. For indexing, we pass pNext for VkDescriptorSetLayoutBindingFlagsCreateInfo
        VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

    private:
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    
    
    //Manages VkDescriptorPool logic to prevent fragmentation and repeated creation.
    class DescriptorAllocator 
    {
    public:
        struct PoolSizeRatio 
        {
            VkDescriptorType type;
            float ratio;
        };

        void init_pool(VkDevice device, uint32_t maxSets, std::vector<PoolSizeRatio> poolRatios);
        void clear_descriptors(VkDevice device);
        void destroy_pool(VkDevice device);
		VkDescriptorPool get_pool() { return pool; }

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);

    private:
        VkDescriptorPool pool;
    };

    
    
    //Aggregates VkWriteDescriptorSet structures to update sets easily.
    struct DescriptorWriter 
    {
        std::deque<VkDescriptorImageInfo> imageInfos;
        std::deque<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writes;

        void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
        void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

        void clear();
        void update_set(VkDevice device, VkDescriptorSet set);
    };

} // namespace retr0