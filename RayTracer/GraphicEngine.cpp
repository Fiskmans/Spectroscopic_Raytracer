#include "stdafx.h"


#include "GraphicEngine.h"
#include "DirectX11Framework.h"
#include "WindowHandler.h"
#include <array>
#include "RenderManager.h"
#include <d3d11.h>

#include "DebugTools.h"

CGraphicsEngine::CGraphicsEngine()
{
	myFrameWork = new DirectX11Framework();
	myWindowHandler = new WindowHandler();
	myRendreManarger = new RenderManager();
}

CGraphicsEngine::~CGraphicsEngine()
{
	myRendreManarger->Release();

	delete myFrameWork;
	myFrameWork = nullptr;
	delete myWindowHandler;
	myWindowHandler = nullptr;
	SAFE_DELETE(myRendreManarger);
}

bool CGraphicsEngine::Init(const Window::WindowData& aWindowData)
{
	if (!myWindowHandler->Init(aWindowData, myFrameWork))
	{
		SYSCRASH("Could not init window :c");
		return false;
	}
	if (!myFrameWork->Init(myWindowHandler->GetWindowHandle()))
	{
		SYSCRASH("Could not init graphics framework :c");
		return false;
	}


	if (!myRendreManarger->Init(myFrameWork, myWindowHandler))
	{
		SYSCRASH("Could not init rendremanargre :c");
		return false;
	}

	return true;
}

void CGraphicsEngine::BeginFrame(float aClearColor[4])
{
	myRendreManarger->BeginFrame(aClearColor);
}

void CGraphicsEngine::RenderFrame()
{
	myRendreManarger->Render();
}

void CGraphicsEngine::EndFrame()
{
	myRendreManarger->EndFrame();
	myFrameWork->EndFrame();
}

void CGraphicsEngine::Imgui()
{
	myRendreManarger->Imgui();
}

DirectX11Framework* CGraphicsEngine::GetFrameWork()
{
	return myFrameWork;
}

RenderManager* CGraphicsEngine::GetRendreManarger()
{
	return myRendreManarger;
}

WindowHandler* CGraphicsEngine::GetWindowHandler()
{
	return myWindowHandler;
}
