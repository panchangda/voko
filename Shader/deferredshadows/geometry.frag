#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/mesh.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out float outMetallic;
layout (location = 4) out float outRoughness;
layout (location = 5) out float outAO;

void main() 
{
	outPosition = vec4(inWorldPos, 1.0);

	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	outNormal = vec4(tnorm, 1.0);

	// from matConstants
	outAlbedo = ssboMesh.matConstants.rgba;
	outMetallic = ssboMesh.matConstants.metallic;
	outRoughness = ssboMesh.matConstants.roughness;
	outAO = ssboMesh.matConstants.ao;

	// from textures
	if((ssboMesh.usedSamplers & ALBEDO) != 0){
		outAlbedo = texture(samplerAlbedo, inUV);
	}
	if((ssboMesh.usedSamplers & METALLIC) != 0){
		outMetallic = texture(samplerMetallic, inUV).r;
	}
	if((ssboMesh.usedSamplers & ROUGHNESS) != 0){
		outRoughness = texture(samplerRoughness, inUV).r;
	}
	if((ssboMesh.usedSamplers & AO) != 0){
		outAO = texture(samplerAO, inUV).r;
	}
}