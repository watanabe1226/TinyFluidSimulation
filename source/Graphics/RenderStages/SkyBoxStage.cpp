#include "Graphics/RenderStages/SkyBoxStage.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/DX12DescriptorHeap.h""
#include "Graphics/Camera.h""
#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Utilities/Utility.h"

SkyBoxStage::SkyBoxStage(Renderer* pRenderer) : RenderStage(pRenderer)
{
	CreateSkyBoxMesh();
	CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);

	auto path = L"assets/HDRI/testDome.hdr";
	m_pHDRITexture = std::make_unique<Texture>(pRenderer, path, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTestTexture = std::make_unique<Texture>(pRenderer, path);
}

SkyBoxStage::~SkyBoxStage()
{
}

void SkyBoxStage::RecordStage(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	if (m_pScene == nullptr)
	{
		assert(false && "シーンがnullです");
		return;
	}
	auto backBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	auto cbvHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pCmdList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
	pCmdList->SetPipelineState(m_pPSO->GetPipelineStatePtr());
	pCmdList->SetGraphicsRootDescriptorTable(0, gpuHandle);
	m_SkydomeTranBuufer.View = m_pCamera->GetView();
	auto pos = Vector3D(m_SkydomeTranBuufer.View.m_mat[3][0], m_SkydomeTranBuufer.View.m_mat[3][1], m_SkydomeTranBuufer.View.m_mat[3][2]);
	m_SkydomeTranBuufer.World = Matrix4x4::ScalingToMatrix(Vector3D(1000.0f, 1000.0f, 1000.0f)) * Matrix4x4::TransitionToMatrix(pos);
	m_SkydomeTranBuufer.Proj = m_pCamera->GetProj();
	pCmdList->SetGraphicsRoot32BitConstants(1, 48, &m_SkydomeTranBuufer, 0);

	pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
	pCmdList->IASetIndexBuffer(nullptr);
	pCmdList->DrawInstanced(36, 1, 0, 0);
}

void SkyBoxStage::SetScene(Scene* newScene)
{
	m_pScene = newScene;
	m_pCamera = m_pScene->GetCamera();
}

D3D12_GPU_DESCRIPTOR_HANDLE SkyBoxStage::GetSkyBoxGPUHandle()
{
	auto CBVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return CBVHeap->GetGpuHandle(m_pHDRITexture->GetSRVIndex());
}

D3D12_RESOURCE_DESC SkyBoxStage::GetHDRIDesc() const
{
	return D3D12_RESOURCE_DESC();
}

void SkyBoxStage::CreateSkyBoxMesh()
{
	Vector3D vertices[] = {
		Vector3D(-1.0f,  1.0f,  1.0f), Vector3D(1.0f,  1.0f,  1.0f), Vector3D(-1.0f, -1.0f,  1.0f),
		Vector3D(-1.0f, -1.0f,  1.0f), Vector3D(1.0f,  1.0f,  1.0f), Vector3D(1.0f, -1.0f,  1.0f),
		Vector3D(1.0f,  1.0f, -1.0f), Vector3D(-1.0f,  1.0f, -1.0f), Vector3D(1.0f, -1.0f, -1.0f),
		Vector3D(1.0f, -1.0f, -1.0f), Vector3D(-1.0f,  1.0f, -1.0f), Vector3D(-1.0f, -1.0f, -1.0f),
		Vector3D(1.0f,  1.0f,  1.0f), Vector3D(1.0f,  1.0f, -1.0f), Vector3D(1.0f, -1.0f,  1.0f),
		Vector3D(1.0f, -1.0f,  1.0f), Vector3D(1.0f,  1.0f, -1.0f), Vector3D(1.0f, -1.0f, -1.0f),
		Vector3D(-1.0f,  1.0f, -1.0f), Vector3D(-1.0f,  1.0f,  1.0f), Vector3D(-1.0f, -1.0f, -1.0f),
		Vector3D(-1.0f, -1.0f, -1.0f), Vector3D(-1.0f,  1.0f,  1.0f), Vector3D(-1.0f, -1.0f,  1.0f),
		Vector3D(-1.0f,  1.0f,  1.0f), Vector3D(-1.0f,  1.0f, -1.0f), Vector3D(1.0f,  1.0f,  1.0f),
		Vector3D(1.0f,  1.0f,  1.0f), Vector3D(-1.0f,  1.0f, -1.0f), Vector3D(1.0f,  1.0f, -1.0f),
		Vector3D(-1.0f, -1.0f, -1.0f), Vector3D(-1.0f, -1.0f,  1.0f), Vector3D(1.0f, -1.0f, -1.0f),
		Vector3D(1.0f, -1.0f, -1.0f), Vector3D(-1.0f, -1.0f,  1.0f), Vector3D(1.0f, -1.0f,  1.0f),
	};

	auto pDevice = m_pRenderer->GetDevice().Get();
	auto vertCount = static_cast<uint32_t>(sizeof(vertices) / sizeof(vertices[0]));
	auto vertSize = vertCount * sizeof(Vector3D);
	// ヒーププロパティ
	D3D12_HEAP_PROPERTIES vertProp = {};
	vertProp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込み可能なヒープ
	vertProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	vertProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	vertProp.CreationNodeMask = 1;
	vertProp.VisibleNodeMask = 1;

	// リソースの設定
	D3D12_RESOURCE_DESC vertDesc = {};
	vertDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertDesc.Alignment = 0;
	vertDesc.Width = static_cast<uint64_t>(vertSize); // 頂点データのサイズ
	vertDesc.Height = 1;
	vertDesc.DepthOrArraySize = 1;
	vertDesc.MipLevels = 1;
	vertDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertDesc.SampleDesc.Count = 1;
	vertDesc.SampleDesc.Quality = 0;
	vertDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// リソースを生成
	auto hr = pDevice->CreateCommittedResource(
		&vertProp,
		D3D12_HEAP_FLAG_NONE,
		&vertDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVB.GetAddressOf())
	);
	ThrowFailed(hr, "頂点バッファの生成に失敗しました");
	// マッピング
	void* ptr = nullptr;
	hr = m_pVB->Map(0, nullptr, &ptr);
	ThrowFailed(hr, "頂点バッファのマッピングに失敗しました");
	// 頂点データをマッピング先に設定
	memcpy(ptr, vertices, vertSize);

	// マッピング解除
	m_pVB->Unmap(0, nullptr);

	// 頂点バッファビューの設定
	m_VBV.BufferLocation = m_pVB->GetGPUVirtualAddress();
	m_VBV.SizeInBytes = static_cast<UINT>(vertSize);
	m_VBV.StrideInBytes = static_cast<UINT>(sizeof(Vector3D));
}

void SkyBoxStage::CreateRootSignature(Renderer* pRenderer)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// ルートパラメータの設定
	D3D12_DESCRIPTOR_RANGE range = {};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0; // t0
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ
	D3D12_ROOT_PARAMETER param[2] = {};

	// hdri tex
	param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].DescriptorTable.NumDescriptorRanges = 1;
	param[0].DescriptorTable.pDescriptorRanges = &range;
	param[0].Descriptor.RegisterSpace = 0;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// transform
	param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[1].Constants.ShaderRegister = 0;  // b0
	param[1].Constants.RegisterSpace = 0;
	param[1].Constants.Num32BitValues = 48; // matrix4x4 * 3
	param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// スタティックサンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = -D3D12_FLOAT32_MAX;
	samplerDesc.MaxLOD = +D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(param);
	desc.NumStaticSamplers = 1;
	desc.pParameters = param;
	desc.pStaticSamplers = &samplerDesc;
	desc.Flags = flag;

	// ルートシグネチャの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &desc);
}

void SkyBoxStage::CreatePipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定
	D3D12_INPUT_ELEMENT_DESC elements[1];
	elements[0].SemanticName = "POSITION";
	elements[0].SemanticIndex = 0;
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].InputSlot = 0;
	elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[0].InstanceDataStepRate = 0;

	// ラスタライザーステートの設定
	D3D12_RASTERIZER_DESC descRS;
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_NONE;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = FALSE;
	descRS.MultisampleEnable = FALSE;
	descRS.AntialiasedLineEnable = FALSE;
	descRS.ForcedSampleCount = 0;
	descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// レンダーターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	// ブレンドステートの設定
	D3D12_BLEND_DESC descBS;
	descBS.AlphaToCoverageEnable = FALSE;
	descBS.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBS.RenderTarget[i] = descRTBS;
	}

	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	// 頂点シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";
	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"SkyBoxVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr);

	// ピクセルシェーダー読み込み
	hr = D3DReadFileToBlob((ShaderFilePathName + L"SkyBoxPS.cso").c_str(), psBlob.GetAddressOf());
	ThrowFailed(hr);

	// 深度ステンシルステートの設定
	D3D12_DEPTH_STENCIL_DESC descDSS = {};
	descDSS.DepthEnable = TRUE;
	descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	descDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	descDSS.StencilEnable = FALSE;

	// パイプラインステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { elements, _countof(elements) };
	desc.pRootSignature = m_pRootSignature->GetRootSignaturePtr();
	desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	desc.RasterizerState = descRS;
	desc.BlendState = descBS;
	desc.DepthStencilState = descDSS;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// パイプラインステートの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}
