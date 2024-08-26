# pragma once

#include "RenderPass.h"


class ShadowPass : public RenderPass
{
public:
    ShadowPass(vks::VulkanDevice* inVulkanDevice) : RenderPass(inVulkanDevice){}
    virtual void setupFrameBuffer(int width, int height) override;
    virtual void setupDescriptors() override;
    virtual void preparePipeline() override;
    virtual void buildCommandBuffer() override;
};


