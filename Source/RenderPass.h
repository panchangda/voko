#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "SceneGraph/Mesh.h"

namespace vks
{
    struct Framebuffer;
    class VulkanDevice;
}

enum class RenderPassType
{
    Mesh = 0x01,
    FullScreen = 0x02,
};
class RenderPass
{
public:
    RenderPass() = delete;
    explicit RenderPass(vks::VulkanDevice* inVulkanDevice) : vulkanDevice(inVulkanDevice){}
    virtual ~RenderPass();
    
    RenderPass(const std::string& name):passName(name){}
    std::string get_name(){return passName;}
    
    RenderPassType PassType;
    
    virtual void setupFrameBuffer(int width, int height) = 0;
    virtual void setupPipelineLayouts() = 0;
    virtual void preparePipeline() = 0;
    virtual void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InLayouts);
    virtual void buildCommandBuffer(const std::vector<Mesh *>& sceneMeshes) = 0;
    virtual vks::Framebuffer* getFrameBuffer(){return frameBuffer;}

    
    std::string passName;
    vks::VulkanDevice* vulkanDevice;
    vks::Framebuffer *frameBuffer;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    VkPipelineLayout pipelineLayout;
    
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;

    VkCommandBuffer cmdBuffer;

    VkSemaphore passSemaphore;
private:
    
};

inline void RenderPass::setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& InLayouts)
{
    descriptorSetLayouts = InLayouts;
}
