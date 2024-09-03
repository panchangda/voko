#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "SceneGraph/Mesh.h"

namespace vks
{
    struct Framebuffer;
    struct VulkanDevice;
}

enum class ERenderPassType
{
    Mesh = 0x01,
    FullScreen = 0x02,
    RenderPassTypeNum
};
enum class EPassAttachmentType
{
    // Directly access swapchain image
    OnScreen = 0x01,
    OffScreen = 0x02,
    PassAttachmentTypeNum
};
class RenderPass
{
public:
    RenderPass() = delete;

    explicit RenderPass(const std::string& name,
                        vks::VulkanDevice* inVulkanDevice,
                        uint32_t inWidth,
                        uint32_t inHeight,
                        ERenderPassType inPassType,
                        EPassAttachmentType inAttachmentType)
        : passName(name),
          vulkanDevice(inVulkanDevice),
          width(inWidth),
          height(inHeight),
          PassType(inPassType),
          passAttachmentType(inAttachmentType)
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreatePipelineCache(inVulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
    }
    
    virtual ~RenderPass() = default;

    // explicitly called in child class constructor
    // for situation: child override functions using child properties
    virtual void init(){
        setupFrameBuffer();
        setupDescriptorSet();
        preparePipeline();
        buildCommandBuffer();
        bInitialized = true;
    }
    virtual void setupFrameBuffer(){}
    virtual void setupDescriptorSet(){}
    virtual void preparePipeline(){}
    virtual void buildCommandBuffer(){}

    // getter setters:
    std::string get_name(){ return passName; }
    
    vks::Framebuffer* getFrameBuffer() { return frameBuffer; }

    bool isInitialized(){ return bInitialized; }
    
    VkCommandBuffer& getCommandBuffer(uint32_t imageIndex)
    {
        VkCommandBuffer cmdBufferNullHandle = VK_NULL_HANDLE;
        if(!bInitialized)
        {
            vks::tools::exitFatal("Pass Haven't Initialized!", 1);
            return cmdBufferNullHandle;
        }
        
        if(passAttachmentType == EPassAttachmentType::OnScreen)
        {
            if(imageIndex > drawCmdBuffers.size() - 1)
            {
                vks::tools::exitFatal("OnScreen Pass's Draw Command Buffers Size doesn't Match SwapChain Size!", 1);
                return cmdBufferNullHandle;
            }
            
            return drawCmdBuffers[imageIndex];
            
        }else if(passAttachmentType == EPassAttachmentType::OffScreen)
        {
            if(cmdBuffer == VK_NULL_HANDLE)
            {
                vks::tools::exitFatal("Pass Doesn't Build a Command Buffer!", 1);
                return cmdBufferNullHandle;
            }

            return cmdBuffer;

            
        }
        
        return cmdBufferNullHandle;
    }
    
    std::string passName;
    vks::VulkanDevice* vulkanDevice;

    uint32_t width;
    uint32_t height;
    vks::Framebuffer *frameBuffer;

    // Pass ds related:
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    VkSemaphore passSemaphore;
    

    ERenderPassType PassType;
    
    EPassAttachmentType passAttachmentType;
    // special for on screen pass rendering
    std::vector<VkCommandBuffer> drawCmdBuffers;

private:
    bool bInitialized = false;
};

