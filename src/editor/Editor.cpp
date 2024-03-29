#include <string>
#include <glad/glad.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include "ecs/View.h"
#include "core/Components.h"
#include "core/Serializer.h"
#include "core/CameraSystem.h"
#include "renderer/Renderer.h"
#include "SceneCamera.h"
#include "Editor.h"

#include <iostream>

void Editor::Init(GLFWwindow* window) {
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", 19, nullptr, io.Fonts->GetGlyphRangesDefault());

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init((char*)glGetString(GL_VERSION_4_6));

	s_SelectedEntity = Entity::Null();

	s_SceneFrameBuffer = FrameBuffer(glm::i32vec2(600, 800), FrameBuffer::RGB);
	Renderer::s_FrameBuffer = &s_SceneFrameBuffer;

	CameraSystem::SetActiveCamera(&SceneCamera::s_Camera, &SceneCamera::s_Transform);
	s_TransGizmos = new TransformGizmos();
	s_ArrowMesh = Mesh::FromFile("Assets/Model/ArrowGizmo.fbx");
}

void Editor::Update() {
	DrawEditor();
}

void Editor::DrawEditor() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	//ImGui::ShowDemoWindow();

	DrawMenuBar();
	DrawScene();
	DrawWorld();
	DrawInspector();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backupCurrentContext);
	}
}

void Editor::DrawMenuBar() {
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File")) {
		ImGui::MenuItem("Save");
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void Editor::DrawScene() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	
	ImGui::Begin("Scene");
	ImGui::BeginChild("Scene Frame Buffer");

	const ImVec2 windowSize = ImGui::GetWindowSize();
	const glm::i32vec2 sceneWindowSize(windowSize.x, windowSize.y);

	const glm::vec2 sceneWindowPos = glm::vec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
	const ImVec2 mousePos = ImGui::GetMousePos();

	const glm::vec2 minScreenPos(sceneWindowPos.x, sceneWindowPos.y);
	const glm::vec2 maxScreenPos(sceneWindowPos.x + sceneWindowSize.x, sceneWindowPos.y + sceneWindowSize.y);

	const bool mouseXInsideScene = mousePos.x >= minScreenPos.x && mousePos.x <= maxScreenPos.x;
	const bool mouseYInsideScene = mousePos.y >= minScreenPos.y && mousePos.y <= maxScreenPos.y;
	const bool leftMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool leftMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

	static bool selectedGizmo = false;
	static bool needToSetStart = false;
	static i32 gizmoId = 0;
	static f32 lastDragZ = 0.0f;
	if (selectedGizmo && !leftMouseDown) {
		selectedGizmo = false;
	}

	static Shader s_SelectionShader("src/shaders/Selection.vert", "src/shaders/Selection.frag");
	static FrameBuffer s_SelectionBuffer(sceneWindowSize, FrameBuffer::RED_INTEGER);

	if (mouseXInsideScene && mouseYInsideScene && leftMouseClicked) {

		if (sceneWindowSize != s_SelectionBuffer.Size()) {
			s_SelectionBuffer.Resize(sceneWindowSize);
		}

		s_SelectionBuffer.Bind();
		s_SelectionBuffer.RedIntegerFill(-1);

		const auto view = View<LocalToWorld, MeshRenderer>();
		for (const auto entity : view) {
			auto& toWorld = Registry::Get<LocalToWorld>(entity);
			auto& meshRenderer = Registry::Get<MeshRenderer>(entity);
			s_SelectionShader.SetInt("entityId", entity.Id());
			Renderer::DrawMesh(meshRenderer, toWorld, s_SelectionShader);
		}

		if (s_SelectedEntity != Entity::Null()) {
			auto& ltw = s_SelectedEntity.Get<LocalToWorld>();
			auto gizmoTrans = ltw.ToTransform();

			gizmoTrans.rotation = glm::quatLookAt(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

			//f32 dist = glm::distance(CameraSystem::ActiveCamPos(), gizmoTrans.position);
			//gizmoTrans.scale = glm::vec3(dist * 0.03f);

			//s_SelectionShader.SetInt("entityId", -2);
			//Renderer::DrawMesh(s_ArrowGizmo, LocalToWorld::FromTransform(gizmoTrans), s_SelectionShader);

			s_TransGizmos->NotifyStartDrag();
			s_TransGizmos->SelectionDraw(gizmoTrans.position, s_SelectionShader);

			//s_TransGizmos->TransformArrow(-2, pos, glm::quat(glm::vec3(0.0, glm::radians(90.0), 0.0)), sceneWindowPos, usage);
			//s_TransGizmos->TransformArrow(-3, pos, glm::quat(glm::vec3(glm::radians(-90.0), 0.0, 0.0)), sceneWindowPos, usage);
			//s_TransGizmos->TransformArrow(-4, pos, glm::quat(glm::vec3(0)), sceneWindowPos, usage);

			//s_TransGizmos->TransformArrow(-2, gizmoTrans.position, gizmoTrans.rotation, sceneWindowPos, TransformGizmos::Selection);
		}

		const glm::vec2 pixelCoords(mousePos.x - sceneWindowPos.x, sceneWindowSize.y - (mousePos.y - sceneWindowPos.y));
		const i32 possibleEntityId = s_SelectionBuffer.ReadPixel(pixelCoords);

		if (possibleEntityId == -2 || possibleEntityId == -3 || possibleEntityId == -4) {
			selectedGizmo = true;
			needToSetStart = true;
			gizmoId = possibleEntityId;
		}
		else {
			s_SelectedEntity = possibleEntityId >= 0 ? Registry::m_Entities[possibleEntityId] : Entity::Null();
			gizmoId = 0;
		}

		s_SelectionBuffer.UnBind();
	}

	SceneCamera::Update(sceneWindowSize);
	glViewport(0, 0, sceneWindowSize.x, sceneWindowSize.y);

	if (sceneWindowSize != s_SceneFrameBuffer.Size()) {
		s_SceneFrameBuffer.Resize(sceneWindowSize);
	}
		
	Transform gizmoTrans;
	if (s_SelectedEntity != Entity::Null()) {
		gizmoTrans = s_SelectedEntity.Get<LocalToWorld>().ToTransform();
	}

	Renderer::PerformShadowPass();

	Renderer::s_FrameBuffer = &s_SceneFrameBuffer;
	s_SceneFrameBuffer.Bind();

	Renderer::PerformSkyboxPass();
	Renderer::RenderScene();

	if (s_SelectedEntity != Entity::Null()) {
		glm::vec3 gizmoPos = s_SelectedEntity.Get<LocalToWorld>().ToTransform().position;
	//	glm::vec3 movePos = s_TransGizmos->TransformHandles(gizmoPos, sceneWindowPos, TransformGizmos::Translation);
		s_TransGizmos->Draw(gizmoPos);

		if (selectedGizmo) {
			glm::vec3 movePos = s_TransGizmos->TransformArrow(gizmoId, gizmoPos, sceneWindowPos);
			glm::mat4 invParentLTW = glm::inverse(s_SelectedEntity.Get<Parent>().entity.Get<LocalToWorld>().matrix);
			s_SelectedEntity.Get<Transform>().position = invParentLTW * glm::vec4(movePos, 1.0f);
		}
	}

	s_SceneFrameBuffer.UnBind();

	ImGui::Image((ImTextureID)s_SceneFrameBuffer.Texture(), windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::PopStyleVar();
}

void Editor::DrawWorld() {
	ImGui::Begin("World", 0, ImGuiWindowFlags_NoCollapse);

	auto rootView = View<LocalToWorld, Transform, Children>().Exclude<Parent>();
	for (Entity& entity : rootView) {
		DrawEntityHierarchy(entity);
	}

	ImGui::End();
}

void Editor::DrawEntityHierarchy(Entity entity) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool isLeaf = !Registry::Has<Children>(entity);
	if (isLeaf) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	if (s_SelectedEntity != Entity::Null() && s_SelectedEntity == entity) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool open = ImGui::TreeNodeEx((void*)entity.Id(), flags, "Entity %d", entity.Id());

	if (ImGui::IsItemClicked()) {
		s_SelectedEntity = entity;
	}

	if (open && !isLeaf) {
		for (Entity& child : Registry::Get<Children>(entity).entities) {
			DrawEntityHierarchy(child);
		}
	}

	if (open) {
		ImGui::TreePop();
	}
}

void Editor::DrawInspector() {
	ImGui::Begin("Inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	if (s_SelectedEntity != Entity::Null()) {
		std::string entityName = "Selected Entity ";
		entityName.append(std::to_string(s_SelectedEntity.Id()));
		ImGui::Text(entityName.c_str());

		if (Registry::Has<Transform>(s_SelectedEntity)) {
			auto* trans = &Registry::Get<Transform>(s_SelectedEntity);
			ImGui::PushItemWidth(60);
			ImGui::DragFloat("x", &trans->position.x, 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("y", &trans->position.y, 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("z", &trans->position.z, 0.1f);
			ImGui::PopItemWidth();
		}
	}
	ImGui::End();
}
