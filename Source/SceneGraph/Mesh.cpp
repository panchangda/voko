#include "Mesh.h"

Mesh::Mesh(const std::string& name) : Component{name}
{}

std::type_index Mesh::get_type()
{
    return typeid(Mesh);
}

void Mesh::set_node(Node &n)
{
    node = &n;
}

Node *Mesh::get_node()
{
    return node;
}

void Mesh::draw_mesh()
{
    
}

void Mesh::draw_mesh(VkCommandBuffer cmdBuffer)
{
    size_t instanceCount = MeshInstanceSSBO.size();
    if(instanceCount > 0)
    {
        VkGltfModel.bindBuffers(cmdBuffer);
        vkCmdDrawIndexed(cmdBuffer, VkGltfModel.indices.count, 3, 0, 0, 0);
    }else
    {
        VkGltfModel.draw(cmdBuffer);
    }
    
}
