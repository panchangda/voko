#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "Component.h"
#include "Node.h"
#include "VulkanglTFModel.h"
#include "VulkanTexture.h"

class Mesh : public Component
{
public:
    Mesh(const std::string &name);

    virtual ~Mesh() = default;

    virtual std::type_index get_type() override;

    void set_node(Node& node);

    Node *get_node();

    vkglTF::Model VkGltfModel;
    struct
    {
        vks::Texture2D ColorMap;
        vks::Texture2D NormalMap;
    }Textures;

    
private:
    Node* node;
};
