#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/mesh.glsl"

layout (location = 0) in vec4 inPos;

layout (location = 0) out int outInstanceIndex;

void main()
{
	outInstanceIndex = gl_InstanceIndex;

	gl_Position =  ssboMesh.modelMatrix * inPos;
}