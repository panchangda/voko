#pragma once

// vk
#include <vulkan/vulkan.h>
// std
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <memory>
#include <cassert>
#include <chrono>
#include <fstream>
// self defined
#include "debug.h"
#include "camera.hpp"
#include "VulkanTools.h"

// 3rdparty
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>




// Macro to check and display Vulkan return results
#if defined(__ANDROID__)
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		LOGE("Fatal : VkResult is \" %s \" in %s at line %d", vks::tools::errorString(res).c_str(), __FILE__, __LINE__); \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#else
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#endif

typedef struct _SwapChainBuffers {
    VkImage image;
    VkImageView view;
} SwapChainBuffer;

// We want to keep GPU and CPU busy. To do that we may start building a new command buffer while the previous one is still being executed
// This number defines how many frames may be worked on simultaneously at once
// Increasing this number may improve performance but will also introduce additional latency
#define MAX_CONCURRENT_FRAMES 3

// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

class voko{
public:
    voko() = default;
    ~voko();

    // Frame counter to display fps
    uint32_t frameCounter = 0;
    bool prepared = false;
    virtual void render();
    void updateUniformBuffers();
    void nextFrame();
    void prepareFrame();
    void submitFrame();
    void renderLoop();
    void draw();

    void prepare();
    // Belows functions are prepare procedures
    // Only Test on Windows 
    void initSurface();
    void createCommandPool();
    void createSwapchain();
    void createCommandBuffers();
    void createSynchronizationPrimitives();
    void setupDepthStencil();
    void setupRenderPass();
    void createPipelineCache();
    void setupFrameBuffer();
    void buildCommandBuffers();

    // Hello Triangle Preparetions & structs
    // Vertex layout used in this example
    struct Vertex {
        float position[3];
        float color[3];
    };
    // Vertex buffer and attributes
    struct {
        VkDeviceMemory memory{ VK_NULL_HANDLE }; // Handle to the device memory for this buffer
        VkBuffer buffer;						 // Handle to the Vulkan buffer object that the memory is bound to
    } vertices;
    // Index buffer
    struct {
        VkDeviceMemory memory{ VK_NULL_HANDLE };
        VkBuffer buffer;
        uint32_t count{ 0 };
    } indices;
    // Uniform buffer block object
    struct UniformBuffer {
        VkDeviceMemory memory;
        VkBuffer buffer;
        // The descriptor set stores the resources bound to the binding points in a shader
        // It connects the binding points of the different shaders with the buffers and images used for those bindings
        VkDescriptorSet descriptorSet;
        // We keep a pointer to the mapped buffer, so we can easily update it's contents via a memcpy
        uint8_t* mapped{ nullptr };
    };
    // We use one UBO per frame, so we can have a frame overlap and make sure that uniforms aren't updated while still in use
    std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

    // For simplicity we use the same uniform block layout as in the shader:
    //
    //	layout(set = 0, binding = 0) uniform UBO
    //	{
    //		mat4 projectionMatrix;
    //		mat4 modelMatrix;
    //		mat4 viewMatrix;
    //	} ubo;
    //
    // This way we can just memcopy the ubo data to the ubo
    // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
    struct UBOMatrices {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    }uboMatrices;

    // The pipeline layout is used by a pipeline to access the descriptor sets
    // It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
    // A pipeline layout can be shared among multiple pipelines as long as their interfaces match
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

    // Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
    // While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
    // So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
    // Even though this adds a new dimension of planning ahead, it's a great opportunity for performance optimizations by the driver
    VkPipeline pipeline{ VK_NULL_HANDLE };

    // The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
    // Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };



    void createTriangleVertexBuffer();
	void createVertexBuffer();
    void createUniformBuffers();
    void createDescriptorSetLayout();
	void createDescriptorPool();
    void createDescriptorSets();
    void createPipelines();
	VkShaderModule loadSPIRVShader(std::string filename);
    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);






	void init();
    void initSDL();
    VkExtent2D windowExtent;
    SDL_Window* SDLWindow = nullptr;

    void initVulkan();

    VkResult createInstance();

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;

    /** @brief Last frame time measured using a high performance timer (if available) */
    float frameTimer = 1.0f;

    Camera camera;
    glm::vec2 mousePos;

    std::string title = "VOKO 0.0";
	std::string name = "voko";
    uint32_t apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> enabledInstanceExtensions;
    std::vector<std::string> supportedInstanceExtensions;
    /** @brief Example settings that can be changed e.g. by command line arguments */
    struct Settings {
        /** @brief Activates validation layers (and message output) when set to true */
        bool validation = true;
        /** @brief Set to true if fullscreen mode has been requested via command line */
        bool fullscreen = false;
        /** @brief Set to true if v-sync will be forced for the swapchain */
        bool vsync = false;
        /** @brief Enable UI overlay */
        bool overlay = true;
    } settings;

    struct {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
		} depthStencil;

    /** @brief Optional pNext structure for passing extension structures to device creation */
    void* deviceCreatepNextChain = nullptr;
    // check if a extension is in `supportedDeviceExtensions`
    bool extensionSupported(std::string extension)
    {
        return (std::find(supportedDeviceExtensions.begin(), supportedDeviceExtensions.end(), extension) != supportedDeviceExtensions.end());
    }
    VkResult createPhysicalDevice();


    uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags)const;
    VkResult createDeviceAndQueueAndCommandPool(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);


    /** @brief Logical device, application's view of the physical device (GPU) */
    VkDevice device{ VK_NULL_HANDLE };
    // Handle to the device graphics queue that command buffers are submitted to
    VkQueue queue{ VK_NULL_HANDLE };
    // Depth buffer format (selected during Vulkan initialization)
    VkFormat depthFormat;
    // Command buffer pool
    VkCommandPool commandPool;
    // Command buffers used for rendering
    std::vector<VkCommandBuffer> drawCmdBuffers;
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo submitInfo;
    // Global render pass for frame buffer writes
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    // List of available frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer>frameBuffers;
    // Active frame buffer index
    uint32_t currentBuffer = 0;
    // Descriptor set pool
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    // List of shader modules created (stored for cleanup)
    std::vector<VkShaderModule> shaderModules;
    // Pipeline cache object
    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };

    // Synchronization semaphores
    struct {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
    } semaphores;
    std::vector<VkFence> waitFences;
    bool requiresStencil{ false };



    VmaAllocator allocator;
    VkResult createVMA();

protected:

    
private:

    // Swapchain
    VkSwapchainKHR swapChain;
    // swapchain size
    uint32_t width = 1280;
    uint32_t height = 960;
    // swapchain image count
    uint32_t imageCount;
    // swapchain images
    std::vector<VkImage> images;
    // swapchain buffers: image + view
    std::vector<SwapChainBuffer> buffers;


    // Surface related vars
    VkSurfaceKHR surface;
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;
    uint32_t queueNodeIndex = UINT32_MAX;


    // Vulkan instance, stores all per-application states
    VkInstance instance{ VK_NULL_HANDLE };

    // Physical device (GPU) that Vulkan will use
    VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };

    // Stores physical device properties (for e.g. checking device limits)
    VkPhysicalDeviceProperties deviceProperties{};
    // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
    VkPhysicalDeviceFeatures deviceFeatures{};
    // Stores all available memory (type) properties for the physical device
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
    /** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
    VkPhysicalDeviceFeatures enabledFeatures{};
    /** @brief Queue family properties of the physical device */
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;

    /** @brief List of extensions supported by the device */
    std::vector<std::string> supportedDeviceExtensions;

};