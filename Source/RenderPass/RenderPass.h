#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "voko_globals.h"
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
        //
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            VK_CHECK_RESULT(vkCreatePipelineCache(inVulkanDevice->logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

            VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
            VK_CHECK_RESULT(vkCreateSemaphore(vulkanDevice->logicalDevice, &semaphoreCreateInfo, nullptr, &passSemaphore));
        }

        
        
        if(inVulkanDevice)
        {
            physicalDevice = inVulkanDevice->physicalDevice;
            device = inVulkanDevice->logicalDevice;
        }else
        {
            std::cerr << "Failed to init " << passName << " inVulkanDevice is nullptr!\n";
        }

        // special construct for onScreen
        if(passAttachmentType == EPassAttachmentType::OnScreen)
        {
            // initialize onscreen cmd buffers, same size as swapchain image count
            drawCmdBuffers.resize(voko_global::frameBuffers.size());
            // Create one command buffer for each swap chain image and reuse for rendering
            VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.commandPool = voko_global::commandPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(drawCmdBuffers.size());

            VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &commandBufferAllocateInfo, drawCmdBuffers.data()));
        }else
        {
            if (cmdBuffer == VK_NULL_HANDLE)
            {
                cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
            }
        }



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
    virtual void RenderScene()
    {
        for (int Mesh_Index = 0; Mesh_Index < voko_global::SceneMeshes.size(); Mesh_Index++)
        {
            const auto mesh = voko_global::SceneMeshes[Mesh_Index];
            // Bind Per Mesh Ds
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &voko_global::PerMeshDescriptorSets[Mesh_Index], 0, NULL);
            mesh->draw_mesh(cmdBuffer);
        }
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
    
    std::string passName = "";
    vks::VulkanDevice* vulkanDevice = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    vks::Framebuffer *frameBuffer = nullptr;

    // Pass ds related:
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    VkSemaphore passSemaphore = VK_NULL_HANDLE;
    

    ERenderPassType PassType;
    
    EPassAttachmentType passAttachmentType;
    // special for on screen pass rendering
    std::vector<VkCommandBuffer> drawCmdBuffers;

private:
    bool bInitialized = false;
};

