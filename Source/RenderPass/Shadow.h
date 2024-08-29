# pragma once

#include "RenderPass.h"


class ShadowPass : public RenderPass
{
public:
    ShadowPass(vks::VulkanDevice* inVulkanDevice);
    virtual void setupFrameBuffer(int width, int height) override;
    virtual void preparePipeline() override;
    virtual void buildCommandBuffer(const std::vector<Mesh *>& sceneMeshes) override;

    // Properties
    // Depth bias (and slope) are used to avoid shadowing artifacts
    float depthBiasConstant = 1.25f;
    float depthBiasSlope = 1.75f;
};


