#include <cassert>
#include <glad/glad.h>
#include "Renderer.h"
#include "core/Primatives.h"
#include "ecs/Registry.h"
#include "ecs/View.h"
#include "ShadowMapper.h"
#include "core/CameraSystem.h"
#include <iostream>

void Renderer::Init() {
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShadowMapper::PerformShadowPass();

    for (auto entity : View<Transform, MeshRenderer>()) {
        auto transform = entity.Get<Transform>();
        auto meshRenderer = entity.Get<MeshRenderer>();
        
        assert(meshRenderer->material);
		Material* mat = meshRenderer->material;

        mat->Bind(transform);

        glBindVertexArray(meshRenderer->mesh.vao);
        glDrawElements(GL_TRIANGLES, meshRenderer->mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

	DrawSkybox();
}

void Renderer::DrawSkybox() {
	static Mesh skyboxMesh = Primatives::Cube(true);
	static Shader skyboxShader = Shader("src/shaders/Skybox.vert", "src/shaders/Skybox.frag");

	glDepthFunc(GL_LEQUAL);

	skyboxShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	Enviroment::Instance()->m_Skybox->Bind();

	glBindVertexArray(skyboxMesh.vao);
	glDrawElements(GL_TRIANGLES, skyboxMesh.numIndices, GL_UNSIGNED_INT, 0);

	glDepthFunc(GL_LESS);
}

void Renderer::DebugDrawBounds(glm::vec3* points) {
	static Shader debugShader("src/shaders/Debug.vert", "src/shaders/Debug.frag");
	static Mesh cube = Primatives::Cube(false);

	debugShader.Bind();
	debugShader.SetMat4("model", glm::mat4(1.0f));

	for (int i = 0; i < 8; i++) {
		cube.verts[i].position = points[i];
	}
	cube.UpdateVertexBuffer();

	glBindVertexArray(cube.vao);
	glDrawElements(GL_LINE_LOOP, cube.numIndices, GL_UNSIGNED_INT, 0);
}

void Renderer::DebugDrawPoint(const glm::vec3 point) {
	static Shader debugShader("src/shaders/Debug.vert", "src/shaders/Debug.frag");
	static Mesh cube = Primatives::Cube(false);

	debugShader.Bind();

	glm::mat4 model(1.0);
	model = glm::translate(model, point);
	model = glm::scale(model, glm::vec3(0.1));

	debugShader.SetMat4("model", model);

	glBindVertexArray(cube.vao);
	glDrawElements(GL_TRIANGLES, cube.numIndices, GL_UNSIGNED_INT, 0);
}
