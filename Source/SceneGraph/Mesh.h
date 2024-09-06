#pragma once

#include <string>
#include <vector>

#include "voko_buffers.h"

#include "Component.h"
#include "Node.h"
#include "VulkanglTFModel.h"
#include "VulkanTexture.h"



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
        vks::Texture2D ColorMap;
        vks::Texture2D NormalMap;
    }Textures;

    vks::Buffer meshPropSSBO;
    vks::Buffer instanceSSBO;

    std::vector<PerInstanceSSBO> Instances;
    
    void draw_mesh();
    void draw_mesh(VkCommandBuffer cmdBuffer);
    
    
    
private:
    Node* node;
};
