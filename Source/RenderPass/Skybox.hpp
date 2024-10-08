#pragma once

#include "RenderPass.h"
#include "voko_globals.h"
#include "VulkanFrameBuffer.hpp"


class SkyboxPass : public RenderPass
{
public:
    SkyboxPass(const std::string& name,
                        vks::VulkanDevice* inVulkanDevice,
                        uint32_t inWidth,
                        uint32_t inHeight,
                        ERenderPassType inPassType,
                        EPassAttachmentType inAttachmentType):
	RenderPass(name, inVulkanDevice, inWidth, inHeight, inPassType, inAttachmentType)
    {
	    init();
    }
	virtual void setupFrameBuffer() override {

    	frameBuffer = new vks::Framebuffer(vulkanDevice);
    	frameBuffer->width = width;
    	frameBuffer->height = height;

    	// load scene color:
    	frameBuffer->SetSceneColorUsage(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
    	frameBuffer->SetDepthStencilUsage(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    	frameBuffer->createRenderPass();

    }
	virtual void setupDescriptorSet() override {
    	std::array<VkDescriptorSetLayout ,1> skyboxDsLayouts = {voko_global::SceneDescriptorSetLayout};
    	// Layout
    	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
			skyboxDsLayouts.data(), static_cast<uint32_t>(skyboxDsLayouts.size()));
    	VK_CHECK_RESULT(
			vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    }
    virtual void preparePipeline() override
    {
    	// Shaders
    	std::string VSPath = "postprocess/skybox.vert.spv";
    	std::string FSPath = "postprocess/skybox.frag.spv";

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
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

		// fullscreen composition pass pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    	shaderStages[0] = vks::tools::loadShader(getShaderBasePath() + VSPath, VK_SHADER_STAGE_VERTEX_BIT, vulkanDevice->logicalDevice);
    	shaderStages[1] = vks::tools::loadShader(getShaderBasePath() + FSPath, VK_SHADER_STAGE_FRAGMENT_BIT, vulkanDevice->logicalDevice);
		// Empty vertex input state, vertices are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
    }

    virtual void buildCommandBuffer() override
    {
    	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    	VkClearValue clearValues[2];
    	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
    	clearValues[1].depthStencil = { 1.0f, 0 };

    	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    	renderPassBeginInfo.renderPass = frameBuffer->renderPass;
    	renderPassBeginInfo.framebuffer = frameBuffer->framebuffer;
    	renderPassBeginInfo.renderArea.extent.width = frameBuffer->width;
    	renderPassBeginInfo.renderArea.extent.height = frameBuffer->height;
    	renderPassBeginInfo.clearValueCount = 0;
    	renderPassBeginInfo.pClearValues = nullptr;

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

    	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

    	vkCmdEndRenderPass(cmdBuffer);

    	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
    }
    virtual ~SkyboxPass() override {};
};


