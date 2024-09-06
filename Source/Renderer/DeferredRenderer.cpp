
#include "DeferredRenderer.h"

#include "voko_globals.h"
#include "RenderPass/FullScreen.hpp"
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
    defaultSubmitPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &defaultSubmitPipelineStageFlags;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderComplete;
    submitInfo.commandBufferCount = 1;


    /* Test FullScreen Pass */
    // RenderPasses.push_back(std::make_shared<FullScreenPass>(
    //     "FullScreenPass_Test",
    //     vulkanDevice,
    //     width, height,
    //     ERenderPassType::FullScreen,
    //     EPassAttachmentType::OnScreen));
    
    
    /* Prepare passes */
    // shadow pass
    std::pair<uint32_t, uint32_t> ShadowResolution = std::make_pair
#ifdef __ANDROID__ // Use smaller shadow maps on mobile due to performance reasons
   (1024, 1024);
#else
        (2048, 2048);
#endif
    
    std::shared_ptr<ShadowPass> shadow_pass = std::make_shared<ShadowPass>(
        "ShadowPass",
        vulkanDevice,
        ShadowResolution.first, ShadowResolution.second,
        ERenderPassType::Mesh,
        EPassAttachmentType::OffScreen,
        1.25f, 1.75f);
    RenderPasses.push_back(shadow_pass);

    // geometry pass
    std::pair<uint32_t, uint32_t> GBufferResolution = std::make_pair
#ifdef __ANDROID__ // Use smaller shadow maps on mobile due to performance reasons
   (1024, 1024);
#else
    (2048, 2048);
#endif
    std::shared_ptr<GeometryPass> geometry_pass = std::make_shared<GeometryPass>(
        "GeometryPass",
        vulkanDevice,
        GBufferResolution.first, GBufferResolution.second,
        ERenderPassType::Mesh,
        EPassAttachmentType::OffScreen);
    RenderPasses.push_back(geometry_pass);
    
    // lighting pass
    std::unique_ptr<LightingPass> lighting_pass = std::make_unique<LightingPass>(
        "LightingPass",
        vulkanDevice,
        voko_global::width, voko_global::height,
        ERenderPassType::FullScreen,
        EPassAttachmentType::OnScreen,
        shadow_pass,
        geometry_pass);
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

