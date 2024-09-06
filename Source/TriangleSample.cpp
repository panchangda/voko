#include "voko.h"
#include "voko_globals.h"

// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
// Upon success it will return the index of the memory type that fits our requested memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties.
// You can check https://vulkan.gpuinfo.org/ for details on different memory configurations
uint32_t voko::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    // Iterate over all memory types available for the device used in this example
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    throw "Could not find a suitable memory type!";
}

void voko::createTriangleVertexBuffer()
{
    // Setup vertices
    std::vector<Vertex> vertexBuffer = {
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

    // Setup indices
    std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
    uint32_t indexBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);
    
    // Create vertex buffer
    VmaAllocation vertexBufferAllocation;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &vertices.buffer, &vertexBufferAllocation, nullptr);

    vmaCopyMemoryToAllocation(allocator, vertexBuffer.data(), vertexBufferAllocation, 0, vertexBufferSize);



    VmaAllocation indexBufferAllocation;

    bufferInfo.size = indexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &indices.buffer, &indexBufferAllocation, nullptr);

    void* data;
    // Upload index data to GPU
    vmaMapMemory(allocator, indexBufferAllocation, &data);
    memcpy(data, indexBuffer.data(), indexBufferSize);
    vmaUnmapMemory(allocator, indexBufferAllocation);
    indices.count = 3;

    // Assuming `device` and `queue` are initialized elsewhere
    // You would also need to submit a copy command if data is uploaded to a staging buffer first

    // Cleanup is handled automatically when using VMA
    // vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
    // vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
}

// Prepare vertex and index buffers for an indexed triangle
    // Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
void voko::createVertexBuffer()
{
    // A note on memory management in Vulkan in general:
    //	This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
    //	what should be done a real-world application, where you should allocate large chunks of memory at once instead.

    // Setup vertices
    std::vector<Vertex> vertexBuffer{
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

    // Setup indices
    std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
    indices.count = static_cast<uint32_t>(indexBuffer.size());
    uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    // Static data like vertex and index buffer should be stored on the device memory for optimal (and fastest) access by the GPU
    //
    // To achieve this we use so-called "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering
    //
    // Note: On unified memory architectures where host (CPU) and GPU share the same memory, staging is not necessary
    // To keep this sample easy to follow, there is no check for that in place

    struct StagingBuffer {
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    void* data;

    // Vertex buffer
    VkBufferCreateInfo vertexBufferInfoCI{};
    vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfoCI.size = vertexBufferSize;
    // Buffer is used as the copy source
    vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // Create a host-visible buffer to copy the vertex data to (staging buffer)
    VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingBuffers.vertices.buffer));
    vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // Request a host visible memory type that can be used to copy our data do
    // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
    // Map and copy
    VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
    memcpy(data, vertexBuffer.data(), vertexBufferSize);
    vkUnmapMemory(device, stagingBuffers.vertices.memory);
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

    // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
    vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
    vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

    // Index buffer
    VkBufferCreateInfo indexbufferCI{};
    indexbufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexbufferCI.size = indexBufferSize;
    indexbufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // Copy index data to a buffer visible to the host (staging buffer)
    VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &stagingBuffers.indices.buffer));
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
    VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
    memcpy(data, indexBuffer.data(), indexBufferSize);
    vkUnmapMemory(device, stagingBuffers.indices.memory);
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

    // Create destination buffer with device only visibility
    indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indices.buffer));
    vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

    // Buffer copies have to be submitted to a queue, so we need a command buffer for them
    // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
    VkCommandBuffer copyCmd;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = voko_global::commandPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    // Put buffer region copies into command buffer
    VkBufferCopy copyRegion{};
    // Vertex buffer
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
    // Index buffer
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    // Submit the command buffer to the queue to finish the copy
    VkSubmitInfo copySubmitInfo{};
    copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    copySubmitInfo.commandBufferCount = 1;
    copySubmitInfo.pCommandBuffers = &copyCmd;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = 0;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &copySubmitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, voko_global::commandPool, 1, &copyCmd);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
    vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
}

void voko::createUniformBuffers()
{

    // Vertex shader uniform buffer block
    VkBufferCreateInfo bufferCreateInfo{};

    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(uboMatrices);
    // This buffer will be used as a uniform buffer
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // Create the buffers
    for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        // VkBuffer buf;
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &uniformBuffers[i].buffer, &alloc, &allocInfo);

        // Buffer is already mapped. You can access its memory.
        memcpy(allocInfo.pMappedData, &uboMatrices, sizeof(uboMatrices));
        
        uniformBuffers[i].mapped = (uint8_t*) allocInfo.pMappedData;
    }
}

void voko::createDescriptorSetLayout()
{
    // Descriptor set layouts define the interface between our application and the shader
    // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
    // So every shader binding should map to one descriptor set layout binding

        // Binding 0: Uniform buffer (Vertex shader)
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
        descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutCI.pNext = nullptr;
        descriptorLayoutCI.bindingCount = 1;
        descriptorLayoutCI.pBindings = &layoutBinding;
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
        // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext = nullptr;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));



}

void voko::createDescriptorPool()
{
    // We need to tell the API the number of max. requested descriptors per type
    VkDescriptorPoolSize descriptorTypeCounts[1];
    // This example only one descriptor type (uniform buffer)
    descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // We have one buffer (and as such descriptor) per frame
    descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;
    // For additional types you need to add new entries in the type count list
    // E.g. for two combined image samplers :
    // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // typeCounts[1].descriptorCount = 2;

    // Create the global descriptor pool
    // All descriptors used in this example are allocated from this pool
    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.pNext = nullptr;
    descriptorPoolCI.poolSizeCount = 1;
    descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
    // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
    // Our sample will create one set per uniform buffer per frame
    descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
}

void voko::createDescriptorSets()
{
    // Allocate one descriptor set per frame from the global descriptor pool
    for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

        // Update the descriptor set determining the shader binding points
        // For every binding point used in a shader there needs to be one
        // descriptor set matching that binding point
        VkWriteDescriptorSet writeDescriptorSet{};

        // The buffer's information is passed using a descriptor info structure
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].buffer;
        bufferInfo.range = sizeof(uboMatrices);

        // Binding 0 : Uniform buffer
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &bufferInfo;
        writeDescriptorSet.dstBinding = 0;
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }
}

void voko::createPipelines()
{
    // Create the graphics pipeline used in this example
        // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
        // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
        // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
    pipelineCI.layout = pipelineLayout;
    // Renderpass this pipeline is attached to
    pipelineCI.renderPass = voko_global::renderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
    inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
    rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
    rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCI.depthClampEnable = VK_FALSE;
    rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCI.depthBiasEnable = VK_FALSE;
    rasterizationStateCI.lineWidth = 1.0f;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used)
    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments = &blendAttachmentState;

    // Viewport state sets the number of viewports and scissor used in this pipeline
    // Note: This is actually overridden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
    // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
    // For this example we will set the viewport and scissor using dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
    depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCI.depthTestEnable = VK_TRUE;
    depthStencilStateCI.depthWriteEnable = VK_TRUE;
    depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCI.stencilTestEnable = VK_FALSE;
    depthStencilStateCI.front = depthStencilStateCI.back;

    // Multi sampling state
    // This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
    VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
    multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCI.pSampleMask = nullptr;

    // Vertex input descriptions
    // Specifies the vertex input parameters for a pipeline

    // Vertex input binding
    // This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
    VkVertexInputBindingDescription vertexInputBinding{};
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(Vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Input attribute bindings describe shader attribute locations and memory layouts
    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
    // These match the following shader layout (see triangle.vert):
    //	layout (location = 0) in vec3 inPos;
    //	layout (location = 1) in vec3 inColor;
    // Attribute location 0: Position
    vertexInputAttributs[0].binding = 0;
    vertexInputAttributs[0].location = 0;
    // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    vertexInputAttributs[1].binding = 0;
    vertexInputAttributs[1].location = 1;
    // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[1].offset = offsetof(Vertex, color);

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
    vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCI.vertexBindingDescriptionCount = 1;
    vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputStateCI.vertexAttributeDescriptionCount = 2;
    vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Load binary SPIR-V shader
    shaderStages[0].module = loadSPIRVShader(getShaderBasePath() + "triangle/triangle.vert.spv");
    // Main entry point for the shader
    shaderStages[0].pName = "main";
    assert(shaderStages[0].module != VK_NULL_HANDLE);

    // Fragment shader
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Set pipeline stage for this shader
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Load binary SPIR-V shader
    shaderStages[1].module = loadSPIRVShader(getShaderBasePath() + "triangle/triangle.frag.spv");
    // Main entry point for the shader
    shaderStages[1].pName = "main";
    assert(shaderStages[1].module != VK_NULL_HANDLE);

    // Set pipeline shader stage info
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCI.pVertexInputState = &vertexInputStateCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
    pipelineCI.pRasterizationState = &rasterizationStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pMultisampleState = &multisampleStateCI;
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &depthStencilStateCI;
    pipelineCI.pDynamicState = &dynamicStateCI;

    // Create rendering pipeline using the specified states
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}
