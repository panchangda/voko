/**
    .vh: voko header
    Scene Uniform Buffer Definitions 
*
*/

#ifndef UNIFORM_BUFFER_SCENE_DECLARED
#define UNIFORM_BUFFER_SCENE_DECLARED



struct UniformBufferView{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 viewProjectionMatrix;
    vec4 viewPos;
};

// Size macros must be same as CPU definitions
#define SPOT_LIGHT_MAX 3

struct SpotLight {
    vec4 position;
    vec4 target;
    vec4 color;
    mat4 viewMatrix;
    float range;
    float lightCosInnerAngle;
    float lightCosOuterAngle;
};

struct UniformBufferLighting{
    // lights
    uint lightModel;
    uint spotLightCount;
    SpotLight spotLights[SPOT_LIGHT_MAX];
    float ambientCoef;
    // shadows
    uint useShadows;
    uint shadowFilterMethod; // 0:PCF, 1:PCSS
    float shadowFactor;
};

struct UniformBufferDebug{
    uint debugGBuffer;
    uint debugLighting;
};

// declare ds set & binding; assign global const uboVar
layout (set = 0, binding = 0) uniform UniformBufferScene
{
    UniformBufferView view;
    UniformBufferLighting lighting;
    UniformBufferDebug debug;
} uboScene;


// e.g. translate uboView -> uboScene.view
#define uboView uboScene.view
#define uboLighting uboScene.lighting
#define uboDebug uboScene.debug

#endif // UNIFORM_BUFFER_SCENE_DECLARED