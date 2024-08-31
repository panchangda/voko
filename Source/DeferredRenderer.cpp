
#include "DeferredRenderer.h"

#include "RenderPass/Geometry.h"
#include "RenderPass/Shadow.h"

DeferredRenderer::DeferredRenderer(
    vks::VulkanDevice* inVulkanDeivce,
    const std::vector<Mesh*>& sceneMeshes,
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

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
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
    shadow_pass->setDescriptorSetLayouts();
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
        auto& pass = RenderPasses[i];
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
        VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }

}

