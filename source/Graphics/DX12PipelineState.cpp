#include "Graphics/DX12PipelineState.h"
#include "Graphics/DX12Utilities.h"

DX12PipelineState::DX12PipelineState(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc)
{
	// パイプラインステートの生成
	auto hr = pDevice->CreateGraphicsPipelineState(
		pDesc,
		IID_PPV_ARGS(m_pPipelineState.GetAddressOf()));
	ThrowFailed(hr);
}

DX12PipelineState::DX12PipelineState(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc)
{
	// パイプラインステートの生成
	auto hr = pDevice->CreateComputePipelineState(
		pDesc,
		IID_PPV_ARGS(m_pPipelineState.GetAddressOf()));
	ThrowFailed(hr);
}
