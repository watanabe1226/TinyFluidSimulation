#pragma once
#include "pch.h"
#include "Graphics/Window.h"

class DX12Commands
{
public:
	DX12Commands(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~DX12Commands();

	void ExecuteCommandList();
	void ResetCommand();

	void WaitGpu(uint32_t timeout);

	ComPtr<ID3D12CommandQueue> GetCommandQueue() { return m_pCommandQueue; }
	ComPtr<ID3D12GraphicsCommandList> GetGraphicsCommandList() { return m_pCommandList;}

private:
	void CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);
	void CreateCommandList(D3D12_COMMAND_LIST_TYPE type);
	void CreateCommandAllocators(D3D12_COMMAND_LIST_TYPE type);
	void CreateFence();

	ID3D12Device* m_pDevice = nullptr;

	/// <summary>
	/// コマンドアロケータ
	/// </summary>
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators = { nullptr }; //!< コマンドアロケータ
	/// <summary>
	/// コマンドリスト
	/// </summary>
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	/// <summary>
	/// コマンドキュー
	/// </summary>
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	/// <summary>
	/// フェンス
	/// </summary>
	ComPtr<ID3D12Fence> m_pFence;
	/// <summary>
	/// フェンスイベント
	/// </summary>
	HANDLE m_FenceEvent;
	/// <summary>
	/// フェンスカウンター
	/// </summary>
	uint64_t m_FenceCounter = 0;

	/// <summary>
	/// アロケータのインデックス
	/// </summary>
	uint32_t m_AllocatorIndex = 0;
};