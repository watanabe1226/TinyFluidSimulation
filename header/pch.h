#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <string>
#include <chrono>
#include <filesystem>
#include <unordered_map>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
#define SMALL_NUMBER 1.e-8f

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")
#ifdef _DEBUG
// Debug\¬‚Ì‚Æ‚« (––”ö‚É 'd' ‚ª‚Â‚­‚±‚Æ‚ª‘½‚¢)
#pragma comment(lib, "assimp-vc143-mtd.lib")
#else
// Release\¬‚Ì‚Æ‚«
#pragma comment(lib, "assimp-vc143-mt.lib")
#endif