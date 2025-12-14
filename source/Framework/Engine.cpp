#include "Framework/Engine.h"
#include "Framework/Editor.h"
#include "Framework/Input.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/MathUtility.h"
#include "Utilities/Utility.h"

#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Framework/Editor.h"

#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace EngineInternal
{
	bool doResize = false;
}
using namespace EngineInternal;

/// <summary>
/// コンストラクタ
/// </summary>
/// <param name="width"> ウィンドウの横幅 </param>
/// <param name="height"> ウィンドウの縦幅 </param>
Engine::Engine(uint32_t width, uint32_t height)
{
	RegisterWindowClass();
	m_pRenderer = std::make_unique<Renderer>(width, height);
	m_pActiveScene = std::make_unique<Scene>(m_pRenderer.get(), width, height);
	m_pEditor = std::make_unique<Editor>(m_pActiveScene.get());

	m_pRenderer->SetScene(m_pActiveScene.get());
	m_pEditor->SetScene(m_pActiveScene.get());

	t0 = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now().time_since_epoch());
}

/// <summary>
/// デストラクタ
/// </summary>
Engine::~Engine()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

/// <summary>
/// 実行します．
/// </summary>
void Engine::Run()
{
	MainLoop();
	TermApplication();
}

void Engine::RegisterWindowClass()
{
	// ウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX); // WNDCLASSEX構造体のサイズを設定
	wc.style = CS_HREDRAW | CS_VREDRAW; // ウィンドウのスタイルを設定
	wc.lpfnWndProc = WindowProc; // ウィンドウプロシージャの関数ポインタを設定
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // アプリケーションのアイコンを設定
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND); // 背景色を設定
	wc.lpszMenuName = nullptr; // メニュー名を設定しない
	wc.lpszClassName = Utility::windowClassName.c_str(); // ウィンドウクラス名を設定
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // 小さいアイコンを設定

	// ウィンドウクラスの登録
	static auto atom = RegisterClassEx(&wc);
	assert(atom > 0);
}

/// <summary>
/// アプリ終了処理を行います．
/// </summary>
void Engine::TermApplication()
{
}

/// <summary>
/// メインループです.
/// </summary>
void Engine::MainLoop()
{
	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			// メッセージを変換
			TranslateMessage(&msg);
			// メッセージをディスパッチ
			DispatchMessage(&msg);
		}
		else
		{
			Start();
			Update();
			Render();
		}
	}
}

void Engine::Start()
{
	if (doResize)
	{
		m_pRenderer->Resize();
		doResize = false;
	}
	auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now().time_since_epoch());
	deltaTime = (t1 - t0).count() * .001;
	t0 = t1;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_pRenderer->NewFrame();
}

void Engine::Update()
{
	m_pEditor->Update(deltaTime);
	m_pActiveScene->Update(deltaTime);
	m_pRenderer->Update(deltaTime);
}

void Engine::Render()
{
	ImGui::Render();
	m_pRenderer->Render();
}

/// <summary>
/// ウィンドウプロシージャです．
/// </summary>
/// <param name="hWnd"> ウィンドウハンドル </param>
/// <param name="message"> メッセージ </param>
/// <param name="wParam"> 追加のメッセージ情報 </param>
/// <param name="lParam"> 追加のメッセージ情報 </param>
/// <returns></returns>
LRESULT Engine::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0); // アプリケーションの終了を通知
			break;
		}
		case WM_SIZE:
		{
			doResize = true;
			break;
		}
	}

	// ImGui Windows callback //
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
