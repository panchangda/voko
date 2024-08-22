#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

#include "Component.h"
class Node;


class Transform : public Component
{
public:
    Transform(Node &node);

    virtual ~Transform() override = default;

    Node &get_node();

    virtual std::type_index get_type() override;

    void set_translation(const glm::vec3 &translation);

    void set_rotation(const glm::quat &rotation);

    void set_scale(const glm::vec3 &scale);

    const glm::vec3 &get_translation() const;

    const glm::quat &get_rotation() const;

    const glm::vec3 &get_scale() const;

    void set_matrix(const glm::mat4 &matrix);

    glm::mat4 get_matrix() const;

    glm::mat4 get_world_matrix();

    /**
     * @brief Marks the world transform invalid if any of
     *        the local transform are changed or the parent
     *        world transform has changed.
     */
    void invalidate_world_matrix();

private:
    Node &node;

    glm::vec3 translation = glm::vec3(0.0, 0.0, 0.0);

    glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);

    glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

    glm::mat4 world_matrix = glm::mat4(1.0);

    bool update_world_matrix = false;

    void update_world_transform();
};
