#pragma once

#include "RenderPass.h"
#include "voko_globals.h"
#include "VulkanFrameBuffer.hpp"


class TonePass : public RenderPass
{
public:
	VkSampler sceneColorSampler = VK_NULL_HANDLE;
	uint32_t toneAttachmentIndex = 0;
public:
    TonePass(const std::string& name,
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
    	// frameBuffer->SetSceneColorUsage(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    	// Output Color attachments
    	// Attachment 0: Scene Color After Toning
    	vks::AttachmentCreateInfo attachmentInfo = {};
    	attachmentInfo.width = frameBuffer->width;
    	attachmentInfo.height = frameBuffer->height;
    	attachmentInfo.layerCount = 1;
    	attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    	attachmentInfo.format = voko_global::sceneColor.format;

    	toneAttachmentIndex = frameBuffer->addAttachment(attachmentInfo);

    	// No need to use depth stencil
   //  	frameBuffer->SetDepthStencilUsage(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
			// VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);


    	frameBuffer->createRenderPass();

    }
	virtual void setupDescriptorSet() override {


    	// Declare ds layouts && pipeline layouts
    	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
    		// Binding 0: Position texture
    		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
    	};
    	VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

    	// ds layouts: 0 for scene, 1 for scene color samplers
    	std::array<VkDescriptorSetLayout ,2> toneDsLayouts = {voko_global::SceneDescriptorSetLayout, descriptorSetLayout};

    	// Pipeline layout
    	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
			toneDsLayouts.data(), static_cast<uint32_t>(toneDsLayouts.size()));
    	VK_CHECK_RESULT(
			vkCreatePipelineLayout(vulkanDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));




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


    	// create sampler for scene color sampling
    	VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
    	samplerInfo.magFilter = VK_FILTER_NEAREST;
    	samplerInfo.minFilter = VK_FILTER_NEAREST;
    	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	samplerInfo.mipLodBias = 0.0f;
    	samplerInfo.maxAnisotropy = 1.0f;
    	samplerInfo.minLod = 0.0f;
    	samplerInfo.maxLod = 1.0f;
    	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    	VK_CHECK_RESULT(vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sceneColorSampler));

    	// Image descriptors for color attachments
    	VkDescriptorImageInfo sceneColorTexDesc =
			vks::initializers::descriptorImageInfo(
				sceneColorSampler,
				voko_global::sceneColor.view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    	writeDescriptorSets = {
    		// Binding 0: Scene Color after toning
    		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &sceneColorTexDesc),
		};

    	vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

    }
    virtual void preparePipeline() override
    {
    	// Shaders
    	std::string VSPath = "util/fullscreen.vert.spv";
    	std::string FSPath = "postprocess/tone.frag.spv";

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
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

		// Final fullscreen composition pass pipeline
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
    	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    	clearValues[1].depthStencil = { 1.0f, 0 };

    	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    	renderPassBeginInfo.renderPass = frameBuffer->renderPass;
    	renderPassBeginInfo.framebuffer = frameBuffer->framebuffer;
    	renderPassBeginInfo.renderArea.extent.width = frameBuffer->width;
    	renderPassBeginInfo.renderArea.extent.height = frameBuffer->height;
    	renderPassBeginInfo.clearValueCount = 1;
    	renderPassBeginInfo.pClearValues = clearValues;

    	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	    vks::tools::setImageLayout(
		    cmdBuffer,
		    voko_global::sceneColor.image,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	    );


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
		// bind tone pass ds
    	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
								&descriptorSet, 0, nullptr);

    	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

    	vkCmdEndRenderPass(cmdBuffer);




    	/*
    	 * Copy attachment back to sceneColor
    	 */

    	// transfer image layouts
		vks::tools::setImageLayout(
			cmdBuffer,
			frameBuffer->attachments[toneAttachmentIndex].image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	    vks::tools::setImageLayout(
		    cmdBuffer,
		    voko_global::sceneColor.image,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    	// Copy full image
	    VkImageSubresourceLayers imageSubresource = vks::initializers::imageSubresourceLayers(
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    0,
		    0,
		    1);
	    VkImageCopy imageCopy = vks::initializers::imageCopy(
		    imageSubresource,
		    imageSubresource,
		    VkExtent3D(voko_global::sceneColor.width, voko_global::sceneColor.height, 1)
	    );
	    imageCopy.srcSubresource.aspectMask = imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	    vkCmdCopyImage(
		    cmdBuffer,
		    frameBuffer->attachments[toneAttachmentIndex].image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    voko_global::sceneColor.image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &imageCopy
	    );

    	// reset image layouts
		vks::tools::setImageLayout(
			cmdBuffer,
			frameBuffer->attachments[toneAttachmentIndex].image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	    vks::tools::setImageLayout(
		    cmdBuffer,
		    voko_global::sceneColor.image,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		    );

    	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
    }
    virtual ~TonePass() override {};
};