/**
    .vh: voko header
    Scene Uniform Buffer Definitions 
*
*/

#ifndef SCENE_VH
#define SCENE_VH


#define PI 3.1415926535897932384626433832795


struct UniformBufferView{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 viewProjectionMatrix;
    mat4 inverseViewMatrix;

    vec4 viewPos;
};

// Size macros must be same as CPU definitions
#define SHADOW_MAP_CASCADE_COUNT 4
#define SPOT_LIGHT_MAX 3
#define DIR_LIGHT_MAX 4
struct DirectionalLight{
    vec4 direction;
    vec4 color;
    float intensity;
};

struct SpotLight {
    vec4 position;
    vec4 target;
    vec4 color;
    mat4 viewMatrix;
    float range;
    float lightCosInnerAngle;
    float lightCosOuterAngle;
};
struct Cascade {
    float splitDepth;
    mat4 viewProjMatrix;
};
struct UniformBufferLighting{
    // lights
    uint lightModel;

    uint dirLightCount;
    DirectionalLight dirLights[DIR_LIGHT_MAX];

    uint spotLightCount;
    SpotLight spotLights[SPOT_LIGHT_MAX];

    float ambientCoef;

    uint useIBL;

    // shadows
    // cascade params
    Cascade cascade[SHADOW_MAP_CASCADE_COUNT];

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
// IBL:
layout (set = 0, binding = 1) uniform samplerCube environmentMap;
layout (set = 0, binding = 2) uniform samplerCube irradianceMap;
layout (set = 0, binding = 3) uniform sampler2D brdfLut;
layout (set = 0, binding = 4) uniform samplerCube prefilteredMap;


// e.g. translate uboView -> uboScene.view
#define uboView uboScene.view
#define uboLighting uboScene.lighting
#define uboDebug uboScene.debug


#endif // SCENE_VH