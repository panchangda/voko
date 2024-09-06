#version 450

layout (binding = 1) uniform sampler2D samplerposition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 5) uniform sampler2DArray samplerShadowMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	
	outFragColor = vec4(fragPos, 1.0);
}