#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inTangent;


#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"
#include "../util/mesh.glsl"


layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;

void main() 
{
	vec4 tmpPos = vec4(inPos.xyz, 1.0) + ssboInstance.instances[gl_InstanceIndex].instancePos;

	mat4 modelMatrix = ssboMesh.modelMatrix;

	gl_Position = uboView.projectionMatrix * uboView.viewMatrix * modelMatrix * tmpPos;
	
	outUV = inUV;

	// Vertex position in world space
	outWorldPos = vec3(modelMatrix * tmpPos);

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(modelMatrix)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent.xyz);
	
	// Currently just vertex color
	outColor = inColor;
}
