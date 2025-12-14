#include "Graphics/RenderStage.h"
#include "Framework/Renderer.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"

RenderStage::RenderStage(Renderer* pRenderer) : m_pRenderer(pRenderer)
{
	m_pWindow = pRenderer->GetWindow();
}

RenderStage::~RenderStage()
{
}

void RenderStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
}

void RenderStage::RecordStage(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
}
