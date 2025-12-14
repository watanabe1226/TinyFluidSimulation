#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Graphics/Transform.h"

class Renderer;
class ConstantBuffer;
class DX12DescriptorHeap;

class IBLBakerStage : public RenderStage
{

	struct alignas(256) CbBake
	{
		int     FaceIndex;
		float   Roughness;
		float   Width;
		float   MipCount;
	};

public:
	IBLBakerStage(Renderer* pRenderer);
	~IBLBakerStage();

	void RecordStage(ID3D12GraphicsCommandList* pCmdList) override;
	void IntegrateDFG(ID3D12GraphicsCommandList* pCmdList);
	void IntegrateLD(ID3D12GraphicsCommandList* pCmdList,
		uint32_t mapSize,
		uint32_t mipCount,
		D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap);
	void IntegrateDiffuseLD(ID3D12GraphicsCommandList* pCmdList,
		D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap);
	void IntegrateSpecularLD(ID3D12GraphicsCommandList* pCmdList,
		D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap);
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DFG() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DiffuseLD() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_SpecularLD() const;
private:
	void CreateVBV();
	void CreateDFGRTV();
	void CreateDiffuseLDRTV();
	void CreateSpecularLDRTV();
	void CreateBakeDatas();
	void CreateDFGRootSignature(Renderer* pRenderer);
	void CreateDFGPipeline(Renderer* pRenderer);
	void CreateLDRootSignature(Renderer* pRenderer);
	void CreateDiffuseLDPipeline(Renderer* pRenderer);
	void CreateSpecularLDPipeline(Renderer* pRenderer);

	static const uint32_t DFGTextureSize = 512;
	static const uint32_t LDTextureSize = 128;
	static const uint32_t MipCount = 7;

	Renderer* m_pRenderer = nullptr;
	DX12DescriptorHeap* SRVHeap = nullptr;

	// 頂点、インデックスデータ
	ComPtr<ID3D12Resource> m_pVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VBV = {};

	ComPtr<ID3D12Resource> m_TexDFG; //!< DFGテクスチャ
	ComPtr<ID3D12Resource> m_TexDiffuseLD; //!< DiffuseLDテクスチャ
	ComPtr<ID3D12Resource> m_TexSpecularLD; //!< SpecularLDテクスチャ
	std::vector<CbBake> m_BakeCBDatas;
	uint32_t m_RTV_DFG_Index = 0;
	std::vector<uint32_t> m_RTV_DiffuseLD_Indeies;
	std::vector<uint32_t> m_RTV_SpecularLD_Indeies;

	uint32_t m_SRV_DFG_Index = 0;
	uint32_t m_SRV_DiffuseLD_Index = 0;
	uint32_t m_SRV_SpecularLD_Index = 0;

	std::unique_ptr<DX12RootSignature> m_pLDRootSignature = nullptr;
	std::unique_ptr<DX12PipelineState> m_pDiffuseLDPSO = nullptr;
	std::unique_ptr<DX12PipelineState> m_pSpecularLDPSO = nullptr;
};