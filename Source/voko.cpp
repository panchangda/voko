#include "voko.h"
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html
// without below definition, voko.obj will have link errors: why? what's wrong with these "stb-style" single header file?
#define VMA_IMPLEMENTATION 
#include <vk_mem_alloc.h>

#include "VulkanglTFModel.h"
#include "SceneGraph/Light.h"
#include "SceneGraph/Mesh.h"
#include "Renderer/DeferredRenderer.h"
#include "SceneGraph/SpotLight.h"

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

    
    // std::cout << "device minStorageBufferOffsetAlignment: " << deviceProperties.limits.minStorageBufferOffsetAlignment << std::endl;
    // std::cout << sizeof(PerInstanceSSBO) << std::endl;

    // std::cout << "offset of lighting struct : " << offsetof(voko_buffer::UniformBufferScene, lighting) << std::endl;
    // std::cout << "offset of debug struct: " << offsetof(voko_buffer::UniformBufferScene, debug) << std::endl;

    

    /** Initialize: */
    // Setup a default look-at camera
    camera.type = Camera::CameraType::firstperson;
    camera.movementSpeed = 5.0f;
    camera.rotationSpeed = 0.25f;
    camera.position = { 2.15f, 0.3f, -8.75f };
    camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
    camera.setPerspective(60.0f, (float)voko_global::width / (float)voko_global::height, zNear, zFar);
    timerSpeed *= 0.25f;
    
    // Load Assets & Create Scene graph
    // loadScene();
    loadScene2();
    buildScene();

    // Scene Renderer
    SceneRenderer = new DeferredRenderer(
        vulkanDevice,
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

    if(deviceFeatures.tessellationShader)
    {
        enabledFeatures.tessellationShader = VK_TRUE;
    }else
    {
        vks::tools::exitFatal("Selected GPU does not support tesselation shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
    }

    if (deviceFeatures.samplerAnisotropy) {
        enabledFeatures.samplerAnisotropy = VK_TRUE;
    }else {
        vks::tools::exitFatal("Selected GPU does not support samplerAnisotropy!", VK_ERROR_FEATURE_NOT_PRESENT);
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
    CurrentScene = std::make_unique<Scene>("Scene1: Deferred + Shadow");
    const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;

    // Meshes: model + texture
    
    std::unique_ptr<Node> ArmorKnightMeshNode = std::make_unique<Node>(0, "ArmorKnight");;
    std::unique_ptr<Mesh> ArmorKnight = std::make_unique<Mesh>("ArmorKnight");
    ArmorKnight->VkGltfModel.loadFromFile(getAssetPath() + "models/armor/armor.gltf", vulkanDevice, queue, glTFLoadingFlags);
    ArmorKnight->Textures.albedoMap.loadFromFile(getAssetPath() + "models/armor/colormap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    ArmorKnight->Textures.normalMap.loadFromFile(getAssetPath() + "models/armor/normalmap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    // Set per instance pos for mesh instance drawing
    ArmorKnight->Instances.push_back(voko_buffer::PerInstanceSSBO{.instancePos = glm::vec4(0.0f)});
    ArmorKnight->Instances.push_back(voko_buffer::PerInstanceSSBO{.instancePos = glm::vec4(-7.0f, 0.0, -4.0f, 0.0f)});
    ArmorKnight->Instances.push_back(voko_buffer::PerInstanceSSBO{.instancePos = glm::vec4(4.0f, 0.0, -6.0f, 0.0f)});
    ArmorKnight->set_node(*ArmorKnightMeshNode);

    
    std::unique_ptr<Node> StoneFloor02Node = std::make_unique<Node>(0, "StoneFloor02");
    std::unique_ptr<Mesh> StoneFloor02 = std::make_unique<Mesh>("StoneFloor02");
    StoneFloor02->VkGltfModel.loadFromFile(getAssetPath() + "models/deferred_box.gltf", vulkanDevice, queue, glTFLoadingFlags);
    StoneFloor02->Textures.albedoMap.loadFromFile(getAssetPath() + "textures/stonefloor02_color_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->Textures.normalMap.loadFromFile(getAssetPath() + "textures/stonefloor02_normal_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->set_node(*StoneFloor02Node);
    
    // components are collected & managed independently, now collected by scene
    CurrentScene->add_component(std::move(ArmorKnight));
    CurrentScene->add_component(std::move(StoneFloor02));
    CurrentScene->add_node(std::move(ArmorKnightMeshNode));
    CurrentScene->add_node(std::move(StoneFloor02Node));

    // Lights
    const std::vector<glm::vec3> spotLightPos = {
        glm::vec3(-14.0f, -0.5f, 15.0f),
        glm::vec3(14.0f, -4.0f, 12.0f),
        glm::vec3(0.0f, -10.0f, 4.0f)
    };

    const std::vector<LightProperties> spotLightProperties = {
        LightProperties(
            std::in_place_type<voko_buffer::SpotLight>,
            glm::vec4(-14.0f, -0.5f, 15.0f, 1.0f), // position
            glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f), // target
            glm::vec4(1.0f, 0.5f, 0.5f, 0.0f), // color
            glm::mat4(1.0f), // viewMatrix
            100.0f, // range
            std::cos(glm::radians(15.0f)), // lightCosInnerAngle
            std::cos(glm::radians(25.0f)) // lightCosOuterAngle
        ),
        LightProperties(
            std::in_place_type<voko_buffer::SpotLight>,
            glm::vec4(14.0f, -4.0f, 12.0f, 1.0f), // position
            glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), // target
            glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), // color
            glm::mat4(1.0f), // viewMatrix
            100.0f, // range
            std::cos(glm::radians(15.0f)), // lightCosInnerAngle
            std::cos(glm::radians(25.0f)) // lightCosOuterAngle
        ),
        LightProperties(
            std::in_place_type<voko_buffer::SpotLight>,
            glm::vec4(0.0f, -10.0f, 4.0f, 1.0f), // position
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), // target
            glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), // color
            glm::mat4(1.0f), // viewMatrix
            100.0f, // range
            std::cos(glm::radians(15.0f)), // lightCosInnerAngle
            std::cos(glm::radians(25.0f)) // lightCosOuterAngle
        )
    };
    
    for(uint32_t i = 0; i < spotLightProperties.size();i++)
    {
        std::unique_ptr<SpotLight> spotLight = std::make_unique<SpotLight>("SpotLight: " + i);
        std::unique_ptr<Node> OwnerNode = std::make_unique<Node>(0, "LightNode");
        
        Transform& TF = dynamic_cast<Transform&>(OwnerNode->get_component(typeid(Transform)));
        TF.set_translation(spotLightPos[i]);

        spotLight->set_node(*OwnerNode);
        spotLight->set_properties(spotLightProperties[i]);
        
        CurrentScene->add_node(std::move(OwnerNode));
        CurrentScene->add_component(std::move(spotLight));
    }
}

void voko::loadScene2() {
    CurrentScene = std::make_unique<Scene>("Scene2: PBR Texture + IBL");
    const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;

    // cerberus mesh + pbr textures
    std::unique_ptr<Node> cerberusNode = std::make_unique<Node>(0, "cerberus");;
    std::unique_ptr<Mesh> cerberus = std::make_unique<Mesh>("cerberus");
    cerberus->VkGltfModel.loadFromFile(getAssetPath() + "models/cerberus/cerberus.gltf", vulkanDevice, queue, glTFLoadingFlags);
    cerberus->Textures.albedoMap.loadFromFile(getAssetPath() + "models/cerberus/albedo.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    cerberus->Textures.normalMap.loadFromFile(getAssetPath() + "models/cerberus/normal.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    cerberus->Textures.aoMap.loadFromFile(getAssetPath() + "models/cerberus/ao.ktx", VK_FORMAT_R8_UNORM, vulkanDevice, queue);
    cerberus->Textures.metallicMap.loadFromFile(getAssetPath() + "models/cerberus/metallic.ktx", VK_FORMAT_R8_UNORM, vulkanDevice, queue);
    cerberus->Textures.roughnessMap.loadFromFile(getAssetPath() + "models/cerberus/roughness.ktx", VK_FORMAT_R8_UNORM, vulkanDevice, queue);
    cerberus->set_node(*cerberusNode);
    // components are collected & managed independently, now collected by scene
    CurrentScene->add_component(std::move(cerberus));
    CurrentScene->add_node(std::move(cerberusNode));

    std::unique_ptr<Node> StoneFloor02Node = std::make_unique<Node>(0, "StoneFloor02");
    std::unique_ptr<Mesh> StoneFloor02 = std::make_unique<Mesh>("StoneFloor02");
    StoneFloor02->VkGltfModel.loadFromFile(getAssetPath() + "models/deferred_box.gltf", vulkanDevice, queue, glTFLoadingFlags);
    StoneFloor02->Textures.albedoMap.loadFromFile(getAssetPath() + "textures/stonefloor02_color_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->Textures.normalMap.loadFromFile(getAssetPath() + "textures/stonefloor02_normal_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    StoneFloor02->set_node(*StoneFloor02Node);
    CurrentScene->add_component(std::move(StoneFloor02));
    CurrentScene->add_node(std::move(StoneFloor02Node));



    // IBL: cubes & env cube map
    // enable IBL
    bComputeIBL = true;
    // cube mesh
    skybox.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
    // environment cube map
    iblTextures.environmentCube.loadFromFile(getAssetPath() + "textures/hdr/gcanyon_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, vulkanDevice, queue);
}

void voko::buildScene() {
    // Precompute IBL
    if(bComputeIBL) {
        generateBRDFLUT();
        generateIrradianceCube();
        generatePrefilteredCube();
        uniformBufferLighting.skybox = 1;
        bComputeIBL = false;
    }

    buildMeshes();

    buildLights();

    CreateSceneUniformBuffer();
    CreateSceneDescriptor();
}

void voko::buildMeshes()
{
    CreatePerMeshDescriptor();

    voko_global::SceneMeshes = CurrentScene->get_components<Mesh>();

    for(int Mesh_Index=0;Mesh_Index< voko_global::SceneMeshes.size();Mesh_Index++)
    {
        CreateAndUploadPerMeshBuffer(voko_global::SceneMeshes[Mesh_Index], Mesh_Index);
    }
}

void voko::buildLights() {
    auto spotLights = CurrentScene->get_components<Light>();

    voko_global::SPOT_LIGHT_COUNT = spotLights.size();

    for(uint32_t i = 0; i < spotLights.size();i++) {
        const auto spotLight = spotLights[i];
        uniformBufferLighting.spotLights[i] = std::get<voko_buffer::SpotLight>(spotLight->get_properties());
    }

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

    float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
    if (fpsTimer > 1000.0f)
    {
        lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
        frameCounter = 0;
        lastTimestamp = tEnd;
    }
}

void voko::prepareFrame()
{
    // Acquire the next image from the swap chain
    VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, &voko_global::currentBuffer);
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

    VkResult result = swapChain.queuePresent(queue, voko_global::currentBuffer, semaphores.renderComplete);
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
    bool bQuit = false;

    // main loop
    while (!bQuit)
    {
        processInput(bQuit);

        if (prepared) {
            nextFrame();
        }
    }
}
void voko::processInput(bool& bQuit) {
    SDL_Event e;
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

            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                switch (e.button.button)
                {
                case SDL_BUTTON_LEFT:
                    mouseButtons.left = false;

                case SDL_BUTTON_RIGHT:
                    mouseButtons.right = false;

                case SDL_BUTTON_MIDDLE:
                    mouseButtons.middle = false;
                }
            }
            if(e.type == SDL_EVENT_MOUSE_MOTION)
            {
                float dy = e.motion.yrel;
                float dx = e.motion.xrel;
                if (mouseButtons.left) {
                    camera.rotate(glm::vec3( -dy* camera.rotationSpeed, dx * camera.rotationSpeed, 0.0f));
                }
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
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &voko_global::SceneDescriptorSetLayout));

    // Sets
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(SceneDescriptorPool, &voko_global::SceneDescriptorSetLayout, 1);
    
    // mapping 
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &voko_global::SceneDescriptorSet));
    writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(voko_global::SceneDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &SceneUB.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void voko::CreateSceneUniformBuffer()
{
    // std::cout << "Scene UB Size: "<< sizeof(uniformBufferScene) << std::endl;

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
     &SceneUB, sizeof(uniformBufferScene)));


    // Map persistent
    VK_CHECK_RESULT(SceneUB.map());
}

void voko::UpdateSceneUniformBuffer()
{
    // Update view (camera)
    uniformBufferView.projectionMatrix = camera.matrices.perspective;
    uniformBufferView.viewMatrix = camera.matrices.view;

    // why revert x&z? 
    uniformBufferView.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);;


    // Update spot lights
    // Animate
    uniformBufferLighting.spotLights[0].position.x = -14.0f + std::abs(sin(glm::radians(timer * 360.0f)) * 20.0f);
    uniformBufferLighting.spotLights[0].position.z = 15.0f + cos(glm::radians(timer *360.0f)) * 1.0f;
    
    uniformBufferLighting.spotLights[1].position.x = 14.0f - std::abs(sin(glm::radians(timer * 360.0f)) * 2.5f);
    uniformBufferLighting.spotLights[1].position.z = 13.0f + cos(glm::radians(timer *360.0f)) * 4.0f;
    
    uniformBufferLighting.spotLights[2].position.x = 0.0f + sin(glm::radians(timer *360.0f)) * 4.0f;
    uniformBufferLighting.spotLights[2].position.z = 4.0f + cos(glm::radians(timer *360.0f)) * 2.0f;
    
    for (int i = 0; i < voko_global::SPOT_LIGHT_COUNT; i++) {
        // mvp from light's pov (for shadows)
        glm::mat4 shadowProj = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
        glm::mat4 shadowView = glm::lookAt(glm::vec3(uniformBufferLighting.spotLights[i].position), glm::vec3(uniformBufferLighting.spotLights[i].target), glm::vec3(0.0f, 1.0f, 0.0f));
    
        uniformBufferLighting.spotLights[i].viewMatrix = shadowProj * shadowView;
    }
    memcpy(SceneUB.mapped, &uniformBufferScene, sizeof(uniformBufferScene));
}

void voko::CreatePerMeshDescriptor()
{
    // Create Per Mesh SSBO Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        // 1 for meshPropsSSBO, 1 for meshInstanceSSBO
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, voko_global::MESH_MAX * 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, voko_global::MESH_MAX * voko_global::MESH_SAMPLER_MAX)
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, voko_global::MESH_MAX);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &PerMeshDescriptorPool));

    // Declare DescriptorSet Layout    
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: Mesh Props SSBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 0),
        // Binding 1: Mesh Instance SSBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1),
        // Binding 2: Mesh Albedo map
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        // Binding 3: Mesh Normal map
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
        // Binding 4: Mesh AO map
       vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
        // Binding 5: Mesh Metallic map
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        // Binding 6: Mesh Roughness map
       vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &voko_global::PerMeshDescriptorSetLayout));


    voko_global::PerMeshDescriptorSets.resize(voko_global::MESH_MAX);
    for(int Mesh_Index = 0; Mesh_Index < voko_global::MESH_MAX; Mesh_Index++)
    {
        // pre allocate ds for meshes
        VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(PerMeshDescriptorPool, &voko_global::PerMeshDescriptorSetLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &voko_global::PerMeshDescriptorSets[Mesh_Index]));
    }
}

void voko::CreateAndUploadPerMeshBuffer(Mesh* mesh, uint32_t MeshIndex) const
{
    // 0: Mesh Prop
    vks::Buffer& meshPropSSBO = mesh->meshPropSSBO;
    uint32_t meshPropsSSBOSize = sizeof(voko_buffer::MeshProperty);
    // 1: Mesh Instance
    vks::Buffer& instanceSSBO = mesh->instanceSSBO;
    uint32_t instanceSSBOSize = sizeof(voko_buffer::PerInstanceSSBO) * mesh->Instances.size();
    // 3-4: Samplers
	vks::Texture2D& colorMap = mesh->Textures.albedoMap;
    vks::Texture2D& normalMap = mesh->Textures.normalMap;

    // Create -> map -> upload mesh ssbo
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &meshPropSSBO, meshPropsSSBOSize));
    VK_CHECK_RESULT(meshPropSSBO.map());
    glm::mat4 meshModelMatrix = mesh->get_node()->get_transform().get_matrix();
    memcpy(meshPropSSBO.mapped, 
        &meshModelMatrix, 
        meshPropsSSBOSize);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = 
    {
        // Binding 0: Mesh Prop Buffer
        vks::initializers::writeDescriptorSet(voko_global::PerMeshDescriptorSets[MeshIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &meshPropSSBO.descriptor),
        // Binding 2: Mesh Color map
        vks::initializers::writeDescriptorSet(voko_global::PerMeshDescriptorSets[MeshIndex], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &colorMap.descriptor),
        // Binding 3: Mesh Normal map
        vks::initializers::writeDescriptorSet(voko_global::PerMeshDescriptorSets[MeshIndex], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &normalMap.descriptor),
    };

    if (instanceSSBOSize != 0) {
        // Create -> map -> upload instance ssbo
        VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &instanceSSBO, instanceSSBOSize));
        VK_CHECK_RESULT(instanceSSBO.map());
        memcpy(instanceSSBO.mapped,
            mesh->Instances.data(),
            instanceSSBOSize);

    	// Binding 1: Mesh Instance Buffer
        writeDescriptorSets.emplace_back(
            vks::initializers::writeDescriptorSet(voko_global::PerMeshDescriptorSets[MeshIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &instanceSSBO.descriptor));
    }

    // Update pre-allocated descriptor sets
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}



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
