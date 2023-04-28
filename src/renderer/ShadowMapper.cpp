#include <glad/glad.h>
#include "Model.h"
#include "ecs/Registry.h"
#include "ecs/View.h"
#include "Renderer.h"
#include "core/Primatives.h"
#include "core/CameraSystem.h"
#include "Enviroment.h"
#include "ShadowMapper.h"

void ShadowMapper::Init(u32 textureSize, u32 shadowDist) {
	m_ShadowMap = DepthTexture(textureSize, textureSize);
	m_ShadowDist = shadowDist;
	m_LightViewProjection = glm::mat4(1.0f);
	m_TextureSize = textureSize;
	m_DepthShader = Shader("src/shaders/Depth.vert", "src/shaders/Depth.frag");

	glGenFramebuffers(1, &m_DepthFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_DepthFrameBuffer);
	m_ShadowMap.AttachToActiveFrameBuffer();

	// Need to explicitly tell OpenGL we're not using color
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapper::PerformShadowPass() {
	CalculateLightViewProjection();

	i32 viewportDimensions[4];
	glGetIntegerv(GL_VIEWPORT, viewportDimensions);
	i32 viewportWidth = viewportDimensions[2];
	i32 viewportHeight = viewportDimensions[3];

	glViewport(0, 0, m_TextureSize, m_TextureSize);
	glBindFramebuffer(GL_FRAMEBUFFER, m_DepthFrameBuffer);
	glClear(GL_DEPTH_BUFFER_BIT);

	m_DepthShader.Bind();
	m_DepthShader.SetMat4("viewProjection", m_LightViewProjection);

    for (auto entity : View<Transform, MeshRenderer>()) {
        auto transform = Registry::Get<Transform>(entity);
        auto meshRenderer = Registry::Get<MeshRenderer>(entity);
        
		m_DepthShader.SetMat4("model", transform->Model());

        glBindVertexArray(meshRenderer->mesh.vao);
        glDrawElements(GL_TRIANGLES, meshRenderer->mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, viewportWidth, viewportHeight);

	Enviroment::Instance()->SetLightViewProjection(m_LightViewProjection);
}

void ShadowMapper::CalculateLightViewProjection() {
	glm::vec3 lightDir = Enviroment::Instance()->GetLightDir();
	glm::vec3 center = CameraSystem::GetActiveTransform().Position();
	glm::mat4 view = glm::lookAt(center - lightDir, center, glm::vec3(0, 1, 0));

	f32 halfShadowDist = m_ShadowDist / 2.0f;
	glm::mat4 projection = glm::ortho(-halfShadowDist, halfShadowDist, -halfShadowDist, halfShadowDist, -halfShadowDist, halfShadowDist);

	m_LightViewProjection = projection * view;
}
