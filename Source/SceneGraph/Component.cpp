#include "Component.h"

#include <algorithm>

#include "Node.h"

Component::Component(const std::string &name) :
    name{name}
{}

const std::string &Component::get_name() const
{
    return name;
}
