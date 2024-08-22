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