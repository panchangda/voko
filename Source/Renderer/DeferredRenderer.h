#pragma once
#include "SceneRenderer.h"

class TonePass;
class SkyboxPass;
class LightingPass;
class GeometryPass;
class ShadowPass;

class DeferredRenderer : public SceneRenderer
{
public:
    DeferredRenderer(vks::VulkanDevice* inVulkanDeivce,
    VkSemaphore inPresentComplete,
    VkSemaphore inRenderComplete,
    VkQueue inGfxQueue);
    
    virtual ~DeferredRenderer() override = default;
    
    virtual void Render() override;

    std::vector< std::shared_ptr<RenderPass> > RenderPasses;

    // Capsulated vks device ptr
    vks::VulkanDevice* vulkanDevice;
    
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo submitInfo;
    VkPipelineStageFlags defaultSubmitPipelineStageFlags;
    VkSemaphore presentComplete;
    VkSemaphore renderComplete;
    VkQueue gfxQueue;


    void buildBlitPass();
    std::vector<VkCommandBuffer> blitCmdBuffers;

private:
    std::shared_ptr<ShadowPass> shadow_pass;
    std::shared_ptr<GeometryPass> geometry_pass;
    std::unique_ptr<LightingPass> lighting_pass;
    std::unique_ptr<SkyboxPass> skybox_pass;
    std::unique_ptr<TonePass> tone_pass;


};


