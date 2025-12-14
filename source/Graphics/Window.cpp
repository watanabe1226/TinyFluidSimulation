#include "Graphics/Window.h"
#include "Graphics/DX12Utilities.h"
#include "Graphics/DX12Commands.h"
#include "Graphics/DX12DescriptorHeap.h"
#include "Framework/Renderer.h"

Window::Window(Renderer* pRenderer, const std::wstring& applicationName, uint32_t width, uint32_t height)
	: m_WindowName(applicationName),
	m_Width(width),
	m_Height(height)
{
	m_pDevice = pRenderer->GetDevice().Get();
	m_pRTVHeap = pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDSVHeap = pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// RTVインデックスの取得
	for (int i = 0; i < FrameCount; i++)
	{
		m_pScreenBufferRTVs[i] = m_pRTVHeap->GetNextAvailableIndex();
	}
	m_DepthBufferDSV = m_pDSVHeap->GetNextAvailableIndex();

	SetupWindow();
	CreateSwapChain(pRenderer);

	m_pScreenBuffers.resize(FrameCount);
	UpdateRenderBuffers();
	UpdateScreenBuffers();
	UpdateDepthBuffer();

	// ビューポートとシザー矩形の設定
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = static_cast<float>(m_Width);
	m_Viewport.Height = static_cast<float>(m_Height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_Scissor.left = 0;
	m_Scissor.right = static_cast<LONG>(m_Width);
	m_Scissor.top = 0;
	m_Scissor.bottom = static_cast<LONG>(m_Height);

	// ウィンドウを表示
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	// ウィンドウを更新
	UpdateWindow(m_hWnd);
	// ウィンドウにフォーカスを設定
	SetFocus(m_hWnd);
}

Window::~Window()
{
	// ウィンドウの登録を解除
	if (m_HInstance != nullptr)
	{
		UnregisterClass(m_WindowName.c_str(), m_HInstance);
	}

	m_HInstance = nullptr;
	m_hWnd = nullptr;
}

/// <summary>
/// 画面に表示し、次のフレームの準備を行います．
/// </summary>
/// <param name="interval"> ディスプレイの垂直同期とフレームの表示を同期する方法 0 : 同期なし 1 : 同期あり</param>
void Window::Present(uint32_t interval)
{
	// 画面に表示
	m_pSwapChain->Present(interval, 0);
}

void Window::Resize()
{
	RECT clientRect = {};
	GetClientRect(m_hWnd, &clientRect);

	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	if (m_Width != width || m_Height != height)
	{
		m_Width = width > 1 ? width : 1;
		m_Height = height > 1 ? height : 1;
		m_Viewport.TopLeftX = 0.0f;
		m_Viewport.TopLeftY = 0.0f;
		m_Viewport.Width = static_cast<float>(m_Width);
		m_Viewport.Height = static_cast<float>(m_Height);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;

		m_Scissor.left = 0;
		m_Scissor.right = static_cast<LONG>(m_Width);
		m_Scissor.top = 0;
		m_Scissor.bottom = static_cast<LONG>(m_Height);

		UpdateRenderBuffers();
		UpdateScreenBuffers();
		UpdateDepthBuffer();
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentScreenRTV() const
{
	return m_pRTVHeap->GetCpuHandle(m_pScreenBufferRTVs[GetCurrentBackBufferIndex()]);
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetDepthDSV() const
{
	return m_pDSVHeap->GetCpuHandle(m_DepthBufferDSV);
}

ID3D12Resource* Window::GetCurrentScreenBuffer() const
{
	return m_pScreenBuffers[GetCurrentBackBufferIndex()].Get();
}

uint32_t Window::GetCurrentBackBufferIndex() const
{
	return m_pSwapChain->GetCurrentBackBufferIndex();
}

void Window::SetupWindow()
{
	// インスタンスハンドルを取得
	auto hInst = GetModuleHandle(nullptr);
	assert(hInst != nullptr);

	// インスタンスハンドルの設定
	m_HInstance = hInst;

	// ウィンドウのサイズを設定
	RECT rect = {};
	rect.right = static_cast<LONG>(m_Width);
	rect.bottom = static_cast<LONG>(m_Height);

	// ウィンドウサイズを調整
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	style = WS_OVERLAPPEDWINDOW;
	AdjustWindowRect(&rect, style, FALSE);

	// ウィンドウを生成
	m_hWnd = CreateWindowEx(
		0,
		m_WindowName.c_str(),
		m_WindowName.c_str(),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		m_HInstance,
		nullptr
	);
	assert(m_hWnd != nullptr);

	GetWindowRect(m_hWnd, &rect);
}

void Window::CreateSwapChain(Renderer* pRenderer)
{
	// DXGIファクトリーの生成
	ComPtr<IDXGIFactory4> pFactory = nullptr;
	auto hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
	ThrowFailed(hr);

	// スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferDesc.Width = m_Width;
	desc.BufferDesc.Height = m_Height;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = FrameCount;
	desc.OutputWindow = m_hWnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// スワップチェーンの生成
	ComPtr<IDXGISwapChain> pSwapChain = nullptr;
	ID3D12CommandQueue* pCommandQueue = pRenderer->GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetCommandQueue().Get();
	hr = pFactory->CreateSwapChain(pCommandQueue, &desc, pSwapChain.GetAddressOf());
	ThrowFailed(hr);

	// IDXGISwapChain3を取得
	hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.GetAddressOf()));
	ThrowFailed(hr);
}

void Window::UpdateRenderBuffers()
{
}

void Window::UpdateScreenBuffers()
{
	// 既存の画面バッファを解放
	for (uint32_t i = 0; i < FrameCount; ++i)
	{
		m_pScreenBuffers[i].Reset();
	}

	// スワップチェーンのリサイズ
	DXGI_SWAP_CHAIN_DESC desc = {};
	ThrowFailed(m_pSwapChain->GetDesc(&desc));
	ThrowFailed(m_pSwapChain->ResizeBuffers(FrameCount, m_Width, m_Height, desc.BufferDesc.Format, desc.Flags));

	// RTVの作成
	for (uint32_t i = 0; i < FrameCount; ++i)
	{
		// スワップチェーンから画面バッファを取得
		ThrowFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pScreenBuffers[i].GetAddressOf())));

		// RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = desc.BufferDesc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		// RTVの作成
		m_pDevice->CreateRenderTargetView(
			m_pScreenBuffers[i].Get(),
			&rtvDesc,
			m_pRTVHeap->GetCpuHandle(m_pScreenBufferRTVs[i])
		);
	}

}

void Window::UpdateDepthBuffer()
{
	m_pDepthBuffer.Reset();

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0;
	resDesc.Width = m_Width;
	resDesc.Height = m_Height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// DSVの作成
	auto hr = m_pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(m_pDepthBuffer.GetAddressOf())
	);
	m_pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &viewDesc, m_pDSVHeap->GetCpuHandle(m_DepthBufferDSV));
}
