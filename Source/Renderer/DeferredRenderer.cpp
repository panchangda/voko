
#include "DeferredRenderer.h"

#include "voko_globals.h"
#include "RenderPass/Geometry.h"
#include "RenderPass/Lighting.h"
#include "RenderPass/Shadow.h"

DeferredRenderer::DeferredRenderer(
    vks::VulkanDevice* inVulkanDeivce,
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
    std::shared_ptr<ShadowPass> shadow_pass = std::make_shared<ShadowPass>(
        "ShadowPass",
        vulkanDevice,
        width, height,
        ERenderPassType::Mesh,
        EPassAttachmentType::OffScreen,
        1.25f, 1.75f);
    RenderPasses.push_back(shadow_pass);


    // geometry pass
    // std::unique_ptr<GeometryPass> geometry_pass;
    // RenderPasses.push_back(std::move(geometry_pass));


    // lighting pass
    std::unique_ptr<LightingPass> lighting_pass = std::make_unique<LightingPass>(
        "LightingPass",
        vulkanDevice,
        voko_global::width, voko_global::height,
        ERenderPassType::FullScreen,
        EPassAttachmentType::OnScreen,
        shadow_pass);
    RenderPasses.push_back(std::move(lighting_pass));
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
        submitInfo.pCommandBuffers = &pass->getCommandBuffer(voko_global::currentBuffer);
        VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }

}

