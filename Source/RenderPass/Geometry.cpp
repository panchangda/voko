#include "Geometry.h"
#include "voko_globals.h"
#include "VulkanFrameBuffer.hpp"


GeometryPass::GeometryPass(const std::string& name, vks::VulkanDevice* inVulkanDevice, uint32_t inWidth,
                           uint32_t inHeight, ERenderPassType inPassType, EPassAttachmentType inAttachmentType)
        : RenderPass(name, inVulkanDevice, inWidth, inHeight, inPassType, inAttachmentType)
{

    init();
}

GeometryPass::~GeometryPass()
{
}

void GeometryPass::setupFrameBuffer()
{
    frameBuffer = new vks::Framebuffer(vulkanDevice);
    frameBuffer->width = width;
    frameBuffer->height = height;
    
    // Four attachments (3 color, 1 depth)
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.width = frameBuffer->width;
    attachmentInfo.height = frameBuffer->height;
    attachmentInfo.layerCount = 1;
    attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    // Color attachments
    // Attachment 0: (World space) Positions
    attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    frameBuffer->addAttachment(attachmentInfo);

    // Attachment 1: (World space) Normals
    attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    frameBuffer->addAttachment(attachmentInfo);

    // Attachment 2: Albedo (color)
    attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    frameBuffer->addAttachment(attachmentInfo);

    // Attachment 3: Metallic
    attachmentInfo.format = VK_FORMAT_R8_UNORM;
    frameBuffer->addAttachment(attachmentInfo);

    // Attachment 4: Roughness
    attachmentInfo.format = VK_FORMAT_R8_UNORM;
    frameBuffer->addAttachment(attachmentInfo);

    // Attachment 5: Ao
    attachmentInfo.format = VK_FORMAT_R8_UNORM;
    frameBuffer->addAttachment(attachmentInfo);

    // geometry pass depth stencil ops:
    // clear -> write -> store
    frameBuffer->SetDepthStencilUsage(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Create sampler to sample from the color attachments
    VK_CHECK_RESULT(frameBuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(frameBuffer->createRenderPass());
}

void GeometryPass::setupDescriptorSet()
{
    std::array<VkDescriptorSetLayout ,2> geometryDsLayouts = {voko_global::SceneDescriptorSetLayout, voko_global::PerMeshDescriptorSetLayout};
    // Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        geometryDsLayouts.data(), static_cast<uint32_t>(geometryDsLayouts.size()));
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    
}

void GeometryPass::preparePipeline()
{
     // Shader Paths:
    std::string VSPath = "deferredshadows/geometry.vert.spv";
    std::string FSPath = "deferredshadows/geometry.frag.spv";

    // Shadow mapping pipeline
    // The shadow mapping pipeline uses geometry shader instancing (invocations layout modifier) to output
    // shadow maps for multiple lights sources into the different shadow map layers in one single render pass
    
    // Pipelines
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    // Shadow pass doesn't use any color attachments
    VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(
        0, nullptr);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(
        VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(
        VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(
        dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0] = vks::tools::loadShader(getShaderBasePath() + VSPath,
                                             VK_SHADER_STAGE_VERTEX_BIT, vulkanDevice->logicalDevice);
    shaderStages[1] = vks::tools::loadShader(getShaderBasePath() + FSPath,
                                             VK_SHADER_STAGE_FRAGMENT_BIT, vulkanDevice->logicalDevice);

    VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(
        pipelineLayout, frameBuffer->renderPass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    // Vertex input state from glTF model for pipeline rendering models
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Tangent });
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    
    // Blend attachment states required for all color attachments
    // This is important, as color write mask will otherwise be 0x0 and you
    // won't see anything rendered to the attachment
    std::array<VkPipelineColorBlendAttachmentState, 6> blendAttachmentStates =
    {
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
    };
    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();
    
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
    
}

void GeometryPass::buildCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    std::array<VkClearValue, 7> clearValues = {};
    // Clear values for all attachments written in the fragment shader
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[5].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[6].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();


    renderPassBeginInfo.renderPass = frameBuffer->renderPass;
    renderPassBeginInfo.framebuffer = frameBuffer->framebuffer;
    renderPassBeginInfo.renderArea.extent.width = frameBuffer->width;
    renderPassBeginInfo.renderArea.extent.height = frameBuffer->height;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();



    
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

    VkViewport viewport;
    VkRect2D scissor;
    viewport = vks::initializers::viewport((float)frameBuffer->width, (float)frameBuffer->height, 0.0f, 1.0f);
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    scissor = vks::initializers::rect2D(frameBuffer->width, frameBuffer->height, 0, 0);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Bind Scene Ds
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &voko_global::SceneDescriptorSet, 0 , NULL);
    // Bind Per Mesh Ds & Draw
    RenderScene();

    vkCmdEndRenderPass(cmdBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}


