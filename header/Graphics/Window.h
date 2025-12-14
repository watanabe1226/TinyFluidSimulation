#pragma once
#include "pch.h"

class DX12DescriptorHeap;
class Renderer;

class Window
{
public:
	Window(Renderer* pRenderer, const std::wstring& applicationName, uint32_t width, uint32_t height);
	~Window();
	void Present(uint32_t interval);
	void Resize();

	/// <summary>
	/// フレームバッファ数
	/// </summary>
	static const uint32_t FrameCount = 2;

	/// <summary>
	/// ゲッター類
	/// </summary>
	HWND GetHwnd() const { return m_hWnd; }
	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	D3D12_VIEWPORT GetViewport() const { return m_Viewport; }
	D3D12_RECT GetScissorRect() const { return m_Scissor; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentScreenRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthDSV() const;
	ID3D12Resource* GetCurrentScreenBuffer() const;
	uint32_t GetCurrentBackBufferIndex() const;

private:

	void SetupWindow();
	void CreateSwapChain(Renderer* pRenderer);
	void UpdateRenderBuffers();
	void UpdateScreenBuffers();
	void UpdateDepthBuffer();
	/// <summary>
	/// インスタンスハンドル
	/// </summary>
	HINSTANCE m_HInstance;
	/// <summary>
	/// ウィンドウハンドル
	/// </summary>
	HWND m_hWnd;
	/// <summary>
	/// ウィンドウの横幅
	/// </summary>
	uint32_t m_Width;
	/// <summary>
	/// ウィンドウの縦幅
	/// </summary>
	uint32_t m_Height;
	/// <summary>
	/// スワップチェーン
	/// </summary>
	ComPtr<IDXGISwapChain3> m_pSwapChain;
	/// <summary>
	/// ビューポート
	/// </summary>
	D3D12_VIEWPORT m_Viewport;
	/// <summary>
	/// シザー矩形
	/// </summary>
	D3D12_RECT m_Scissor;

	/// <summary>
	/// ウィンドウの名前
	/// </summary>
	std::wstring m_WindowName;

	/// <summary>
	/// レンダーバッファー
	/// </summary>
	uint32_t m_RenderBufferRTVs[FrameCount];

	/// <summary>
	/// スクリーンバッファ
	/// </summary>
	std::vector<ComPtr<ID3D12Resource>> m_pScreenBuffers = {nullptr};
	uint32_t m_pScreenBufferRTVs[FrameCount];

	/// <summary>
	/// 深度バッファ
	/// </summary>
	ComPtr<ID3D12Resource> m_pDepthBuffer;
	uint32_t m_DepthBufferDSV;

	ID3D12Device* m_pDevice = nullptr;
	DX12DescriptorHeap* m_pRTVHeap = nullptr;
	DX12DescriptorHeap* m_pDSVHeap = nullptr;
};