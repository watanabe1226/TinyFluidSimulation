#pragma once
#include "pch.h"

class Renderer;
class DX12DescriptorHeap;

class DepthBuffer
{
public:
	DepthBuffer(Renderer* pRenderer, uint32_t width, uint32_t height);
	~DepthBuffer();

	void Resize(uint32_t width, uint32_t height);

	ID3D12Resource* GetResource() { return m_pDepthBuffer.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV();
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV();
	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
private:
	uint32_t m_Width = 1;
	uint32_t m_Height = 1;

	ComPtr<ID3D12Resource> m_pDepthBuffer = nullptr;
	uint32_t m_DepthDSVIndex = -1;
	uint32_t m_DepthSRVIndex = -1;
	
	Renderer* m_pRenderer = nullptr;
	DX12DescriptorHeap* m_pDSVHeap = nullptr;
	DX12DescriptorHeap* m_pSRVHeap = nullptr;
};