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
    virtual ~RenderPass() = default;
    
    RenderPass(const std::string& name):passName(name){}
    std::string get_name(){return passName;}
    
    RenderPassType PassType;
    
    virtual void setupFrameBuffer(int width, int height) = 0;
    virtual void preparePipeline() = 0;
    virtual void createDescriptorSet() = 0;
    virtual void buildCommandBuffer(const std::vector<Mesh *>& sceneMeshes) = 0;

    // getter setters:
    virtual vks::Framebuffer* getFrameBuffer() { return frameBuffer; }

    virtual void setDescriptorSetLayouts(
        VkDescriptorSetLayout inSceneDsLayout,
        VkDescriptorSet inSceneDs,
        VkDescriptorSetLayout inPerMeshDsLayout,
        const std::vector<VkDescriptorSet>& inPerMeshDss)
    {
        sceneDescriptorSetLayout = inSceneDsLayout;
        sceneDescriptorSet = inSceneDs;

        perMeshDescriptorSetLayout = inPerMeshDsLayout;
        perMeshDescriptorSets = inPerMeshDss;
    }
    
    std::string passName;
    vks::VulkanDevice* vulkanDevice;
    vks::Framebuffer *frameBuffer;

    VkPipelineLayout pipelineLayout;
    
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;

    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

    VkSemaphore passSemaphore;

    // Descriptor sets:
    VkDescriptorSetLayout sceneDescriptorSetLayout;
    VkDescriptorSet sceneDescriptorSet;
    VkDescriptorSetLayout perMeshDescriptorSetLayout;
    std::vector<VkDescriptorSet> perMeshDescriptorSets;
private:
    
};

