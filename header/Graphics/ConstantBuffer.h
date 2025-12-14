#pragma once
#include "pch.h"

class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device* pDevice, uint32_t size, const std::string& name, void* srcData);
	~ConstantBuffer();

	void Init(ID3D12Device* pDevice, uint32_t size, const std::string& name, void* srcData = nullptr);
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const { return m_Desc.BufferLocation; }
	void* GetPtr() const { return m_pMappedPtr; }
	D3D12_CONSTANT_BUFFER_VIEW_DESC GetDesc() const { return m_Desc; }
	void CopyToVRAM(void* data);

	//! @brief メモリマッピング済みのポインタを取得
	template<typename T>
	T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

private:
	ComPtr<ID3D12Resource> m_pCB = nullptr;
	uint32_t m_allocSize = 0;						//!< バッファのサイズ 256にアライメント
	uint32_t m_size = 0;						//!< アライメント前のサイズ
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc = {};	//!< 定数バッファビューの構成設定
	void* m_pMappedPtr = nullptr;					//!< CPU側からアクセスする定数バッファのアドレス

};