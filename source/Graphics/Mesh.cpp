#include "Graphics/Mesh.h"
#include "Graphics/DX12Utilities.h"
#include "Graphics/DX12Device.h"
#include "Framework/Renderer.h"

Mesh::Mesh(Renderer* pRenderer, const aiMesh* pSrcMesh)
{
	m_MaterialIndex = pSrcMesh->mMaterialIndex;
	m_Name = pSrcMesh->mName.C_Str();
	m_pRenderer = pRenderer;

	m_Vertices.shrink_to_fit();
	m_Vertices.resize(pSrcMesh->mNumVertices);
	Vector3D zero3D = Vector3D();
	for (auto i = 0u; i < pSrcMesh->mNumVertices; ++i)
	{
		auto pPosition = &(pSrcMesh->mVertices[i]);
		m_Vertices[i].m_Position = Vector3D(pPosition->x, pPosition->y, pPosition->z);
		auto pNormal = &(pSrcMesh->mNormals[i]);
		m_Vertices[i].m_Normal = Vector3D(pNormal->x, pNormal->y, pNormal->z);
		if (pSrcMesh->HasTextureCoords(0))
		{
			auto pTexCoord = &(pSrcMesh->mTextureCoords[0][i]);
			m_Vertices[i].m_TexCoord = Vector2D(pTexCoord->x, pTexCoord->y);
		}
		else
		{
			m_Vertices[i].m_TexCoord = Vector2D();
		}
		if (pSrcMesh->HasTangentsAndBitangents())
		{
			auto pTangent = &(pSrcMesh->mTangents[i]);
			m_Vertices[i].m_Tangent = Vector3D(pTangent->x, pTangent->y, pTangent->z);
		}
		else
		{
			m_Vertices[i].m_Tangent = Vector3D();
		}
	}

	// インデックス配列の作成
	m_Indices.shrink_to_fit();;
	m_Indices.resize(pSrcMesh->mNumFaces * 3);
	for (auto i = 0u; i < pSrcMesh->mNumFaces; ++i)
	{
		auto pFace = &(pSrcMesh->mFaces[i]);
		assert(pFace->mNumIndices == 3); // 三角形化しているはずなので3であることを確認
		m_Indices[i * 3 + 0] = pFace->mIndices[0];
		m_Indices[i * 3 + 1] = pFace->mIndices[1];
		m_Indices[i * 3 + 2] = pFace->mIndices[2];
	}

	UploadBuffers(pRenderer->GetDevice().Get());
}

Mesh::~Mesh()
{
}

void Mesh::UploadBuffers(ID3D12Device* pDevice)
{
	auto vertSize = m_Vertices.size() * sizeof(Vertex);
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
	auto hr = pDevice->CreateCommittedResource(
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
	if(m_Vertices.data())
	memcpy(ptr, m_Vertices.data(), vertSize);

	// マッピング解除
	m_pVB->Unmap(0, nullptr);

	// 頂点バッファビューの設定
	m_VBV.BufferLocation = m_pVB->GetGPUVirtualAddress();
	m_VBV.SizeInBytes = static_cast<UINT>(vertSize);
	m_VBV.StrideInBytes = static_cast<UINT>(sizeof(Vertex));

	auto indicesSize = sizeof(uint32_t) * m_Indices.size();

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
	memcpy(ptr, m_Indices.data(), indicesSize);

	// マッピング解除
	m_pIB->Unmap(0, nullptr);

	// インデックスバッファビューの設定
	m_IBV.BufferLocation = m_pIB->GetGPUVirtualAddress();
	m_IBV.Format = DXGI_FORMAT_R32_UINT;
	m_IBV.SizeInBytes = static_cast<UINT>(indicesSize);

	m_IndexCount = static_cast<uint32_t>(m_Indices.size());
	m_Vertices.clear();
	m_Indices.clear();
}
