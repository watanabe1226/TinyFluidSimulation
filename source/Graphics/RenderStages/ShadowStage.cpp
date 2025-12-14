#include "Graphics/RenderStages/ShadowStage.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/DepthBuffer.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Camera.h"
#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Utilities/Utility.h"
#include "Math/MathUtility.h"

#include <imgui.h>

ShadowStage::ShadowStage(Renderer* pRenderer) : RenderStage(pRenderer)
{
	m_pDepthBuffer = std::make_unique<DepthBuffer>(pRenderer, m_DepthBufferWidth, m_DepthBufferHeight);
	// ビューポートとシザー矩形の設定
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = static_cast<float>(m_DepthBufferWidth);
	m_Viewport.Height = static_cast<float>(m_DepthBufferHeight);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_Scissor.left = 0;
	m_Scissor.right = static_cast<LONG>(m_DepthBufferWidth);
	m_Scissor.top = 0;
	m_Scissor.bottom = static_cast<LONG>(m_DepthBufferHeight);

	CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);
}

ShadowStage::~ShadowStage()
{
}

void ShadowStage::Update(float deltaTime)
{
	auto depthSRV = m_pDepthBuffer->GetSRV();

	ImGui::Begin("Depth Buffer");
	ImGui::Image((ImTextureID)depthSRV.ptr, ImVec2(256, 256));
	ImGui::DragFloat("light X", &lightX, 1, 0, 100);
	ImGui::DragFloat("light Y", &lightY, 1, -30, -60);
	ImGui::DragFloat("shadowDistance", &m_LightDistance, 1, 0, 200);
	ImGui::End();
	SetDirectionalLightRotation(Vector3D(lightX, lightY, 0.0f));
}

void ShadowStage::SetScene(Scene* newScene)
{
	m_pScene = newScene;
	m_pMainCamera = m_pScene->GetCamera();
	SetDirectionalLightRotation(Vector3D(50.0f, -45.0f, 0.0f));
}

void ShadowStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
	if (m_pScene == nullptr)
	{
		assert(false && "シーンがセットされていません");
		return;
	}

	auto depthView = m_pDepthBuffer->GetDSV();
	auto depthBuffer = m_pDepthBuffer->GetResource();
	m_pRenderer->TransitionResource(depthBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	auto pCommandList = m_pRenderer->GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList().Get();

	pCommandList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCommandList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
	pCommandList->SetPipelineState(m_pPSO->GetPipelineStatePtr());

	pCommandList->RSSetViewports(1, &m_Viewport);
	pCommandList->RSSetScissorRects(1, &m_Scissor);
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &depthView);
	auto lightMat = GetVPMat();
	for (const auto& model : m_pScene->GetModels())
	{
		pCommandList->SetGraphicsRoot32BitConstants(0, 16, &lightMat, 0);
		auto world = model->GetTransform().World;
		pCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 16);

		for (const auto& mesh : model->GetMeshes())
		{
			auto vbv = mesh->GetVBV();
			auto ibv = mesh->GetIBV();
			pCommandList->IASetVertexBuffers(0, 1, &vbv);
			pCommandList->IASetIndexBuffer(&ibv);

			pCommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
		}
	}

	m_pRenderer->TransitionResource(depthBuffer,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

const Matrix4x4& ShadowStage::GetVPMat() const
{
	auto view = m_DirectionalLightTrans.GetView();
	auto proj = Matrix4x4::setOrthoLH(m_LightViewSize, m_LightViewSize, 1, 1000);
	return view * proj;
}

const Vector3D& ShadowStage::GetLightDir() const
{
	return m_DirectionalLightTrans.GetForward();
}

void ShadowStage::CreateRootSignature(Renderer* pRenderer)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// ルートパラメータ
	D3D12_ROOT_PARAMETER param[1] = {};

	// Light Model Matrix CB : 32bitconst
	param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[0].Constants.Num32BitValues = 32;
	param[0].Constants.ShaderRegister = 0;
	param[0].Constants.RegisterSpace = 0;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(param);
	desc.NumStaticSamplers = 0;
	desc.pParameters = param;
	desc.pStaticSamplers = nullptr;
	desc.Flags = flag;

	// ルートシグネチャの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &desc);
}

void ShadowStage::CreatePipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定
	D3D12_INPUT_ELEMENT_DESC elements[1];
	elements[0].SemanticName = "POSITION";
	elements[0].SemanticIndex = 0;
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].InputSlot = 0;
	elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[0].InstanceDataStepRate = 0;

	// ラスタライザーステートの設定
	D3D12_RASTERIZER_DESC descRS;
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_FRONT;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = TRUE;
	descRS.MultisampleEnable = FALSE;
	descRS.AntialiasedLineEnable = FALSE;
	descRS.ForcedSampleCount = 0;
	descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// レンダーターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	// ブレンドステートの設定
	D3D12_BLEND_DESC descBS;
	descBS.AlphaToCoverageEnable = FALSE;
	descBS.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBS.RenderTarget[i] = descRTBS;
	}

	ComPtr<ID3DBlob> vsBlob;

	// 頂点シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";
	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"ShadowVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr);

	// 深度ステンシルステートの設定
	D3D12_DEPTH_STENCIL_DESC descDSS = {};
	descDSS.DepthEnable = TRUE;
	descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	descDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	descDSS.StencilEnable = FALSE;

	// パイプラインステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { elements, _countof(elements) };
	desc.pRootSignature = m_pRootSignature->GetRootSignaturePtr();
	desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	desc.PS = { nullptr, 0 }; // シャドウマップ生成に色は不要
	desc.RasterizerState = descRS;
	desc.BlendState = descBS;
	desc.DepthStencilState = descDSS;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 0;
	desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// パイプラインステートの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}

void ShadowStage::SetDirectionalLightRotation(const Vector3D& vec)
{
	m_DirectionalLightTrans.SetRotation(vec);
	auto forward = m_DirectionalLightTrans.GetForward();
	auto target = m_pMainCamera->GetTarget();
	auto pos = target - (forward * m_LightDistance);
	m_DirectionalLightTrans.SetPosition(pos);
}
