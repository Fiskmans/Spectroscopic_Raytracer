#pragma once
#include <Windows.h>
#include "WindowData.h"


class DirectX11Framework;


class WindowHandler
{
public:
	WindowHandler();
	~WindowHandler();

	static LRESULT CALLBACK WinProc(_In_ HWND aHWND, _In_ UINT aUMsg, _In_ WPARAM aWParam, _In_ LPARAM aLParam);
	bool Init(Window::WindowData aWindowData, DirectX11Framework* aFramework);
	HWND GetWindowHandle();

	unsigned short GetWidth();
	unsigned short GetHeight();


private:
	HWND myWindowHandle;
	static Window::WindowData myWindowData;
	static DirectX11Framework* myFrameWork;
	UINT myTaskbarButtonCreatedMessageId;
	DWORD myWindowStyle;


};

