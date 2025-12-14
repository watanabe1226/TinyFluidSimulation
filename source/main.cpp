#include "pch.h"
#include "Framework/Engine.h"


/// <summary>
/// エントリーポイントです．
/// </summary>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// ウィンドウのサイズを指定
	Engine engine(1920, 1080);
	engine.Run();

	return 0;
}