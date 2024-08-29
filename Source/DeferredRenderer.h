#pragma once
#include "SceneRenderer.h"

class DeferredRenderer : public SceneRenderer
{
public:
    DeferredRenderer(vks::VulkanDevice* inVulkanDeivce,
        const std::vector<Mesh*>& sceneMeshes,
        VkSemaphore& RenderCompleteSemaphore);
    
    virtual void Render() override;

    std::vector< std::unique_ptr<RenderPass> > RenderPasses;

    vks::VulkanDevice* vulkanDevice;
};


