#pragma once
#include "pch.h"
#include "Graphics/Lights.h"

class Model;
class Camera;
class Renderer;

class Scene
{
public:
	Scene(Renderer* pRenderer, uint32_t width, uint32_t height);
	~Scene();
	void Update(float deltaTime);

	void AddModel(const std::string filePath);
	Camera* GetCamera() const;
	const std::vector<std::unique_ptr<Model>>& GetModels() const;
	const LightData& GetLightData();

private:
	std::vector<std::unique_ptr<Model>> m_pModels;
	LightData m_LightData;
	bool m_IsEditedLight = false;
	float m_SceneRuntime = 0.f;

	Renderer* m_pRenderer = nullptr;

	// ÉJÉÅÉâä÷åW
	std::unique_ptr<Camera> m_pCamera = nullptr;
	float m_CameraSpeed = 12.0f;
	float m_CameraSpeedMultiplier = 6.0f;

};