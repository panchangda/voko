#include "Lighting.h"
#include "voko_globals.h"
#include "VulkanFrameBuffer.hpp"

LightingPass::LightingPass(const std::string& name, vks::VulkanDevice* inVulkanDevice, uint32_t inWidth, uint32_t inHeight,
                           ERenderPassType inPassType, EPassAttachmentType inAttachmentType,
                           // Lighting pass specials:
                           std::shared_ptr<RenderPass> ShadowPass):
RenderPass(name, inVulkanDevice, inWidth, inHeight, inPassType, inAttachmentType),
m_ShadowPass(ShadowPass)
{
    init();
}

void LightingPass::setupFrameBuffer()
{
    // Use global scene framebuffers, no need for pass setup
}

void LightingPass::setupDescriptorSet()
{
	// Declare ds layouts && pipeline layouts
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 5: Shadow map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
	};
	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

	// Shared pipeline layout used by all pipelines
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));


	// create pool for ds allocation
	std::vector<VkDescriptorPoolSize> poolSizes =
		{
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)
		};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		vks::initializers::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizes.size()),
			poolSizes.data(),
			4);

	VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

	// allocate ds from pool
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &allocInfo, &descriptorSet));

	// update ds
	
	// Image descriptors for the offscreen color attachments
	VkDescriptorImageInfo texDescriptorShadowMap =
	vks::initializers::descriptorImageInfo(
		m_ShadowPass->getFrameBuffer()->sampler,
		m_ShadowPass->getFrameBuffer()->attachments[0].view,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	writeDescriptorSets = {
		// Binding 5: Shadow map
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &texDescriptorShadowMap)
	};
	
	vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

}


void LightingPass::preparePipeline()
{
	std::string VSPath = "deferredshadows/deferred.vert.spv";
	std::string FSPath = "deferredshadows/deferred.frag.spv";
	
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, voko_global::renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Final fullscreen composition pass pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		shaderStages[0] = vks::tools::loadShader(getShaderBasePath() + VSPath, VK_SHADER_STAGE_VERTEX_BIT, vulkanDevice->logicalDevice);
		shaderStages[1] = vks::tools::loadShader(getShaderBasePath() + FSPath, VK_SHADER_STAGE_FRAGMENT_BIT, vulkanDevice->logicalDevice);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

}

void LightingPass::buildCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = voko_global::renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = voko_global::width;
    renderPassBeginInfo.renderArea.extent.height = voko_global::height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = voko_global::frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // Final composition as full screen quad
        // Note: Also used for debug display if debugDisplayTarget > 0
        vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}
