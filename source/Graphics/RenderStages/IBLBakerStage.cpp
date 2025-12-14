#include "Graphics/RenderStages/IBLBakerStage.h"
#include "Framework/Renderer.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/DX12DescriptorHeap.h"
#include "Graphics/ConstantBuffer.h"
#include "Framework/Scene.h"
#include "Utilities/Utility.h"
#include "Math/MathUtility.h"
#include "Math/Vector2D.h"

#include <pix_win.h>

IBLBakerStage::IBLBakerStage(Renderer* pRenderer) : RenderStage(pRenderer)
{
	m_pRenderer = pRenderer;
	SRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CreateVBV();
	CreateDFGRTV();
	CreateDiffuseLDRTV();
	CreateSpecularLDRTV();
	CreateBakeDatas();
	CreateDFGRootSignature(pRenderer);
	CreateLDRootSignature(pRenderer);
	CreateDFGPipeline(pRenderer);
	CreateDiffuseLDPipeline(pRenderer);
	CreateSpecularLDPipeline(pRenderer);
}

IBLBakerStage::~IBLBakerStage()
{
}

void IBLBakerStage::CreateVBV()
{
	struct Vertex
	{
		Vector2D Position;
		Vector2D TexCoord;
	};

	Vertex vertices[] = {
		{ Vector2D(-1.0f,  1.0f), Vector2D(0.0f,  1.0f) },
		{ Vector2D(3.0f,  1.0f), Vector2D(2.0f,  1.0f) },
		{ Vector2D(-1.0f, -3.0f), Vector2D(0.0f, -1.0f) },
	};

	auto pDevice = m_pRenderer->GetDevice().Get();
	auto vertCount = static_cast<uint32_t>(sizeof(vertices) / sizeof(vertices[0]));
	auto vertSize = vertCount * sizeof(Vertex);
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
	m_VBV.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
}

void IBLBakerStage::CreateDFGRTV()
{
	auto pDevice = m_pRenderer->GetDevice().Get();
	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = DFGTextureSize;
	texDesc.Height = DFGTextureSize;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES props = {};
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R32G32_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	auto hr = pDevice->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_TexDFG.GetAddressOf()));
	ThrowFailed(hr);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	m_RTV_DFG_Index = RTVHeap->GetNextAvailableIndex();
	pDevice->CreateRenderTargetView(m_TexDFG.Get(), &rtvDesc, RTVHeap->GetCpuHandle(m_RTV_DFG_Index));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_SRV_DFG_Index = SRVHeap->GetNextAvailableIndex();
	pDevice->CreateShaderResourceView(m_TexDFG.Get(), &srvDesc, SRVHeap->GetCpuHandle(m_SRV_DFG_Index));
}

void IBLBakerStage::CreateDiffuseLDRTV()
{
	auto pDevice = m_pRenderer->GetDevice().Get();
	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = LDTextureSize;
	texDesc.Height = LDTextureSize;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES props = {};
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	auto hr = pDevice->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_TexDiffuseLD.GetAddressOf()));
	ThrowFailed(hr);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	m_RTV_DiffuseLD_Indeies.resize(6);
	for (auto i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		m_RTV_DiffuseLD_Indeies[i] = RTVHeap->GetNextAvailableIndex();
		pDevice->CreateRenderTargetView(m_TexDiffuseLD.Get(), &rtvDesc, RTVHeap->GetCpuHandle(m_RTV_DiffuseLD_Indeies[i]));
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0;

	m_SRV_DiffuseLD_Index = SRVHeap->GetNextAvailableIndex();
	pDevice->CreateShaderResourceView(m_TexDiffuseLD.Get(), &srvDesc, SRVHeap->GetCpuHandle(m_SRV_DiffuseLD_Index));
}

void IBLBakerStage::CreateSpecularLDRTV()
{
	auto pDevice = m_pRenderer->GetDevice().Get();
	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = LDTextureSize;
	texDesc.Height = LDTextureSize;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = MipCount;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES props = {};
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	auto hr = pDevice->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_TexSpecularLD.GetAddressOf()));
	ThrowFailed(hr);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	m_RTV_SpecularLD_Indeies.resize(6 * MipCount);
	auto idx = 0;
	for (auto i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		for (auto m = 0; m < MipCount; ++m)
		{
			rtvDesc.Texture2DArray.MipSlice = m;
			m_RTV_SpecularLD_Indeies[idx] = RTVHeap->GetNextAvailableIndex();
			pDevice->CreateRenderTargetView(m_TexSpecularLD.Get(), &rtvDesc, RTVHeap->GetCpuHandle(m_RTV_SpecularLD_Indeies[idx]));
			idx++;
		}
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MipLevels = MipCount;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0;

	m_SRV_SpecularLD_Index = SRVHeap->GetNextAvailableIndex();
	pDevice->CreateShaderResourceView(m_TexSpecularLD.Get(), &srvDesc, SRVHeap->GetCpuHandle(m_SRV_SpecularLD_Index));
}

void IBLBakerStage::CreateBakeDatas()
{
	const auto RoughnessStep = 1.0f / float(MipCount - 1);
	auto idx = 0;
	m_BakeCBDatas.resize(6 * MipCount);
	for (auto i = 0; i < 6; ++i)
	{
		auto roughness = 0.0f;

		for (auto m = 0; m < MipCount; ++m)
		{
			m_BakeCBDatas[idx].FaceIndex = i;
			m_BakeCBDatas[idx].MipCount = MipCount - 1;
			m_BakeCBDatas[idx].Roughness = roughness;
			m_BakeCBDatas[idx].Width = LDTextureSize;

			idx++;
			roughness += RoughnessStep;
		}
	}
}

void IBLBakerStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
}

/// <summary>
/// DFG項を積分します
/// </summary>
void IBLBakerStage::IntegrateDFG(ID3D12GraphicsCommandList* pCmdList)
{
	m_pRenderer->TransitionResource(m_TexDFG.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = float(DFGTextureSize);
	viewport.Height = float(DFGTextureSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.right = DFGTextureSize;
	scissor.top = 0;
	scissor.bottom = DFGTextureSize;

	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto pRTV = RTVHeap->GetCpuHandle(m_RTV_DFG_Index);
	pCmdList->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
	pCmdList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
	pCmdList->SetPipelineState(m_pPSO->GetPipelineStatePtr());
	pCmdList->RSSetViewports(1, &viewport);
	pCmdList->RSSetScissorRects(1, &scissor);

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
	pCmdList->IASetIndexBuffer(nullptr);
	pCmdList->DrawInstanced(3, 1, 0, 0);

	m_pRenderer->TransitionResource(m_TexDFG.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

/// <summary>
/// LD項を積分します
/// </summary>
void IBLBakerStage::IntegrateLD(ID3D12GraphicsCommandList* pCmdList,
	uint32_t mapSize,
	uint32_t mipCount,
	D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap)
{
	auto idx = 0;

	// 出力テクスチャのミップマップ数で割ってステップ数を求める.
	// ※ 入力テクスチャサイズによって mipCount != MipCount となることに注意.
	const auto RoughnessStep = 1.0f / (MipCount - 1);

	for (auto i = 0; i < 6; ++i)
	{
		auto roughness = 0.0f;

		// 出力テクスチャのミップマップ数分ループする.
		for (auto m = 0; m < MipCount; ++m)
		{
			m_BakeCBDatas[idx].FaceIndex = i;
			m_BakeCBDatas[idx].MipCount = float(mipCount - 1);  // 入力テクスチャのミップマップ数.
			m_BakeCBDatas[idx].Width = float(mapSize);       // 入力テクスチャのサイズ.
			m_BakeCBDatas[idx].Roughness = roughness * roughness;

			idx++;
			roughness += RoughnessStep;
		}
	}

	// ルートシグニチャを設定.
	pCmdList->SetGraphicsRootSignature(m_pLDRootSignature->GetRootSignaturePtr());

	// Diffuse LD項を積分します.
	IntegrateDiffuseLD(pCmdList, handleCubeMap);

	// Speclar LD項を積分します.
	IntegrateSpecularLD(pCmdList, handleCubeMap);
}

void IBLBakerStage::IntegrateDiffuseLD(ID3D12GraphicsCommandList* pCmdList,
	D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap)
{
	m_pRenderer->TransitionResource(m_TexDiffuseLD.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = float(DFGTextureSize);
	viewport.Height = float(DFGTextureSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.right = DFGTextureSize;
	scissor.top = 0;
	scissor.bottom = DFGTextureSize;

	for (auto i = 0; i < 6; ++i)
	{
		PIXBeginEvent(pCmdList, 0, "IntegrateDiffuseLD%d", i);
		auto pRTV = RTVHeap->GetCpuHandle(m_RTV_DiffuseLD_Indeies[i]);
		pCmdList->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
		pCmdList->RSSetViewports(1, &viewport);
		pCmdList->RSSetScissorRects(1, &scissor);
		pCmdList->SetPipelineState(m_pDiffuseLDPSO->GetPipelineStatePtr());
		auto cbGpuAddress = m_pRenderer->AllocateConstantBuffer<CbBake>(m_BakeCBDatas[i * MipCount], 0);
		pCmdList->SetGraphicsRootConstantBufferView(0, cbGpuAddress);
		pCmdList->SetGraphicsRootDescriptorTable(1, handleCubeMap);

		pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
		pCmdList->IASetIndexBuffer(nullptr);
		pCmdList->DrawInstanced(3, 1, 0, 0);
		PIXEndEvent(pCmdList);
	}

	m_pRenderer->TransitionResource(m_TexDiffuseLD.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void IBLBakerStage::IntegrateSpecularLD(ID3D12GraphicsCommandList* pCmdList,
	D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap)
{
	m_pRenderer->TransitionResource(m_TexSpecularLD.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto idx = 0;
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor = {};
	scissor.left = 0;
	scissor.top = 0;

	for (auto i = 0; i < 6; ++i)
	{
		auto w = LDTextureSize;
		auto h = LDTextureSize;

		for (auto m = 0; m < MipCount; ++m)
		{
			viewport.Width = float(w);
			viewport.Height = float(h);

			scissor.right = viewport.Width;
			scissor.bottom = viewport.Height;

			auto pRTV = RTVHeap->GetCpuHandle(m_RTV_SpecularLD_Indeies[idx]);
			pCmdList->OMSetRenderTargets(1, &pRTV, FALSE, nullptr);
			pCmdList->RSSetViewports(1, &viewport);
			pCmdList->RSSetScissorRects(1, &scissor);
			pCmdList->SetPipelineState(m_pSpecularLDPSO->GetPipelineStatePtr());
			auto cbGpuAddress = m_pRenderer->AllocateConstantBuffer<CbBake>(m_BakeCBDatas[idx], 0);
			pCmdList->SetGraphicsRootConstantBufferView(0, cbGpuAddress);
			pCmdList->SetGraphicsRootDescriptorTable(1, handleCubeMap);

			pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
			pCmdList->IASetIndexBuffer(nullptr);
			pCmdList->DrawInstanced(3, 1, 0, 0);

			w >>= 1;
			h >>= 1;

			if (w < 1)
			{
				w = 1;
			}

			if (h < 1)
			{
				h = 1;
			}

			idx++;
		}
	}
	m_pRenderer->TransitionResource(m_TexSpecularLD.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBakerStage::GetHandleGPU_DFG() const
{
	return SRVHeap->GetGpuHandle(m_SRV_DFG_Index);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBakerStage::GetHandleGPU_DiffuseLD() const
{
	return SRVHeap->GetGpuHandle(m_SRV_DiffuseLD_Index);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBakerStage::GetHandleGPU_SpecularLD() const
{
	return SRVHeap->GetGpuHandle(m_SRV_SpecularLD_Index);
}

void IBLBakerStage::CreateDFGRootSignature(Renderer* pRenderer)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_DESCRIPTOR_RANGE range = {};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = 0;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_ANISOTROPIC;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
	sampler.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = MipCount;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_PARAMETER param = {};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param.Descriptor.RegisterSpace = 0;
	param.Descriptor.ShaderRegister = 0;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC sigDesc = {};
	sigDesc.NumParameters = 1;
	sigDesc.NumStaticSamplers = 1;
	sigDesc.pParameters = &param;
	sigDesc.pStaticSamplers = &sampler;
	sigDesc.Flags = flag;

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	auto hr = D3D12SerializeRootSignature(
		&sigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	ThrowFailed(hr);

	// ルートシグネチャの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &sigDesc);
}

void IBLBakerStage::CreateDFGPipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定 (POSITION + TEXCOORD)
	D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ラスタライザーステート (CullNone)
	D3D12_RASTERIZER_DESC descRS = {};
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_NONE;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = TRUE;
	descRS.MultisampleEnable = FALSE;
	descRS.AntialiasedLineEnable = FALSE;
	descRS.ForcedSampleCount = 0;
	descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// レンダーターゲットのブレンド設定 (Opaque)
	D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	D3D12_BLEND_DESC descBS = {};
	descBS.AlphaToCoverageEnable = FALSE;
	descBS.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBS.RenderTarget[i] = descRTBS;
	}

	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	// シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";

	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"QuadVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr, "QuadVS.cso read failed");

	hr = D3DReadFileToBlob((ShaderFilePathName + L"IntegrateDFG_PS.cso").c_str(), psBlob.GetAddressOf());
	ThrowFailed(hr, "IntegrateDFG_PS.cso read failed");

	// 深度ステンシルステート (DepthNone相当)
	D3D12_DEPTH_STENCIL_DESC descDSS = {};
	descDSS.DepthEnable = FALSE;
	descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	descDSS.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	descDSS.StencilEnable = FALSE;
	descDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	descDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_COMPARISON_FUNC_ALWAYS
	};
	descDSS.FrontFace = defaultStencilOp;
	descDSS.BackFace = defaultStencilOp;

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
	desc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}

void IBLBakerStage::CreateLDRootSignature(Renderer* pRenderer)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_DESCRIPTOR_RANGE range[1] = {};
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].NumDescriptors = 1;
	range[0].BaseShaderRegister = 0;
	range[0].RegisterSpace = 0;
	range[0].OffsetInDescriptorsFromTableStart = 0;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_ANISOTROPIC;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
	sampler.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = -D3D12_FLOAT32_MAX;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_PARAMETER param[2] = {};
	param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param[0].Descriptor.RegisterSpace = 0;
	param[0].Descriptor.ShaderRegister = 0;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[1].DescriptorTable.NumDescriptorRanges = 1;
	param[1].DescriptorTable.pDescriptorRanges = range;
	param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC sigDesc = {};
	sigDesc.NumParameters = 2;
	sigDesc.NumStaticSamplers = 1;
	sigDesc.pParameters = param;
	sigDesc.pStaticSamplers = &sampler;
	sigDesc.Flags = flag;

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	auto hr = D3D12SerializeRootSignature(
		&sigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	ThrowFailed(hr);

	// ルートシグネチャの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pLDRootSignature = std::make_unique<DX12RootSignature>(pDevice, &sigDesc);
}

void IBLBakerStage::CreateDiffuseLDPipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定 (POSITION + TEXCOORD)
	D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ラスタライザーステート (CullNone)
	D3D12_RASTERIZER_DESC descRS = {};
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_NONE;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = TRUE;
	descRS.MultisampleEnable = FALSE;
	descRS.AntialiasedLineEnable = FALSE;
	descRS.ForcedSampleCount = 0;
	descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// レンダーターゲットのブレンド設定 (Opaque)
	D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	D3D12_BLEND_DESC descBS = {};
	descBS.AlphaToCoverageEnable = FALSE;
	descBS.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBS.RenderTarget[i] = descRTBS;
	}

	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	// シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";

	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"QuadVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr, "QuadVS.cso read failed");

	hr = D3DReadFileToBlob((ShaderFilePathName + L"IntegrateDiffuseLD_PS.cso").c_str(), psBlob.GetAddressOf());
	ThrowFailed(hr, "IntegrateDiffuseLD_PS.cso read failed");

	// 深度ステンシルステート (DepthNone相当)
	D3D12_DEPTH_STENCIL_DESC descDSS = {};
	descDSS.DepthEnable = FALSE;
	descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	descDSS.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	descDSS.StencilEnable = FALSE;
	descDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	descDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_COMPARISON_FUNC_ALWAYS
	};
	descDSS.FrontFace = defaultStencilOp;
	descDSS.BackFace = defaultStencilOp;

	// パイプラインステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { elements, _countof(elements) };
	desc.pRootSignature = m_pLDRootSignature->GetRootSignaturePtr();
	desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	desc.RasterizerState = descRS;
	desc.BlendState = descBS;
	desc.DepthStencilState = descDSS;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pDiffuseLDPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}

void IBLBakerStage::CreateSpecularLDPipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定 (POSITION + TEXCOORD)
	D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ラスタライザーステート (CullNone)
	D3D12_RASTERIZER_DESC descRS = {};
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_NONE;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = TRUE;
	descRS.MultisampleEnable = FALSE;
	descRS.AntialiasedLineEnable = FALSE;
	descRS.ForcedSampleCount = 0;
	descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// レンダーターゲットのブレンド設定 (Opaque)
	D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	D3D12_BLEND_DESC descBS = {};
	descBS.AlphaToCoverageEnable = FALSE;
	descBS.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBS.RenderTarget[i] = descRTBS;
	}

	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	// シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";

	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"QuadVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr, "QuadVS.cso read failed");

	hr = D3DReadFileToBlob((ShaderFilePathName + L"IntegrateSpecularLD_PS.cso").c_str(), psBlob.GetAddressOf());
	ThrowFailed(hr, "IntegrateSpecularLD_PS.cso read failed");

	// 深度ステンシルステート (DepthNone相当)
	D3D12_DEPTH_STENCIL_DESC descDSS = {};
	descDSS.DepthEnable = FALSE;
	descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	descDSS.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	descDSS.StencilEnable = FALSE;
	descDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	descDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_KEEP,
		D3D12_COMPARISON_FUNC_ALWAYS
	};
	descDSS.FrontFace = defaultStencilOp;
	descDSS.BackFace = defaultStencilOp;

	// パイプラインステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { elements, _countof(elements) };
	desc.pRootSignature = m_pLDRootSignature->GetRootSignaturePtr();
	desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	desc.RasterizerState = descRS;
	desc.BlendState = descBS;
	desc.DepthStencilState = descDSS;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pSpecularLDPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}
