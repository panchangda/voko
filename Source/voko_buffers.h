#pragma once
#include "voko_globals.h"
#include "glm/vec4.hpp"
#include "glm/matrix.hpp"

// voko buffer definitions:
namespace voko_buffer {
    /*
     * Mesh buffers:
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



    /**
     * Light buffers
     */
    struct DirectionalLight
    {
        glm::vec3 direction;
        glm::vec3 color;
        // directional light range
        glm::vec2 lr;
        glm::vec2 tb;
        glm::vec2 nf;
    };

    struct PointLight
    {
        glm::vec4 position;
        glm::vec4 color;
        glm::mat4 viewMatrix;
        float intensity;
    };

    // 112 B
    struct alignas(16) SpotLight {
        glm::vec4 position;
        glm::vec4 target;
        glm::vec4 color;
        glm::mat4 viewMatrix;
        float range;
        float lightCosInnerAngle;
        float lightCosOuterAngle;
        SpotLight():
        position({}),
        target({}),
        color({}),
        viewMatrix({}),
        range(0.0f),
        lightCosInnerAngle(0.f),
        lightCosOuterAngle(0.f)
        {}

        SpotLight(glm::vec4 _position,
            glm::vec4 _target,
            glm::vec4 _color,
            glm::mat4 _viewMatrix,
            float _range,
            float _innerConeAngle,
            float _outerConeAngle):
        position(_position),
        target(_target),
        color(_color),
        viewMatrix(_viewMatrix),
        range(_range),
        lightCosInnerAngle(_innerConeAngle),
        lightCosOuterAngle(_outerConeAngle)
        {}

    };

    // View: 208 B
    struct alignas(16) UniformBufferView {
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 viewProjectionMatrix;
        glm::vec4 viewPos;
    };

    struct alignas(16) UniformBufferLighting {
        // lights
        uint32_t lightModel = 1; // 0:PBR, 1:Blinn-Phong, 2:Phong
        uint32_t spotLightCount = 3;
        SpotLight spotLights[voko_global::SPOT_LIGHT_MAX];
        float ambientCoef = 0.1f;
        uint32_t skybox = 1;

        // shadow
        uint32_t useShadows = 1;
        uint32_t shadowFilterMethod = 1; // 0:Fixed size shadow factor, 1:PCF, 2:PCSS
        float shadowFactor = 0.25f;
    };

    struct alignas(16) UniformBufferDebug {
        // debug vars
        uint32_t debugGBuffer = 0; // 0:off, 1:shadow, 2:fragPos, 3:normal, 4:albedo.rgb, 5:albedo.aaa
        uint32_t debugLighting = 0; // 0:off, 1:ambient, 2:diffuse, 3:specular,
    };

    struct UniformBufferScene
    {
        UniformBufferView view;
        UniformBufferLighting lighting;
        UniformBufferDebug debug;
    };
}


