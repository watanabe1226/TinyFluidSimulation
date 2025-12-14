#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Graphics/Transform.h"

class Renderer;

class SphereMapConverterStage : public RenderStage
{
public:
	SphereMapConverterStage(Renderer* pRenderer,
		const D3D12_RESOURCE_DESC& sphereMapDesc);
	~SphereMapConverterStage();

	void DrawToCube(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE shpereMapHandle);
	void RecordStage(ID3D12GraphicsCommandList* pCmdList) override;
	D3D12_RESOURCE_DESC GetCubeMapDesc() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCubeMapHandleCPU() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetCubeMapHandleGPU() const;
private:
	void CreateTexture(const D3D12_RESOURCE_DESC& sphereMapDesc);
	void CreateRootSignature(Renderer* pRenderer);
	void CreatePipeline(Renderer* pRenderer);
	void DrawSphere(ID3D12GraphicsCommandList* pCmdList);

	Renderer* m_pRenderer = nullptr;
	uint32_t m_MipCount; //!< ミップレベル数
	ComPtr<ID3D12Resource> m_pCubeTex; //!< キューブマップテクスチャ
	uint32_t m_SRVIndex = 0;
	std::vector<uint32_t> m_RTVIndeies;
	TransformBuffer m_CBTrans[6];
	// 頂点、インデックスデータ
	ComPtr<ID3D12Resource> m_pVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VBV = {};

	ComPtr<ID3D12Resource> m_pIB = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IBV = {};
	uint32_t m_IndexCount = 0;
};