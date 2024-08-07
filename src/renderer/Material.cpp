#include <glad/glad.h>
#include "renderer/ShadowMapper.h"
#include "renderer/Enviroment.h"
#include "Material.h"

#include "Renderer.h"

Material::Material(Shader shader)
	: m_Shader(std::move(shader)), m_RenderOrder(RenderOrder::opaque), m_AlphaCutoff(0.0f),
	m_Metallicness(0.5f), m_Roughness(0.5f), m_Specularity(1.0f), m_Tiling(1.0f, 1.0f) { }

void Material::Bind(const LocalToWorld& toWorld) {
	m_Shader.Bind();
	m_Shader.SetMat4("model", toWorld.matrix);

	BindTextureIfExists("albedoMap", m_AlbedoTexture, 0);
	BindTextureIfExists("normalMap", m_NormalTexture, 1);
	BindTextureIfExists("metallicRoughnessMap", m_MetalRoughTexture, 2);

	bool alphaClippingEnabled = m_RenderOrder == RenderOrder::cutout || m_AlphaCutoff > 0.5f;
	m_Shader.SetInt("alphaClippingEnabled", alphaClippingEnabled);
	if (alphaClippingEnabled) {
		m_Shader.SetFloat("alphaCutoff", m_AlphaCutoff);
	}

	m_Shader.SetFloat("roughness", m_Roughness);
	m_Shader.SetFloat("specularStrength", m_Specularity);
	m_Shader.SetFloat("metallic", m_Metallicness);
	m_Shader.SetVec2("tiling", m_Tiling);
	
	ShadowMapper::BindShadowMap(3);
	m_Shader.SetInt("shadowMap", 3);

	Enviroment::Instance()->BindSkybox(4);
	m_Shader.SetInt("skybox", 4);

	Enviroment::Instance()->BindPcfShadow(5);
	m_Shader.SetInt("shadowPcfMap", 5);
}

void Material::BindTextureIfExists(const std::string& uniformName, const Ref<Texture> texture, const u32 textureUnit) {
	bool useTexture = texture != nullptr;
	m_Shader.SetInt(uniformName + "Enabled", useTexture);
	if (useTexture) {
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		texture->Bind();
		m_Shader.SetInt(uniformName, textureUnit);
	}
}

Material* Material::NewPbrMaterial() {
	static Shader pbrShader("src/shaders/PBR.vert", "src/shaders/PBR.frag");
	return new Material(pbrShader);
}