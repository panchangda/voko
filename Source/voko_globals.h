#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "SceneGraph/Mesh.h"

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
    extern std::vector<Mesh*> SceneMeshes;

    extern VkDescriptorSetLayout SceneDescriptorSetLayout;
    extern VkDescriptorSet SceneDescriptorSet;

    extern VkDescriptorSetLayout PerMeshDescriptorSetLayout;
    extern std::vector<VkDescriptorSet> PerMeshDescriptorSets;

    extern uint32_t width;
    extern uint32_t height;
}
