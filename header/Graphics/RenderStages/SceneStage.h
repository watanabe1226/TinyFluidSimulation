#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Graphics/Lights.h"
#include "Graphics/DX12Utilities.h"

class Scene;
class Camera;
class ShadowStage;
class IBLBakerStage;

class SceneStage : public RenderStage
{
public:
	SceneStage(Renderer* pRenderer, ShadowStage* pShadowStage, IBLBakerStage* pIBLBakerStage);
	~SceneStage() override;
	void SetScene(Scene* newScene);

	void RecordStage(ID3D12GraphicsCommandList* pCmdList) override;

private:
	void CreateRootSignature(Renderer* pRenderer);
	D3D12_STATIC_SAMPLER_DESC& SetStaticSamplerDesc(DX12Utility::SamplerState samplerState, uint32_t reg);
	void CreatePipeline(Renderer* pRenderer);

	Scene* m_pScene = nullptr;
	Camera* m_pCamera = nullptr;
	ShadowStage* m_pShadowStage = nullptr;
	IBLBakerStage* m_IBLBakerStage = nullptr;
	ShadowLightData m_ShadowLightData;
};