#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"
#include "../util/mesh.glsl"


layout (triangles, invocations = SPOT_LIGHT_MAX) in;
layout (triangle_strip, max_vertices = 3) out;



layout (location = 0) in int inInstanceIndex[];

void main() 
{
	vec4 instancedPos = ssboInstance.instances[inInstanceIndex[0]].instancePos; 
	for (int i = 0; i < gl_in.length(); i++)
	{
		gl_Layer = gl_InvocationID;
		vec4 tmpPos = gl_in[i].gl_Position + instancedPos;
		gl_Position = uboLighting.spotLights[gl_InvocationID].viewMatrix * tmpPos;
		EmitVertex();
	}
	EndPrimitive();
}
