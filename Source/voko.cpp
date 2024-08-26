#include "voko.h"
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html
// without below definition, voko.obj will have link errors: why? what's wrong with these "stb-style" single header file?
#define VMA_IMPLEMENTATION 
#include <vk_mem_alloc.h>


#include "VulkanglTFModel.h"
#include "SceneGraph/Light.h"
#include "SceneGraph/Mesh.h"

void voko::init()
{
    initSDL();

    initVulkan();

    prepare();
}

void voko::prepare()
{
    // init swapchain's surface firstly
    initSurface();
    createCommandPool();
    createSwapchain();
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();


    // init hello triangle
	// Setup a default look-at camera
    camera.type = Camera::CameraType::firstperson;
    camera.movementSpeed = 5.0f;
    camera.rotationSpeed = 0.25f;
    camera.position = { 2.15f, 0.3f, -8.75f };
    camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
    camera.setPerspective(60.0f, (float)width / (float)height, zNear, zFar);
    timerSpeed *= 0.25f;

    
    // Hello Triangle Functions
    // createTriangleVertexBuffer();
    // createUniformBuffers();
    // createDescriptorSetLayout();
    // createDescriptorPool();
    // createDescriptorSets();
    // createPipelines();



    // Deferred Shadow Funcs
    // loadAssets();
    // deferredSetup();
    // shadowSetup();
    // initLights();
    // prepareUniformBuffers();
    // setupDescriptors();
    // preparePipelines();
    // buildCommandBuffers();
    // buildDeferredCommandBuffer();
    prepared = true;
    
}

void voko::initSurface()
{
    int surfaceCreateRes =  SDL_Vulkan_CreateSurface(SDLWindow, instance, nullptr, &surface);
    if (surfaceCreateRes != 0) {
        // The function failed
        std::cerr << "Failed to create Vulkan surface: " << SDL_GetError() << std::endl;
    } else {
        // The function succeeded
        // std::cout << "Vulkan surface created successfully!" << std::endl;
    }


// Create the os-specific surface
//#if defined(VK_USE_PLATFORM_WIN32_KHR)
//    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
//    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//    surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
//    surfaceCreateInfo.hwnd = (HWND)platformWindow;
//    err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
//
//    if (err != VK_SUCCESS) {
//        vks::vks::tools::exitFatal("Could not create surface!", err);
//    }

    // Get available queue family properties
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
    assert(queueCount >= 1);

    std::vector<VkQueueFamilyProperties> queueProps(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

    // Iterate over each queue to learn whether it supports presenting:
    // Find a queue with present support
    // Will be used to present the swap chain images to the windowing system
    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueNodeIndex == UINT32_MAX)
            {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX)
    {
        // If there's no queue that supports both present and graphics
        // try to find a separate present queue
        for (uint32_t i = 0; i < queueCount; ++i)
        {
            if (supportsPresent[i] == VK_TRUE)
            {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

    // Exit if either a graphics or a presenting queue hasn't been found
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
    {
        std::cerr << "Could not find a graphics and/or presenting queue!";
    }

    if (graphicsQueueNodeIndex != presentQueueNodeIndex)
    {
        std::cerr << "Separate graphics and presenting queues are not supported yet!";
    }

    queueNodeIndex = graphicsQueueNodeIndex;

    // Get list of supported surface formats
    uint32_t formatCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

    // We want to get a format that best suits our needs, so we try to get one from a set of preferred formats
    // Initialize the format to the first one returned by the implementation in case we can't find one of the preffered formats
    VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
    std::vector<VkFormat> preferredImageFormats = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32
    };

    for (auto& availableFormat : surfaceFormats) {
        if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {
            selectedFormat = availableFormat;
            break;
        }
    }

    colorFormat = selectedFormat.format;
    colorSpace = selectedFormat.colorSpace;
}

void voko::createCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));
}

void voko::createSwapchain()
{
    // Store the current swap chain handle so we can use it later on to ease up recreation
    VkSwapchainKHR oldSwapchain = swapChain;

    // Get physical device surface properties and formats
        VkSurfaceCapabilitiesKHR surfCaps;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));

    // Get available present modes
    uint32_t presentModeCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

    VkExtent2D swapchainExtent = {};
    // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
    if (surfCaps.currentExtent.width == (uint32_t)-1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = width;
        swapchainExtent.height = height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCaps.currentExtent;
        width = surfCaps.currentExtent.width;
        height = surfCaps.currentExtent.height;
    }

    // Select a present mode for the swapchain

// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
// This mode waits for the vertical blank ("v-sync")
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    if (!settings.vsync)
    {
        for (size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
#if (defined(VK_USE_PLATFORM_MACOS_MVK) && defined(VK_EXAMPLE_XCODE_GENERATED))
    // SRS - Work around known MoltenVK issue re 2x frame rate when vsync (VK_PRESENT_MODE_FIFO_KHR) enabled
    struct utsname sysInfo;
    uname(&sysInfo);
    // SRS - When vsync is on, use minImageCount when not in fullscreen or when running on Apple Silcon
    // This forces swapchain image acquire frame rate to match display vsync frame rate
    if (vsync && (!fullscreen || strcmp(sysInfo.machine, "arm64") == 0))
    {
        desiredNumberOfSwapchainImages = surfCaps.minImageCount;
    }
#endif
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
    {
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }

    // Find the transformation of the surface
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        // We prefer a non-rotated transform
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfCaps.currentTransform;
    }

    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = surface;
    swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat = colorFormat;
    swapchainCI.imageColorSpace = colorSpace;
    swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.presentMode = swapchainPresentMode;
    // Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
    swapchainCI.oldSwapchain = oldSwapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.compositeAlpha = compositeAlpha;

    // Enable transfer source on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < imageCount; i++)
        {
            vkDestroyImageView(device, buffers[i].view, nullptr);
        }
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

    // Get the swap chain images
    images.resize(imageCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

    // Get the swap chain buffers containing the image and imageview
    buffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = NULL;
        colorAttachmentView.format = colorFormat;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        buffers[i].image = images[i];

        colorAttachmentView.image = buffers[i].image;

        VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
    }
}

void voko::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    drawCmdBuffers.resize(imageCount);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(drawCmdBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, drawCmdBuffers.data()));
}

void voko::createSynchronizationPrimitives()
{
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    waitFences.resize(drawCmdBuffers.size());
    for (auto& fence : waitFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }
}

void voko::setupDepthStencil()
{
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = { width, height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem));
    VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = depthStencil.image;
    imageViewCI.format = depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
}

void voko::setupRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void voko::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void voko::setupFrameBuffer()
{
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthStencil.view;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;

    // Create frame buffers for every swap chain image
    frameBuffers.resize(imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        attachments[0] = buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

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

void voko::getEnabledFeatures(){}

void voko::getEnabledExtensions(){}


void voko::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;



    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.composition, 0, nullptr);

        // Final composition as full screen quad
        // Note: Also used for debug display if debugDisplayTarget > 0
        vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred);
        vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(drawCmdBuffers[i]);
        
        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }

}



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
    std::unique_ptr<Node> ArmorKnightMeshNode;
    std::unique_ptr<Mesh> ArmorKnight = std::make_unique<Mesh>("ArmorKnight");
    ArmorKnight->VkGltfModel.loadFromFile(getAssetPath() + "models/armor/armor.gltf", vulkanDevice, queue, glTFLoadingFlags);
    ArmorKnight->Textures.ColorMap.loadFromFile(getAssetPath() + "models/armor/colormap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    ArmorKnight->Textures.NormalMap.loadFromFile(getAssetPath() + "models/armor/normalmap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    ArmorKnight->set_node(*ArmorKnightMeshNode);

    std::unique_ptr<Node> StoneFloor02Node;
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
    std::vector<glm::vec3>LightPos = {
    glm::vec3(-14.0f, -0.5f, 15.0f),
    glm::vec3(14.0f, -4.0f, 12.0f),
    glm::vec3(0.0f, -10.0f, 4.0f)};
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
        std::unique_ptr<Light> SpotLight;
        std::unique_ptr<Node> OwnerNode;
        
        Transform& TF = dynamic_cast<Transform&>(OwnerNode->get_component(typeid(Transform)));
        TF.set_translation(LightPos[i]);

        SpotLight->set_node(*OwnerNode);
        SpotLight->set_properties(LightProperties[i]);
        SpotLight->set_light_type(Spot);
        
        CurrentScene->add_node(std::move(OwnerNode));
        CurrentScene->add_component(std::move(SpotLight));
    }
}

void voko::prepareSceneUniformBuffer()
{
    prepareLightUniformBuffer();
    prepareViewUniformBuffer();
}

void voko::prepareLightUniformBuffer()
{
    // // Offscreen vertex shader
    // VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     &offscreenUB, sizeof(UniformDataOffscreen)));
    //
    // // Deferred fragment shader
    // VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     &compositionUB, sizeof(UniformDataComposition)));
    //
    // // Shadow map vertex shader (matrices from shadow's pov)
    // VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     &shadowGeometryShaderUB, sizeof(UniformDataShadows)));
    //
    // // Map persistent
    // VK_CHECK_RESULT(offscreenUB.map());
    // VK_CHECK_RESULT(compositionUB.map());
    // VK_CHECK_RESULT(shadowGeometryShaderUB.map());
    //
    // // Setup instanced model positions
    // uniformDataOffscreen.instancePos[0] = glm::vec4(0.0f);
    // uniformDataOffscreen.instancePos[1] = glm::vec4(-7.0f, 0.0, -4.0f, 0.0f);
    // uniformDataOffscreen.instancePos[2] = glm::vec4(4.0f, 0.0, -6.0f, 0.0f);

    
}

void voko::prepareViewUniformBuffer()
{
    
}

void voko::render()
{
    if (!prepared) 
    	return;
    
    UpdateViewUniformBuffer();
    UpdateLightUniformBuffer();
    
    // UpdateUniformBufferDeferred();
    // updateUniformBufferOffscreen();

    draw();
}


void voko::draw()
{
    prepareFrame();

    // Offscreen rendering

    // Wait for swap chain presentation to finish
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    // Signal ready with offscreen semaphore
    submitInfo.pSignalSemaphores = &offscreenSemaphore;

    // Submit work

    // Shadow map pass
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &offScreenCmdBuffer;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    // Scene rendering

    // Wait for offscreen semaphore
    submitInfo.pWaitSemaphores = &offscreenSemaphore;
    // Signal ready with render complete semaphore
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;

    // Submit work
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
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
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, semaphores.presentComplete, (VkFence)nullptr, &currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
    // SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until submitFrame() in case number of swapchain images will change on resize
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            //windowResize();
        }
        return;
    }
    else {
        VK_CHECK_RESULT(result);
    }
}

void voko::submitFrame()
{

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &currentBuffer;
    // Check if a wait semaphore has been specified to wait for before presenting the image
    if (semaphores.renderComplete != VK_NULL_HANDLE)
    {
        presentInfo.pWaitSemaphores = &semaphores.renderComplete;
        presentInfo.waitSemaphoreCount = 1;
    }

    VkResult result = vkQueuePresentKHR(queue, &presentInfo);


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



void voko::initSDL(){
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    uint32_t width = 1920, height = 1080;
    windowExtent = VkExtent2D{ width , height};
    SDLWindow = SDL_CreateWindow(
        "Vulkan Engine",
        windowExtent.width,
        windowExtent.height,
        window_flags);
}

void voko::initVulkan(){
    VkResult err;

    // Vulkan Instance
    err = createInstance();
    if (err) {
        std::cout << "Could not create Vulkan Instance :" << vks::tools::errorString(err) << '\n';
        return;
    }

    // Vulkan Physical Device
    err = createPhysicalDevice();
    if (err) {
        std::cout << "Create Vulkan Physical Device Failed!"  << err << '\n';
        return;
    }

    // Vulkan Device&Queue&CommandPool
    err = createDeviceAndQueueAndCommandPool(enabledFeatures, enabledInstanceExtensions, deviceCreatepNextChain);
    if (err) {
        std::cout << "Create Vulkan Device&Queue&CommandPool Failed!"  << err << '\n';
        return;
    }

    err = createVMA();
    if (err) {
        std::cout << "Create Vulkan Memory Allocation Failed!" << err << '\n';
        return;
    }

}

VkResult voko::createVMA()
{
    
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;

    VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
    return result;
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

void voko::CreateLightUniformBuffer()
{
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         &lightUniformBuffer, sizeof(UniformBufferLight)));
    lightUniformBuffer.mapped();
}

void voko::CreateLightDescriptor()
{
    // Light Pool: UB size = 1
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &lightDesciptorPool));
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: Vertex or Geometry shader uniform buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &lightDescriptorSetLayout));

    // Sets
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(lightDesciptorPool, &lightDescriptorSetLayout, 1);


    // mapping to Light ub 
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &lightDescriptorSet));
    writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(lightDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &lightUniformBuffer.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void voko::UpdateLightUniformBuffer()
{
    // Update lights
    // Animate
    uniformBufferLight.lights[0].position.x = -14.0f + std::abs(sin(glm::radians(timer * 360.0f)) * 20.0f);
    uniformBufferLight.lights[0].position.z = 15.0f + cos(glm::radians(timer *360.0f)) * 1.0f;
    
    uniformBufferLight.lights[1].position.x = 14.0f - std::abs(sin(glm::radians(timer * 360.0f)) * 2.5f);
    uniformBufferLight.lights[1].position.z = 13.0f + cos(glm::radians(timer *360.0f)) * 4.0f;
    
    uniformBufferLight.lights[2].position.x = 0.0f + sin(glm::radians(timer *360.0f)) * 4.0f;
    uniformBufferLight.lights[2].position.z = 4.0f + cos(glm::radians(timer *360.0f)) * 2.0f;
    
    for (uint32_t i = 0; i < LIGHT_COUNT; i++) {
        // mvp from light's pov (for shadows)
        glm::mat4 shadowProj = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
        glm::mat4 shadowView = glm::lookAt(glm::vec3(uniformBufferLight.lights[i].position), glm::vec3(uniformBufferLight.lights[i].target), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 shadowModel = glm::mat4(1.0f);
    
        uniformBufferLight.lights[i].mvpMatrix = shadowProj * shadowView * shadowModel;
    }
    memcpy(lightUniformBuffer.mapped, &uniformBufferLight, sizeof(uniformBufferLight));
}

void voko::CreateViewUniformBuffer()
{
    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         &viewUniformBuffer, sizeof(UniformBufferView)));
    viewUniformBuffer.mapped();
    
}
void voko::UpdateViewUniformBuffer()
{
    // Update view (camera)
    uniformBufferView.projectionMatrix = camera.matrices.perspective;
    uniformBufferView.viewMatrix = camera.matrices.view;
    uniformBufferView.modelMatrix = glm::mat4(1.0f);
    // why revert x&z? 
    uniformBufferView.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);;
    memcpy(viewUniformBuffer.mapped, &uniformBufferView, sizeof(UniformBufferView));


}

void voko::CreateViewDescriptor()
{
    // View Pool: UB size = 1
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &viewDesciptorPool));
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: Vertex or Geometry shader uniform buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &viewDescriptorSetLayout));

    // Sets
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(viewDesciptorPool, &viewDescriptorSetLayout, 1);
    
    // mapping to light ub 
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &viewDescriptorSet));
    writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(viewDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &viewUniformBuffer.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}


uint32_t voko::getQueueFamilyIndex(VkQueueFlags queueFlags) const
{
    // Dedicated queue for compute
// Try to find a queue family index that supports compute but not graphics
    if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
            {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
            {
                return i;
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
    {
        if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
        {
            return i;
        }
    }

    std::cerr << "Could not find a matching queue family index";
}




VkResult voko::createInstance(){
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = name.c_str();
    appInfo.pEngineName = name.c_str();
    appInfo.apiVersion = apiVersion;


    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

    // Enable surface extensions depending on os
#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif




    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        {
            for (VkExtensionProperties& extension : extensions)
            {
                supportedInstanceExtensions.push_back(extension.extensionName);
}
        }
    }

    // IOS & MacOS 
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    // SRS - When running on iOS/macOS with MoltenVK, enable VK_KHR_get_physical_device_properties2 if not already enabled by the example (required by VK_KHR_portability_subset)
    if (std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == enabledInstanceExtensions.end())
    {
        enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
#endif


    // Enabled requested instance extensions
    if (enabledInstanceExtensions.size() > 0)
    {
        for (const char* enabledExtension : enabledInstanceExtensions)
        {
            // Output message if requested extension is not available
            if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
            {
                std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
            }
            instanceExtensions.push_back(enabledExtension);
        }
    }

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    if (settings.validation) {
        vks::debug::setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
        debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
        instanceCreateInfo.pNext = &debugUtilsMessengerCI;
    }

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)) && defined(VK_KHR_portability_enumeration)
    // SRS - When running on iOS/macOS with MoltenVK and VK_KHR_portability_enumeration is defined and supported by the instance, enable the extension and the flag
    if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) != supportedInstanceExtensions.end())
    {
        instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    // Enable the debug utils extension if available (e.g. when debugging tools are present)
    if (settings.validation || std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (instanceExtensions.size() > 0)
    {
        instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    // Note that on Android this layer requires at least NDK r20
    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (settings.validation)
    {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties& layer : instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        }
        else {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
        }
    }
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    // If the debug utils extension is present we set up debug functions, so samples can label objects for debugging
    if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
        vks::debugutils::setup(instance);
    }

    return result;
}

VkResult voko::createPhysicalDevice()
{
    VkResult result;

    // Physical device
    uint32_t gpuCount = 0;
    // Get number of available physical devices
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    if (gpuCount == 0) {
        std::cerr << "No device with Vulkan support found ";
        return VkResult{ VK_ERROR_UNKNOWN };
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    result = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
    if (result){
        std::cerr << "Could not enumerate physical devices: \n";
        return result;
    }

    // GPU selection

    // Select physical device to be used for the Vulkan example
    // Defaults to the first device unless specified by command line
    uint32_t selectedDevice = 0;
    physicalDevice = physicalDevices[selectedDevice];


    
    // Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
    getEnabledFeatures();

    // Vulkan device creation
    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device
    vulkanDevice = new vks::VulkanDevice(physicalDevice);

    // Derived examples can enable extensions based on the list of supported extensions read from the physical device
    getEnabledExtensions();
    
    result = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
    if (result != VK_SUCCESS) {
        vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(result), result);
        return result;
    }
    device = vulkanDevice->logicalDevice;
    
    return result;
}

VkResult voko::createDeviceAndQueueAndCommandPool(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
{
    // Get a graphics queue from the device
    vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);

    // Find a suitable depth and/or stencil format
    VkBool32 validFormat{ false };
    // Samples that make use of stencil will require a depth + stencil format, so we select from a different list
    if (requiresStencil) {
        validFormat = vks::tools::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
    }
    else {
        validFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
    }
    assert(validFormat);

    // Create Semaphores
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // Create a semaphore used to synchronize image presentation
    // Ensures that the image is displayed before we start submitting new commands to the queue
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
    // Create a semaphore used to synchronize command submission
    // Ensures that the image is not presented until all commands have been submitted and executed
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));


    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;

    
    return VkResult();

}

void voko::loadAssets()
{
    const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
    models.model.loadFromFile(getAssetPath() + "models/armor/armor.gltf", vulkanDevice, queue, glTFLoadingFlags);
    models.background.loadFromFile(getAssetPath() + "models/deferred_box.gltf", vulkanDevice, queue, glTFLoadingFlags);
    textures.model.colorMap.loadFromFile(getAssetPath() + "models/armor/colormap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    textures.model.normalMap.loadFromFile(getAssetPath() + "models/armor/normalmap_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    textures.background.colorMap.loadFromFile(getAssetPath() + "textures/stonefloor02_color_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
    textures.background.normalMap.loadFromFile(getAssetPath() + "textures/stonefloor02_normal_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
}

void voko::deferredSetup()
{
    deferredFrameBuffer = new vks::Framebuffer(vulkanDevice);
#if defined(__ANDROID__)
    // Use max. screen dimension as deferred framebuffer size
    deferredFrameBuffer->width = std::max(width, height);
    deferredFrameBuffer->height = std::max(width, height);
#else
    deferredFrameBuffer->width = 2048;
    deferredFrameBuffer->height = 2048;
#endif
    
    // Four attachments (3 color, 1 depth)
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.width = deferredFrameBuffer->width;
    attachmentInfo.height = deferredFrameBuffer->height;
    attachmentInfo.layerCount = 1;
    attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    // Color attachments
    // Attachment 0: (World space) Positions
    attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    deferredFrameBuffer->addAttachment(attachmentInfo);

    // Attachment 1: (World space) Normals
    attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    deferredFrameBuffer->addAttachment(attachmentInfo);

    // Attachment 2: Albedo (color)
    attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    deferredFrameBuffer->addAttachment(attachmentInfo);

    // Depth attachment
    // Find a suitable depth format
    VkFormat attDepthFormat;
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &attDepthFormat);
    assert(validDepthFormat);

    attachmentInfo.format = attDepthFormat;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    deferredFrameBuffer->addAttachment(attachmentInfo);

    // Create sampler to sample from the color attachments
    VK_CHECK_RESULT(deferredFrameBuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(deferredFrameBuffer->createRenderPass());
}

void voko::shadowSetup()
{
    shadowFrameBuffer = new vks::Framebuffer(vulkanDevice);

    // Shadowmap properties
#if defined(__ANDROID__)
    // Use smaller shadow maps on mobile due to performance reasons
    shadowFrameBuffer->width = 1024;
    shadowFrameBuffer->height = 1024;
#else
    shadowFrameBuffer->width = 2048;
    shadowFrameBuffer->height = 2048;
#endif

    // Find a suitable depth format
    VkFormat shadowMapFormat;
    VkBool32 validShadowMapFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &shadowMapFormat);
    assert(validShadowMapFormat);

    // Create a layered depth attachment for rendering the depth maps from the lights' point of view
    // Each layer corresponds to one of the lights
    // The actual output to the separate layers is done in the geometry shader using shader instancing
    // We will pass the matrices of the lights to the GS that selects the layer by the current invocation
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.format = shadowMapFormat;
    attachmentInfo.width = shadowFrameBuffer->width;
    attachmentInfo.height = shadowFrameBuffer->height;
    attachmentInfo.layerCount = LIGHT_COUNT;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadowFrameBuffer->addAttachment(attachmentInfo);

    // Create sampler to sample from to depth attachment
    // Used to sample in the fragment shader for shadowed rendering
    VK_CHECK_RESULT(shadowFrameBuffer->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(shadowFrameBuffer->createRenderPass());
}
// voko::Light voko::initLight(glm::vec3 pos, glm::vec3 target, glm::vec3 color)
// {
//     Light light;
//     light.position = glm::vec4(pos, 1.0f);
//     light.target = glm::vec4(target, 0.0f);
//     light.color = glm::vec4(color, 0.0f);
//     return light;
// }
// void voko::initLights()
// {
//     uniformDataComposition.lights[0] = initLight(glm::vec3(-14.0f, -0.5f, 15.0f), glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.5f));
//     uniformDataComposition.lights[1] = initLight(glm::vec3(14.0f, -4.0f, 12.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//     uniformDataComposition.lights[2] = initLight(glm::vec3(0.0f, -10.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
// }

// void voko::prepareUniformBuffers()
// {
//     // Offscreen vertex shader
//     VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//         &offscreenUB, sizeof(UniformDataOffscreen)));
//
//     // Deferred fragment shader
//     VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//         &compositionUB, sizeof(UniformDataComposition)));
//
//     // Shadow map vertex shader (matrices from shadow's pov)
//     VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//         &shadowGeometryShaderUB, sizeof(UniformDataShadows)));
//
//     // Map persistent
//     VK_CHECK_RESULT(offscreenUB.map());
//     VK_CHECK_RESULT(compositionUB.map());
//     VK_CHECK_RESULT(shadowGeometryShaderUB.map());
//
//     // Setup instanced model positions
//     uniformDataOffscreen.instancePos[0] = glm::vec4(0.0f);
//     uniformDataOffscreen.instancePos[1] = glm::vec4(-7.0f, 0.0, -4.0f, 0.0f);
//     uniformDataOffscreen.instancePos[2] = glm::vec4(4.0f, 0.0, -6.0f, 0.0f);
// }

void voko::setupDescriptors()
{
    // Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo =vks::initializers::descriptorPoolCreateInfo(poolSizes, 4);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0),
			// Binding 1: Position texture
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			// Binding 2: Normals texture
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			// Binding 3: Albedo texture
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			// Binding 4: Fragment shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
			// Binding 5: Shadow map
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Sets
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		// Image descriptors for the offscreen color attachments
		VkDescriptorImageInfo texDescriptorPosition =
			vks::initializers::descriptorImageInfo(
				deferredFrameBuffer->sampler,
				deferredFrameBuffer->attachments[0].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorNormal =
			vks::initializers::descriptorImageInfo(
				deferredFrameBuffer->sampler,
				deferredFrameBuffer->attachments[1].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorAlbedo =
			vks::initializers::descriptorImageInfo(
				deferredFrameBuffer->sampler,
				deferredFrameBuffer->attachments[2].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo texDescriptorShadowMap =
			vks::initializers::descriptorImageInfo(
				shadowFrameBuffer->sampler,
				shadowFrameBuffer->attachments[0].view,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		// Deferred composition
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.composition));
		writeDescriptorSets = {
			// Binding 1: World space position texture
			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorPosition),
			// Binding 2: World space normals texture
			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorNormal),
			// Binding 3: Albedo texture
			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texDescriptorAlbedo),
			// Binding 4: Fragment shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &compositionUB.descriptor),
			// Binding 5: Shadow map
			vks::initializers::writeDescriptorSet(descriptorSets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &texDescriptorShadowMap),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Offscreen (scene)

		// Model
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.model));
		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform bfuffer
			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &offscreenUB.descriptor),
			// Binding 1: Color map
			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.model.colorMap.descriptor),
			// Binding 2: Normal map
			vks::initializers::writeDescriptorSet(descriptorSets.model, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.model.normalMap.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Background
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.background));
		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &offscreenUB.descriptor),
			// Binding 1: Color map
			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.background.colorMap.descriptor),
			// Binding 2: Normal map
			vks::initializers::writeDescriptorSet(descriptorSets.background, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.background.normalMap.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Shadow mapping
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadow));
		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.shadow, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shadowGeometryShaderUB.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void voko::preparePipelines()
{
    // Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        &descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

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
    
    VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
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
    shaderStages[0] = loadShader((getShaderBasePath() + "deferredshadows/deferred.vert.spv").c_str(),
                                 VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader((getShaderBasePath() + "deferredshadows/deferred.frag.spv").c_str(),
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
    // Empty vertex input state, vertices are generated by the vertex shader
    VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
    pipelineCI.pVertexInputState = &emptyInputState;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.deferred));

    // Vertex input state from glTF model for pipeline rendering models
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({
        vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color,
        vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Tangent
    });
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    // Offscreen pipeline
    // Separate render pass
    pipelineCI.renderPass = deferredFrameBuffer->renderPass;

    // Blend attachment states required for all color attachments
    // This is important, as color write mask will otherwise be 0x0 and you
    // won't see anything rendered to the attachment
    std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates =
    {
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
    };
    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();

    shaderStages[0] = loadShader(getShaderBasePath() + "deferredshadows/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShaderBasePath() + "deferredshadows/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));

    // Shadow mapping pipeline
    // The shadow mapping pipeline uses geometry shader instancing (invocations layout modifier) to output
    // shadow maps for multiple lights sources into the different shadow map layers in one single render pass
    std::array<VkPipelineShaderStageCreateInfo, 2> shadowStages;
    shadowStages[0] = loadShader(getShaderBasePath() + "deferredshadows/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shadowStages[1] = loadShader(getShaderBasePath() + "deferredshadows/shadow.geom.spv",
                                 VK_SHADER_STAGE_GEOMETRY_BIT);

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
    pipelineCI.renderPass = shadowFrameBuffer->renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.shadowpass));
}

void voko::buildDeferredCommandBuffer()
{
    if (offScreenCmdBuffer == VK_NULL_HANDLE) {
			offScreenCmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		// Create a semaphore used to synchronize offscreen rendering and usage
		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenSemaphore));

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		std::array<VkClearValue, 4> clearValues = {};
		VkViewport viewport;
		VkRect2D scissor;

		// First pass: Shadow map generation
		// -------------------------------------------------------------------------------------------------------

		clearValues[0].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.renderPass = shadowFrameBuffer->renderPass;
		renderPassBeginInfo.framebuffer = shadowFrameBuffer->framebuffer;
		renderPassBeginInfo.renderArea.extent.width = shadowFrameBuffer->width;
		renderPassBeginInfo.renderArea.extent.height = shadowFrameBuffer->height;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues.data();

		VK_CHECK_RESULT(vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo));

		viewport = vks::initializers::viewport((float)shadowFrameBuffer->width, (float)shadowFrameBuffer->height, 0.0f, 1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		scissor = vks::initializers::rect2D(shadowFrameBuffer->width, shadowFrameBuffer->height, 0, 0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		// Set depth bias (aka "Polygon offset")
		vkCmdSetDepthBias(
			offScreenCmdBuffer,
			depthBiasConstant,
			0.0f,
			depthBiasSlope);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowpass);
		renderScene(offScreenCmdBuffer, true);
		vkCmdEndRenderPass(offScreenCmdBuffer);

		// Second pass: Deferred calculations
		// -------------------------------------------------------------------------------------------------------

		// Clear values for all attachments written in the fragment shader
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.renderPass = deferredFrameBuffer->renderPass;
		renderPassBeginInfo.framebuffer = deferredFrameBuffer->framebuffer;
		renderPassBeginInfo.renderArea.extent.width = deferredFrameBuffer->width;
		renderPassBeginInfo.renderArea.extent.height = deferredFrameBuffer->height;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport = vks::initializers::viewport((float)deferredFrameBuffer->width, (float)deferredFrameBuffer->height, 0.0f, 1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		scissor = vks::initializers::rect2D(deferredFrameBuffer->width, deferredFrameBuffer->height, 0, 0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
		renderScene(offScreenCmdBuffer, false);
		vkCmdEndRenderPass(offScreenCmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(offScreenCmdBuffer));
}

void voko::renderScene(VkCommandBuffer cmdBuffer, bool shadow)
{
    // Background
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, shadow ? &descriptorSets.shadow : &descriptorSets.background, 0, NULL);
    models.background.draw(cmdBuffer);

    // Objects
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, shadow ? &descriptorSets.shadow : &descriptorSets.model, 0, NULL);
    models.model.bindBuffers(cmdBuffer);
    vkCmdDrawIndexed(cmdBuffer, models.model.indices.count, 3, 0, 0, 0);
}

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
}
