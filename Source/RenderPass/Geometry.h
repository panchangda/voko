#include "RenderPass.h"

class GeometryPass : public RenderPass
{
    GeometryPass(vks::VulkanDevice* inVulkanDevice):RenderPass(inVulkanDevice){}

    virtual void setupFrameBuffer(int width, int height) override;
    virtual void buildCommandBuffer(const std::vector<Mesh*>& sceneMeshes) override;
};