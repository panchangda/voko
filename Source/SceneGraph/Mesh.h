#pragma once

#include <string>
#include <vector>

#include "Component.h"
#include "Node.h"
#include "VulkanglTFModel.h"
#include "VulkanTexture.h"

// forward
namespace voko_buffer {
    struct PerInstanceSSBO;
}

class Mesh : public Component
{
public:
    Mesh(const std::string &name);

    virtual ~Mesh() override = default;

    virtual std::type_index get_type() override;

    void set_node(Node& node);

    Node *get_node();

    vkglTF::Model VkGltfModel;
    
    struct
    {
        vks::Texture2D albedoMap;
        vks::Texture2D normalMap;
        vks::Texture2D aoMap;
        vks::Texture2D metallicMap;
        vks::Texture2D roughnessMap;
    }Textures;


    vks::Buffer meshPropSSBO;
    vks::Buffer instanceSSBO;

    std::vector<voko_buffer::PerInstanceSSBO> Instances;
    
    void draw_mesh();
    void draw_mesh(VkCommandBuffer cmdBuffer);
    
    
    
private:
    Node* node;
};
