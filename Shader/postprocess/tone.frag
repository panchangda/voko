#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"
#include "../util/color.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D samplerSceneColor;

void main()
{
    // Sample sceneColor as input
    vec3 color = texture(samplerSceneColor, inUV).rgb;

    // Tone mapping
    const float exposure = 4.5;
    color = Uncharted2Tonemap( color.rgb * exposure).rgb;
    color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));

    // Gamma correction
    color = linearToGamma(color);

    outFragColor = vec4(color, 1.0);
}

