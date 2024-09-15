
#include "DeferredRenderer.h"

#include "voko_globals.h"
#include "RenderPass/FullScreen.hpp"
#include "RenderPass/Geometry.h"
#include "RenderPass/Lighting.h"
#include "RenderPass/Shadow.h"
#include "RenderPass/Skybox.hpp"
#include "RenderPass/Tone.hpp"

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
    
    shadow_pass = std::make_shared<ShadowPass>(
        "ShadowPass",
        vulkanDevice,
        ShadowResolution.first, ShadowResolution.second,
        ERenderPassType::Mesh,
        EPassAttachmentType::OffScreen,
        1.25f, 1.75f);
    // RenderPasses.push_back(shadow_pass);

    // geometry pass
    std::pair<uint32_t, uint32_t> GBufferResolution = std::make_pair
#ifdef __ANDROID__
    (voko_global::width / 2, voko_global::height / 2);
#else
    (voko_global::width, voko_global::height);
#endif
    geometry_pass = std::make_shared<GeometryPass>(
        "GeometryPass",
        vulkanDevice,
        GBufferResolution.first, GBufferResolution.second,
        ERenderPassType::Mesh,
        EPassAttachmentType::OffScreen);
    // RenderPasses.push_back(geometry_pass);
    
    // lighting pass
    lighting_pass = std::make_unique<LightingPass>(
        "LightingPass",
        vulkanDevice,
        voko_global::width, voko_global::height,
        ERenderPassType::FullScreen,
        EPassAttachmentType::OffScreen,
        shadow_pass,
        geometry_pass);
    // RenderPasses.push_back(std::move(lighting_pass));

    // process skybox
    if (voko_global::bDisplaySkybox) {
        skybox_pass = std::make_unique<SkyboxPass>(
            "SkyboxPass",
            vulkanDevice,
            voko_global::width, voko_global::height,
            ERenderPassType::FullScreen,
            EPassAttachmentType::OffScreen);
        // RenderPasses.push_back(std::move(skybox_pass));
    }

    // post process tone pass
    tone_pass = std::make_unique<TonePass>(
        "TonePass",
        vulkanDevice,
        voko_global::width, voko_global::height,
        ERenderPassType::FullScreen,
        EPassAttachmentType::OffScreen
    );

    // todo: use ping pong to replace full screen blit
    // Build blit pass
    buildBlitPass();
}

void DeferredRenderer::Render()
{
    // // Set submitInfo for sequential passes
    // for(size_t i=0;i<RenderPasses.size();i++)
    // {
    //     const auto& pass = RenderPasses[i];
    //     if(i == 0) // wait for present
    //     {
    //         submitInfo.pWaitSemaphores = &presentComplete;
    //
    //     }else // wait for previous
    //     {
    //         submitInfo.pWaitSemaphores = &RenderPasses[i-1]->passSemaphore;
    //     }
    //
    //     // signal self pass semaphore
    //     submitInfo.pSignalSemaphores = &pass->passSemaphore;
    //     if(i == RenderPasses.size()-1) // signal render complete
    //     {
    //         submitInfo.pSignalSemaphores = &renderComplete;
    //     }
    //
    //     submitInfo.pCommandBuffers = pass->getCommandBuffer(voko_global::currentBuffer);
    //     VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
    // }


    // shadow waits for presentComplete
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &defaultSubmitPipelineStageFlags;
    submitInfo.pWaitSemaphores = &presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &shadow_pass->passSemaphore;
    submitInfo.pCommandBuffers = shadow_pass->getCommandBuffer(voko_global::currentBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // geometry waits for presentComplete
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &defaultSubmitPipelineStageFlags;
    submitInfo.pWaitSemaphores = &shadow_pass->passSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &geometry_pass->passSemaphore;
    submitInfo.pCommandBuffers = geometry_pass->getCommandBuffer(voko_global::currentBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // lighting waits for:
    // 1: geometry pass, 2: shadow pass

    // submitInfo.waitSemaphoreCount = 2;
    // VkPipelineStageFlags light_pass_wait_stages[2] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // submitInfo.pWaitDstStageMask = light_pass_wait_stages;
    // VkSemaphore lighting_pass_wait_semaphore[2] = {shadow_pass->passSemaphore, geometry_pass->passSemaphore};
    // submitInfo.pWaitSemaphores = lighting_pass_wait_semaphore;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &geometry_pass->passSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &lighting_pass->passSemaphore;
    submitInfo.pCommandBuffers = lighting_pass->getCommandBuffer(voko_global::currentBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // skybox waits for lighting
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &defaultSubmitPipelineStageFlags;
    submitInfo.pWaitSemaphores = &lighting_pass->passSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &skybox_pass->passSemaphore;
    submitInfo.pCommandBuffers = skybox_pass->getCommandBuffer(voko_global::currentBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // tone pass wait for skybox
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &defaultSubmitPipelineStageFlags;
    submitInfo.pWaitSemaphores = &skybox_pass->passSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &tone_pass->passSemaphore;
    submitInfo.pCommandBuffers = tone_pass->getCommandBuffer(voko_global::currentBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));


    // blit waits for tone,
    // after blit, signal renderComplete
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &tone_pass->passSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderComplete;
    submitInfo.pCommandBuffers = &blitCmdBuffers[voko_global::currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));

}

void DeferredRenderer::buildBlitPass() {
    // Build blitBuffers for swapChain images
    blitCmdBuffers.resize(voko_global::swapChain->images.size());

    for (int i = 0; i < voko_global::swapChain->images.size(); i++) {
        blitCmdBuffers[i] = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

        VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(blitCmdBuffers[i], &cmdBufInfo));

        VkImage &dstImage = voko_global::swapChain->buffers[i].image;

        // Set image layout for transfer
        vks::tools::setImageLayout(
            blitCmdBuffers[i],
            voko_global::sceneColor.image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vks::tools::setImageLayout(
            blitCmdBuffers[i],
            dstImage,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


        VkImageSubresourceLayers imageSubresource = vks::initializers::imageSubresourceLayers(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            0,
            1);

        int32_t srcWidth, srcHeight, dstWidth, dstHeight;
        srcWidth = dstWidth = static_cast<int32_t>(voko_global::width);
        srcHeight = dstHeight = static_cast<int32_t>(voko_global::height);
        // offset0 point to image left top, offset1 point to image right bottom
        // offset0&1 set image bounds
        VkOffset3D srcOffsets[2] = {VkOffset3D(0, 0, 0), VkOffset3D(srcWidth, srcHeight, 1)};
        VkOffset3D dstOffsets[2] = {VkOffset3D(0, 0, 0), VkOffset3D(dstWidth, dstHeight, 1)};
        VkImageBlit imageBlit = vks::initializers::imageBlit(
            imageSubresource, srcOffsets,
            imageSubresource, dstOffsets);

        // Blit scene color to swapChain buffer
        vkCmdBlitImage(blitCmdBuffers[i],
                       voko_global::sceneColor.image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dstImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_LINEAR);


        // Revert scene color for color write
        // Set swapChain image for present
        vks::tools::setImageLayout(
            blitCmdBuffers[i],
            voko_global::sceneColor.image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        vks::tools::setImageLayout(
            blitCmdBuffers[i],
            dstImage,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


        VK_CHECK_RESULT(vkEndCommandBuffer(blitCmdBuffers[i]));
    }
}

