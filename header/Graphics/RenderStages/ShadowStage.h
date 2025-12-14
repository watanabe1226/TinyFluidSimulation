#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Math/Vector3D.h"
#include "Math/Matrix4x4.h"
#include "Graphics/Transform.h"

class Scene;
class DepthBuffer;
class Camera;

class ShadowStage : public RenderStage
{
public:
	ShadowStage(Renderer* pRenderer);
	~ShadowStage();

	void Update(float deltaTime);
	void SetScene(Scene* newScene);

	void RecordStage(ID3D12GraphicsCommandList* pCmdList) override;
	DepthBuffer* GetDepthBuffer() const { return m_pDepthBuffer.get(); }
	const Matrix4x4& GetVPMat() const;
	const Vector3D& GetLightDir() const;

private:
	void CreateRootSignature(Renderer* pRenderer);
	void CreatePipeline(Renderer* pRenderer);
	void SetDirectionalLightRotation(const Vector3D& vec);

	Scene* m_pScene = nullptr;
	Camera* m_pMainCamera = nullptr;
	std::unique_ptr<DepthBuffer> m_pDepthBuffer = nullptr;
	Transform m_DirectionalLightTrans;
	float m_DepthBufferWidth = 4096;
	float m_DepthBufferHeight = 4096;
	D3D12_RECT m_Scissor;
	D3D12_VIEWPORT m_Viewport;
	float m_LightViewSize = 30.0f; // 影を落とす範囲の広さ（メートル）
	float m_LightDistance = 50.0f;// ライトカメラと注視点の距離
	float lightY = -45.0f;
	float lightX = 50.0f;
};