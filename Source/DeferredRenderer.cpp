
#include "DeferredRenderer.h"
#include "RenderPass/Shadow.h"

DeferredRenderer::DeferredRenderer(
    vks::VulkanDevice* inVulkanDeivce,
    const std::vector<Mesh*>& sceneMeshes) : SceneRenderer()
{
    vulkanDevice = inVulkanDeivce;
    int width = 2048, height = 2048;

    // Prepare shadow pass
    std::unique_ptr<ShadowPass> shadow_pass = std::make_unique<ShadowPass>(vulkanDevice);
    
    shadow_pass->setupFrameBuffer(width, height);
    shadow_pass->preparePipeline();
    shadow_pass->buildCommandBuffer(sceneMeshes);

    RenderPasses.push_back(std::move(shadow_pass));


    
}

void DeferredRenderer::Render()
{
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;
}

