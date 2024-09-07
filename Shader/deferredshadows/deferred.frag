#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"

layout (set = 1, binding = 1) uniform sampler2D samplerposition;
layout (set = 1, binding = 2) uniform sampler2D samplerNormal;
layout (set = 1, binding = 3) uniform sampler2D samplerAlbedo;
layout (set = 1, binding = 5) uniform sampler2DArray samplerShadowMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outfragColor;

float textureProj(vec4 P, float layer, vec2 offset)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
	{
		float dist = texture(samplerShadowMap, vec3(shadowCoord.st + offset, layer)).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z)
		{
			shadow = uboLighting.shadowFactor;
		}
	}
	return shadow;
}

float filterPCF(vec4 shadowClip, float layer)
{
	ivec2 texDim = textureSize(samplerShadowMap, 0).xy;
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(shadowClip, layer, vec2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
}
float filterPCSS(){
	return 0.0;
}

vec3 shadow(vec3 fragColor, vec3 fragPos) {
	// SpotLight Shadows:
	for(int i = 0; i < uboLighting.spotLightCount; ++i)
	{
		SpotLight spot_light = uboLighting.spotLights[i];

		vec4 shadowClip	= spot_light.viewMatrix * vec4(fragPos, 1.0);

		float shadowFactor;
		switch(uboLighting.shadowFilterMethod){
			case 0:
				shadowFactor = textureProj(shadowClip, i, vec2(0.0));
				break;
			case 1:
				shadowFactor = filterPCF(shadowClip, i);
				break;
			case 2:
				shadowFactor = filterPCSS();
				break;
		}
		fragColor *= shadowFactor;
	}
	return fragColor;
}

void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);


	// Debug GBuffer
	if(uboScene.debug.debugGBuffer > 0){
		switch (uboScene.debug.debugGBuffer) {
			case 1: 
				outfragColor.rgb = shadow(vec3(1.0), fragPos).rgb;
				break;
			case 2: 
				outfragColor.rgb = fragPos;
				break;
			case 3: 
				outfragColor.rgb = normal;
				break;
			case 4: 
				outfragColor.rgb = albedo.rgb;
				break;
			case 5: 
				outfragColor.rgb = albedo.aaa;
				break;
		}		
		outfragColor.a = 1.0;
		return;
	}

	// Ambient part
	vec3 fragColor = albedo.rgb * uboLighting.ambientCoef;

	vec3 N = normalize(normal);

	/*
	* Lighting Calculations:
	*/
	// spotLight
	for(int i=0;i<uboLighting.spotLightCount;i++){

		SpotLight spot_light = uboLighting.spotLights[i];

		vec3 L = spot_light.position.xyz - fragPos;
		float dist = length(L);
		L = normalize(L);
		vec3 V = uboView.viewPos.xyz - fragPos;
		V= normalize(V);
		vec3 H = normalize(V+L);
		float NdotL = max(0.0, dot(N, L));
		
		// Diffuse lighting
		vec3 diff = vec3(NdotL);
		// Specular Lighting
		vec3 spec = vec3(0.0);
		// Dual cone spot light with smooth transition between inner and outer angle
		vec3 dir = normalize(spot_light.position.xyz -  spot_light.target.xyz);
		float cosDir = dot(L, dir);
		float spotEffect = smoothstep(spot_light.lightCosOuterAngle, spot_light.lightCosInnerAngle, cosDir);
		float heightAttenuation = smoothstep(spot_light.range, 0.0f, dist);
		const float shininess = 16.0;

		if(uboLighting.lightModel == 0) // Default PBR Shading
		{

		}
		else if(uboLighting.lightModel == 1) // Blinn-Phong
		{
			float NdotH = max(0.0, dot(N, H));
			spec = vec3(pow(NdotH, shininess));
			fragColor += vec3((diff + spec) * spotEffect * heightAttenuation) * spot_light.color.rgb * albedo.rgb;

		}else if(uboLighting.lightModel == 2)  // Phong
		{
			vec3 R = reflect(-L, N);
			float VdotR = max(0.0, dot(R, V));
			spec = vec3(pow(VdotR, shininess) * albedo.a * 2.5); // same as `Vulkan` deferredshadows shader
			fragColor += vec3((diff + spec) * spotEffect * heightAttenuation) * spot_light.color.rgb * albedo.rgb;
		}

		// Debug Lighting
		if(uboDebug.debugLighting > 0){
			switch(uboDebug.debugLighting){
				case 1:
					outfragColor.rgb = vec3(uboLighting.ambientCoef);
					break;
				case 2:
					outfragColor.rgb = vec3(diff);
					break;
				case 3:
					outfragColor.rgb = vec3(spec);
					break;
			}
			outfragColor.a = 1.0;
			return;
		}
	}


	/**
	* Shadow Calculations
	*/
	if(uboLighting.useShadows > 0){
		fragColor = shadow(fragColor, fragPos);
	}
	outfragColor = vec4(fragColor, 1.0);
}