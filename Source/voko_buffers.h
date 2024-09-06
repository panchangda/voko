#pragma once
#include "glm/vec4.hpp"
#include "glm/matrix.hpp"

// Buffer Definitions:

/**
 * \brief Mesh buffers
 */
struct PerInstanceSSBO
{
    glm::vec4 instancePos;
    //glm::mat4 modelMatrix;
};

/*
 *
 * In glsl, dynamic sized ssbo are used like this:
struct PerInstanceSSBO{
    vec4 instancePos;
};
layout (std430, set = 1, binding = 1) buffer SSBOInstance
{
    PerInstanceSSBO instances[];
} ssboInstance

vec4 insPos = ssboInstance.instances[i].instancePos
*
*/

// mesh properties
struct MeshProperty
{
    glm::mat4 modelMatrix;
};



