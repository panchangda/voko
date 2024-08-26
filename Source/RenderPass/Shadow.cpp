#include "Shadow.h"

#include "VulkanFrameBuffer.hpp"

// Declared in voko.h
extern int LIGHT_COUNT;

void ShadowPass::setupFrameBuffer(int width, int height)
{
    frameBuffer = new vks::Framebuffer(vulkanDevice);

    // Shadowmap properties
#if defined(__ANDROID__)
    // Use smaller shadow maps on mobile due to performance reasons
    shadowFrameBuffer->width = 1024;
    shadowFrameBuffer->height = 1024;
#else
    frameBuffer->width = width;
    frameBuffer->height = height;
#endif

    // Find a suitable depth format
    VkFormat shadowMapFormat;
    
    VkBool32 validShadowMapFormat = vks::tools::getSupportedDepthFormat(vulkanDevice->physicalDevice, &shadowMapFormat);
    assert(validShadowMapFormat);

    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.format = shadowMapFormat;
    attachmentInfo.width = frameBuffer->width;
    attachmentInfo.height = frameBuffer->height;
    attachmentInfo.layerCount = LIGHT_COUNT;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    frameBuffer->addAttachment(attachmentInfo);

    // Create sampler to sample from to depth attachment
    // Used to sample in the fragment shader for shadowed rendering
    VK_CHECK_RESULT(frameBuffer->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(frameBuffer->createRenderPass());
}

void ShadowPass::setupDescriptors()
{
}

void ShadowPass::preparePipeline()
{
    // Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        &descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

}
