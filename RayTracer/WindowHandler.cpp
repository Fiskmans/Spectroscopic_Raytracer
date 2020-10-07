#include "stdafx.h"
#include "WindowHandler.h"
#include "DirectX11Framework.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




DirectX11Framework* WindowHandler::myFrameWork = nullptr;
Window::WindowData WindowHandler::myWindowData;

WindowHandler::WindowHandler() :
	myWindowHandle(NULL),
	myTaskbarButtonCreatedMessageId(0),
	myWindowStyle(0)
{
}


WindowHandler::~WindowHandler()
{
	if (CloseWindow(myWindowHandle) != TRUE)
	{
		SYSERROR("Could not close window","");
	}
	Logger::Shutdown();
}

LRESULT CALLBACK WindowHandler::WinProc(_In_ HWND aHWND, _In_ UINT aUMsg, _In_ WPARAM aWParam, _In_ LPARAM aLParam)
{
	static WindowHandler* windowHandler = nullptr;
	static bool hwndIsSet = false;


	if (windowHandler)
	{
		if (aUMsg == windowHandler->myTaskbarButtonCreatedMessageId)
		{
			Logger::SetupIcons(aHWND);
		}
	}
	static bool isClosing = false;
	if (aUMsg == WM_DESTROY || aUMsg == WM_CLOSE)
	{
		isClosing = true;
		PostQuitMessage(0);
		return 0;
	}
	if (isClosing)
	{
		return 0;
	}

	if (aUMsg == WM_CREATE)
	{
		CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(aLParam);
		windowHandler = reinterpret_cast<WindowHandler*>(createStruct->lpCreateParams);
	}
	else if (myFrameWork && aUMsg == WM_SIZE)
	{
		myFrameWork->Resize(aHWND);

		RECT rect;
		GetWindowRect(static_cast<HWND>(aHWND), &rect);

		myWindowData.myX = static_cast<unsigned short>(rect.left);
		myWindowData.myY = static_cast<unsigned short>(rect.top);
		myWindowData.myWidth = static_cast<unsigned short>(rect.right - rect.left);
		myWindowData.myHeight = static_cast<unsigned short>(rect.bottom - rect.top);
	}

	if (ImGui_ImplWin32_WndProcHandler(aHWND, aUMsg, aWParam, aLParam))
	{
		return TRUE;
	}
	return DefWindowProc(aHWND, aUMsg, aWParam, aLParam);
}

bool WindowHandler::Init(Window::WindowData aWindowData, DirectX11Framework* aFramework)
{
	if (aFramework)
	{
		WNDCLASSW windowClass = {};
		windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = WindowHandler::WinProc;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = L"Gamla Bettan";
		RegisterClassW(&windowClass);
		myWindowStyle = WS_VISIBLE;

#if USEFULLSCREEN
		myWindowStyle |= WS_MAXIMIZE | WS_OVERLAPPED;
		aWindowData.myX = 0;
		aWindowData.myY = 0;
		aWindowData.myWidth = CAST(unsigned short, GetSystemMetrics(SM_CXSCREEN));
		aWindowData.myHeight = CAST(unsigned short,  GetSystemMetrics(SM_CYSCREEN));
#else
		myWindowStyle |= WS_OVERLAPPEDWINDOW;
#endif

		myWindowHandle = CreateWindowW(L"Gamla Bettan", L"MoonView Mountain", myWindowStyle,
										aWindowData.myX, aWindowData.myY, aWindowData.myWidth, aWindowData.myHeight, 
										nullptr, nullptr, nullptr, this);



		myTaskbarButtonCreatedMessageId = RegisterWindowMessage(L"TaskbarButtonCreated");

		ChangeWindowMessageFilterEx(myWindowHandle, myTaskbarButtonCreatedMessageId, MSGFLT_ALLOW, NULL);
	}


	myWindowData = aWindowData;
	myFrameWork = aFramework;
	return true;
}

HWND WindowHandler::GetWindowHandle()
{
	return myWindowHandle;
}

unsigned short WindowHandler::GetWidth()
{
	return myWindowData.myWidth;
}

unsigned short WindowHandler::GetHeight()
{
	return myWindowData.myHeight;
}
