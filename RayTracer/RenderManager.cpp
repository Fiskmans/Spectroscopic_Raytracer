#include "stdafx.h"
#include "RenderManager.h"
#include "DirectX11Framework.h"
#include "WindowHandler.h"
#include "DebugDrawer.h"

#include "ShaderFlags.h"


float RenderManager::ourTotalTime = 0.f;


RenderManager::RenderManager() :
	myFrameworkPtr(nullptr),
#if USEIMGUI
	myShouldRenderWireFrame(false),
#endif // USEIMGUI
	myStateManerger(RenderStateManager()),
	myStartedAt(0)
{
}

bool RenderManager::Init(DirectX11Framework* aFramework, WindowHandler* aWindowHandler)
{
	if (!myStateManerger.Init(aFramework))
	{
		return false;
	}

	if (!myFullscreenRenderer.Init(aFramework))
	{
		return false;
	}
	if (!myFullscreenFactory.Init(aFramework))
	{
		return false;
	}

	DebugDrawer::GetInstance().Init(aFramework);
	DebugDrawer::GetInstance().SetColor(V4F(0.8f, 0.2f, 0.2f, 1.f));
	myFrameworkPtr = aFramework;
	myScreenSize = { aWindowHandler->GetWidth(), aWindowHandler->GetHeight() };
	if (!CreateTextures(aWindowHandler->GetWidth(), aWindowHandler->GetHeight()))
	{
		return false;
	}

	myStartedAt = Tools::GetTotalTime();
	Tools::ImguiHelperGlobals::Setup(aFramework->GetDevice(), aFramework->GetContext(), &myFullscreenRenderer, &myFullscreenFactory);

	return true;
}

bool RenderManager::Release()
{
	for (auto& i : myTextures)
	{
		i.Release();
	}
	return true;
}

void RenderManager::BeginFrame(float aClearColor[4])
{
	myTextures[static_cast<int>(Textures::BackBuffer)].ClearTexture({ 0 });
	myTextures[static_cast<int>(Textures::IntermediateTexture)].ClearTexture({ aClearColor[0],aClearColor[1],aClearColor[2],aClearColor[3] });
	myTextures[static_cast<int>(Textures::IntermediateDepth)].ClearDepth();
}

void RenderManager::EndFrame()
{
	Tools::ImguiHelperGlobals::ResetCounter();
}

void RenderManager::Render()
{
	ourTotalTime = Tools::GetTotalTime() - myStartedAt;

	myStateManerger.SetAllStates();

	FullscreenPass(
		{
			Textures::RayTraceOutput
		},
		Textures::IntermediateTexture,
		FullscreenRenderer::Shader::COPY);

	if (myDoDebugLines)
	{
		PERFORMANCETAG("Debuglines");
		myStateManerger.SetAllStates();
		myStateManerger.SetBlendState(RenderStateManager::BlendState::AlphaBlend);
		myTextures[static_cast<int>(Textures::IntermediateTexture)].SetAsActiveTarget(&myTextures[static_cast<int>(Textures::IntermediateDepth)]);
		DebugDrawer::GetInstance().Render(myCamera);
		myStateManerger.SetBlendState(RenderStateManager::BlendState::Disable);
	}

	FullscreenPass(
		{
			Textures::IntermediateTexture
		},
		Textures::BackBuffer,
		FullscreenRenderer::Shader::COPY);
}

FullscreenTexture RenderManager::GetOutputTexture()
{
	return myTextures[static_cast<int>(Textures::RayTraceOutput)];
}

float RenderManager::GetTotalTime()
{
	return ourTotalTime;
}

void RenderManager::Imgui()
{
	ImGui::Separator();
	Tools::ZoomableImGuiSnapshot(myTextures[static_cast<int>(Textures::BackBuffer)].GetResourceView(),ImVec2(192*2,108*2));
	ImGui::Separator();

	ImGui::EndGroup();

}

bool RenderManager::CreateTextures(const unsigned int aWidth, const unsigned int aHeight)
{
	ID3D11Texture2D* backBufferTexture = myFrameworkPtr->GetBackbufferTexture();
	if (!backBufferTexture)
	{
		SYSERROR("Could not get back buffer texture", "");
		return false;
	}

	myTextures[static_cast<int>(Textures::BackBuffer)] = myFullscreenFactory.CreateTexture(backBufferTexture);
	myTextures[static_cast<int>(Textures::IntermediateDepth)] = myFullscreenFactory.CreateDepth({ aWidth, aHeight },"Main Depth");

	myTextures[static_cast<int>(Textures::IntermediateTexture)] = myFullscreenFactory.CreateTexture({ aWidth, aHeight }, DXGI_FORMAT_R8G8B8A8_UNORM,"Intermediate texture");
	myTextures[static_cast<int>(Textures::RayTraceOutput)] = myFullscreenFactory.CreateTexture({ aWidth, aHeight }, DXGI_FORMAT_R32G32B32A32_FLOAT, "RaytraceOutput",D3D11_USAGE_DYNAMIC);


	for (auto& i : myTextures)
	{
		if (!i)
		{
			SYSERROR("Could not initialize all fullscreen textures", "");
			return false;
		}
	}
	return true;
}

void RenderManager::Resize(const unsigned int aWidth, const unsigned int aHeight)
{
	if (myScreenSize.x != aWidth || myScreenSize.y != aHeight)
	{
		myFrameworkPtr->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);

		for (auto& tex : myTextures)
		{
			tex.Release();
		}


		HRESULT result;
		IDXGISwapChain* chain = myFrameworkPtr->GetSwapChain();
		if (chain)
		{
			result = chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			if (FAILED(result))
			{
				SYSERROR("Could not resize swap chain buffer.", "");
			}
		}

		CreateTextures(aWidth, aHeight);
		myTextures[static_cast<int>(Textures::BackBuffer)].SetAsActiveTarget();

		myScreenSize = { aWidth,  aHeight };
	}
}

void RenderManager::FullscreenPass(std::vector<Textures> aSources, Textures aTarget, FullscreenRenderer::Shader aShader)
{
	UnbindTargets();
	UnbindResources();
	for (size_t i = 0; i < aSources.size(); i++)
	{
		myTextures[static_cast<int>(aSources[i])].SetAsResourceOnSlot(static_cast<unsigned int>(i));
	}
	myTextures[static_cast<int>(aTarget)].SetAsActiveTarget();
	myFullscreenRenderer.Render(aShader);
}

void RenderManager::UnbindResources()
{
	ID3D11ShaderResourceView* views[16] = { nullptr };
	myFrameworkPtr->GetContext()->PSSetShaderResources(0, 16, views);
}

void RenderManager::UnbindTargets()
{
	myFrameworkPtr->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
}

