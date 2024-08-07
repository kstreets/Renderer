#include <glm/gtx/string_cast.hpp>
#include "Input.h"
#include "CameraSystem.h"
#include "Components.h"
#include "editor/Editor.h"

void CameraSystem::Init() {
	s_UniformBuffer = MakeScope<UniformBuffer>(0);
	s_UniformBuffer->Register("camPos", sizeof(glm::vec3));
	s_UniformBuffer->Register("view", sizeof(glm::mat4));
	s_UniformBuffer->Register("projection", sizeof(glm::mat4));
	s_UniformBuffer->Register("viewProjection", sizeof(glm::mat4));
	s_UniformBuffer->FinishedRegistering();
}

void CameraSystem::Update() {
    static Camera lastCamera;

    // Optamization: Don't update buffer unless camera data changed
    if (*s_ActiveCamera != lastCamera) {
        UpdateUniformBuffer();
    }

    lastCamera = *s_ActiveCamera;
}

void CameraSystem::SetActiveCamera(Camera* camera, Transform* transform) {
    s_ActiveCamera = camera;
    s_ActiveCameraTransform = transform;
    UpdateUniformBuffer();
}

FrustrumPoints CameraSystem::GetViewFrustumPoints(f32 zDist = 0.0f) {
    auto& trans = *s_ActiveCameraTransform;
    auto& cam = *s_ActiveCamera;

    f32 farDist = zDist == 0.0f ? cam.far : zDist;
    f32 nearDist = cam.near;
    f32 aspect = cam.aspect;
    f32 theta = glm::radians(cam.fov / 2.0f);

    const glm::vec3 pos = trans.position;
    const glm::vec3 forward = trans.Forward();
    const glm::vec3 right = trans.Right();
    const glm::vec3 up = trans.Up();

    // Calculate far plane positions

    f32 halfFarPlaneHeight = glm::tan(theta) * farDist;
    f32 halfFarPlaneWidth = halfFarPlaneHeight * aspect;

    const glm::vec3 farUp     = up * halfFarPlaneHeight;
    const glm::vec3 farRight  = right * halfFarPlaneWidth;
    const glm::vec3 farCenter = pos + (forward * farDist);

    glm::vec3 fTopRight    = farCenter + farRight + farUp;
    glm::vec3 fBottomRight = farCenter + farRight - farUp;
    glm::vec3 fTopLeft     = farCenter - farRight + farUp;
    glm::vec3 fBottomLeft  = farCenter - farRight - farUp;

    // Calculate near plane positions

    f32 halfNearPlaneHeight = glm::tan(theta) * nearDist;
    f32 halfNearPlaneWidth = halfNearPlaneHeight * aspect;

    const glm::vec3 nearUp     = up * halfNearPlaneHeight;
    const glm::vec3 nearRight  = right * halfNearPlaneWidth;
    const glm::vec3 nearCenter = pos + (forward * nearDist);

    glm::vec3 nTopRight    = nearCenter + nearRight + nearUp;
    glm::vec3 nBottomRight = nearCenter + nearRight - nearUp;
    glm::vec3 nTopLeft     = nearCenter - nearRight + nearUp;
    glm::vec3 nBottomLeft  = nearCenter - nearRight - nearUp;

    return {
        fTopRight, fBottomRight, fTopLeft, fBottomLeft,
        nTopRight, nBottomRight, nTopLeft, nBottomLeft
    };
}

f32 CameraSystem::ViewFrustumDiagonal(f32 zDist = 0.0f) {
    auto& trans = *s_ActiveCameraTransform;
    auto& cam = *s_ActiveCamera;

    f32 farDist = zDist == 0.0f ? cam.far : zDist;
    f32 nearDist = cam.near;
    f32 aspect = cam.aspect;
    f32 theta = glm::radians(cam.fov / 2.0f);

    const glm::vec3 pos = trans.position;
    const glm::vec3 forward = trans.Forward();
    const glm::vec3 right = trans.Right();
    const glm::vec3 up = trans.Up();

    // Calculate far plane top right

    f32 halfFarPlaneHeight = glm::tan(theta) * farDist;
    f32 halfFarPlaneWidth = halfFarPlaneHeight * aspect;

    const glm::vec3 farUp     = up * halfFarPlaneHeight;
    const glm::vec3 farRight  = right * halfFarPlaneWidth;
    const glm::vec3 farCenter = pos + (forward * farDist);
    glm::vec3 fTopRight = farCenter + farRight + farUp;

    // Calculate near plane bottom left

    f32 halfNearPlaneHeight = glm::tan(theta) * nearDist;
    f32 halfNearPlaneWidth = halfNearPlaneHeight * aspect;

    const glm::vec3 nearUp     = up * halfNearPlaneHeight;
    const glm::vec3 nearRight  = right * halfNearPlaneWidth;
    const glm::vec3 nearCenter = pos + (forward * nearDist);
    glm::vec3 nBottomLeft = nearCenter - nearRight - nearUp;

    return glm::distance(nBottomLeft, fTopRight);
}

glm::vec3 CameraSystem::ActiveCamPos() {
    return s_ActiveCameraTransform->position;
}

glm::vec3 CameraSystem::ActiveCamForward() {
    return s_ActiveCameraTransform->Forward();
}

glm::mat4 CameraSystem::ActiveCamViewProjection() {
    return s_ActiveCamera->ViewProjection();
}

glm::mat4 CameraSystem::ActiveCamView() {
    return s_ActiveCamera->view;
}

glm::mat4 CameraSystem::ActiveCamProjection() {
    return s_ActiveCamera->projection;
}

void CameraSystem::UpdateUniformBuffer() {
    glm::vec3 camPos = s_ActiveCameraTransform->position;
    glm::mat4 viewProjection = s_ActiveCamera->ViewProjection();

	s_UniformBuffer->SubBufferData("camPos", &camPos);
	s_UniformBuffer->SubBufferData("view", &s_ActiveCamera->view);
	s_UniformBuffer->SubBufferData("projection", &s_ActiveCamera->projection);
	s_UniformBuffer->SubBufferData("viewProjection", &viewProjection);
}
