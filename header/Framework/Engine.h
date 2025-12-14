#pragma once
#include "pch.h"
#include "Math/Matrix4x4.h"

class Renderer;
class Scene;
class Editor;

class Engine
{
public:
	Engine(uint32_t width, uint32_t height);
	~Engine();
	void Run();
private:

	/// <summary>
	/// フレームバッファ数
	/// </summary>
	static const uint32_t FrameCount = 2;
	
	void RegisterWindowClass();
	void TermApplication();
	void MainLoop();
	void Start();
	void Update();
	void Render();

private:
	std::unique_ptr<Renderer> m_pRenderer = nullptr;
	std::unique_ptr<Scene> m_pActiveScene = nullptr;
	std::unique_ptr<Editor> m_pEditor = nullptr;

	// Time //
	float deltaTime = 1.0f;
	std::chrono::milliseconds t0;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};