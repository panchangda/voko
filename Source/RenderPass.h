#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vks
{
    struct Framebuffer;
    class VulkanDevice;
}

enum RenderPassType
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
    virtual void buildCommandBuffer() = 0;
    virtual vks::Framebuffer* getFrameBuffer(){return frameBuffer;}

    
    std::string passName;
    vks::VulkanDevice* vulkanDevice;
    vks::Framebuffer *frameBuffer;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
private:
    
};