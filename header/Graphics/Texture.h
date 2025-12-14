#pragma once
#include "pch.h"

class DX12DescriptorHeap;
class Renderer;

class Texture
{
public:
	Texture(Renderer* pRenderer, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
	~Texture();

	uint32_t GetSRVIndex() const { return srvIndex; }
	ComPtr<ID3D12Resource> GetResource() const { return m_pResource; }
	ID3D12Resource* GetResourcePtr() const { return m_pResource.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPULocation() const;

private:
	D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(D3D12_RESOURCE_DESC desc);
	ComPtr<ID3D12Resource> m_pResource = nullptr;
	ComPtr<ID3D12Resource> m_pUploadResource = nullptr;
	uint32_t srvIndex = 0;
	DX12DescriptorHeap* SRVHeap = nullptr;

	std::wstring FileExtension(const std::wstring& filePath);
	std::wstring ExChangeFileExtension(const std::wstring& filePath);
	std::unordered_map<std::wstring, Texture*> m_pTextures;
};