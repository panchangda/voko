#pragma once
#include "RenderPass/RenderPass.h"

class GeometryPass : public RenderPass
{
    public:
    GeometryPass(const std::string& name,
                        vks::VulkanDevice* inVulkanDevice,
                        uint32_t inWidth,
                        uint32_t inHeight,
                        ERenderPassType inPassType,
                        EPassAttachmentType inAttachmentType);
    ~GeometryPass() override;
    virtual void setupFrameBuffer() override;
    virtual void setupDescriptorSet() override;
    virtual void preparePipeline() override;
    virtual void buildCommandBuffer() override;
};