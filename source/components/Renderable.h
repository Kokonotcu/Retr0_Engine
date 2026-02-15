#pragma once

#include <components/Component.h>

#include <resources/vk_mesh.h>
#include <resources/vk_descriptors.h>
#include <resources/Material.h> 

namespace retro 
{
	class Renderable : public Component, public retro::Drawable
    {

    public:
        // Overload == so std::remove in RemoveRenderable still works
        bool operator==(const Renderable& other) const 
        {
            // Change this later with uuids
            return mesh == other.mesh && material == other.material;
        }

        void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, glm::mat4x4 worldMatrix) override;
        void Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, CPUPushConstant pC);
        
        bool IsValid() const override 
        {
            return mesh != nullptr && material != nullptr;
		}
	public:
        std::shared_ptr<retro::Mesh> mesh;
        std::shared_ptr<retro::Material> material;
        glm::mat4 transformMatrix{ 1.0f };
    };
}