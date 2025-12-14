#pragma once
#include "pch.h"

class DX12PipelineState
{
public:
	DX12PipelineState(
		ID3D12Device* pDevice,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc);
	DX12PipelineState(
		ID3D12Device* pDevice,
		const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc);

	ComPtr<ID3D12PipelineState> GetPipelineState() { return m_pPipelineState; }
	ID3D12PipelineState* GetPipelineStatePtr() { return m_pPipelineState.Get(); }

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;
};