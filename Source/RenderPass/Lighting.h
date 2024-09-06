#pragma once

#include "RenderPass.h"

class LightingPass : public RenderPass
{
public:
    LightingPass(const std::string& name,
                        vks::VulkanDevice* inVulkanDevice,
                        uint32_t inWidth,
                        uint32_t inHeight,
                        ERenderPassType inPassType,
                        EPassAttachmentType inAttachmentType,
                // Lighting pass specials:
                std::shared_ptr<RenderPass> ShadowPass,
                std::shared_ptr<RenderPass> GeometryPass);
    virtual ~LightingPass() override = default;
    virtual void setupFrameBuffer() override;
    virtual void setupDescriptorSet() override;
    virtual void preparePipeline() override;
    virtual void buildCommandBuffer() override;

private:
    std::shared_ptr<RenderPass> m_ShadowPass;
    std::shared_ptr<RenderPass> m_GeometryPass;
};

