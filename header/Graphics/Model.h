#pragma once
#include "pch.h"
#include "Graphics/Transform.h"
#include "Graphics/DX12Utilities.h"
#include "Graphics/Materials.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Mesh;
class Window;
class Renderer;

class Model
{
public:
	Model(Renderer* pRenderer, const std::wstring& filePath);
	Model(const Model& model) = delete;
	Model& operator=(const Model& model) = delete;
	~Model();
	void Update(float deltaTime);

	void Draw(const Matrix4x4& viewMat, const Matrix4x4& projMat);

	void SetPosition(const Vector3D& pos);
	void SetScale(const Vector3D& scale);

	const std::string& GetName() const { return m_Name; }
	Mesh* GetMesh(uint32_t index);
	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const;
	const TransformBuffer& GetTransform() const { return m_Transform; }
	MaterialBuffer m_MaterialBuffer;
	std::string m_Name;

private:
	void PerseMaterial(const aiMaterial* pSrcMat, Material& dstMat);

	void SetTextureId(const aiMaterial* pSrcMat,
		aiString& texturePath,
		const aiTextureType& texType,
		TextureID& texId);

	std::vector<std::unique_ptr<Mesh>> m_pMeshes;
	std::vector<Material> m_Materials;

	ID3D12GraphicsCommandList* m_pCommandList = nullptr;
	Window* m_pWindow = nullptr;
	Renderer* m_pRenderer = nullptr;
	TransformBuffer m_Transform;
	float count = 0.f;
};