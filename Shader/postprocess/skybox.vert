#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"

layout (location = 0) out vec3 outUVW;

void main()
{
    vec2 outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

    vec4 posClip = vec4(outUV * 2.0f - 1.0f, 1.0f, 1.0f);
    // default skybox mvp transform is `projection * mat4(mat3(view))`
    // now revert to cube world pos by inverse transform
    mat4 matClipToWorldNoTranslation = mat4(inverse(mat3(uboView.viewMatrix))) * inverse(uboView.projectionMatrix);
    vec4 vecView = matClipToWorldNoTranslation * posClip;
    outUVW = vecView.xyz / vecView.w;

    gl_Position = vec4(outUV * 2.0f - 1.0f, 1.0f, 1.0f);
}