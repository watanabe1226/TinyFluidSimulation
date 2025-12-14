#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Math/Vector2D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/MathUtility.h"

#include "Utilities/Utility.h"
#include "Graphics/DX12Device.h"
#include "Graphics/DX12Commands.h"
#include "Graphics/Window.h"
#include "Graphics/DX12DescriptorHeap.h"

#include "Graphics/Model.h"
#include "Graphics/ConstantBuffer.h"
#include "Graphics/Transform.h"
#include "Graphics/Texture.h"
#include "Graphics/Camera.h"
#include "Graphics/RenderStages/SceneStage.h"
#include "Graphics/RenderStages/ShadowStage.h"
#include "Graphics/RenderStages/SkyBoxStage.h"
#include "Graphics/RenderStages/SphereMapConverterStage.h"
#include "Graphics/RenderStages/IBLBakerStage.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

/// <summary>
/// コンストラクタ
/// </summary>
/// <param name="width"> ウィンドウの横幅 </param>
/// <param name="height"> ウィンドウの縦幅 </param>
Renderer::Renderer(uint32_t width, uint32_t height)
{
	// デバイスの生成
	m_pDevice = std::make_unique<DX12Device>();
	auto pDevice = m_pDevice->GetDevice().Get();
	// ディスクリプタヒープの生成
	m_pRTVHeap = std::make_unique<DX12DescriptorHeap>(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, "RTVHeap", 128);
	m_pDSVHeap = std::make_unique<DX12DescriptorHeap>(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, "DSVHeap", 8);
	m_pCBV_SRV_UAV = std::make_unique<DX12DescriptorHeap>(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "CBV_SRV_UAVHeap", 5000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	// コマンドの生成
	m_pDirectCommand = std::make_unique<DX12Commands>(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCopyCommand = std::make_unique<DX12Commands>(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);

	// ウィンドウの作成
	m_pWindow = std::make_unique<Window>(this, Utility::windowClassName, width, height);
	m_Width = width;
	m_Height = height;

	// ミッシング用テクスチャの作成
	m_pMissingTextures = std::make_unique<Texture>(this, Utility::GetCurrentDir() + L"/assets/textures/White_Missing.png");
	CreateConstantBuffer();

	InitializeImGui();

	// レンダーステージの作成
	m_pShadowStage = std::make_unique<ShadowStage>(this);
	m_pIBLBakerStage = std::make_unique<IBLBakerStage>(this);
	m_pSceneStage = std::make_unique<SceneStage>(this, m_pShadowStage.get(), m_pIBLBakerStage.get());
	m_pSkyBoxStage = std::make_unique<SkyBoxStage>(this);
	m_pSphereMapConverterStage = std::make_unique<SphereMapConverterStage>(this, m_pSkyBoxStage->GetHDRITex()->GetResource()->GetDesc());
	
	// SphericalMapをCubeMapに変換
	auto pCommandList = m_pDirectCommand->GetGraphicsCommandList().Get();
	// コマンドの記録を開始とリセット
	m_pDirectCommand->ResetCommand();
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	auto rtv = m_pWindow->GetCurrentScreenRTV();
	auto dsv = m_pWindow->GetDepthDSV();
	m_pSphereMapConverterStage->DrawToCube(pCommandList, m_pSkyBoxStage->GetSkyBoxGPUHandle());
	
	// ベイク処理
	m_pIBLBakerStage->IntegrateDFG(pCommandList);

	auto desc = m_pSphereMapConverterStage->GetCubeMapDesc();
	auto GPUHandle = m_pSphereMapConverterStage->GetCubeMapHandleGPU();
	m_pIBLBakerStage->IntegrateLD(pCommandList, static_cast<uint32_t>(desc.Width), desc.MipLevels, GPUHandle);
	
	// コマンドリストの実行
	m_pDirectCommand->ExecuteCommandList();
	// GPUの処理完了を待機
	m_pDirectCommand->WaitGpu(INFINITE);

}

Renderer::~Renderer()
{
}

void Renderer::NewFrame()
{
	// フレームごとの初期化処理
	m_CurrentOffset = 0;
}

/// <summary>
/// 描画処理
/// </summary>
void Renderer::Render()
{
	// コマンドリストを取得
	auto pCommandList = m_pDirectCommand->GetGraphicsCommandList().Get();
	// コマンドの記録を開始とリセット
	m_pDirectCommand->ResetCommand();

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetDescriptorHeaps(1, m_pCBV_SRV_UAV->GetHeap().GetAddressOf());
	// SceneのRender処理
	m_pShadowStage->RecordStage(pCommandList);

	// リソースバリアの設定
	TransitionResource(m_pWindow->GetCurrentScreenBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pSceneStage->RecordStage(pCommandList);
	m_pSkyBoxStage->RecordStage(pCommandList, m_pSphereMapConverterStage->GetCubeMapHandleGPU());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCommandList);

	// リソースバリアの設定
	TransitionResource(m_pWindow->GetCurrentScreenBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	// コマンドリストの実行
	m_pDirectCommand->ExecuteCommandList();

	// 画面に表示
	m_pWindow->Present(1);

	// GPUの処理完了を待機
	m_pDirectCommand->WaitGpu(INFINITE);
}

void Renderer::Update(float deltaTime)
{
	m_pShadowStage->Update(deltaTime);
}

void Renderer::SetScene(Scene* newScene)
{
	m_pScene = newScene;
	m_pSceneStage->SetScene(newScene);
	m_pShadowStage->SetScene(newScene);
	m_pSkyBoxStage->SetScene(newScene);
	for (const auto& model : m_pScene->GetModels())
	{
		if (model->GetName() == "assets/models/SciFiHelm/SciFiHelmet.gltf")
		{
			model->SetPosition(Vector3D(0.0f, 1.5f, 0.0f));
		}
	}
}

void Renderer::CreateTextureFromFile(const std::wstring& filePath)
{
	auto id = DX12Utility::StringHash(filePath.c_str());
	if (m_pTextures.find(id) != m_pTextures.end()) return;
	auto fullFilePath = Utility::GetCurrentDir() + L"/assets/textures/" + filePath;
	m_pTextures[id] = std::make_unique<Texture>(this, fullFilePath);
}

void Renderer::CreateConstantBuffer()
{
	uint32_t totalSize = m_MaxAllocations * CBAlignment;

	for (auto i = 0u; i < Window::FrameCount; ++i)
	{
		auto name = "CB_" + std::to_string(i);
		m_pCBs[i] = std::make_unique<ConstantBuffer>(m_pDevice->GetDevice().Get(), totalSize, name, nullptr);
	}
}

void Renderer::InitializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_pWindow->GetHwnd());

	const uint32_t cbvIndex = m_pCBV_SRV_UAV->GetNextAvailableIndex();
	ImGui_ImplDX12_Init(m_pDevice->GetDevice().Get(), Window::FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_pCBV_SRV_UAV->GetHeapPtr(), m_pCBV_SRV_UAV->GetCpuHandle(cbvIndex), m_pCBV_SRV_UAV->GetGpuHandle(cbvIndex));
}

void Renderer::Resize()
{
	m_pDirectCommand->WaitGpu(INFINITE);
	m_pWindow->Resize();
}

void Renderer::TransitionResource(ID3D12Resource* resource,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState)
{
	ID3D12GraphicsCommandList* pCmdList = m_pDirectCommand->GetGraphicsCommandList().Get();
	// リソースバリアの設定
	D3D12_RESOURCE_BARRIER barrior = {};
	barrior.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrior.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrior.Transition.pResource = resource;
	barrior.Transition.StateBefore = beforeState;
	barrior.Transition.StateAfter = afterState;
	barrior.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// リソースバリア
	pCmdList->ResourceBarrier(1, &barrior);
}

void Renderer::BindAndClearRenderTarget(Window* window,
	D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
	D3D12_CPU_DESCRIPTOR_HANDLE* depthStencil,
	float* clearColor)
{
	ID3D12GraphicsCommandList* pCmdList = m_pDirectCommand->GetGraphicsCommandList().Get();
	const float defaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (clearColor)
	{
		pCmdList->ClearRenderTargetView(*renderTarget, clearColor, 0, nullptr);
	}
	else
	{
		pCmdList->ClearRenderTargetView(*renderTarget, defaultClearColor, 0, nullptr);
	}


	if (depthStencil != nullptr)
	{
		pCmdList->ClearDepthStencilView(*depthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	auto viewport = window->GetViewport();
	auto scissor = window->GetScissorRect();
	pCmdList->RSSetViewports(1, &viewport);
	pCmdList->RSSetScissorRects(1, &scissor);
	pCmdList->OMSetRenderTargets(1, renderTarget, FALSE, depthStencil);
}

ComPtr<ID3D12Device> Renderer::GetDevice()
{
	if (m_pDevice == nullptr)
	{
		assert(false && "DXDevice hasn't been initialized yet, call will return nullptr");
	}

	return m_pDevice.get()->GetDevice();
}

DX12Commands* Renderer::GetCommands(D3D12_COMMAND_LIST_TYPE type)
{
	if (!m_pDirectCommand || !m_pCopyCommand)
	{
		assert(false && "Commands haven't been initialized yet, call will return nullptr");
	}

	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_pCopyCommand.get();
		break;

	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_pDirectCommand.get();
		break;
	}

	assert(false && "This command type hasn't been added yet!");
	return nullptr;

	return m_pDirectCommand.get();
}

DX12DescriptorHeap* Renderer::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	switch (type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return m_pCBV_SRV_UAV.get();
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return m_pDSVHeap.get();
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return m_pRTVHeap.get();
		break;
	}

	// Invalid type passed
	assert(false && "Descriptor type passed isn't a valid or created type!");
	return nullptr;
}

Texture* Renderer::GetTexture(TextureID id)
{
	if (m_pTextures.find(id) == m_pTextures.end())
	{
		return m_pMissingTextures.get();
	}
	return m_pTextures.at(id).get();;
}

Window* Renderer::GetWindow()
{
	return m_pWindow.get();
}
