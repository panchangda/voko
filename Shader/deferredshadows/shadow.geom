#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"
#include "../util/mesh.glsl"


layout (triangles, invocations =
	SPOT_LIGHT_MAX + SHADOW_MAP_CASCADE_COUNT
) in;
layout (triangle_strip, max_vertices = 3) out;



layout (location = 0) in int inInstanceIndex[];

void main() 
{
	vec4 instancedPos = ssboInstance.instances[inInstanceIndex[0]].instancePos; 
	for (int i = 0; i < gl_in.length(); i++)
	{
		gl_Layer = gl_InvocationID;
		vec4 tmpPos = gl_in[i].gl_Position + instancedPos;
		if(gl_InvocationID < SPOT_LIGHT_MAX)
		{
			uint spotLightIndex = gl_InvocationID - 0;
			gl_Position = uboLighting.spotLights[gl_InvocationID].viewMatrix * tmpPos;
		}
		else if(gl_InvocationID < SPOT_LIGHT_MAX + SHADOW_MAP_CASCADE_COUNT)
		{
			uint cascadeIndex = gl_InvocationID - SPOT_LIGHT_MAX;
			gl_Position = uboLighting.cascade[cascadeIndex].viewProjMatrix * tmpPos;
		}

		EmitVertex();
	}
	EndPrimitive();
}
