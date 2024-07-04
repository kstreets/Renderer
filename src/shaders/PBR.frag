﻿#version 460 core

in vec3 fragPos;
in vec3 modelNormal;
in vec2 textureCoord;
in mat3 tbn;
in vec4 lightFragPos;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D shadowMap;
uniform samplerCube skybox;

uniform bool albedoMapEnabled;
uniform bool normalMapEnabled;
uniform bool metallicRoughnessMapEnabled;
uniform bool alphaClippingEnabled;
uniform float alphaCutoff;

uniform float roughness;
uniform float specularStrength;
uniform float metallic;

layout (std140, binding = 0) uniform camera {
	vec3 camPos;
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
};

layout (std140, binding = 1) uniform enviorment {
    vec3 lightColor;
    vec3 ambientColor;
    float ambientStrength;
    float lightStrength;
    vec3 lightDir;
    mat4 lightViewProjection;
};

out vec4 fragColor;

#define PI 3.1415926538

struct BrdfData {
    vec3 color;
    vec3 normal;
    vec3 halfway;
    vec3 lightDir;
    vec3 viewDir;
    float metallic;
    float roughness;
};

float Sqr(float x) {
    return x * x;
}

vec3 SchlickFresnel(vec3 color, float viewDotHalf, float metallic) {
    vec3 f0 = mix(vec3(0.04f), color, metallic);
    return (f0 + (1.0f - f0) * pow(1.0f - viewDotHalf, 5));
}

float GGX(float normalDotHalf, float roughness) {
    float aSqr = Sqr(Sqr(roughness));
    return aSqr / (PI * Sqr(Sqr(normalDotHalf) * (aSqr - 1.0f) + 1.0f));
}

float GeometrySchlickGGX(float k, float cosTheta) {
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

float SmithGGX(float lightDotHalf, float viewDotHalf, float roughness) {
    float k = Sqr(roughness + 1) / 8.0f;
    float smithL = GeometrySchlickGGX(k, lightDotHalf);
    float smithV = GeometrySchlickGGX(k, viewDotHalf);
    return smithL * smithV;
}

float DisneyDiffuseFresnel(float f90, float theta) {
    float f0 = 1.0f;
    return mix(1.0f, f90, pow(clamp(f0 - theta, 0.0f, 1.0f), 5));
}

vec3 BRDF(BrdfData brdf) {
    float lightDotNormal = max(dot(brdf.lightDir, brdf.normal), 0.0f);
    float viewDotNormal  = max(dot(brdf.viewDir, brdf.normal), 0.0f);
    float lightDotHalf   = max(dot(brdf.lightDir, brdf.halfway), 0.0f);
    float normalDotHalf  = max(dot(brdf.normal, brdf.halfway), 0.0f);
    float viewDotHalf    = max(dot(brdf.viewDir, brdf.halfway), 0.0f);
    
    // Disney Diffuse
    float f90 = 0.5f + 2 * roughness * Sqr(lightDotHalf);
    float fresnelLight = DisneyDiffuseFresnel(f90, lightDotNormal);
    float fresnelView = DisneyDiffuseFresnel(f90, viewDotNormal);
    vec3 diffuse = (brdf.color / PI) * (fresnelLight * fresnelView);
     
    // Metallic
    diffuse *= (1.0f - brdf.metallic);
    
    // Specular
    float specularDist = GGX(normalDotHalf, brdf.roughness);
    float geometricAtt = SmithGGX(lightDotNormal, viewDotNormal, brdf.roughness);
    vec3 fresnel = SchlickFresnel(brdf.color, viewDotHalf, brdf.metallic);
    vec3 specular = specularDist * geometricAtt * fresnel * specularStrength;

    // Prevent the microfacet denominator from being 0
    float microfacetFactor = max(4.0f * lightDotNormal * viewDotNormal, 0.001f);
    return (diffuse + specular / microfacetFactor) * lightDotNormal;
}

void main() {
    vec4 albedoColorWithAlpha = vec4(1.0f);
    vec3 albedoColor = vec3(1.0f);
    
    if (albedoMapEnabled) {
        albedoColorWithAlpha = texture(albedoMap, textureCoord);
        albedoColor = albedoColorWithAlpha.rgb;
        if (alphaClippingEnabled && albedoColorWithAlpha.a < alphaCutoff) {
            discard;
        }
    }

    vec3 normal = modelNormal;
    if (normalMapEnabled) {
        normal = normalize(texture(normalMap, textureCoord).rgb * 2.0f - 1.0f);
        normal = normalize(tbn * normal);
    }
    
    float mappedMetallic = metallic;
    float mappedRoughness = roughness;
    if (metallicRoughnessMapEnabled) {
        vec3 metalRoughness = texture(metallicRoughnessMap, textureCoord).rgb;
        mappedMetallic = metalRoughness.b;
        mappedRoughness = metalRoughness.g;
    }
    
    vec3 lightNormal = -lightDir;
    vec3 viewDir = normalize(camPos - fragPos);
    
    BrdfData brdf;
    brdf.color = albedoColor;
    brdf.normal = normal;
    brdf.halfway = normalize(viewDir + lightNormal);
    brdf.lightDir = lightNormal;
    brdf.viewDir = viewDir;
    brdf.metallic = mappedMetallic * metallic;
    brdf.roughness = mappedRoughness * roughness;
    
    vec3 light = lightColor * lightStrength;
    vec3 ambient = ambientColor * albedoColor * ambientStrength;
     
    if (albedoMapEnabled) {
        vec3 finalColor = BRDF(brdf) * light + ambient;
        fragColor = vec4(finalColor, albedoColorWithAlpha.a);
    }
    else {
        fragColor = vec4(1.0f); 
    }
    
}