#include "Graphics/RenderStages/SceneStage.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/Model.h"
#include "Graphics/Camera.h"
#include "Graphics/DepthBuffer.h"
#include "Framework/Renderer.h"
#include "Framework/Scene.h"
#include "Utilities/Utility.h"

#include "Graphics/RenderStages/ShadowStage.h"
#include "Graphics/RenderStages/IBLBakerStage.h"

#include <backends/imgui_impl_dx12.h>

SceneStage::SceneStage(Renderer* pRenderer, ShadowStage* pShadowStage, IBLBakerStage* pIBLBakerStage)
	: RenderStage(pRenderer), m_pShadowStage(pShadowStage), m_IBLBakerStage(pIBLBakerStage)
{
	CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);
}

SceneStage::SceneStage(Renderer* pRenderer) : RenderStage(pRenderer)
{
	CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);
}

SceneStage::~SceneStage()
{
}

void SceneStage::SetScene(Scene* newScene)
{
	m_pScene = newScene;
	m_pCamera = m_pScene->GetCamera();
}

void SceneStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
	if (m_pScene == nullptr)
	{
		assert(false && "シーンがnullです");
		return;
	}

	pCmdList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
	pCmdList->SetPipelineState(m_pPSO->GetPipelineStatePtr());

	// クリアカラーの設定
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	auto rtv = m_pWindow->GetCurrentScreenRTV();
	auto dsv = m_pWindow->GetDepthDSV();
	m_pRenderer->BindAndClearRenderTarget(m_pWindow, &rtv, &dsv, clearColor);

	auto cameraPos = m_pCamera->GetPosition();
	auto cameraPosVec4D = Vector4D(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
	auto view = m_pCamera->GetView();
	auto proj = m_pCamera->GetProj();
	pCmdList->SetGraphicsRoot32BitConstants(1, 4, &cameraPosVec4D, 0);
	pCmdList->SetGraphicsRootDescriptorTable(7, m_IBLBakerStage->GetHandleGPU_DFG());
	pCmdList->SetGraphicsRootDescriptorTable(8, m_IBLBakerStage->GetHandleGPU_DiffuseLD());
	pCmdList->SetGraphicsRootDescriptorTable(9, m_IBLBakerStage->GetHandleGPU_SpecularLD());
	pCmdList->SetGraphicsRootDescriptorTable(10, m_pShadowStage->GetDepthBuffer()->GetSRV());

	// ライト行列
	m_ShadowLightData.ViewProj = m_pShadowStage->GetVPMat();
	m_ShadowLightData.Direction = m_pShadowStage->GetLightDir();
	m_ShadowLightData.Padding = 0.0f;
	auto lightVP = m_pShadowStage->GetVPMat();
	pCmdList->SetGraphicsRoot32BitConstants(3, 20, &m_ShadowLightData, 0);

	for (const auto& model : m_pScene->GetModels())
	{
		model->Draw(view, proj);
	}
}

void SceneStage::CreateRootSignature(Renderer* pRenderer)
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// ルートパラメータの設定
	D3D12_DESCRIPTOR_RANGE BaseColorRange = {};
	// t0 basecolor
	BaseColorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	BaseColorRange.NumDescriptors = 1;
	BaseColorRange.BaseShaderRegister = 0; // t0
	BaseColorRange.RegisterSpace = 0;
	BaseColorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t1 normal
	D3D12_DESCRIPTOR_RANGE NormalRange = {};
	NormalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	NormalRange.NumDescriptors = 1;
	NormalRange.BaseShaderRegister = 1; // t1
	NormalRange.RegisterSpace = 0;
	NormalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t2 gltf roughnessMetallic
	D3D12_DESCRIPTOR_RANGE RoughtnessMetallicRange = {};
	RoughtnessMetallicRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	RoughtnessMetallicRange.NumDescriptors = 1;
	RoughtnessMetallicRange.BaseShaderRegister = 2; // t2
	RoughtnessMetallicRange.RegisterSpace = 0;
	RoughtnessMetallicRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t3 DFG 
	D3D12_DESCRIPTOR_RANGE DFGRange = {};
	DFGRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DFGRange.NumDescriptors = 1;
	DFGRange.BaseShaderRegister = 3; // t3
	DFGRange.RegisterSpace = 0;
	DFGRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t4 Diffuse DL
	D3D12_DESCRIPTOR_RANGE DiffuseDLRange = {};
	DiffuseDLRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DiffuseDLRange.NumDescriptors = 1;
	DiffuseDLRange.BaseShaderRegister = 4; // t4
	DiffuseDLRange.RegisterSpace = 0;
	DiffuseDLRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t5 Specular DL
	D3D12_DESCRIPTOR_RANGE SpecularDLRange = {};
	SpecularDLRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	SpecularDLRange.NumDescriptors = 1;
	SpecularDLRange.BaseShaderRegister = 5; // t5
	SpecularDLRange.RegisterSpace = 0;
	SpecularDLRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t6 shadowmap
	D3D12_DESCRIPTOR_RANGE shadowRange = {};
	shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowRange.NumDescriptors = 1;
	shadowRange.BaseShaderRegister = 6; // t6
	shadowRange.RegisterSpace = 0;
	shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ
	D3D12_ROOT_PARAMETER param[11] = {};

	// Transform CB : RootCBV
	param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param[0].Descriptor.ShaderRegister = 0; // b0
	param[0].Descriptor.RegisterSpace = 0;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// CameraPosition CB : RootCBV
	param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[1].Constants.ShaderRegister = 1;  // b1
	param[1].Constants.RegisterSpace = 0;
	param[1].Constants.Num32BitValues = 4; // float4つ
	param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Material CB : RootCBV
	param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param[2].DescriptorTable.NumDescriptorRanges = 2;  // b2
	param[2].Descriptor.RegisterSpace = 0;
	param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// LIghtMat // シャドウマップ計算用
	param[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[3].Constants.ShaderRegister = 3;  // b3
	param[3].Constants.RegisterSpace = 0;
	param[3].Constants.Num32BitValues = 20; // Mat4x4 + vector3 + pad
	param[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Textures Table : DescriptorTable t0
	param[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[4].DescriptorTable.NumDescriptorRanges = 1;
	param[4].DescriptorTable.pDescriptorRanges = &BaseColorRange;
	param[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[5].DescriptorTable.NumDescriptorRanges = 1;
	param[5].DescriptorTable.pDescriptorRanges = &NormalRange;
	param[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[6].DescriptorTable.NumDescriptorRanges = 1;
	param[6].DescriptorTable.pDescriptorRanges = &RoughtnessMetallicRange;
	param[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[7].DescriptorTable.NumDescriptorRanges = 1;
	param[7].DescriptorTable.pDescriptorRanges = &DFGRange;
	param[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[8].DescriptorTable.NumDescriptorRanges = 1;
	param[8].DescriptorTable.pDescriptorRanges = &DiffuseDLRange;
	param[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	param[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[9].DescriptorTable.NumDescriptorRanges = 1;
	param[9].DescriptorTable.pDescriptorRanges = &SpecularDLRange;
	param[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ShadowMap Table : DescriptorTable t1
	param[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[10].DescriptorTable.NumDescriptorRanges = 1;
	param[10].DescriptorTable.pDescriptorRanges = &shadowRange;
	param[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// スタティックサンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc[7] = {};
	samplerDesc[0] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 0);
	samplerDesc[1] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 1);
	samplerDesc[2] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 2);
	samplerDesc[3] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 3);
	samplerDesc[4] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 4);
	samplerDesc[5] = SetStaticSamplerDesc(DX12Utility::SamplerState::LinearWrap, 5);
	samplerDesc[6] = SetStaticSamplerDesc(DX12Utility::SamplerState::ShadowMapSampler, 6);

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(param);
	desc.pParameters = param;
	desc.NumStaticSamplers = _countof(samplerDesc);
	desc.pStaticSamplers = samplerDesc;
	desc.Flags = flag;

	// ルートシグネチャの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &desc);
}

D3D12_STATIC_SAMPLER_DESC& SceneStage::SetStaticSamplerDesc(DX12Utility::SamplerState samplerState, uint32_t reg)
{
	D3D12_STATIC_SAMPLER_DESC desc = {};
	desc.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
	desc.MaxAnisotropy = 1;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	desc.MinLOD = -D3D12_FLOAT32_MAX;
	desc.MaxLOD = +D3D12_FLOAT32_MAX;
	desc.ShaderRegister = reg;
	desc.RegisterSpace = 0;
	desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	switch (samplerState)
	{
	case DX12Utility::SamplerState::PointWrap:
	{
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	break;

	case DX12Utility::SamplerState::PointClamp:
	{
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
	break;

	case DX12Utility::SamplerState::LinearWrap:
	{
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	break;

	case DX12Utility::SamplerState::LinearClamp:
	{
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
	break;

	case DX12Utility::SamplerState::AnisotropicWrap:
	{
		desc.Filter = D3D12_FILTER_ANISOTROPIC;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
	}
	break;

	case DX12Utility::SamplerState::AnisotropicClamp:
	{
		desc.Filter = D3D12_FILTER_ANISOTROPIC;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
	}
	break;

	case DX12Utility::SamplerState::ShadowMapSampler:
	{
		desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}
	break;
	default:
	{
		assert(false && "未対応のサンプラーステートです");
	}
	break;
	}

	return desc;
}

void SceneStage::CreatePipeline(Renderer* pRenderer)
{
	// 入力レイアウトの設定
	D3D12_INPUT_ELEMENT_DESC elements[4];
	elements[0].SemanticName = "POSITION";
	elements[0].SemanticIndex = 0;
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].InputSlot = 0;
	elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[0].InstanceDataStepRate = 0;

	elements[1].SemanticName = "NORMAL";
	elements[1].SemanticIndex = 0;
	elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[1].InputSlot = 0;
	elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[1].InstanceDataStepRate = 0;

	elements[2].SemanticName = "TEXCOORD";
	elements[2].SemanticIndex = 0;
	elements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[2].InputSlot = 0;
	elements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[2].InstanceDataStepRate = 0;

	elements[3].SemanticName = "TANGENT";
	elements[3].SemanticIndex = 0;
	elements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[3].InputSlot = 0;
	elements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	elements[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elements[3].InstanceDataStepRate = 0;

	// ラスタライザーステートの設定
	D3D12_RASTERIZER_DESC descRS;
	descRS.FillMode = D3D12_FILL_MODE_SOLID;
	descRS.CullMode = D3D12_CULL_MODE_NONE;
	descRS.FrontCounterClockwise = FALSE;
	descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descRS.DepthClipEnable = FALSE;
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
	ComPtr<ID3DBlob> psBlob;

	// 頂点シェーダー読み込み
	static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";
	auto hr = D3DReadFileToBlob((ShaderFilePathName + L"DefaultVS.cso").c_str(), vsBlob.GetAddressOf());
	ThrowFailed(hr);

	// ピクセルシェーダー読み込み
	hr = D3DReadFileToBlob((ShaderFilePathName + L"DefaultPS.cso").c_str(), psBlob.GetAddressOf());
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
	desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	desc.RasterizerState = descRS;
	desc.BlendState = descBS;
	desc.DepthStencilState = descDSS;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// パイプラインステートの生成
	auto pDevice = m_pRenderer->GetDevice().Get();
	m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}
