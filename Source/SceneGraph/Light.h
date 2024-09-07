#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include "Component.h"
#include "Node.h"
#include "voko_buffers.h"

enum LightType
{
    Directional = 0,
    Point       = 1,
    Spot        = 2,
    // Insert new light type here
    Max
};
using LightProperties = std::variant<
    voko_buffer::DirectionalLight,
voko_buffer::SpotLight,
voko_buffer::PointLight>;

class Light : public Component
{
public:
    explicit Light(const std::string &name);

    Light(Light &&other) = default;

    virtual ~Light() = default;

    virtual std::type_index get_type() override;

    void set_node(Node &node);

    Node *get_node();
    
    void set_light_type(const LightType &type);

    const LightType &get_light_type();

    virtual void set_properties(const LightProperties &properties);

    virtual const LightProperties &get_properties();


private:
    Node *node{nullptr};
    
    LightType light_type;

    LightProperties properties;
};