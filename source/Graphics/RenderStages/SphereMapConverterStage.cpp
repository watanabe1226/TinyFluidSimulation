#include "Graphics/RenderStages/SphereMapConverterStage.h"
#include "Framework/Renderer.h"
#include "Graphics/DX12RootSignature.h"
#include "Graphics/DX12PipelineState.h"
#include "Graphics/DX12DescriptorHeap.h"
#include "Framework/Scene.h"
#include "Utilities/Utility.h"
#include "Math/MathUtility.h"
#include "Math/Vector2D.h"

SphereMapConverterStage::SphereMapConverterStage(Renderer* pRenderer,
    const D3D12_RESOURCE_DESC& sphereMapDesc) : RenderStage(pRenderer)
{
	m_pRenderer = pRenderer;
	CreateTexture(sphereMapDesc);
    CreateRootSignature(pRenderer);
	CreatePipeline(pRenderer);
}

SphereMapConverterStage::~SphereMapConverterStage()
{
}

void SphereMapConverterStage::DrawToCube(ID3D12GraphicsCommandList* pCmdList, D3D12_GPU_DESCRIPTOR_HANDLE shpereMapHandle)
{
    m_pRenderer->TransitionResource(m_pCubeTex.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto desc = m_pCubeTex->GetDesc();

    auto idx = 0;
    auto handleSRV = shpereMapHandle;
    auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto SRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    pCmdList->SetDescriptorHeaps(1, SRVHeap->GetHeap().GetAddressOf());
    for (auto i = 0; i < 6; ++i)
    {
        auto w = static_cast<uint32_t>(desc.Width);
        auto h = desc.Height;

        for (auto m = 0u; m < m_MipCount; ++m)
        {
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = float(w);
            viewport.Height = float(h);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;

            D3D12_RECT scissor = {};
            scissor.left = 0;
            scissor.right = w;
            scissor.top = 0;
            scissor.bottom = h;

            auto handleRTV = RTVHeap->GetCpuHandle(m_RTVIndeies[idx]);
            pCmdList->OMSetRenderTargets(1, &handleRTV, FALSE, nullptr);
            pCmdList->SetGraphicsRootSignature(m_pRootSignature->GetRootSignaturePtr());
            pCmdList->SetPipelineState(m_pPSO->GetPipelineStatePtr());
            pCmdList->SetGraphicsRoot32BitConstants(0,  48, &m_CBTrans[i], 0);
            pCmdList->SetGraphicsRootDescriptorTable(1, handleSRV);
            pCmdList->RSSetViewports(1, &viewport);
            pCmdList->RSSetScissorRects(1, &scissor);

            DrawSphere(pCmdList);

            w >>= 1;
            h >>= 1;

            if (w < 1)
            {
                w = 1;
            }

            if (h < 1)
            {
                h = 1;
            }

            idx++;
        }
    }

    m_pRenderer->TransitionResource(m_pCubeTex.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void SphereMapConverterStage::RecordStage(ID3D12GraphicsCommandList* pCmdList)
{
}

D3D12_RESOURCE_DESC SphereMapConverterStage::GetCubeMapDesc() const
{
    return m_pCubeTex->GetDesc();
}

D3D12_CPU_DESCRIPTOR_HANDLE SphereMapConverterStage::GetCubeMapHandleCPU() const
{
    auto SRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return SRVHeap->GetCpuHandle(m_SRVIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE SphereMapConverterStage::GetCubeMapHandleGPU() const
{
    auto SRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return SRVHeap->GetGpuHandle(m_SRVIndex);
}

void SphereMapConverterStage::DrawSphere(ID3D12GraphicsCommandList* pCmdList)
{
    pCmdList->IASetIndexBuffer(&m_IBV);
    pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
    pCmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

void SphereMapConverterStage::CreateTexture(const D3D12_RESOURCE_DESC& sphereMapDesc)
{
    // 暫定サイズを求める
    auto tempSize = sphereMapDesc.Width / 4;

    uint32_t currSize = 1;
    uint32_t currMipLevels = 1;
    uint32_t prevSize = 0;
    uint32_t size = 0;
    uint32_t mipLevels = 0;

    // ループしながら一番近い2のべき乗を探す
    for (;;)
    {
        // 暫定サイズが含まれるかどうか判定
        if (prevSize < tempSize && tempSize <= currSize)
        {
            size = currSize;
            mipLevels = currMipLevels;
            break;
        }

        // 前のサイズを更新
        prevSize = currSize;

        // テクスチャサイズを２倍
        currSize <<= 1;

        // みっぷレベルをカウントアップ
        currMipLevels++;
    }

    m_MipCount = mipLevels;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = size;
    desc.Height = UINT(size);
    desc.DepthOrArraySize = 6;
    desc.MipLevels = m_MipCount;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    auto pDevice = m_pRenderer->GetDevice().Get();
    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(m_pCubeTex.GetAddressOf()));
    ThrowFailed(hr);

    // SRVの作成
    auto SRVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_SRVIndex = SRVHeap->GetNextAvailableIndex();

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.TextureCube.MipLevels = m_MipCount;
    viewDesc.TextureCube.MostDetailedMip = 0;
    viewDesc.TextureCube.ResourceMinLODClamp = 0;

    pDevice->CreateShaderResourceView(
        m_pCubeTex.Get(),
        &viewDesc,
        SRVHeap->GetCpuHandle(m_SRVIndex)
    );

    // RTVの作成
    auto RTVHeap = m_pRenderer->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_RTVIndeies.resize(m_MipCount * 6);
    auto idx = 0;
    for (auto i = 0; i < 6; ++i)
    {
        for (auto m = 0u; m < m_MipCount; ++m)
        {
            m_RTVIndeies[idx] = RTVHeap->GetNextAvailableIndex();

            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.ArraySize = 1;
            desc.Texture2DArray.FirstArraySlice = i;
            desc.Texture2DArray.MipSlice = m;
            desc.Texture2DArray.PlaneSlice = 0;

            pDevice->CreateRenderTargetView(m_pCubeTex.Get(), &desc, RTVHeap->GetCpuHandle(m_RTVIndeies[idx]));

            idx++;
        }
    }

    // 変換バッファの生成
    auto pos = Vector3D(0.0f, 0.0f, 0.0f);
    Vector3D target[6] = {
            Vector3D(1.0f,  0.0f,  0.0f),  // +X
            Vector3D(-1.0f,  0.0f,  0.0f), // -X
            Vector3D(0.0f,  1.0f,  0.0f),  // +Y
            Vector3D(0.0f, -1.0f,  0.0f),  // -Y
            Vector3D(0.0f,  0.0f,  1.0f),  // +Z (DX左手系の前方) 
            Vector3D(0.0f,  0.0f, -1.0f),  // -Z (DX左手系の後方)
    };

    Vector3D upward[6] = {
        Vector3D(0.0f, 1.0f,  0.0f),   // +X Up
        Vector3D(0.0f, 1.0f,  0.0f),   // -X Up
        Vector3D(0.0f, 0.0f, -1.0f),   // +Y Up 
        Vector3D(0.0f, 0.0f,  1.0f),   // -Y Up 
        Vector3D(0.0f, 1.0f,  0.0f),   // +Z Up
        Vector3D(0.0f, 1.0f,  0.0f),   // -Z Up
    };

    for (auto i = 0u; i < 6; ++i)
    {
        m_CBTrans[i].World = Matrix4x4::Identity();
        m_CBTrans[i].View = Matrix4x4::setLookAtLH(pos, target[i], upward[i]);
        m_CBTrans[i].Proj = Matrix4x4::setPerspectiveFovLH(MathUtility::PI / 2, 1.0f, 0.1f, 10000.0f);
    }

    struct Vertex
    {
        Vector3D Position;
        Vector2D Texcoord;
    };

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    // 分割数
    auto tessellation = 20u;

    uint32_t verticalSegments = tessellation * 2;
    uint32_t horizontalSegments = tessellation * 2;

    // 球の半径.
    float radius = 10.0f;

    // 頂点座標を求める
    for (size_t i = 0; i <= verticalSegments; i++)
    {
        float v = 1 - (float)i / verticalSegments;
        float latitude = (i * MathUtility::PI / verticalSegments) - MathUtility::PI / 2;

        float dy = sinf(latitude);
        float dxz = cosf(latitude);

        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            float u = (float)j / horizontalSegments;

            // ★修正1： PI / 2 ではなく、PI * 2.0f (360度) に変更
            float longitude = j * MathUtility::PI * 2.0f / horizontalSegments;

            float dx = sinf(longitude);
            float dz = cosf(longitude);

            dx *= dxz;
            dz *= dxz;

            auto normal = Vector3D(dx, dy, dz);
            auto uv = Vector2D(u, v);

            Vertex vert;
            vert.Position = normal * radius;
            vert.Texcoord = uv;

            vertices.push_back(vert);
        }
    }

    uint32_t stride = horizontalSegments + 1;

    // 頂点インデックスを求める.
    // ★修正2： j <= ... ではなく、 j < ... に変更して無駄な縮退ポリゴンを防ぐ
    for (auto i = 0u; i < verticalSegments; i++)
    {
        for (auto j = 0u; j < horizontalSegments; j++)
        {
            auto nextI = i + 1;
            // j < horizontalSegments なので、% stride をしなくても配列外参照にはなりませんが、
            // 安全策として残しても問題ありません（挙動は変わりません）
            auto nextJ = j + 1;

            indices.push_back(i * stride + j);
            indices.push_back(nextI * stride + j);
            indices.push_back(i * stride + nextJ);

            indices.push_back(i * stride + nextJ);
            indices.push_back(nextI * stride + j);
            indices.push_back(nextI * stride + nextJ);
        }
    }

    auto vertSize = vertices.size() * sizeof(Vertex);
    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES vertProp = {};
    vertProp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込み可能なヒープ
    vertProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    vertProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    vertProp.CreationNodeMask = 1;
    vertProp.VisibleNodeMask = 1;

    // リソースの設定
    D3D12_RESOURCE_DESC vertDesc = {};
    vertDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertDesc.Alignment = 0;
    vertDesc.Width = static_cast<uint64_t>(vertSize); // 頂点データのサイズ
    vertDesc.Height = 1;
    vertDesc.DepthOrArraySize = 1;
    vertDesc.MipLevels = 1;
    vertDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertDesc.SampleDesc.Count = 1;
    vertDesc.SampleDesc.Quality = 0;
    vertDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // リソースを生成
    hr = pDevice->CreateCommittedResource(
        &vertProp,
        D3D12_HEAP_FLAG_NONE,
        &vertDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_pVB.GetAddressOf())
    );
    ThrowFailed(hr, "頂点バッファの生成に失敗しました");
    // マッピング
    void* ptr = nullptr;
    hr = m_pVB->Map(0, nullptr, &ptr);
    ThrowFailed(hr, "頂点バッファのマッピングに失敗しました");
    // 頂点データをマッピング先に設定
    if (vertices.data())
        memcpy(ptr, vertices.data(), vertSize);

    // マッピング解除
    m_pVB->Unmap(0, nullptr);

    // 頂点バッファビューの設定
    m_VBV.BufferLocation = m_pVB->GetGPUVirtualAddress();
    m_VBV.SizeInBytes = static_cast<UINT>(vertSize);
    m_VBV.StrideInBytes = static_cast<UINT>(sizeof(Vertex));

    auto indicesSize = sizeof(uint32_t) * indices.size();

    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES indicesProp = {};
    indicesProp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込み可能なヒープ
    indicesProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    indicesProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    indicesProp.CreationNodeMask = 1;
    indicesProp.VisibleNodeMask = 1;

    // リソースの設定
    D3D12_RESOURCE_DESC indicesDesc = {};
    indicesDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    indicesDesc.Alignment = 0;
    indicesDesc.Width = static_cast<uint64_t>(indicesSize); // インデックスデータのサイズ
    indicesDesc.Height = 1;
    indicesDesc.DepthOrArraySize = 1;
    indicesDesc.MipLevels = 1;
    indicesDesc.Format = DXGI_FORMAT_UNKNOWN;
    indicesDesc.SampleDesc.Count = 1;
    indicesDesc.SampleDesc.Quality = 0;
    indicesDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    indicesDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // リソースを生成
    hr = pDevice->CreateCommittedResource(
        &indicesProp,
        D3D12_HEAP_FLAG_NONE,
        &indicesDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_pIB.GetAddressOf())
    );
    ThrowFailed(hr, "インデックスバッファの生成に失敗しました");
    // マッピング
    ptr = nullptr;
    hr = m_pIB->Map(0, nullptr, &ptr);
    ThrowFailed(hr, "インデックスバッファのマッピングに失敗しました");

    // インデックスデータをマッピング先に設定
    memcpy(ptr, indices.data(), indicesSize);

    // マッピング解除
    m_pIB->Unmap(0, nullptr);

    // インデックスバッファビューの設定
    m_IBV.BufferLocation = m_pIB->GetGPUVirtualAddress();
    m_IBV.Format = DXGI_FORMAT_R32_UINT;
    m_IBV.SizeInBytes = static_cast<UINT>(indicesSize);

    m_IndexCount = static_cast<uint32_t>(indices.size());
    vertices.clear();
    indices.clear();
}

void SphereMapConverterStage::CreateRootSignature(Renderer* pRenderer)
{
    auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    // レンジの設定
    D3D12_DESCRIPTOR_RANGE range[1] = {};
    // range[1]: SRV (t0) -> SphereMapテクスチャ用
    range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range[0].NumDescriptors = 1;
    range[0].BaseShaderRegister = 0;
    range[0].RegisterSpace = 0;
    range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // ルートパラメータの設定
    D3D12_ROOT_PARAMETER param[2] = {};
    // param[0]: Vertex Shader用 (CBV)
    param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param[0].Constants.ShaderRegister = 0;  // b0
    param[0].Constants.RegisterSpace = 0;
    param[0].Constants.Num32BitValues = 48; // matrix4x4 * 3
    param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // param[1]: Pixel Shader用 (SRV)
    param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param[1].DescriptorTable.NumDescriptorRanges = 1;
    param[1].DescriptorTable.pDescriptorRanges = range;
    param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // スタティックサンプラーの設定
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // ルートシグネチャ記述子の設定
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = _countof(param);
    desc.NumStaticSamplers = 1;
    desc.pParameters = param;
    desc.pStaticSamplers = &sampler;
    desc.Flags = flag;

    // ルートシグネチャの生成
    auto pDevice = m_pRenderer->GetDevice().Get();
    m_pRootSignature = std::make_unique<DX12RootSignature>(pDevice, &desc);
}

void SphereMapConverterStage::CreatePipeline(Renderer* pRenderer)
{
    // 入力レイアウトの設定 (POSITION + TEXCOORD)
    D3D12_INPUT_ELEMENT_DESC elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ラスタライザーステート (CullNone)
    D3D12_RASTERIZER_DESC descRS = {};
    descRS.FillMode = D3D12_FILL_MODE_SOLID;
    descRS.CullMode = D3D12_CULL_MODE_NONE;
    descRS.FrontCounterClockwise = FALSE;
    descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    descRS.DepthClipEnable = TRUE;
    descRS.MultisampleEnable = FALSE;
    descRS.AntialiasedLineEnable = FALSE;
    descRS.ForcedSampleCount = 0;
    descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // レンダーターゲットのブレンド設定 (Opaque)
    D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };

    D3D12_BLEND_DESC descBS = {};
    descBS.AlphaToCoverageEnable = FALSE;
    descBS.IndependentBlendEnable = FALSE;
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        descBS.RenderTarget[i] = descRTBS;
    }

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    // シェーダー読み込み
    static const std::wstring ShaderFilePathName = Utility::GetCurrentDir() + L"/assets/shaders/";

    auto hr = D3DReadFileToBlob((ShaderFilePathName + L"SphereToCubeVS.cso").c_str(), vsBlob.GetAddressOf());
    ThrowFailed(hr, "SphereToCubeVS.cso read failed");

    hr = D3DReadFileToBlob((ShaderFilePathName + L"SphereToCubePS.cso").c_str(), psBlob.GetAddressOf());
    ThrowFailed(hr, "SphereToCubePS.cso read failed");

    // 深度ステンシルステート (DepthNone相当)
    D3D12_DEPTH_STENCIL_DESC descDSS = {};
    descDSS.DepthEnable = FALSE;
    descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    descDSS.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    descDSS.StencilEnable = FALSE;
    descDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    descDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS
    };
    descDSS.FrontFace = defaultStencilOp;
    descDSS.BackFace = defaultStencilOp;

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
    desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    auto pDevice = m_pRenderer->GetDevice().Get();
    m_pPSO = std::make_unique<DX12PipelineState>(pDevice, &desc);
}