#pragma once
#include "pch.h"
#include "Graphics/Window.h"
#include "Graphics/DX12Utilities.h"
#include "Graphics/ConstantBuffer.h"
#include "Math/Vector3D.h"

class DX12Device;
class Texture;
class DX12Commands;
class DX12DescriptorHeap;
class Scene;
class SceneStage;
class ShadowStage;
class SkyBoxStage;
class SphereMapConverterStage;
class IBLBakerStage;

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	~Renderer();
	void NewFrame();
	void Render();
	void Update(float deltaTime);
	void Resize();

	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS AllocateConstantBuffer(const T& data, uint32_t frameIndex)
	{
		// コンパイル時アサート: Tのサイズが256バイトを超えていないかチェック
		static_assert(sizeof(T) <= CBAlignment, "Constant Buffer size must be <= 256 bytes");

		// バッファあふれチェック
		if (m_CurrentOffset + CBAlignment > m_pCBs[frameIndex]->GetDesc().SizeInBytes)
		{
			assert(false && "CB Heap Full!");
			return 0;
		}

		// 書き込みポインタ取得
		uint8_t* pHeap = static_cast<uint8_t*>(m_pCBs[frameIndex]->GetPtr());
		uint8_t* pDest = pHeap + m_CurrentOffset;

		memcpy(pDest, &data, sizeof(T));

		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pCBs[frameIndex]->GetAddress() + m_CurrentOffset;

		// オフセットを進める
		m_CurrentOffset += CBAlignment;

		return gpuAddress;
	}
	DX12Commands* GetCommands(D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12Device> GetDevice();
	DX12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
	Texture* GetTexture(TextureID id);
	Window* GetWindow();
	const Vector3D& GetHalfVector3D() const { return m_HalfVector3D; }
	const Vector3D& GetOneVector3D() const { return m_OneVector3D; }
	const Vector3D& GetZeroVector3D() const { return m_ZeroVector3D; }
	
	void SetScene(Scene* newScene);

	void CreateTextureFromFile(const std::wstring& filePath);
	void TransitionResource(ID3D12Resource* resource,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState);
	void BindAndClearRenderTarget(Window* window,
		D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
		D3D12_CPU_DESCRIPTOR_HANDLE* depthStencil = nullptr,
		float* clearColor = nullptr);
private:
	void CreateConstantBuffer();
	void InitializeImGui();

	std::unique_ptr<Window> m_pWindow = nullptr;
	std::unique_ptr<DX12Device> m_pDevice = nullptr;
	std::unique_ptr<DX12Commands> m_pDirectCommand = nullptr;
	std::unique_ptr<DX12Commands> m_pCopyCommand = nullptr;

	std::unique_ptr<DX12DescriptorHeap> m_pRTVHeap = nullptr;
	std::unique_ptr<DX12DescriptorHeap> m_pDSVHeap = nullptr;
	std::unique_ptr<DX12DescriptorHeap> m_pCBV_SRV_UAV = nullptr;

	/// <summary>
	/// ハッシュ値をキーにして所有権(unique_ptr)を管理
	/// </summary>
	std::unordered_map<TextureID, std::unique_ptr<Texture>> m_pTextures;
	// デフォルトテクスチャ
	std::unique_ptr<Texture> m_pMissingTextures;

	/// <summary>
	/// 定数バッファの最大アロケーション数 (スロット数)
	/// </summary>
	static const uint32_t m_MaxAllocations = 20000;
	// <summary>
	// CBは256バイトアライメント必須
	// </summary>
	static const uint32_t CBAlignment = 256;
	// <summary>
	// バッファ全体のサイズ (20,000個 * 256byte = 約5MB)
	// </summary>
	static const uint32_t CBSize = m_MaxAllocations * CBAlignment;
	// <summary>
	// 定数バッファ
	// </summary>
	std::unique_ptr<ConstantBuffer> m_pCBs[Window::FrameCount];

	// <summary>
	// 現在の使用位置 (バイト単位のオフセット)
	// </summary>
	uint32_t m_CurrentOffset = 0;

	Vector3D m_HalfVector3D = Vector3D(0.5f, 0.5f, 0.5f);
	Vector3D m_OneVector3D = Vector3D(1.0f, 1.0f, 1.0f);
	Vector3D m_ZeroVector3D = Vector3D(1.0f, 1.0f, 1.0f);

	// シーン関連
	Scene* m_pScene = nullptr;
	std::unique_ptr<SceneStage> m_pSceneStage = nullptr;
	std::unique_ptr<ShadowStage> m_pShadowStage = nullptr;
	std::unique_ptr<SkyBoxStage> m_pSkyBoxStage = nullptr;
	std::unique_ptr<SphereMapConverterStage> m_pSphereMapConverterStage = nullptr;
	std::unique_ptr<IBLBakerStage> m_pIBLBakerStage = nullptr;

	uint32_t m_Width;
	uint32_t m_Height;
};
