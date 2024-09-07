/**
    .vh: voko header
    Per Mesh Buffer Definitions 
*
*/

layout (set = 1, binding = 0) buffer SSBOMeshProperty
{
	mat4 modelMatrix;
} ssboMesh;


struct PerInstanceSSBO{
    vec4 instancePos;
};
layout (std430, set = 1, binding = 1) buffer SSBOInstance
{
	PerInstanceSSBO instances[]; 
} ssboInstance;

layout (set = 1, binding = 2) uniform sampler2D samplerColor;
layout (set = 1, binding = 3) uniform sampler2D samplerNormalMap;
