#pragma once
#include "WindowData.h"

class DirectX11Framework;
class ForwardRenderer;
class LightLoader;
class WindowHandler;
class SpriteRenderer;
class Scene;
class Camera;
class Skybox;
class TextInstance;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

class CGraphicsEngine
{
public:
	CGraphicsEngine();
	~CGraphicsEngine();

	bool Init(const Window::WindowData& aWindowData);
	void BeginFrame(float aClearColor[4]);
	void RenderFrame();
	void EndFrame();


	WindowHandler* GetWindowHandler();
	DirectX11Framework* GetFrameWork();
	LightLoader* GetLightLoader();
	class SpriteRenderer* GetSpriteRenderer();
	class RenderManager* GetRendreManarger();

	void Imgui();


private:
	class RenderManager* myRendreManarger;

	DirectX11Framework* myFrameWork;
	WindowHandler* myWindowHandler;
	LightLoader* myLightLoader;
};