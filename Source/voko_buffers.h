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
    struct alignas(16) MaterialConstants {
        glm::vec4 rgba;
        float metallic;
        float roughness;
        float ao;

        MaterialConstants(): rgba(glm::vec4(1.0f)), metallic(0.f), roughness(1.0f), ao(0.f)
        {}

        MaterialConstants(glm::vec4 _rgba, float _m, float _r, float _a):
            rgba(_rgba), metallic(_m), roughness(_r),ao(_a)
        {}
    };
    struct MeshProperty
    {
        glm::mat4 modelMatrix;
        uint32_t usedSamplers;
        MaterialConstants matConstants;

        MeshProperty(): modelMatrix(glm::mat4(1.0f)),
                        usedSamplers(voko_global::EMeshSamplerFlags::ALL),
                        matConstants()
        {}
    };


    /**
     * Light buffers
     */
    struct alignas(16) DirectionalLight
    {
        glm::vec4 direction;
        glm::vec4 color;
        float intensity;

        DirectionalLight():
        direction(glm::vec4(1.0f)),color(glm::vec4(1.0f)), intensity(1.0f){}
        DirectionalLight(glm::vec4 _direction, glm::vec4 _color, float _intensity):
        direction(_direction), color(_color), intensity(_intensity){}
    };

    struct alignas(16) PointLight
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
        position(glm::vec4(0.0f)),
        target(glm::vec4(0.0f)),
        color(glm::vec4(1.0f)),
        viewMatrix(glm::mat4(0.0f)),
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
        uint32_t lightModel = 0; // 0:PBR, 1:Blinn-Phong, 2:Phong
        uint32_t dirLightCount = 0;
        DirectionalLight dirLights[voko_global::DIR_LIGHT_MAX];
        uint32_t spotLightCount = 0;
        SpotLight spotLights[voko_global::SPOT_LIGHT_MAX];

        float ambientCoef = 0.1f;

        uint32_t useIBL = 1;

        // shadow
        uint32_t useShadows = 1;
        uint32_t shadowFilterMethod = 1; // 0:Fixed size shadow factor, 1:PCF, 2:PCSS
        float shadowFactor = 0.1f;
    };
    // debug switch & offs
    struct UniformBufferDebug {
        uint32_t debugGBuffer = 0; // 0:off, 1:shadow, 2:fragPos, 3:normal, 4:albedo.rgb, 5:albedo.aaa
        uint32_t debugLighting = 0; // 0:off, 1:ambient, 2:diffuse, 3:specular,
    };

    struct UniformBufferScene
    {
        UniformBufferView view;
        UniformBufferLighting lighting;
        UniformBufferDebug debug;

        UniformBufferScene():view(),lighting(),debug(){}
    };
}


