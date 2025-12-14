#pragma once
#include "pch.h"


class Scene;
class Model;
class Texture;

class Editor
{
public:
	Editor(Scene* pScene);
	~Editor();

	void Update(float deltaTime);
	void SetScene(Scene* newScene);

private:
	void ImGuiStyleSettings();
	void LoadModelFilePaths(std::string path, std::string originalPath);
	void ModelSelectionWindow();
	float deltaTime;
	Scene* m_pScene = nullptr;
	std::vector<std::string> m_ModelFilePaths;
	std::vector<std::string> m_ComboDisplayNames;
	std::vector<std::string> m_DisplayModelNames;
	uint32_t m_CurrentModelId = 0;

	Model* hierachySelectedModel = nullptr;
};