#pragma once

#include "RenderPass.h"


class ShadowPass : public RenderPass
{
public:
    ShadowPass(const std::string& name,
                        vks::VulkanDevice* inVulkanDevice,
                        uint32_t inWidth,
                        uint32_t inHeight,
                        ERenderPassType inPassType,
                        EPassAttachmentType inAttachmentType,

                        // Shadow Pass Specials: used for pipeline
                        float inDepthBiasConstant = 1.25f,
                        float inDepthBiasSlope = 1.75f);
    virtual void setupFrameBuffer() override;
    virtual void setupDescriptorSet() override;
    virtual void preparePipeline() override;
    virtual void buildCommandBuffer() override;
    virtual ~ShadowPass() override;
    
    // Shadow Pass Special Properties
    // Depth bias (and slope) are used to avoid shadowing artifacts
    float depthBiasConstant = 1.25f;
    float depthBiasSlope = 1.75f;
};


