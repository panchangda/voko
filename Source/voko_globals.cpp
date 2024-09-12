#include "voko_globals.h"
#include "VulkanSwapChain.h"
#include "SceneGraph/Mesh.h"

namespace voko_global
{
    // Consts & Counts
    int SPOT_LIGHT_COUNT = 3;
    int DIR_LIGHT_COUNT = 4;
    int MESH_COUNT = 0;

    // Global Cmd Buffer Pool
    VkCommandPool commandPool = {VK_NULL_HANDLE};
    // Global render pass for frame buffer writes
    VkRenderPass renderPass = {VK_NULL_HANDLE};
    // List of available frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer> frameBuffers;
    // Active frame buffer index
    uint32_t currentBuffer = 0;


    /* Global Color Textures & Depth Stencil */
    SceneColor sceneColor = {VK_NULL_HANDLE,VK_NULL_HANDLE,VK_NULL_HANDLE, VK_FORMAT_UNDEFINED, 0, 0};

    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    DepthStencil depthStencil = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

    VulkanSwapChain* swapChain = nullptr;

    // Global scene infos for pass rendering
    std::vector<Mesh*> SceneMeshes;

    VkDescriptorSetLayout SceneDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet SceneDescriptorSet = VK_NULL_HANDLE;

    VkDescriptorSetLayout PerMeshDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> PerMeshDescriptorSets;

    uint32_t width = 1280;
    uint32_t height = 720;

    // IBL
    bool bDisplaySkybox = true;
    vkglTF::Model skybox = vkglTF::Model();
}
