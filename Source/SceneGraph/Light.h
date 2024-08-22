#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include <glm/glm.hpp>

#include "Component.h"
#include "Node.h"

enum LightType
{
    Directional = 0,
    Point       = 1,
    Spot        = 2,
    // Insert new light type here
    Max
};

struct LightProperties
{
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    glm::vec3 color{1.0f, 1.0f, 1.0f};

    float intensity{1.0f};

    float range{0.0f};

    float inner_cone_angle{0.0f};

    float outer_cone_angle{0.0f};
};

class Light : public Component
{
public:
    Light(const std::string &name);

    Light(Light &&other) = default;

    virtual ~Light() = default;

    virtual std::type_index get_type() override;

    void set_node(Node &node);

    Node *get_node();
    
    void set_light_type(const LightType &type);

    const LightType &get_light_type();

    void set_properties(const LightProperties &properties);

    const LightProperties &get_properties();

private:
    Node *node{nullptr};
    
    LightType light_type;

    LightProperties properties;
};