#include "Lighting.h"
#include "voko_globals.h"
#include "VulkanFrameBuffer.hpp"

LightingPass::LightingPass(const std::string& name, vks::VulkanDevice* inVulkanDevice, uint32_t inWidth, uint32_t inHeight,
                           ERenderPassType inPassType, EPassAttachmentType inAttachmentType,
                           // Lighting pass specials:
                           std::shared_ptr<RenderPass> ShadowPass,
                           std::shared_ptr<RenderPass> GeometryPass):
RenderPass(name, inVulkanDevice, inWidth, inHeight, inPassType, inAttachmentType),
m_ShadowPass(ShadowPass), m_GeometryPass(GeometryPass)
{
	if(ShadowPass)
	{
		init();
	}else
	{
		std::cerr << "Lighing Pass Init Failed! No Valid ShadowPass Ptr" << std::endl;
	}
    
}

void LightingPass::setupFrameBuffer()
{
	frameBuffer = new vks::Framebuffer(vulkanDevice);
	frameBuffer->width = width;
	frameBuffer->height = height;

	// first pass writing to scene color:
	frameBuffer->SetSceneColorUsage(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	frameBuffer->SetDepthStencilUsage(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

	frameBuffer->createRenderPass();
}

void LightingPass::setupDescriptorSet()
{
	// Declare ds layouts && pipeline layouts
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: Position texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		// Binding 1: Normals texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		// Binding 2: Albedo texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		// Binding 3: Metallic texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		// Binding 4: Roughness texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
		// Binding 5: AO texture
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
		// Binding 6: Shadow map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6)
	};
	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

	// ds layouts: 0 for scene, 1 for lighting samplers
	std::vector<VkDescriptorSetLayout> lightingDSLayout = { voko_global::SceneDescriptorSetLayout, descriptorSetLayout };

	// pipeline layout
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(lightingDSLayout.data(), static_cast<uint32_t>(lightingDSLayout.size()));
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
			1);

	VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

	// allocate ds from pool
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &allocInfo, &descriptorSet));

	// update ds
	
	// Image descriptors for the offscreen color attachments
	VkDescriptorImageInfo texDescriptorPosition =
		vks::initializers::descriptorImageInfo(
			m_GeometryPass->getFrameBuffer()->sampler,
			m_GeometryPass->getFrameBuffer()->attachments[0].view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorNormal =
		vks::initializers::descriptorImageInfo(
			m_GeometryPass->getFrameBuffer()->sampler,
			m_GeometryPass->getFrameBuffer()->attachments[1].view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorAlbedo =
			vks::initializers::descriptorImageInfo(
				m_GeometryPass->getFrameBuffer()->sampler,
				m_GeometryPass->getFrameBuffer()->attachments[2].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorMetallic =
			vks::initializers::descriptorImageInfo(
				m_GeometryPass->getFrameBuffer()->sampler,
				m_GeometryPass->getFrameBuffer()->attachments[3].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorRoughness =
			vks::initializers::descriptorImageInfo(
				m_GeometryPass->getFrameBuffer()->sampler,
				m_GeometryPass->getFrameBuffer()->attachments[4].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorAO =
			vks::initializers::descriptorImageInfo(
				m_GeometryPass->getFrameBuffer()->sampler,
				m_GeometryPass->getFrameBuffer()->attachments[5].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorShadowMap =
	vks::initializers::descriptorImageInfo(
		m_ShadowPass->getFrameBuffer()->sampler,
		m_ShadowPass->getFrameBuffer()->attachments[0].view,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	writeDescriptorSets = {
		// Binding 0: World space position texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptorPosition),
		// Binding 1: World space normals texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorNormal),
		// Binding 2: Albedo texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorAlbedo),
		// Binding 3: Metallic texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texDescriptorMetallic),
		// Binding 4: Roughness texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &texDescriptorRoughness),
		// Binding 5: AO texture
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &texDescriptorAO),
		// Binding 6: Shadow map
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, &texDescriptorShadowMap),
	};

	vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

}


void LightingPass::preparePipeline() {
	std::string VSPath = "deferredshadows/deferred.vert.spv";
	std::string FSPath = "deferredshadows/deferred.frag.spv";

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState =
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(
		1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(
		VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(
		VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(
		dynamicStateEnables);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

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

	// Final fullscreen composition pass pipeline
	rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	shaderStages[0] = vks::tools::loadShader(getShaderBasePath() + VSPath, VK_SHADER_STAGE_VERTEX_BIT,
	                                         vulkanDevice->logicalDevice);
	shaderStages[1] = vks::tools::loadShader(getShaderBasePath() + FSPath, VK_SHADER_STAGE_FRAGMENT_BIT,
	                                         vulkanDevice->logicalDevice);
	// Empty vertex input state, vertices are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	pipelineCI.pVertexInputState = &emptyInputState;
	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
}

void LightingPass::buildCommandBuffer()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = frameBuffer->renderPass;
	renderPassBeginInfo.framebuffer = frameBuffer->framebuffer;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = frameBuffer->width;
    renderPassBeginInfo.renderArea.extent.height = frameBuffer->height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;


	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	VkViewport viewport;
	VkRect2D scissor;
	viewport = vks::initializers::viewport((float)frameBuffer->width, (float)frameBuffer->height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	scissor = vks::initializers::rect2D(frameBuffer->width, frameBuffer->height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // bind scene ds
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &voko_global::SceneDescriptorSet, 0, nullptr);
    // bind lighint pass ds
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSet, 0,
                            nullptr);

	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

    // for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    // {
    //     // Set target frame buffer
    //     renderPassBeginInfo.framebuffer = voko_global::frameBuffers[i];
    //
    //     VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
    //
    //     vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    //
    //     VkViewport viewport = vks::initializers::viewport((float)voko_global::width, (float)voko_global::height, 0.0f, 1.0f);
    //     vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
    //
    //     VkRect2D scissor = vks::initializers::rect2D(voko_global::width, voko_global::height, 0, 0);
    //     vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
    //
    // 	// bind scene ds
    //     vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &voko_global::SceneDescriptorSet, 0, nullptr);
    // 	// bind lighint pass ds
    // 	vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSet, 0, nullptr);
    //
    //     vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    //     vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
    //
    //     vkCmdEndRenderPass(drawCmdBuffers[i]);
    //
    //     VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    // }
}
