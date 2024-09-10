#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main(){
    vec3 skyColor = texture(environmentMap, inUVW).rgb;
    outFragColor = vec4(skyColor, 1.0);
}