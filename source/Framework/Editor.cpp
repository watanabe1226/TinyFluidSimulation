#include "Framework/Editor.h"
#include "Framework/Scene.h"

#include "Graphics/Model.h"

#include <imgui.h>

Editor::Editor(Scene* pScene)
{
	ImGuiStyleSettings();

	//LoadModelFilePaths("Assets/Models/", "Assets/Models/");
}

Editor::~Editor()
{
}
void Editor::Update(float deltaTime)
{
	this->deltaTime = deltaTime;

	ImGui::Begin("FPS");

	ImGui::Text("FPS: %f", 1 / deltaTime);
	ImGui::End();
}

void Editor::SetScene(Scene* newScene)
{
	m_pScene = newScene;
}

void Editor::ImGuiStyleSettings()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	// ÉXÉ^ÉCÉã
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScrollbarRounding = 2;
	style.ScrollbarSize = 12;
	style.WindowRounding = 3;
	style.WindowBorderSize = 0.0f;
	style.WindowTitleAlign = ImVec2(0.0, 0.5f);
	style.WindowPadding = ImVec2(5, 1);
	style.ItemSpacing = ImVec2(12, 5);
	style.FrameBorderSize = 0.5f;
	style.FrameRounding = 3;
	style.GrabMinSize = 5;

	// Color Wheel
	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR |
		ImGuiColorEditFlags_PickerHueBar);
}

void Editor::LoadModelFilePaths(std::string path, std::string originalPath)
{
	for (const auto& file : std::filesystem::directory_iterator(path))
	{
		if (file.is_directory())
		{
			LoadModelFilePaths(file.path().string(), originalPath);
		}

		std::string filePath = file.path().string();
		std::string fileType = filePath.substr(filePath.find_last_of(".") + 1, filePath.size());

		if (fileType == "gltf")
		{
			m_ComboDisplayNames.push_back(filePath.substr(filePath.find_last_of("\\") + 1));
			m_ModelFilePaths.push_back(filePath.c_str());
		}
	}
}

void Editor::ModelSelectionWindow()
{
	ImGui::Begin("Model Selection");
	std::string& selectedPath = m_ComboDisplayNames[m_CurrentModelId];
	if (ImGui::BeginCombo("Model File", selectedPath.c_str()))
	{
		for (auto i = 0; i < m_ComboDisplayNames.size(); ++i)
		{
			bool isSelected = m_CurrentModelId == i;

			if (ImGui::Selectable(m_ComboDisplayNames[i].c_str(), isSelected))
			{
				m_CurrentModelId = i;
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Load Model"))
	{
		const std::string& targetName = m_ModelFilePaths[m_CurrentModelId];
		const auto& models = m_pScene->GetModels();
		bool isAlreadyExists = std::any_of(models.begin(), models.end(),
			[&](const auto& model) {
				return model->GetName() == targetName;
			});
		if (!isAlreadyExists)
		{
			m_pScene->AddModel(m_ModelFilePaths[m_CurrentModelId]);
		}
	}

	ImGui::End();
}
