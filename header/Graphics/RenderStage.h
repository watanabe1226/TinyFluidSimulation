#pragma once
#include "pch.h"

class Renderer;
class Window;
class DX12RootSignature;
class DX12PipelineState;

class RenderStage
{
public:
	RenderStage(Renderer* pRenderer);
	virtual ~RenderStage();

	virtual void RecordStage(ID3D12GraphicsCommandList* pCmdList);
	virtual void RecordStage(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
protected:
	/// <summary>
	/// ルートシグネチャ
	/// </summary>
	std::unique_ptr<DX12RootSignature> m_pRootSignature = nullptr;
	/// <summary>
	/// パイプラインステート
	/// </summary>
	std::unique_ptr<DX12PipelineState> m_pPSO = nullptr;

	Renderer* m_pRenderer = nullptr;
	Window* m_pWindow = nullptr;
};