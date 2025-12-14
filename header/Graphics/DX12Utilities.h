#pragma once
#include "pch.h"
#include "Graphics/Window.h"
#include "Graphics/DX12Device.h"
#include "Graphics/DX12Commands.h"
#include "../external/d3dx12.h"

#define FILE_PREFIX __FILE__ "(" TO_STRING(__LINE__) "): " 
#define DX12MSGThrowIfFailed(hr, msg) DX12Utility::CheckResultCodeD3D12(hr, FILE_PREFIX msg)
#define DX12MSGThrowFailed(msg) DX12Utility::CheckCodeD3D12(msg FILE_PREFIX)
#define ThrowFailed(hr) DX12Utility::ThrowIfFailed(hr)

using TextureID = size_t;
namespace DX12Utility
{
    enum SamplerState
    {
        PointWrap,          //!< ポイントサンプリング - 繰り返し.
        PointClamp,         //!< ポイントサンプリング - クランプ.
        LinearWrap,         //!< トライリニアサンプリング - 繰り返し.
        LinearClamp,        //!< トライリニアサンプリング - クランプ.
        AnisotropicWrap,    //!< 異方性サンプリング - 繰り返し.
        AnisotropicClamp,   //!< 異方性サンプリング - クランプ.
		ShadowMapSampler,  //!< シャドウマップ用サンプラー.
    };

    class DX12Exception : public std::runtime_error
    {
    public:
        DX12Exception(const std::string& msg) : std::runtime_error(msg.c_str()) {}
    };

    inline void CheckResultCodeD3D12(HRESULT hr, const std::string errorMsg)
    {
        if (FAILED(hr))
            throw DX12Exception(errorMsg);
    }

    inline void CheckCodeD3D12(const std::string errorMsg)
    {
        throw DX12Exception(errorMsg);
    }

    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }

    // 文字列からハッシュ値を生成する関数
    inline constexpr TextureID StringHash(const wchar_t* str) {
        size_t hash = 14695981039346656037ULL; // FNV offset basis
        while (*str) {
            hash ^= static_cast<size_t>(*str++);
            hash *= 1099511628211ULL; // FNV prime
        }
        return hash;
    }
}