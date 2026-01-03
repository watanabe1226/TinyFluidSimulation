#pragma once
#include "pch.h"
#include "Graphics/RenderStage.h"
#include "Graphics/DX12Utilities.h"
#include "Math/Matrix4x4.h"

class Scene;
class Camera;
class Renderer;

struct Particle
{
	Vector3D Position;
	float Density;
	Vector3D Velocity;
	float Pressure;
	Vector3D Force;
	float NearDensity;
};

// 定数バッファ用構造体 (Compute Shader用)
struct alignas(256) SimulationParam
{
	float DeltaTime;
	float Gravity;
	float Stiffness;
	float nearStiffness;
	uint32_t ParticleCount;
	Vector3D WallMin;
	float RestDensity;
	Vector3D WallMax;
	float Viscosity;
	float H;
	float Mass;
	uint32_t gridCount;
};

// 定数バッファ用構造体 (Vertex Shader用)
struct alignas(256) ParticleTransform
{
	Matrix4x4 World;
	Matrix4x4 View;
	Matrix4x4 Proj;
};

class FluidStage : public RenderStage
{
public:
	FluidStage(Renderer* pRenderer);
	~FluidStage() override;
	void SetScene(Scene* newScene);

	void RecordStage(ID3D12GraphicsCommandList* pCmdList) override;
	void UpdateSimulationGrid(float deltaTime);
	void UpdateSimulation(float deltaTime);
	void RunFluidSolver(ID3D12GraphicsCommandList* pCmdlist, DX12DescriptorHeap* CBVSRVUAVHeap);
	void Update(float deltaTime);
private:
	void CreateBuffers();
	void CreateBillboardMesh();
	void CreateRootSignature(Renderer* pRenderer);
	void CreatePipeline(Renderer* pRenderer);
	void InitializeParticles();

	// 定数
	static const uint32_t MaxParticles = 5000;
	// グリッド
	static constexpr float ParticleRadius = 0.05f;
	static constexpr float CellSize = 1.0f * ParticleRadius;
	static constexpr float xHalfMax = 2.0f;
	static constexpr float yHalfMax = 2.0f;
	static constexpr float zHalfMax = 2.0f;
	static constexpr float xLen = 2.0f * xHalfMax;
	static constexpr float yLen = 2.0f * yHalfMax;
	static constexpr float zLen = 2.0f * zHalfMax;
	static constexpr float sentinel = 4.0f * CellSize;
	static constexpr float offset = sentinel / 2.0f;
	static constexpr uint32_t GridCountX = static_cast<uint32_t>((xLen + sentinel) / CellSize); // X軸方向の分割数
	static constexpr uint32_t GridCountY = static_cast<uint32_t>((xLen + sentinel) / CellSize);
	static constexpr uint32_t GridCountZ = static_cast<uint32_t>((xLen + sentinel) / CellSize);
	static constexpr uint32_t TotalGridCount = GridCountX * GridCountY * GridCountZ; // 総セル数

	// リソース
	ComPtr<ID3D12Resource> m_pParticleBuffer;      // 粒子の位置などを保持するバッファ
	ComPtr<ID3D12Resource> m_pParticleUploadBuffer; // 初期化用
	ComPtr<ID3D12Resource> m_pGridHeadBuffer; // グリッドの先頭ID
	ComPtr<ID3D12Resource> m_pGridNextBuffer; // 次のパーティクルID
	// ビルボード用頂点バッファ
	ComPtr<ID3D12Resource> m_pBillboardVB;
	D3D12_VERTEX_BUFFER_VIEW m_BillboardVBV = {};

	// パイプライン
	std::unique_ptr<DX12RootSignature> m_pComputeRootSignature;
	std::unique_ptr<DX12PipelineState> m_pComputePSO;
	std::unique_ptr<DX12PipelineState> m_pDensityPSO;
	std::unique_ptr<DX12PipelineState> m_pForcePSO;
	std::unique_ptr<DX12PipelineState> m_pGridBuildPSO; // グリッド構築用PSO
	std::unique_ptr<DX12PipelineState> m_pClearGridPSO; // グリッドクリア用PSO

	// ディスクリプタヒープのインデックス
	uint32_t m_UAVIndex = 0; // Compute Shader書き込み用
	uint32_t m_GridHeadUAVIndex = 0; // Compute Shader書き込み用
	uint32_t m_GridNextUAVIndex = 0; // Compute Shader書き込み用
	uint32_t m_SRVIndex = 0; // Vertex Shader読み込み用
	
	// データ
	SimulationParam m_SimParam;
	ParticleTransform m_ParticleTransform;
	
	Scene* m_pScene = nullptr;
	Camera* m_pCamera = nullptr;
	Renderer* m_pRenderer = nullptr;

	float m_Gravity = -9.81f;
	float m_H = 0.22f; // 影響範囲
	float m_Mass = 1.0f; // 質量
	float m_Viscosity = 10.0; // 粘性係数
	float m_RestDensity = 1000.0f; // 静止密度
	float m_Stiffness = 1000.0f; // 圧力計数
	float m_NearStiffness = 100.0f; // 近傍圧力計数
	float m_BoxWidth = 6.0f; // 初期幅
	float m_MaxAllowableTimestep = 0.006f; // 時間刻み幅
	float m_TimeStep = 0.0f;
	uint32_t m_Iterations = 10; // シミュレーションの1フレーム当たりのイテレーション回数
	// デフォルトの壁の範囲
	Vector3D m_WallMin = Vector3D(-2.0f, 0.0f, -2.0f);
	Vector3D m_WallMax = Vector3D(2.0f, 6.0f, 2.0f);
};