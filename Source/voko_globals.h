#pragma once
#include <vector>
#include <array>
#include <vulkan/vulkan_core.h>

// use forward declaration to avoid multiple definition errors
// `inline` only prevent multiple definition errors after c++17
class Mesh;
namespace vkglTF {
    class Model;
}
class VulkanSwapChain;

namespace voko_global
{
    // Consts & Counts
    constexpr int SPOT_LIGHT_MAX = 3;
    constexpr int DIR_LIGHT_MAX = 4;
    constexpr int MESH_MAX = 100;
    constexpr int MESH_SAMPLER_MAX = 12;
    constexpr int MESH_SAMPLER_COUNT = 2;

    extern int SPOT_LIGHT_COUNT;
    extern int DIR_LIGHT_COUNT;
    extern int MESH_COUNT;

    // Global Resources
    // Global Cmd Buffer Pool
    extern VkCommandPool commandPool;
    // Global render pass for frame buffer writes
    extern VkRenderPass renderPass;
    // List of available frame buffers (same as number of swap chain images)
    extern std::vector<VkFramebuffer> frameBuffers;
    // Active frame buffer index
    extern uint32_t currentBuffer;

    /* Global Color Textures & Depth Stencil */
    extern struct SceneColor {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkFormat format;
        uint32_t width;
        uint32_t height;
    } sceneColor;
    // Depth buffer format (selected during Vulkan initialization)
    extern VkFormat depthFormat;
    extern struct DepthStencil{
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkFormat format;
    } depthStencil;

    extern VulkanSwapChain* swapChain;

    
    // Global scene infos for pass rendering
    extern std::vector<Mesh *> SceneMeshes;

    extern VkDescriptorSetLayout SceneDescriptorSetLayout;
    extern VkDescriptorSet SceneDescriptorSet;

    extern VkDescriptorSetLayout PerMeshDescriptorSetLayout;
    extern std::vector<VkDescriptorSet> PerMeshDescriptorSets;

    extern uint32_t width;
    extern uint32_t height;

    // IBL Resources
    extern bool bDisplaySkybox;
    extern vkglTF::Model skybox;


    // Mesh Samplers
    enum EMeshSamplerFlags
    {
        ALBEDO    = 0x01,   // 0000 0001
        NORMAL    = 0x02,   // 0000 0010
        METALLIC        = 0x04,   // 0000 0100
        ROUGHNESS  = 0x08,   // 0000 1000
        AO = 0x10,    // 0001 0000
        ALL = 0xff      // 1111 1111
    };
    struct MeshSampler {
        uint32_t binding;
        EMeshSamplerFlags flag;
    };

    // Declare mesh samplers & their binding pos
    constexpr MeshSampler meshSamplers[] = {
        {2, ALBEDO},
        {3, NORMAL},
        {4, METALLIC},
        {5, ROUGHNESS},
        {6, AO}
    };




};
