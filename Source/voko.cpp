#include "voko.h"
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html
// without below definition, voko.obj will have link errors: why? what's wrong with these "stb-style" single header file?
#define VMA_IMPLEMENTATION 
#include <vk_mem_alloc.h>



#include "VulkanglTFModel.h"
#include "SceneGraph/Light.h"
#include "SceneGraph/Mesh.h"
#include "Renderer/DeferredRenderer.h"

void voko::init()
{
    initSDL();

    initVulkan();

    prepare();
}

void voko::prepare()
{
    // Implemented in voko_initializer.cpp
    initSwapChain(); // different implement, using sdl to initialize swapchain' surface 
    createCommandPool();
    setupSwapChain();
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();
    

    /** Initialize: */
    // Setup a default look-at camera
    camera.type = Camera::CameraType::firstperson;
    camera.movementSpeed = 5.0f;
    camera.rotationSpeed = 0.25f;
    camera.position = { 2.15f, 0.3f, -8.75f };
    camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
    camera.setPerspective(60.0f, (float)width / (float)height, zNear, zFar);
    timerSpeed *= 0.25f;
    
    // Load Assets & Create Scene graph
    loadScene();
    collectMeshes();

    // Per Mesh SSBO
    CreatePerMeshDescriptor();
    buildMeshesSSBO();

    // Scene UB
    prepareSceneUniformBuffer();

    // Scene Renderer
    SceneRenderer = new DeferredRenderer(
        vulkanDevice,
        SceneMeshes,
        SceneDescriptorSetLayout,
        SceneDescriptorSet,
        PerMeshDescriptorSetLayout,
        PerMeshDescriptorSets,
        semaphores.presentComplete,
        semaphores.renderComplete,
        queue);
    
    prepared = true;
}


void voko::getEnabledFeatures()
{
    // Geometry shader support is required for writing to multiple shadow map layers in one single pass
    if (deviceFeatures.geometryShader) {
        enabledFeatures.geometryShader = VK_TRUE;
    }
    else {
        vks::tools::exitFatal("Selected GPU does not support geometry shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
    }
}

void voko::getEnabledExtensions(){}


// void voko::buildCommandBuffers()
// {
//     VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
//
//     // Set clear values for all framebuffer attachments with loadOp set to clear
//     // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
//     VkClearValue clearValues[2];
//     clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
//     clearValues[1].depthStencil = { 1.0f, 0 };
//
//     VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
//     renderPassBeginInfo.renderPass = renderPass;
//     renderPassBeginInfo.renderArea.offset.x = 0;
//     renderPassBeginInfo.renderArea.offset.y = 0;
//     renderPassBeginInfo.renderArea.extent.width = width;
//     renderPassBeginInfo.renderArea.extent.height = height;
//     renderPassBeginInfo.clearValueCount = 2;
//     renderPassBeginInfo.pClearValues = clearValues;
//
//
//
//     for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
//     {
//         // Set target frame buffer
//         renderPassBeginInfo.framebuffer = frameBuffers[i];
//
//         VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
//
//         vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//         VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
//         vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
//
//         VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
//         vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
//
//         vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.composition, 0, nullptr);
//
//         // Final composition as full screen quad
//         // Note: Also used for debug display if debugDisplayTarget > 0
//         vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred);
//         vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
//
//         vkCmdEndRenderPass(drawCmdBuffers[i]);
//         
//         VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
//     }
//
// }



// Vulkan loads its shaders from an immediate binary representation called SPIR-V
    // Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
    // This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule voko::loadSPIRVShader(std::string filename)
{
    size_t shaderSize;
    char* shaderCode{ nullptr };

#if defined(__ANDROID__)
    // Load shader from compressed asset
    AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
    assert(asset);
    shaderSize = AAsset_getLength(asset);
    assert(shaderSize > 0);

    shaderCode = new char[shaderSize];
    AAsset_read(asset, shaderCode, shaderSize);
    AAsset_close(asset);
#else
    std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        shaderSize = is.tellg();
        is.seekg(0, std::ios::beg);
        // Copy file contents into a buffer
        shaderCode = new char[shaderSize];
        is.read(shaderCode, shaderSize);
        is.close();
        assert(shaderSize > 0);
    }
#endif
    if (shaderCode)
    {
        // Create a new shader module that will be used for pipeline creation
        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = shaderSize;
        shaderModuleCI.pCode = (uint32_t*)shaderCode;

        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

        delete[] shaderCode;

        return shaderModule;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
        return VK_NULL_HANDLE;
    }
}

void voko::loadScene()
{
    CurrentScene = std::make_unique<Scene>("DefaultScene");
    const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;

    // Meshes: model + texture
    
    std::unique_ptr<Node> ArmorKnightMeshNode = std::make_unique<Node>(0, "ArmorKnight");;
    std::unique_ptr<Mesh> ArmorKnight = std::make_unique<Mesh>("ArmorKnight");
    ArmorKnight->VkGltfModel.loadFromFile(getAssetPath() + "models/armor/armor.gltf", vulkanDevice, queue, glTFLoadingFlags);
    ArmorKnight->Textures.ColorMap.loadFromFile(getAssetPath() + "models/armor/colormap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    ArmorKnight->Textures.NormalMap.loadFromFile(getAssetPath() + "models/armor/normalmap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    // Set per instance pos for mesh instance drawing
    ArmorKnight->MeshInstanceSSBO.push_back(PerInstanceSSBO{.pos = glm::vec4(0.0f)});
    ArmorKnight->MeshInstanceSSBO.push_back(PerInstanceSSBO{.pos = glm::vec4(-7.0f, 0.0, -4.0f, 0.0f)});
    ArmorKnight->MeshInstanceSSBO.push_back(PerInstanceSSBO{.pos = glm::vec4(4.0f, 0.0, -6.0f, 0.0f)});
    ArmorKnight->set_node(*ArmorKnightMeshNode);

    
    std::unique_ptr<Node> StoneFloor02Node = std::make_unique<Node>(0, "StoneFloor02");
    std::unique_ptr<Mesh> StoneFloor02 = std::make_unique<Mesh>("StoneFloor02");
    StoneFloor02->VkGltfModel.loadFromFile(getAssetPath() + "models/deferred_box.gltf", vulkanDevice, queue, glTFLoadingFlags);
    StoneFloor02->Textures.ColorMap.loadFromFile(getAssetPath() + "textures/stonefloor02_color_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->Textures.NormalMap.loadFromFile(getAssetPath() + "textures/stonefloor02_normal_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->set_node(*StoneFloor02Node);
    
    // components are collected & managed independently, now collected by scene
    CurrentScene->add_component(std::move(ArmorKnight));
    CurrentScene->add_component(std::move(StoneFloor02));
    CurrentScene->add_node(std::move(ArmorKnightMeshNode));
    CurrentScene->add_node(std::move(StoneFloor02Node));

    // Lights
    std::vector<glm::vec3> LightPos = {
        glm::vec3(-14.0f, -0.5f, 15.0f),
        glm::vec3(14.0f, -4.0f, 12.0f),
        glm::vec3(0.0f, -10.0f, 4.0f)
    };
    std::vector<LightProperties> LightProperties = {
        {
            .direction = glm::vec3(-2.0f, 0.0f, 0.0f),
            .color = glm::vec3(1.0f, 0.5f, 0.5f)
        },
        {
            .direction = glm::vec3(2.0f, 0.0f, 0.0f),
            .color = glm::vec3(0.0f, 0.0f, 1.0f)
        },
        {
            .direction = glm::vec3(0.0f, 0.0f, 0.0f),
            .color = glm::vec3(1.0f, 1.0f, 1.0f)
        },
    };
    
    for(size_t i = 0; i < LightPos.size();i++)
    {
        std::unique_ptr<Light> SpotLight = std::make_unique<Light>("spotLight: "+i);
        std::unique_ptr<Node> OwnerNode = std::make_unique<Node>(0, "LightNode");
        
        Transform& TF = dynamic_cast<Transform&>(OwnerNode->get_component(typeid(Transform)));
        TF.set_translation(LightPos[i]);

        SpotLight->set_node(*OwnerNode);
        SpotLight->set_properties(LightProperties[i]);
        SpotLight->set_light_type(Spot);
        
        CurrentScene->add_node(std::move(OwnerNode));
        CurrentScene->add_component(std::move(SpotLight));
    }
}

void voko::collectMeshes()
{
    SceneMeshes = CurrentScene->get_components<Mesh>();
}

void voko::buildMeshesSSBO()
{
    for(int Mesh_Index=0;Mesh_Index<SceneMeshes.size();Mesh_Index++)
    {
        auto const mesh = SceneMeshes[Mesh_Index];
        int meshInstanceCount = mesh->MeshInstanceSSBO.size();
        if(meshInstanceCount > 0)
        {
            uint32_t SSBOSize = sizeof(PerInstanceSSBO) * mesh->MeshInstanceSSBO.size();
            CreateAndUploadMeshSSBO(mesh->MeshSSBO, SSBOSize, Mesh_Index);
        }
    }
}

void voko::prepareSceneUniformBuffer()
{
    CreateSceneUniformBuffer();
    CreateSceneDescriptor();
}


void voko::windowResize()
{
    
}

void voko::render()
{
    if (!prepared) 
    	return;

    
    UpdateSceneUniformBuffer();

    draw();
}


void voko::draw()
{
    prepareFrame();

    SceneRenderer->Render();
    
    submitFrame();

}

void voko::nextFrame()
{
    auto tStart = std::chrono::high_resolution_clock::now();
    render();
    frameCounter++;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimer = (float)tDiff / 1000.0f;
    camera.update(frameTimer);
    
    // Convert to clamped timer value
    if (!paused)
    {
        timer += timerSpeed * frameTimer;
        if (timer > 1.0)
        {
            timer -= 1.0f;
        }
    }
}

void voko::prepareFrame()
{
    // Acquire the next image from the swap chain
    VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
    // SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until submitFrame() in case number of swapchain images will change on resize
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            windowResize();
        }
        return;
    }
    else {
        VK_CHECK_RESULT(result);
    }
}

void voko::submitFrame()
{

    VkResult result = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        windowResize();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        }
    }
    else {
        VK_CHECK_RESULT(result);
    }
    VK_CHECK_RESULT(vkQueueWaitIdle(queue));


    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        //windowResize();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        }
    }
    else {
        VK_CHECK_RESULT(result);
    }
    VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

void voko::renderLoop()
{

    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit)
    {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_EVENT_QUIT )
            {
                bQuit = true;
            }
                

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                bool stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_MAXIMIZED)
            {
                bool stop_rendering = false;
            }
            // Keyboard key up n Down
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                const SDL_Keycode key_code = e.key.key;
                // Esc: quit
                if (key_code == SDLK_ESCAPE)
                {
                    bQuit = true;
                }
                // Move Camera
                if (camera.type == Camera::firstperson)
                {
                    
                    switch (key_code)
                    {
                    case SDLK_W:
                        camera.keys.up = true;
                        break;
                    case SDLK_S:
                        camera.keys.down = true;
                        break;
                    case SDLK_A:
                        camera.keys.left = true;
                        break;
                    case SDLK_D:
                        camera.keys.right = true;
                        break;
                    }
                }
            }
            if(e.type == SDL_EVENT_KEY_UP)
            {
                // Move Camera
                if (camera.type == Camera::firstperson)
                {
                    switch (e.key.key)
                    {
                    case SDLK_W:
                        camera.keys.up = false;
                        break;
                    case SDLK_S:
                        camera.keys.down = false;
                        break;
                    case SDLK_A:
                        camera.keys.left = false;
                        break;
                    case SDLK_D:
                        camera.keys.right = false;
                        break;
                    }
                }
            }

            // Mouse Button Up n Down
            if(e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                switch(e.button.button)
                {
					case SDL_BUTTON_LEFT:
                        mouseButtons.left = true;

                    case SDL_BUTTON_RIGHT:
                        mouseButtons.right = true;

                    case SDL_BUTTON_MIDDLE:
                        mouseButtons.middle = true;
                }
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                switch (e.button.button)
                {
                case SDL_BUTTON_LEFT:
                    mouseButtons.left = true;

                case SDL_BUTTON_RIGHT:
                    mouseButtons.right = true;

                case SDL_BUTTON_MIDDLE:
                    mouseButtons.middle = true;
                }
            }
            if(e.type == SDL_EVENT_MOUSE_MOTION)
            {
                float dy = e.motion.yrel;
                float dx = e.motion.xrel;
                if (mouseButtons.left) {
                    camera.rotate(glm::vec3( dy* camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));

                }
            }

        }

        if (prepared) {
            nextFrame();
        }
    }
}

VkPipelineShaderStageCreateInfo voko::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
    shaderStage.module = vks::tools::loadShader(fileName.c_str(), device);
#endif
    shaderStage.pName = "main";
    assert(shaderStage.module != VK_NULL_HANDLE);
    shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

void voko::CreateSceneDescriptor()
{
    // Scene pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &SceneDescriptorPool));

    
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: Scene Uniform Buffer: view info, light info...
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 0)
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &SceneDescriptorSetLayout));

    // Sets
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(SceneDescriptorPool, &SceneDescriptorSetLayout, 1);
    
    // mapping 
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &SceneDescriptorSet));
    writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(SceneDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &SceneUB.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void voko::CreateSceneUniformBuffer()
{
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
     &SceneUB, sizeof(uniformBufferScene)));


    // Map persistent
    VK_CHECK_RESULT(SceneUB.map());
}

void voko::UpdateSceneUniformBuffer()
{
    
    // Update view (camera)
    uniformBufferScene.projectionMatrix = camera.matrices.perspective;
    uniformBufferScene.viewMatrix = camera.matrices.view;

    // why revert x&z? 
    uniformBufferScene.cameraPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);;
    memcpy(SceneUB.mapped, &uniformBufferScene, sizeof(uniformBufferScene));


    // Update lights
    // Animate
    uniformBufferScene.lights[0].position.x = -14.0f + std::abs(sin(glm::radians(timer * 360.0f)) * 20.0f);
    uniformBufferScene.lights[0].position.z = 15.0f + cos(glm::radians(timer *360.0f)) * 1.0f;
    
    uniformBufferScene.lights[1].position.x = 14.0f - std::abs(sin(glm::radians(timer * 360.0f)) * 2.5f);
    uniformBufferScene.lights[1].position.z = 13.0f + cos(glm::radians(timer *360.0f)) * 4.0f;
    
    uniformBufferScene.lights[2].position.x = 0.0f + sin(glm::radians(timer *360.0f)) * 4.0f;
    uniformBufferScene.lights[2].position.z = 4.0f + cos(glm::radians(timer *360.0f)) * 2.0f;
    
    for (int i = 0; i < LIGHT_COUNT; i++) {
        // mvp from light's pov (for shadows)
        glm::mat4 shadowProj = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
        glm::mat4 shadowView = glm::lookAt(glm::vec3(uniformBufferScene.lights[i].position), glm::vec3(uniformBufferScene.lights[i].target), glm::vec3(0.0f, 1.0f, 0.0f));
    
        uniformBufferScene.lights[i].viewMatrix = shadowProj * shadowView;
    }
    memcpy(SceneUB.mapped, &uniformBufferScene, sizeof(uniformBufferScene));
}

void voko::CreatePerMeshDescriptor()
{
    // Create Per Mesh SSBO Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MESH_MAX),
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, MESH_MAX);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &PerMeshDescriptorPool));

    // Declare DescriptorSet Layout    
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: Mesh Instance Buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0)
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &PerMeshDescriptorSetLayout));


    PerMeshDescriptorSets.resize(MESH_MAX);
    for(int Mesh_Index = 0; Mesh_Index < MESH_MAX; Mesh_Index++)
    {
        // pre allocate ds for meshes
        VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(PerMeshDescriptorPool, &PerMeshDescriptorSetLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &PerMeshDescriptorSets[Mesh_Index]));
    }
}

void voko::CreateAndUploadMeshSSBO(vks::Buffer& MeshSSBO, uint32_t MeshSSBOSize, uint32_t MeshIndex)
{
    // Create buffer
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &MeshSSBO, MeshSSBOSize));

    // Map persistent
    VK_CHECK_RESULT(MeshSSBO.map());
    
    // Update corresponding pre-allocated sets
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    writeDescriptorSets = {
        // Binding 0: Mesh Instance Buffer
        vks::initializers::writeDescriptorSet(PerMeshDescriptorSets[MeshIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &MeshSSBO.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    
}


// void voko::setupDescriptors()
// {
//     // Pool
// 		std::vector<VkDescriptorPoolSize> poolSizes = {
// 			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12),
// 			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)
// 		};
// 		VkDescriptorPoolCreateInfo descriptorPoolInfo =vks::initializers::descriptorPoolCreateInfo(poolSizes, 4);
// 		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
//
// 		// Layout
// 		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
// 			// Binding 0: Vertex shader uniform buffer
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0),
// 			// Binding 1: Position texture
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
// 			// Binding 2: Normals texture
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
// 			// Binding 3: Albedo texture
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
// 			// Binding 4: Fragment shader uniform buffer
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
// 			// Binding 5: Shadow map
// 			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
// 		};
// 		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
// 		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
//
// 		// Sets
// 		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
// 		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
//
// 		// Image descriptors for the offscreen color attachments
// 		VkDescriptorImageInfo texDescriptorPosition =
// 			vks::initializers::descriptorImageInfo(
// 				deferredFrameBuffer->sampler,
// 				deferredFrameBuffer->attachments[0].view,
// 				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
// 		VkDescriptorImageInfo texDescriptorNormal =
// 			vks::initializers::descriptorImageInfo(
// 				deferredFrameBuffer->sampler,
// 				deferredFrameBuffer->attachments[1].view,
// 				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
// 		VkDescriptorImageInfo texDescriptorAlbedo =
// 			vks::initializers::descriptorImageInfo(
// 				deferredFrameBuffer->sampler,
// 				deferredFrameBuffer->attachments[2].view,
// 				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
// 		VkDescriptorImageInfo texDescriptorShadowMap =
// 			vks::initializers::descriptorImageInfo(
// 				shadowFrameBuffer->sampler,
// 				shadowFrameBuffer->attachments[0].view,
// 				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
//
// 		// Deferred composition
// 		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.composition));
// 		writeDescriptorSets = {
// 			// Binding 1: World space position texture
// 			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorPosition),
// 			// Binding 2: World space normals texture
// 			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorNormal),
// 			// Binding 3: Albedo texture
// 			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texDescriptorAlbedo),
// 			// Binding 4: Fragment shader uniform buffer
// 			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &compositionUB.descriptor),
// 			// Binding 5: Shadow map
// 			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &texDescriptorShadowMap),
// 		};
// 		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//
// 		// Offscreen (scene)
//
// 		// Model
// 		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.model));
// 		writeDescriptorSets = {
// 			// Binding 0: Vertex shader uniform bfuffer
// 			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &offscreenUB.descriptor),
// 			// Binding 1: Color map
// 			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.model.colorMap.descriptor),
// 			// Binding 2: Normal map
// 			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.model.normalMap.descriptor)
// 		};
// 		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//
// 		// Background
// 		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.background));
// 		writeDescriptorSets = {
// 			// Binding 0: Vertex shader uniform buffer
// 			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &offscreenUB.descriptor),
// 			// Binding 1: Color map
// 			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.background.colorMap.descriptor),
// 			// Binding 2: Normal map
// 			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.background.normalMap.descriptor)
// 		};
// 		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//
// 		// Shadow mapping
// 		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadow));
// 		writeDescriptorSets = {
// 			// Binding 0: Vertex shader uniform buffer
// 			vks::initializers::writeDescriptorSet(descriptorSets.shadow, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shadowGeometryShaderUB.descriptor),
// 		};
// 		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
// }
//
// void voko::preparePipelines()
// {
//     // Layout
//     VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
//         &descriptorSetLayout, 1);
//     VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
//
//     // Pipelines
//     VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(
//         VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
//     VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
//         VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
//     VkPipelineColorBlendAttachmentState blendAttachmentState =
//         vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
//     VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(
//         1, &blendAttachmentState);
//     VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(
//         VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
//     VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
//     VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(
//         VK_SAMPLE_COUNT_1_BIT, 0);
//     std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
//     VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(
//         dynamicStateEnables);
//     std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
//     
//     VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
//     pipelineCI.pInputAssemblyState = &inputAssemblyState;
//     pipelineCI.pRasterizationState = &rasterizationState;
//     pipelineCI.pColorBlendState = &colorBlendState;
//     pipelineCI.pMultisampleState = &multisampleState;
//     pipelineCI.pViewportState = &viewportState;
//     pipelineCI.pDepthStencilState = &depthStencilState;
//     pipelineCI.pDynamicState = &dynamicState;
//     pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
//     pipelineCI.pStages = shaderStages.data();
//
//     // Final fullscreen composition pass pipeline
//     rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
//     shaderStages[0] = loadShader((getShaderBasePath() + "deferredshadows/deferred.vert.spv").c_str(),
//                                  VK_SHADER_STAGE_VERTEX_BIT);
//     shaderStages[1] = loadShader((getShaderBasePath() + "deferredshadows/deferred.frag.spv").c_str(),
//                                  VK_SHADER_STAGE_FRAGMENT_BIT);
//     // Empty vertex input state, vertices are generated by the vertex shader
//     VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
//     pipelineCI.pVertexInputState = &emptyInputState;
//     VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.deferred));
//
//     // Vertex input state from glTF model for pipeline rendering models
//     pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({
//         vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color,
//         vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Tangent
//     });
//     rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
//
//     // Offscreen pipeline
//     // Separate render pass
//     pipelineCI.renderPass = deferredFrameBuffer->renderPass;
//
//     // Blend attachment states required for all color attachments
//     // This is important, as color write mask will otherwise be 0x0 and you
//     // won't see anything rendered to the attachment
//     std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates =
//     {
//         vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
//         vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
//         vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
//     };
//     colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
//     colorBlendState.pAttachments = blendAttachmentStates.data();
//
//     shaderStages[0] = loadShader(getShaderBasePath() + "deferredshadows/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
//     shaderStages[1] = loadShader(getShaderBasePath() + "deferredshadows/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
//     VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));
//
//     // Shadow mapping pipeline
//     // The shadow mapping pipeline uses geometry shader instancing (invocations layout modifier) to output
//     // shadow maps for multiple lights sources into the different shadow map layers in one single render pass
//     std::array<VkPipelineShaderStageCreateInfo, 2> shadowStages;
//     shadowStages[0] = loadShader(getShaderBasePath() + "deferredshadows/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
//     shadowStages[1] = loadShader(getShaderBasePath() + "deferredshadows/shadow.geom.spv",
//                                  VK_SHADER_STAGE_GEOMETRY_BIT);
//
//     pipelineCI.pStages = shadowStages.data();
//     pipelineCI.stageCount = static_cast<uint32_t>(shadowStages.size());
//
//     // Shadow pass doesn't use any color attachments
//     colorBlendState.attachmentCount = 0;
//     colorBlendState.pAttachments = nullptr;
//     // Cull front faces
//     rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
//     depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
//     // Enable depth bias
//     rasterizationState.depthBiasEnable = VK_TRUE;
//     // Add depth bias to dynamic state, so we can change it at runtime
//     dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
//     dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
//     // Reset blend attachment state
//     pipelineCI.renderPass = shadowFrameBuffer->renderPass;
//     VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.shadowpass));
// }

// void voko::buildDeferredCommandBuffer()
// {
//     if (offScreenCmdBuffer == VK_NULL_HANDLE) {
// 			offScreenCmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
// 		}
//
// 		// Create a semaphore used to synchronize offscreen rendering and usage
// 		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
// 		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenSemaphore));
//
// 		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
//
// 		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
// 		std::array<VkClearValue, 4> clearValues = {};
// 		VkViewport viewport;
// 		VkRect2D scissor;
//
// 		// First pass: Shadow map generation
// 		// -------------------------------------------------------------------------------------------------------
//
// 		clearValues[0].depthStencil = { 1.0f, 0 };
//
// 		renderPassBeginInfo.renderPass = shadowFrameBuffer->renderPass;
// 		renderPassBeginInfo.framebuffer = shadowFrameBuffer->framebuffer;
// 		renderPassBeginInfo.renderArea.extent.width = shadowFrameBuffer->width;
// 		renderPassBeginInfo.renderArea.extent.height = shadowFrameBuffer->height;
// 		renderPassBeginInfo.clearValueCount = 1;
// 		renderPassBeginInfo.pClearValues = clearValues.data();
//
// 		VK_CHECK_RESULT(vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo));
//
// 		viewport = vks::initializers::viewport((float)shadowFrameBuffer->width, (float)shadowFrameBuffer->height, 0.0f, 1.0f);
// 		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);
//
// 		scissor = vks::initializers::rect2D(shadowFrameBuffer->width, shadowFrameBuffer->height, 0, 0);
// 		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);
//
// 		// Set depth bias (aka "Polygon offset")
// 		vkCmdSetDepthBias(
// 			offScreenCmdBuffer,
// 			depthBiasConstant,
// 			0.0f,
// 			depthBiasSlope);
//
// 		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
// 		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowpass);
// 		renderScene(offScreenCmdBuffer, true);
// 		vkCmdEndRenderPass(offScreenCmdBuffer);
//
// 		// Second pass: Deferred calculations
// 		// -------------------------------------------------------------------------------------------------------
//
// 		// Clear values for all attachments written in the fragment shader
// 		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
// 		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
// 		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
// 		clearValues[3].depthStencil = { 1.0f, 0 };
//
// 		renderPassBeginInfo.renderPass = deferredFrameBuffer->renderPass;
// 		renderPassBeginInfo.framebuffer = deferredFrameBuffer->framebuffer;
// 		renderPassBeginInfo.renderArea.extent.width = deferredFrameBuffer->width;
// 		renderPassBeginInfo.renderArea.extent.height = deferredFrameBuffer->height;
// 		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
// 		renderPassBeginInfo.pClearValues = clearValues.data();
//
// 		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
// 		viewport = vks::initializers::viewport((float)deferredFrameBuffer->width, (float)deferredFrameBuffer->height, 0.0f, 1.0f);
// 		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);
//
// 		scissor = vks::initializers::rect2D(deferredFrameBuffer->width, deferredFrameBuffer->height, 0, 0);
// 		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);
//
// 		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
// 		renderScene(offScreenCmdBuffer, false);
// 		vkCmdEndRenderPass(offScreenCmdBuffer);
//
// 		VK_CHECK_RESULT(vkEndCommandBuffer(offScreenCmdBuffer));
// }



// Update deferred composition fragment shader light position and parameters uniform block

// void voko::UpdateUniformBufferDeferred()
// {
//     // Animate
//     uniformDataComposition.lights[0].position.x = -14.0f + std::abs(sin(glm::radians(timer * 360.0f)) * 20.0f);
//     uniformDataComposition.lights[0].position.z = 15.0f + cos(glm::radians(timer *360.0f)) * 1.0f;
//
//     uniformDataComposition.lights[1].position.x = 14.0f - std::abs(sin(glm::radians(timer * 360.0f)) * 2.5f);
//     uniformDataComposition.lights[1].position.z = 13.0f + cos(glm::radians(timer *360.0f)) * 4.0f;
//
//     uniformDataComposition.lights[2].position.x = 0.0f + sin(glm::radians(timer *360.0f)) * 4.0f;
//     uniformDataComposition.lights[2].position.z = 4.0f + cos(glm::radians(timer *360.0f)) * 2.0f;
//
//     for (uint32_t i = 0; i < LIGHT_COUNT; i++) {
//         // mvp from light's pov (for shadows)
//         glm::mat4 shadowProj = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
//         glm::mat4 shadowView = glm::lookAt(glm::vec3(uniformDataComposition.lights[i].position), glm::vec3(uniformDataComposition.lights[i].target), glm::vec3(0.0f, 1.0f, 0.0f));
//         glm::mat4 shadowModel = glm::mat4(1.0f);
//
//         uniformDataShadows.mvp[i] = shadowProj * shadowView * shadowModel;
//         uniformDataComposition.lights[i].viewMatrix = uniformDataShadows.mvp[i];
//     }
//
//     memcpy(uniformDataShadows.instancePos, uniformDataOffscreen.instancePos, sizeof(UniformDataOffscreen::instancePos));
//     memcpy(shadowGeometryShaderUB.mapped, &uniformDataShadows, sizeof(UniformDataShadows));
//
//     uniformDataComposition.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);;
//     uniformDataComposition.debugDisplayTarget = debugDisplayTarget;
//
//     memcpy(compositionUB.mapped, &uniformDataComposition, sizeof(uniformDataComposition));
// }
//
// void voko::updateUniformBufferOffscreen()
// {
//     uniformDataOffscreen.projection = camera.matrices.perspective;
//     uniformDataOffscreen.view = camera.matrices.view;
//     uniformDataOffscreen.model = glm::mat4(1.0f);
//     memcpy(offscreenUB.mapped, &uniformDataOffscreen, sizeof(uniformDataOffscreen));
// }

voko::~voko()
{
	if(SDLWindow)
	{
        SDL_DestroyWindow(SDLWindow);
	}

    // Clean up Vulkan resources
    swapChain.cleanup();
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    
}
