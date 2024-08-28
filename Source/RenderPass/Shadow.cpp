#include "Shadow.h"

#include <array>

#include "VulkanFrameBuffer.hpp"

// Declared in voko.h
extern int LIGHT_COUNT;

void ShadowPass::setupFrameBuffer(int width, int height)
{
    frameBuffer = new vks::Framebuffer(vulkanDevice);

    // Shadowmap properties
#if defined(__ANDROID__)
    // Use smaller shadow maps on mobile due to performance reasons
    frameBuffer->width = 1024;
    frameBuffer->height = 1024;
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

void ShadowPass::preparePipeline()
{
    // Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        descriptorSetLayouts.data(), descriptorSetLayouts.size());
    VK_CHECK_RESULT(vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    // Pipelines
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(
        1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(
        VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(
        VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(
        dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    
    VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, frameBuffer->renderPass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    // Shadow mapping pipeline
    // The shadow mapping pipeline uses geometry shader instancing (invocations layout modifier) to output
    // shadow maps for multiple lights sources into the different shadow map layers in one single render pass
    std::array<VkPipelineShaderStageCreateInfo, 2> shadowStages;
    shadowStages[0] = vks::tools::loadShader(getShaderBasePath() + "deferredshadows/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, vulkanDevice->logicalDevice);
    shadowStages[1] = vks::tools::loadShader(getShaderBasePath() + "deferredshadows/shadow.geom.spv",VK_SHADER_STAGE_GEOMETRY_BIT, vulkanDevice->logicalDevice);

    pipelineCI.pStages = shadowStages.data();
    pipelineCI.stageCount = static_cast<uint32_t>(shadowStages.size());

    // Shadow pass doesn't use any color attachments
    colorBlendState.attachmentCount = 0;
    colorBlendState.pAttachments = nullptr;
    // Cull front faces
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    // Enable depth bias
    rasterizationState.depthBiasEnable = VK_TRUE;
    // Add depth bias to dynamic state, so we can change it at runtime
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    // Reset blend attachment state
    pipelineCI.renderPass = frameBuffer->renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
}

void ShadowPass::buildCommandBuffer(const std::vector<Mesh *>& sceneMeshes)
{
    if(cmdBuffer == VK_NULL_HANDLE)
    {
        cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
    }else
    {
        vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    if(passSemaphore == VK_NULL_HANDLE)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
        VK_CHECK_RESULT(vkCreateSemaphore(vulkanDevice->logicalDevice, &semaphoreCreateInfo, nullptr, &passSemaphore));
    }
    
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    std::array<VkClearValue, 2> clearValues = {};
    VkViewport viewport;
    VkRect2D scissor;

    // First pass: Shadow map generation
    // -------------------------------------------------------------------------------------------------------

    clearValues[0].depthStencil = { 1.0f, 0 };

    renderPassBeginInfo.renderPass = frameBuffer->renderPass;
    renderPassBeginInfo.framebuffer = frameBuffer->framebuffer;
    renderPassBeginInfo.renderArea.extent.width = frameBuffer->width;
    renderPassBeginInfo.renderArea.extent.height = frameBuffer->height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues.data();

    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

    viewport = vks::initializers::viewport((float)frameBuffer->width, (float)frameBuffer->height, 0.0f, 1.0f);
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    scissor = vks::initializers::rect2D(frameBuffer->width, frameBuffer->height, 0, 0);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // Set depth bias (aka "Polygon offset")
    vkCmdSetDepthBias(
        cmdBuffer,
        depthBiasConstant,
        0.0f,
        depthBiasSlope);

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    for(const auto meshPtr : sceneMeshes)
    {
        meshPtr->draw_mesh();
    }

    vkCmdEndRenderPass(cmdBuffer);

    vkCmdEndRenderPass(cmdBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}

