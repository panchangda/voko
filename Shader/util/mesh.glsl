/**
    .vh: voko header
    Per Mesh Buffer Definitions 
*
*/
#ifndef MESH_VH
#define MESH_VH

// define enum EMeshSamplerFlags
const uint ALBEDO 		= 0x01;
const uint NORMAL 		= 0x02;
const uint METALLIC 	= 0x04;
const uint ROUGHNESS  	= 0x08;
const uint AO 			= 0x10;
const uint ALL 			= 0xff;

struct MaterialConstants{
	vec4 rgba;
	float metallic;
	float roughness;
	float ao;
};

layout (set = 1, binding = 0) readonly buffer SSBOMeshProperty
{
	mat4 modelMatrix;
	uint usedSamplers;
	MaterialConstants matConstants;
} ssboMesh;


struct PerInstanceSSBO{
    vec4 instancePos;
};
layout (std430, set = 1, binding = 1) readonly buffer SSBOInstance
{
	PerInstanceSSBO instances[]; 
} ssboInstance;

layout (set = 1, binding = 2) uniform sampler2D samplerAlbedo;
layout (set = 1, binding = 3) uniform sampler2D samplerNormalMap;
layout (set = 1, binding = 4) uniform sampler2D samplerMetallic;
layout (set = 1, binding = 5) uniform sampler2D samplerRoughness;
layout (set = 1, binding = 6) uniform sampler2D samplerAO;

#endif // MESH_VH