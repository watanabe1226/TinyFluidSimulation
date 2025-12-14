#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Graphics/Transform.h"

class Scene;
class Camera;
class Texture;
class Mesh;
class Model;

class SkyBoxStage : public RenderStage
{
public:
	SkyBoxStage(Renderer* pRenderer);
	~SkyBoxStage();

	void RecordStage(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) override;
	void SetScene(Scene* newScene);

	D3D12_GPU_DESCRIPTOR_HANDLE GetSkyBoxGPUHandle();
	Texture* GetHDRITex() { return m_pHDRITexture.get(); }
	D3D12_RESOURCE_DESC GetHDRIDesc() const;

private:
	void CreateSkyBoxMesh();
	void CreateRootSignature(Renderer* pRenderer);
	void CreatePipeline(Renderer* pRenderer);

	Scene* m_pScene;
	Camera* m_pCamera = nullptr;
	std::unique_ptr<Texture> m_pHDRITexture = nullptr;
	std::unique_ptr<Texture> m_pTestTexture = nullptr;
	TransformBuffer m_SkydomeTranBuufer;

	// 頂点、インデックスデータ
	ComPtr<ID3D12Resource> m_pVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VBV = {};
};