#pragma once
#include "FullscreenTextureFactory.h"
#include "FullscreenTexture.h"

#include "FullscreenRenderer.h"

#include "RenderStateManager.h"

class SpriteRenderer;
class Camera;

class RenderManager
{
public:
	RenderManager();
	~RenderManager() = default;

	bool Init(class DirectX11Framework* aFramework, class WindowHandler* aWindowHandler);
	bool Release();

	void BeginFrame(float aClearColor[4]);
	void EndFrame();

	void Render();

	FullscreenTexture GetOutputTexture();
	static float GetTotalTime();

	void Imgui();
	bool myDoDebugLines = true;

	Camera* myCamera;
private:

	float myStartedAt;
	static float ourTotalTime;
	RenderStateManager myStateManerger;
	FullscreenRenderer myFullscreenRenderer;
	class DirectX11Framework* myFrameworkPtr;
	struct ID3D11ShaderResourceView* myBoneTextureView = nullptr;

	struct ID3D11Texture2D* myBoneBufferTexture = nullptr;

	FullscreenTextureFactory myFullscreenFactory;

	enum class Textures
	{
		BackBuffer,
		IntermediateTexture,
		IntermediateDepth,
		RayTraceOutput,
		Count
	};
	std::array<FullscreenTexture, static_cast<int>(Textures::Count)> myTextures;

	CommonUtilities::Vector2<unsigned int> myScreenSize;

	bool CreateTextures(const unsigned int aWidth, const unsigned int aHeight);
	void Resize(const unsigned int aWidth, const unsigned int aHeight);

	void FullscreenPass(std::vector<Textures> aSources,Textures aTarget, FullscreenRenderer::Shader aShader);
	void UnbindResources();
	void UnbindTargets();
};

