#include "Graphics/RenderStages/FluidStage.h"
#include "Graphics/DX12DescriptorHeap.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/Window.h"
#include "Graphics/Camera.h"
#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Math/Vector2D.h"
#include "Utilities/Utility.h"

#include <imgui.h>

FluidStage::FluidStage(Renderer* pRenderer) : RenderStage(pRenderer)
{
	m_pRenderer = pRenderer;
	CreateBuffers();
	CreateBillboardMesh();
	CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);
	InitializeParticles();
}

FluidStage::~FluidStage()
{
}

void FluidStage::SetScene(Scene* newScene)
{
	m_pScene = newScene;
}

void FluidStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
	// バリア
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// クリアカラーの設定
	float clearColor[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	auto rtv = m_pWindow->GetCurrentScreenRTV();
	auto dsv = m_pWindow->GetDepthDSV();
	m_pRenderer->BindAndClearRenderTarget(m_pWindow, &rtv, &dsv, clearColor);

	pCmdList->SetPipelineState(m_pPSO->GetPipelineStatePtr());
	pCmdList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
	auto CBVSRVUAVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pCmdList->SetDescriptorHeaps(1, CBVSRVUAVHeap->GetHeap().GetAddressOf());

	// SRV設定
	pCmdList->SetGraphicsRootDescriptorTable(0, CBVSRVUAVHeap->GetGpuHandle(m_SRVIndex));

	// カメラ情報更新
	auto pCamera = m_pScene->GetCamera();
	m_ParticleTransform.View = pCamera->GetView();
	m_ParticleTransform.Proj = pCamera->GetProj();

	auto viewInv = pCamera->GetViewInv();
	viewInv.m_mat[3][0] = 0.0f;
	viewInv.m_mat[3][1] = 0.0f;
	viewInv.m_mat[3][2] = 0.0f;
	m_ParticleTransform.World = viewInv;

	auto cbGPUHandle = m_pRenderer->AllocateConstantBuffer(m_ParticleTransform, m_pRenderer->GetWindow()->GetCurrentBackBufferIndex());
	pCmdList->SetGraphicsRootConstantBufferView(1, cbGPUHandle);

	// ビルボード描画
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCmdList->IASetVertexBuffers(0, 1, &m_BillboardVBV);
	pCmdList->DrawInstanced(4, MaxParticles, 0, 0);

	// バリア
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);

}

void FluidStage::UpdateSimulationGrid(float deltaTime)
{
	auto pCmdlist = m_pRenderer->GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList().Get();
	auto CBVSRVUAVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 定数バッファの更新
	m_SimParam.DeltaTime = deltaTime;
	m_SimParam.Gravity = m_Gravity;
	m_SimParam.Stiffness = m_Stiffness;
	m_SimParam.RestDensity = m_RestDensity;
	m_SimParam.nearStiffness = m_NearStiffness;
	m_SimParam.ParticleCount = MaxParticles;
	m_SimParam.WallMin = m_WallMin;
	m_SimParam.WallMax = m_WallMax;
	m_SimParam.Viscosity = m_Viscosity;
	m_SimParam.H = m_H;
	m_SimParam.Mass = m_Mass;
	m_SimParam.gridCount = 32;

	// バリア設定: 処理開始前に Common -> UAV へ遷移
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pRenderer->TransitionResource(m_pGridHeadBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pRenderer->TransitionResource(m_pGridNextBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// 共通設定
	auto cbGPUHandle = m_pRenderer->AllocateConstantBuffer(m_SimParam, m_pRenderer->GetWindow()->GetCurrentBackBufferIndex());
	pCmdlist->SetComputeRootSignature(m_pComputeRootSignature->GetRootSignaturePtr());
	pCmdlist->SetDescriptorHeaps(1, CBVSRVUAVHeap->GetHeap().GetAddressOf());
	pCmdlist->SetComputeRootDescriptorTable(0, CBVSRVUAVHeap->GetGpuHandle(m_UAVIndex));
	pCmdlist->SetComputeRootConstantBufferView(1, cbGPUHandle);

	// シェーダー間同期用のUAVバリア
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = nullptr;

	// グリッドの初期化 headを -1に設定
	pCmdlist->SetPipelineState(m_pClearGridPSO->GetPipelineStatePtr());
	pCmdlist->Dispatch((TotalGridCount + 255) / 256, 1, 1);

	// バリア: グリッドクリアの完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// グリッドの構築
	uint32_t particleGroups = (MaxParticles + 255) / 256;
	pCmdlist->SetPipelineState(m_pGridBuildPSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア: グリッド構築の完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// Density Calculation (密度計算)
	pCmdlist->SetPipelineState(m_pDensityPSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア: 密度計算の完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// Force Calculation (圧力・力計算)
	pCmdlist->SetPipelineState(m_pForcePSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア: 力計算の完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// Integration (位置・速度更新・衝突)
	pCmdlist->SetPipelineState(m_pComputePSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア設定: 処理完了後に UAV -> Common (Vertex Shader用) へ戻す
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	m_pRenderer->TransitionResource(m_pGridHeadBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	m_pRenderer->TransitionResource(m_pGridNextBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
}

void FluidStage::UpdateSimulation(float deltaTime)
{
	auto pCmdlist = m_pRenderer->GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList().Get();
	auto CBVSRVUAVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 定数バッファの更新
	m_SimParam.DeltaTime = 0.003f;
	m_SimParam.Gravity = m_Gravity;
	m_SimParam.Stiffness = m_Stiffness;
	m_SimParam.RestDensity = m_RestDensity;
	m_SimParam.nearStiffness = m_NearStiffness;
	m_SimParam.ParticleCount = MaxParticles;
	m_SimParam.Viscosity = m_Viscosity;
	m_SimParam.H = m_H;
	m_SimParam.Mass = m_Mass;
	m_SimParam.gridCount = 32;
	m_SimParam.WallMin = m_WallMin;
	m_SimParam.WallMax = m_WallMax;

	RunFluidSolver(pCmdlist, CBVSRVUAVHeap);
}

void FluidStage::RunFluidSolver(ID3D12GraphicsCommandList* pCmdlist, DX12DescriptorHeap* CBVSRVUAVHeap)
{
	// バリア設定: 処理開始前に Common -> UAV へ遷移
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// 共通設定
	auto cbGPUHandle = m_pRenderer->AllocateConstantBuffer(m_SimParam, m_pRenderer->GetWindow()->GetCurrentBackBufferIndex());
	pCmdlist->SetComputeRootSignature(m_pComputeRootSignature->GetRootSignaturePtr());
	pCmdlist->SetDescriptorHeaps(1, CBVSRVUAVHeap->GetHeap().GetAddressOf());
	pCmdlist->SetComputeRootDescriptorTable(0, CBVSRVUAVHeap->GetGpuHandle(m_UAVIndex));
	pCmdlist->SetComputeRootConstantBufferView(1, cbGPUHandle);

	// シェーダー間同期用のUAVバリア
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_pParticleBuffer.Get();

	// Density Calculation (密度計算)
	uint32_t particleGroups = (MaxParticles + 255) / 256;
	pCmdlist->SetPipelineState(m_pDensityPSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア: 密度計算の完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// Force Calculation (圧力・力計算)
	pCmdlist->SetPipelineState(m_pForcePSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア: 力計算の完了待ち
	pCmdlist->ResourceBarrier(1, &uavBarrier);

	// Integration (位置・速度更新・衝突)
	pCmdlist->SetPipelineState(m_pComputePSO->GetPipelineStatePtr());
	pCmdlist->Dispatch(particleGroups, 1, 1);

	// バリア設定: 処理完了後に UAV -> Common (Vertex Shader用) へ戻す
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
}

void FluidStage::Update(float deltaTime)
{
	ImGui::Begin("Fluid Simulation Settings");

	if (ImGui::Button("Reset Particles"))
	{
		InitializeParticles();
	}
	ImGui::DragFloat("Gravity", &m_Gravity, 0.1f, -20.0f, 0.0f);
	ImGui::DragFloat("Mass", &m_Mass, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("smoothRadius", &m_H, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Viscosity", &m_Viscosity, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("RestDensity", &m_RestDensity, 1.0f, 0.0f, 5000.0f);
	ImGui::DragFloat("Stiffness", &m_Stiffness, 1.0f, 0.0f, 5000.0f);
	if (ImGui::SliderFloat("Box Width", &m_BoxWidth, 1.0f, 10.0f))
	{
		// 幅が変わったら WallMin/Max の XとZ を更新
		m_WallMin.x = -m_BoxWidth;
		m_WallMax.x = m_BoxWidth;
	}
	ImGui::Text("Particle Count: %d", MaxParticles);
	ImGui::End();
}

void FluidStage::CreateBuffers()
{
	auto pDevice = m_pRenderer->GetDevice().Get();
	auto CBVSRVUAVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ---------------------------------------------------------
	// リソース（バッファ本体）の作成
	// ---------------------------------------------------------
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// 1. Particle Buffer
	{
		uint32_t stride = sizeof(Particle);
		uint32_t bufferSize = MaxParticles * stride;
		bufferDesc.Width = bufferSize;

		ThrowFailed(pDevice->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(m_pParticleBuffer.GetAddressOf())
		));
		m_pParticleBuffer->SetName(L"ParticleBuffer");
	}

	// 2. GridHead Buffer
	{
		uint32_t stride = sizeof(uint32_t);
		uint32_t bufferSize = TotalGridCount * stride;
		bufferDesc.Width = bufferSize;

		ThrowFailed(pDevice->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(m_pGridHeadBuffer.GetAddressOf())
		));
		m_pGridHeadBuffer->SetName(L"GridHeadBuffer");
	}

	// 3. GridNext Buffer
	{
		uint32_t stride = sizeof(uint32_t);
		uint32_t bufferSize = MaxParticles * stride;
		bufferDesc.Width = bufferSize;

		ThrowFailed(pDevice->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(m_pGridNextBuffer.GetAddressOf())
		));
		m_pGridNextBuffer->SetName(L"GridNextBuffer");
	}

	// ---------------------------------------------------------
	// ビュー（ディスクリプタ）の作成
	// ---------------------------------------------------------
	m_UAVIndex = CBVSRVUAVHeap->GetNextAvailableIndex();         // u0
	m_GridHeadUAVIndex = CBVSRVUAVHeap->GetNextAvailableIndex(); // u1
	m_GridNextUAVIndex = CBVSRVUAVHeap->GetNextAvailableIndex(); // u2

	// --- 1. Particle Buffer UAV (u0) ---
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = MaxParticles;
	uavDesc.Buffer.StructureByteStride = sizeof(Particle);

	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	pDevice->CreateUnorderedAccessView(
		m_pParticleBuffer.Get(),
		nullptr,
		&uavDesc,
		CBVSRVUAVHeap->GetCpuHandle(m_UAVIndex)
	);

	// --- Particle Buffer SRV (t0) ---
	m_SRVIndex = CBVSRVUAVHeap->GetNextAvailableIndex();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = MaxParticles;
	srvDesc.Buffer.StructureByteStride = sizeof(Particle);

	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	pDevice->CreateShaderResourceView(
		m_pParticleBuffer.Get(),
		&srvDesc,
		CBVSRVUAVHeap->GetCpuHandle(m_SRVIndex)
	);

	// --- 2. GridHead Buffer UAV (u1) ---
	uavDesc.Buffer.NumElements = TotalGridCount;
	uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);

	pDevice->CreateUnorderedAccessView(
		m_pGridHeadBuffer.Get(),
		nullptr,
		&uavDesc,
		CBVSRVUAVHeap->GetCpuHandle(m_GridHeadUAVIndex)
	);

	// --- 3. GridNext Buffer UAV (u2) ---
	uavDesc.Buffer.NumElements = MaxParticles;
	uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);

	pDevice->CreateUnorderedAccessView(
		m_pGridNextBuffer.Get(),
		nullptr,
		&uavDesc,
		CBVSRVUAVHeap->GetCpuHandle(m_GridNextUAVIndex)
	);
}

void FluidStage::CreateBillboardMesh()
{
	// 単純な四角形
	struct Vertex {
		Vector3D Position;
		Vector2D TexCoord;
	};

	Vertex vertices[] = {
		{ Vector3D(-0.5f,  0.5f, 0.0f), Vector2D(0.0f, 0.0f) }, // 左上
		{ Vector3D(0.5f,  0.5f, 0.0f), Vector2D(1.0f, 0.0f) }, // 右上
		{ Vector3D(-0.5f, -0.5f, 0.0f), Vector2D(0.0f, 1.0f) }, // 左下
		{ Vector3D(0.5f, -0.5f, 0.0f), Vector2D(1.0f, 1.0f) }, // 右下
	};

	// 単純なstripで4頂点を送る
	auto pDevice = m_pRenderer->GetDevice().Get();
	uint32_t size = sizeof(vertices);

	uint32_t bufferSize = (size + 255) & ~255;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowFailed(pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBillboardVB.GetAddressOf())
	));
	m_pBillboardVB->SetName(L"BillboardVB");

	// データ転送
	void* ptr = nullptr;
	m_pBillboardVB->Map(0, nullptr, &ptr);
	memcpy(ptr, vertices, size);
	m_pBillboardVB->Unmap(0, nullptr);

	m_BillboardVBV.BufferLocation = m_pBillboardVB->GetGPUVirtualAddress();
	m_BillboardVBV.StrideInBytes = sizeof(Vertex);
	m_BillboardVBV.SizeInBytes = bufferSize;
}

void FluidStage::InitializeParticles()
{
	auto pDevice = m_pRenderer->GetDevice().Get();
	uint32_t bufferSize = MaxParticles * sizeof(Particle);

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowFailed(pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pParticleUploadBuffer.GetAddressOf())
	));

	// ランダム配置
	std::vector<Particle> particles(MaxParticles);

	// 縦長の柱を作るための次元設定
	// X, Z を狭く、Y を高くする
	uint32_t dimX = 14;
	uint32_t dimZ = 14;
	// 残りを高さ(Y)にする
	uint32_t dimY = (MaxParticles + (dimX * dimZ) - 1) / (dimX * dimZ);

	float spacing = 0.1f; // 粒子間隔 (H=0.2 の半分くらい)

	// 水槽の左隅などに寄せるためのオフセット
	float startX = 0.0f;
	float startY = 0.2f;
	float startZ = -0.0f;

	uint32_t count = 0;
	for (uint32_t y = 0; y < dimY; ++y)
	{
		for (uint32_t x = 0; x < dimX; ++x)
		{
			for (uint32_t z = 0; z < dimZ; ++z)
			{
				if (count >= MaxParticles) break;

				// ランダムな微小なズレ(Jitter)を加えることで、対称性を崩し波を起こしやすくする
				float jitterX = 0.002f * ((float)rand() / RAND_MAX);
				float jitterY = 0.002f * ((float)rand() / RAND_MAX);
				float jitterZ = 0.002f * ((float)rand() / RAND_MAX);

				float posX = startX + x * spacing + jitterX;
				float posY = startY + y * spacing + jitterY;
				float posZ = startZ + z * spacing + jitterZ;

				particles[count].Position = Vector3D(posX, posY, posZ);
				particles[count].Velocity = Vector3D(0, 0, 0);
				particles[count].Density = 0.0f;
				particles[count].Pressure = 0.0f;
				particles[count].Force = Vector3D(0, 0, 0); // 初期力は0
				particles[count].NearDensity = 0.0f;

				count++;
			}
		}
	}

	void* ptr = nullptr;
	m_pParticleUploadBuffer->Map(0, nullptr, &ptr);
	memcpy(ptr, particles.data(), bufferSize);
	m_pParticleUploadBuffer->Unmap(0, nullptr);

	// コピーコマンド
	auto pCmd = m_pRenderer->GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto pCmdList = pCmd->GetGraphicsCommandList().Get();

	pCmd->ResetCommand();

	// UploadからDefaultへコピー
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	pCmdList->CopyResource(m_pParticleBuffer.Get(), m_pParticleUploadBuffer.Get());
	m_pRenderer->TransitionResource(m_pParticleBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);

	pCmd->ExecuteCommandList();
	pCmd->WaitGpu(INFINITE);
}

void FluidStage::CreateRootSignature(Renderer* pRenderer)
{
	auto pDevice = pRenderer->GetDevice().Get();

	// コンピュートルートシグネチャ
	D3D12_DESCRIPTOR_RANGE uavRange = {};
	uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange.NumDescriptors = 3;
	uavRange.BaseShaderRegister = 0; // u0

	D3D12_ROOT_PARAMETER params[2] = {};
	// u0 RWStructuredBuffer
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[0].DescriptorTable.NumDescriptorRanges = 1;
	params[0].DescriptorTable.pDescriptorRanges = &uavRange;

	// b0 SimulationParam
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].Descriptor.ShaderRegister = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(params);
	rootSigDesc.pParameters = params;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	m_pComputeRootSignature = std::make_unique<DX12RootSignature>(pDevice, &rootSigDesc);

	// レンダールートシグネチャ
	D3D12_DESCRIPTOR_RANGE srvRange = {};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0; // t0

	D3D12_ROOT_PARAMETER renderParams[2] = {};
	renderParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	renderParams[0].DescriptorTable.NumDescriptorRanges = 1;
	renderParams[0].DescriptorTable.pDescriptorRanges = &srvRange;
	renderParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// b0 Transform
	renderParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	renderParams[1].Descriptor.ShaderRegister = 0;
	renderParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC renderRootSigDesc = {};
	renderRootSigDesc.NumParameters = _countof(renderParams);
	renderRootSigDesc.pParameters = renderParams;
	renderRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &renderRootSigDesc);
}

void FluidStage::CreatePipeline(Renderer* pRenderer)
{
	auto pDevice = pRenderer->GetDevice().Get();
	std::wstring shaderPath = Utility::GetCurrentDir() + L"/assets/shaders/";
	
	// グリッド構築用パイプライン
	ComPtr<ID3DBlob> gridBuildCSBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"FluidGridBuildCS.cso").c_str(), gridBuildCSBlob.GetAddressOf()));
	D3D12_COMPUTE_PIPELINE_STATE_DESC gridBuildComputePsoDesc = {};
	gridBuildComputePsoDesc.pRootSignature = m_pComputeRootSignature->GetRootSignaturePtr();
	gridBuildComputePsoDesc.CS =
	{ gridBuildCSBlob->GetBufferPointer(), gridBuildCSBlob->GetBufferSize() };
	m_pGridBuildPSO = std::make_unique<DX12PipelineState>(pDevice, &gridBuildComputePsoDesc);

	//グリッドクリア用パイプライン
	ComPtr<ID3DBlob> gridClearCsBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"FluidGridClearCS.cso").c_str(), gridClearCsBlob.GetAddressOf()));
	D3D12_COMPUTE_PIPELINE_STATE_DESC gridClearComputePsoDesc = {};
	gridClearComputePsoDesc.pRootSignature = m_pComputeRootSignature->GetRootSignaturePtr();
	gridClearComputePsoDesc.CS =
	{ gridClearCsBlob->GetBufferPointer(), gridClearCsBlob->GetBufferSize() };
	m_pClearGridPSO = std::make_unique<DX12PipelineState>(pDevice, &gridClearComputePsoDesc);

	// 密度計算パイプライン
	ComPtr<ID3DBlob> densityCsBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"FluidDensityCS.cso").c_str(), densityCsBlob.GetAddressOf()));
	D3D12_COMPUTE_PIPELINE_STATE_DESC densityComputePsoDesc = {};
	densityComputePsoDesc.pRootSignature = m_pComputeRootSignature->GetRootSignaturePtr();
	densityComputePsoDesc.CS =
	{ densityCsBlob->GetBufferPointer(), densityCsBlob->GetBufferSize() };
	m_pDensityPSO = std::make_unique<DX12PipelineState>(pDevice, &densityComputePsoDesc);

	// 力計算パイプライン
	ComPtr<ID3DBlob> forceCsBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"FluidForceCS.cso").c_str(), forceCsBlob.GetAddressOf()));
	D3D12_COMPUTE_PIPELINE_STATE_DESC forceComputePsoDesc = {};
	forceComputePsoDesc.pRootSignature = m_pComputeRootSignature->GetRootSignaturePtr();
	forceComputePsoDesc.CS =
	{ forceCsBlob->GetBufferPointer(), forceCsBlob->GetBufferSize() };
	m_pForcePSO = std::make_unique<DX12PipelineState>(pDevice, &forceComputePsoDesc);

	// コンピュートパイプライン
	ComPtr<ID3DBlob> csBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"FluidSimCS.cso").c_str(), csBlob.GetAddressOf()));
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = m_pComputeRootSignature->GetRootSignaturePtr();
	computePsoDesc.CS = 
	{ csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

	m_pComputePSO = std::make_unique<DX12PipelineState>(pDevice, &computePsoDesc);

	// レンダーパイプライン
	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"ParticleVS.cso").c_str(), vsBlob.GetAddressOf()));
	ThrowFailed(D3DReadFileToBlob((shaderPath + L"ParticlePS.cso").c_str(), psBlob.GetAddressOf()));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPsoDesc = {};
	graphicsPsoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	graphicsPsoDesc.pRootSignature = m_pRootSignature->GetRootSignaturePtr();
	graphicsPsoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	graphicsPsoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	graphicsPsoDesc.RasterizerState = rasterizerDesc;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	graphicsPsoDesc.BlendState = blendDesc;

	graphicsPsoDesc.DepthStencilState.DepthEnable = TRUE;
	graphicsPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	graphicsPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	graphicsPsoDesc.SampleMask = UINT_MAX;
	graphicsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPsoDesc.NumRenderTargets = 1;
	graphicsPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	graphicsPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	graphicsPsoDesc.SampleDesc.Count = 1;

	m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &graphicsPsoDesc);
}