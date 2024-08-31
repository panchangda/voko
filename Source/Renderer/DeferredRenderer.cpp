
#include "DeferredRenderer.h"

#include "RenderPass/Geometry.h"
#include "RenderPass/Shadow.h"

DeferredRenderer::DeferredRenderer(
    vks::VulkanDevice* inVulkanDeivce,
    const std::vector<Mesh*>& sceneMeshes,
    VkDescriptorSetLayout inSceneDsLayout,
    VkDescriptorSet inSceneDs,
    VkDescriptorSetLayout inPerMeshDsLayout,
    const std::vector<VkDescriptorSet>& inPerMeshDSs,
    VkSemaphore inPresentComplete,
    VkSemaphore inRenderComplete,
    
    VkQueue inGfxQueue) : SceneRenderer()
{
    /* Initialize self vars:
     * device, framebuffer size, semaphores, gfx queue, submitInfos...
     */
    vulkanDevice = inVulkanDeivce;
    
    int width = 2048, height = 2048;

    presentComplete = inPresentComplete;
    renderComplete = inRenderComplete;

    gfxQueue = inGfxQueue;

    sceneDescriptorSetLayout = inSceneDsLayout;
    sceneDescriptorSet = inSceneDs;
    
    perMeshDescriptorSetLayout = inPerMeshDsLayout;
    perMeshDescriptorSets = inPerMeshDSs;
    
    submitInfo = vks::initializers::submitInfo();
    /** @brief Pipeline stages used to wait at for graphics queue submissions */
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderComplete;

    /* Prepare passes */
    // shadow pass
    std::unique_ptr<ShadowPass> shadow_pass = std::make_unique<ShadowPass>(vulkanDevice);
    
    shadow_pass->setupFrameBuffer(width, height);
    shadow_pass->setDescriptorSetLayouts(
        sceneDescriptorSetLayout,
        sceneDescriptorSet,
        perMeshDescriptorSetLayout,
         perMeshDescriptorSets );
    shadow_pass->preparePipeline();
    shadow_pass->buildCommandBuffer(sceneMeshes);

    RenderPasses.push_back(std::move(shadow_pass));


    // geometry pass
    // std::unique_ptr<GeometryPass> geometry_pass;
    // RenderPasses.push_back(std::move(geometry_pass));


    // lighting pass
}

void DeferredRenderer::Render()
{
    // Set submitInfo for sequential passes
    for(size_t i=0;i<RenderPasses.size();i++)
    {
        const auto& pass = RenderPasses[i];
        if(i == 0) // wait for present
        {
            submitInfo.pWaitSemaphores = &presentComplete;
            
        }else // wait for previous
        {
            submitInfo.pWaitSemaphores = &RenderPasses[i-1]->passSemaphore;
        }
        // signal self pass semaphore
        submitInfo.pSignalSemaphores = &pass->passSemaphore;
        
        if(i == RenderPasses.size()-1) // signal render complete
        {
            submitInfo.pSignalSemaphores = &renderComplete;
        }
        submitInfo.pCommandBuffers = &pass->cmdBuffer;
        VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }

}

