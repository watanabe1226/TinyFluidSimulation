#pragma once
#include "pch.h"

class DX12Device
{
public:
	DX12Device();

	ComPtr<ID3D12Device> GetDevice() { return m_pDevice; }
	ID3D12Device* GetAddress() { return m_pDevice.Get(); }

private:
	void DegugLayer();
	ComPtr<ID3D12Device> m_pDevice;

};