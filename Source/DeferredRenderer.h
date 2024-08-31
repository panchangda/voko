#pragma once
#include "SceneRenderer.h"

class DeferredRenderer : public SceneRenderer
{
public:
    DeferredRenderer(vks::VulkanDevice* inVulkanDeivce,
    const std::vector<Mesh*>& sceneMeshes,
    VkSemaphore inPresentComplete,
    VkSemaphore inRenderComplete,
    VkQueue inGfxQueue);
    
    virtual void Render() override;

    std::vector< std::unique_ptr<RenderPass> > RenderPasses;

    // Capsulated vks device ptr
    vks::VulkanDevice* vulkanDevice;
    
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo submitInfo;

    VkSemaphore presentComplete;
    VkSemaphore renderComplete;
    VkQueue gfxQueue;
};


