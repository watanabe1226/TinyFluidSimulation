#pragma once
#include "pch.h"

class DX12RootSignature
{
public:
	DX12RootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC* pDesc);

	ComPtr<ID3D12RootSignature> GetRootSignature() { return m_pRootSignature; }
	ID3D12RootSignature* GetRootSignaturePtr() { return m_pRootSignature.Get(); }

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr;
};