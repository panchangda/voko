# pragma once

#include "RenderPass.h"


class ShadowPass : public RenderPass
{
public:
    ShadowPass() = default;
    ShadowPass(const std::string& name);
    virtual void setupFrameBuffer() override;
    virtual void setupDescriptors() override;
    virtual void buildCommandBuffer() override;
    
};

