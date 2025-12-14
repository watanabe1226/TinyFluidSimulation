#include "Graphics/DX12DescriptorHeap.h"
#include "Graphics/DX12Utilities.h"
#include "Utilities/Utility.h"

DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, const std::string& DescriptorName, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: m_NumDescriptors(numDescriptors)
{
	ID3D12Device* device = pDevice;
	// ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = m_NumDescriptors;
	desc.Type = type;
	desc.Flags = flags;
	desc.NodeMask = 0;

	// ディスクリプタヒープの生成
	auto hr = device->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(m_pDescriptorHeap.GetAddressOf()));
	ThrowFailed(hr);
	m_pDescriptorHeap->SetName(Utility::StringToWString(DescriptorName).c_str());
	m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetCpuHandle(uint32_t index) const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE{ m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<uint64_t>(index) * m_DescriptorSize};
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::GetGpuHandle(uint32_t index) const
{
	return D3D12_GPU_DESCRIPTOR_HANDLE{ m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + static_cast<uint64_t>(index) * m_DescriptorSize };
}

uint32_t DX12DescriptorHeap::GetNextAvailableIndex()
{
	if (m_CurrentdescriptorIndex >= m_NumDescriptors)
	{
		assert(false && "ディスクリプタヒープの上限に達しました．");
		return 0;
	}

	uint32_t index = m_CurrentdescriptorIndex;
	++m_CurrentdescriptorIndex;
	return index;
}
