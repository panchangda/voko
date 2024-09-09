#version 450

#extension GL_ARB_shading_language_include : require
#include "../util/scene.glsl"

layout (set = 1, binding = 0) uniform sampler2D samplerposition;
layout (set = 1, binding = 1) uniform sampler2D samplerNormal;
layout (set = 1, binding = 2) uniform sampler2D samplerAlbedo;
layout (set = 1, binding = 3) uniform sampler2D samplerMetallic;
layout (set = 1, binding = 4) uniform sampler2D samplerRoughness;
layout (set = 1, binding = 5) uniform sampler2D samplerAO;
layout (set = 1, binding = 6) uniform sampler2DArray samplerShadowMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outfragColor;


/**
 * shadow helper
 */

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

/**
 * lighting helper
 *
 */
// PBR

// Normal Distribution function --------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float nom    = a2;
	float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
	denom        = PI * denom * denom;

	return nom / denom;
}
// Fresnel function ----------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 albedo, float metalness)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metalness);
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Geometric Shadowing function --------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness+1.0);
	float k = (r*r) / 8.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotV, roughness); // 视线方向的几何遮挡
	float ggx2 = GeometrySchlickGGX(NdotL, roughness); // 光线方向的几何阴影

	return ggx1 * ggx2;
}

vec3 PBR(vec3 N, vec3 V, vec3 L, float metalness, float roughness,
	vec3 albedo, vec3 lightColor, float intensity)
{
	vec3 H = normalize(V+L);
	float D = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	float cosTheta = max(dot(N, L), 0.0);
	vec3 F = fresnelSchlick(cosTheta, albedo, metalness);
	vec3 numerator    = D * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular     = numerator / max(denominator, 0.001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness; // 由于金属表面不折射光，没有漫反射颜色，通过归零kD来实现这个规则

	vec3 radiance = lightColor * intensity;
	// scale light by NdotL
	float NdotL = max(dot(N, L), 0.0);
	return (kD / PI * albedo  + specular) * radiance * NdotL;
}

// define shiness for phong specular calculations
const float shininess = 16.0;
vec3 blinnPhong(vec3 L, vec3 V, vec3 N, vec3 albedo, vec3 lightColor, float intensity){
	float NdotL = max(0.0, dot(N, L));
	float diff = NdotL;
	vec3 H = normalize(V+L);
	float NdotH = max(0.0, dot(N, H));
	float spec = pow(NdotH, shininess);
	return vec3(diff + spec) * intensity * lightColor * albedo;
}
vec3 phong(vec3 L, vec3 V, vec3 N, vec3 albedo, vec3 lightColor, float intensity, float coef){
	float NdotL = max(0.0, dot(N, L));
	float diff = NdotL;
	vec3 R = reflect(-L, N);
	float VdotR = max(0.0, dot(R, V));
	float spec = pow(VdotR, shininess) * coef; // coef = albedo.a * 2.5: same as `Vulkan` deferredshadows shader
	return vec3(diff + spec) * intensity * lightColor * albedo;
}


// ----------------------------------------------------------------------------
void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	float metallic = texture(samplerMetallic, inUV).r;
	float roughness = texture(samplerRoughness, inUV).r;
	float ao = texture(samplerAO, inUV).r;

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



	vec3 N = normalize(normal);

	vec3 V = uboView.viewPos.xyz - fragPos;
	V = normalize(V);

	// Ambient part
	vec3 fragColor = vec3(0.03) * albedo.rgb * ao;

	/*
	* Lighting:
	*/
	// directional lights
	vec4 dirLighted = vec4(0.0);
	for(uint i=0;i<uboLighting.dirLightCount;i++){
		DirectionalLight dir_light = uboLighting.dirLights[i];

		vec3 L = dir_light.direction.xyz;
		L = normalize(L);

		// add to outgoing radiance Lo
		vec3 Lo = vec3(0.0);
		switch (uboLighting.lightModel){
			case 0: // PBR
				fragColor += PBR(L, V, N, metallic, roughness, albedo.rgb, dir_light.color.rgb, dir_light.intensity);
				break;
			case 1: // Blinn-Phong
				fragColor += blinnPhong(L, V, N, albedo.rgb, dir_light.color.rgb, dir_light.intensity);
				break;
			case 2: // Phong
				fragColor += phong(L, V, N, albedo.rgb, dir_light.color.rgb, dir_light.intensity, albedo.a * 2.5);
				break;
			default:
				fragColor += vec3(0.0);
		}
	}

	// spot lights
	for(int i=0;i<uboLighting.spotLightCount;i++){

		SpotLight spot_light = uboLighting.spotLights[i];

		vec3 L = spot_light.position.xyz - fragPos;
		float dist = length(L);
		L = normalize(L);

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

		float intensity = spotEffect * heightAttenuation;

		switch (uboLighting.lightModel){
			case 0: // PBR
			fragColor += PBR(L, V, N, metallic, roughness, albedo.rgb, spot_light.color.rgb, intensity);
				break;
			case 1: // Blinn-Phong
				fragColor += blinnPhong(L, V, N, albedo.rgb, spot_light.color.rgb, intensity);
				break;
			case 2: // Phong
				fragColor += phong(L, V, N, albedo.rgb, spot_light.color.rgb, intensity, albedo.a * 2.5);
				break;
			default:
				fragColor += vec3(0.0);
		}
	}

	// Debug Lighting
//	if(uboDebug.debugLighting > 0){
//		switch(uboDebug.debugLighting){
//			case 1:
//			outfragColor.rgb = vec3(ambient);
//			break;
//			case 2:
//			outfragColor.rgb = vec3(diff);
//			break;
//			case 3:
//			outfragColor.rgb = vec3(spec);
//			break;
//		}
//		outfragColor.a = 1.0;
//		return;
//	}



	/**
	* Shadow Calculations
	*/
//	if(uboLighting.useShadows > 0){
//		fragColor = shadow(fragColor, fragPos);
//	}



	// HDR tonemapping
	fragColor = fragColor / (fragColor + vec3(1.0));
	// gamma correct
	fragColor = pow(fragColor, vec3(1.0/2.2));

	outfragColor = vec4(fragColor, 1.0);
}