#pragma once
#include <vector>
#include <array>
#include <vulkan/vulkan_core.h>

class Mesh;

namespace voko_global
{
    // `inline` only prevent multiple definition errors after c++17
    // Global Cmd Buffer Pool
    extern VkCommandPool commandPool;
    // Global render pass for frame buffer writes
    extern VkRenderPass renderPass;
    // List of available frame buffers (same as number of swap chain images)
    extern std::vector<VkFramebuffer> frameBuffers;
    // Active frame buffer index
    extern uint32_t currentBuffer;
    
    // Global scene infos for pass rendering
    extern std::vector<Mesh *> SceneMeshes;

    extern VkDescriptorSetLayout SceneDescriptorSetLayout;
    extern VkDescriptorSet SceneDescriptorSet;

    extern VkDescriptorSetLayout PerMeshDescriptorSetLayout;
    extern std::vector<VkDescriptorSet> PerMeshDescriptorSets;

    extern uint32_t width;
    extern uint32_t height;



    constexpr int SPOT_LIGHT_MAX = 3;
    constexpr int DIR_LIGHT_MAX = 4;
    constexpr int MESH_MAX = 100;
    constexpr int MESH_SAMPLER_MAX = 12;
    constexpr int MESH_SAMPLER_COUNT = 2;

    extern int SPOT_LIGHT_COUNT;
    extern int DIR_LIGHT_COUNT;
    extern int MESH_COUNT;

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
