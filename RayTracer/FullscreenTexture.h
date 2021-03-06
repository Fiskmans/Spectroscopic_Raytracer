#pragma once
#include "Vector4.hpp"

class FullscreenTexture
{
public:
	friend class FullscreenTextureFactory;
	friend class GBuffer;

public:
	FullscreenTexture() = default;
	~FullscreenTexture() = default;
	void Release();

	void ClearTexture(V4F aClearColor = {});
	void ClearDepth(float aClearDepth = 1.0f, unsigned int aClearStencil = 0);
	void SetAsActiveTarget(FullscreenTexture* aDepth = nullptr);
	void SetAsResourceOnSlot(unsigned int aSlot);

	ID3D11ShaderResourceView* GetResourceView();

	operator bool()
	{
		return !!myContext && !!myTexture && (!!myRenderTarget || !!myDepth || !!myShaderResource) && !!myViewport;
	}

private:
	struct ID3D11DeviceContext* myContext = nullptr; //TODO: Handle pointer so no leak
	struct ID3D11Texture2D* myTexture = nullptr;
	struct ID3D11RenderTargetView* myRenderTarget = nullptr;
	struct ID3D11DepthStencilView* myDepth = nullptr;
	struct ID3D11ShaderResourceView* myShaderResource = nullptr;
	struct D3D11_VIEWPORT* myViewport = nullptr;
};

