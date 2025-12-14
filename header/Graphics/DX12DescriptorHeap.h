#pragma once
#include "pch.h"

class DX12DescriptorHeap
{
public:
	DX12DescriptorHeap(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE type, 
		const std::string& DescriptorName,
		uint32_t numDescriptors, 
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	/// <summary>
	/// ÉQÉbÉ^Å[óﬁ
	/// </summary>
	ComPtr<ID3D12DescriptorHeap> GetHeap() { return m_pDescriptorHeap; }
	ID3D12DescriptorHeap* GetHeapPtr() { return m_pDescriptorHeap.Get(); }
	uint32_t GetDescriptorSize() { return m_DescriptorSize; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index = 0) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index = 0) const;
	uint32_t GetNextAvailableIndex();

private:
	ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	uint32_t m_DescriptorSize;
	uint32_t m_NumDescriptors;
	uint32_t m_CurrentdescriptorIndex = 0;
};