#pragma once
#include "pch.h"
#include "Math/Vector2D.h"
#include "Math/Vector3D.h"
#include "Graphics/DX12Utilities.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Vertex
{
	Vector3D m_Position; // 頂点座標
	Vector3D m_Normal;    // 法線ベクトル
	Vector2D m_TexCoord;    // UV座標
	Vector3D m_Tangent;    // 接線ベクトル
};

class Texture;

class Mesh
{
public:
	Mesh(Renderer* pRenderer, const aiMesh* pSrcMesh);
	~Mesh();
	D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return m_VBV; }
	D3D12_INDEX_BUFFER_VIEW GetIBV() const { return m_IBV; }
	uint32_t GetIndexCount() const { return m_IndexCount; }
	uint32_t GetMaterialIndex() const { return m_MaterialIndex; }
	void SetMaterialIndex(uint32_t index) { m_MaterialIndex = index; }
	void SetDiffuseTex(Texture* pTexture) { m_pDiffuseTexture = pTexture; }
	Texture* GetDiffuseTex() const { return m_pDiffuseTexture; }
	void SetNormalTex(Texture* pTexture) { m_pNormalTextre = pTexture; }
	Texture* GetNormalTex() const { return m_pNormalTextre; }
	void SetGLTFMetaricRoughnessTex(Texture* pTexture) { m_pGLTFMetaricRoughnessTexture = pTexture; }
	Texture* GetGLTFMetaricRoughnessTex() const { return m_pGLTFMetaricRoughnessTexture; }
	void SetShinessTex(Texture* pTexture) { m_pShinessTexture = pTexture; }
	Texture* GetShinessTex() const { return m_pShinessTexture; }
	void SetSpecularTex(Texture* pTexture) { m_pSpecularTexture = pTexture; }
	Texture* GetSpecularTex() const { return m_pSpecularTexture; }
	std::string m_Name;

private:
	void UploadBuffers(ID3D12Device* pDevice);

	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	uint32_t m_MaterialIndex = -1;

	Texture* m_pDiffuseTexture = nullptr;
	Texture* m_pNormalTextre = nullptr;
	Texture* m_pGLTFMetaricRoughnessTexture = nullptr;
	Texture* m_pShinessTexture = nullptr;
	Texture* m_pSpecularTexture = nullptr;

	// 頂点、インデックスデータ
	ComPtr<ID3D12Resource> m_pVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VBV = {};

	ComPtr<ID3D12Resource> m_pIB = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IBV = {};

	uint32_t m_IndexCount = 0;
	Renderer* m_pRenderer = nullptr;
};