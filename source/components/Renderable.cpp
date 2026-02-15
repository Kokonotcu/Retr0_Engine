#include "components/Renderable.h"

void retro::Renderable::Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, glm::mat4x4 worldMatrix)
{
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &mesh->vertexOffset);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(retro::CPUPushConstant), &mesh->cpuPushConstant);
    vkCmdBindIndexBuffer(cmd, indexBuffer, mesh->indexOffset, VK_INDEX_TYPE_UINT32);

    for (const auto& submesh : mesh->submeshes)
    {
        vkCmdDrawIndexed(cmd, submesh.count, 1, submesh.startIndex, 0, 0);
    }
}

void retro::Renderable::Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkBuffer indexBuffer, VkBuffer vertexBuffer, CPUPushConstant pC)
{
    mesh->cpuPushConstant = pC;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &mesh->vertexOffset);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(retro::CPUPushConstant), &mesh->cpuPushConstant);
    vkCmdBindIndexBuffer(cmd, indexBuffer, mesh->indexOffset, VK_INDEX_TYPE_UINT32);
    
    for (const auto& submesh : mesh->submeshes)
    {
        vkCmdDrawIndexed(cmd, submesh.count, 1, submesh.startIndex, 0, 0);
    }   
}
