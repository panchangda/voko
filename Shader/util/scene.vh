/**
    vh: voko header
    Scene Uniform Buffer Definitions 
*
*/

// Same LIGHT_MAX Size as defined in CPU
#define LIGHT_MAX 3

struct SpotLight {
        vec4 position;
        vec4 target;
        vec4 color;
        mat4 viewMatrix;
};
layout (set = 0, binding = 0) uniform UniformBufferScene
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 viewProjectionMatrix;
	vec4 cameraPos;
	SpotLight lights[LIGHT_MAX];
} uboScene;