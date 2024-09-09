#pragma once

#include <string>
#include <vector>

#include "Component.h"
#include "Node.h"
#include "voko_globals.h"
#include "voko_buffers.h"
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
    
    struct MeshTextures
    {
        vks::Texture2D albedoMap;
        vks::Texture2D normalMap;
        vks::Texture2D aoMap;
        vks::Texture2D metallicMap;
        vks::Texture2D roughnessMap;

        vks::Texture2D& GetTexture(voko_global::EMeshSamplerFlags flag)
        {
            switch (flag)
            {
                case voko_global::EMeshSamplerFlags::ALBEDO:
                    return albedoMap;
                case voko_global::EMeshSamplerFlags::NORMAL:
                    return normalMap;
                case voko_global::EMeshSamplerFlags::METALLIC:
                    return metallicMap;
                case voko_global::EMeshSamplerFlags::ROUGHNESS:
                    return roughnessMap;
                case voko_global::EMeshSamplerFlags::AO:
                    return aoMap;
                default:
                    throw std::invalid_argument("Invalid EMeshSamplerFlags value.");
            }
        }
    }Textures;



    voko_buffer::MeshProperty meshProperty;
    vks::Buffer meshPropSSBO;

    std::vector<voko_buffer::PerInstanceSSBO> Instances;
    vks::Buffer instanceSSBO;
    
    void draw_mesh();
    void draw_mesh(VkCommandBuffer cmdBuffer);
    
    
    
private:
    Node* node;
};
