#include "Graphics/DX12Device.h"
#include "Graphics/DX12Utilities.h"

DX12Device::DX12Device()
{
	// デバッグレイヤーを有効化
	DegugLayer();

	// デバイスの生成
	auto hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(m_pDevice.GetAddressOf())
	);
	DX12Utility::ThrowIfFailed(hr);
}

void DX12Device::DegugLayer()
{
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> debug;
	auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));
	if (SUCCEEDED(hr))
	{
		debug->EnableDebugLayer();
	}
#endif
}
