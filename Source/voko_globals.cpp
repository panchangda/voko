#include "voko_globals.h"

namespace voko_global
{
    // ` ` only prevent multiple definition errors after c++17
    
    // Global Cmd Buffer Pool
    VkCommandPool commandPool = {VK_NULL_HANDLE};
    // Global render pass for frame buffer writes
    VkRenderPass renderPass = {VK_NULL_HANDLE};
    // List of available frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer> frameBuffers;
    // Active frame buffer index
    uint32_t currentBuffer = 0;

    // Global scene infos for pass rendering
    std::vector<Mesh*> SceneMeshes;

    VkDescriptorSetLayout SceneDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet SceneDescriptorSet = VK_NULL_HANDLE;

    VkDescriptorSetLayout PerMeshDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> PerMeshDescriptorSets;

    uint32_t width = 1280;
    uint32_t height = 720;
}
