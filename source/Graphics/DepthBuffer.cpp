#include "Graphics/DepthBuffer.h"
#include "Framework/Renderer.h"
#include "Graphics/DX12DescriptorHeap.h"

DepthBuffer::DepthBuffer(Renderer* pRenderer, uint32_t width, uint32_t height)
	: m_pRenderer(pRenderer), m_Width(width), m_Height(height)
{
	m_pDSVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_pSRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_DepthDSVIndex = m_pDSVHeap->GetNextAvailableIndex();
	m_DepthSRVIndex = m_pSRVHeap->GetNextAvailableIndex();

	Resize(width, height);
}

DepthBuffer::~DepthBuffer()
{
}

void DepthBuffer::Resize(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	const DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT;

	ID3D12Device* pDevice = m_pRenderer->GetDevice().Get();

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil = { 1.0f, 0 };

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 0;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Alignment = 0;

	auto hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
		IID_PPV_ARGS(m_pDepthBuffer.GetAddressOf()));
	ThrowFailed(hr);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &dsvDesc, m_pDSVHeap->GetCpuHandle(m_DepthDSVIndex));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	pDevice->CreateShaderResourceView(m_pDepthBuffer.Get(), &srvDesc, m_pSRVHeap->GetCpuHandle(m_DepthSRVIndex));
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSV()
{
	return m_pDSVHeap->GetCpuHandle(m_DepthDSVIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE DepthBuffer::GetSRV()
{
	return m_pSRVHeap->GetGpuHandle(m_DepthSRVIndex);
}
