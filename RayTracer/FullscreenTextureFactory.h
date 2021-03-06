#pragma once

#include "FullscreenTexture.h"
#include "Vector2.hpp"
#include <dxgiformat.h>

class DirectX11Framework;
struct ID3D11Texture2D;

class FullscreenTextureFactory
{
public:
	FullscreenTextureFactory() = default;
	~FullscreenTextureFactory() = default;

	bool Init(DirectX11Framework* aFramework);

	FullscreenTexture CreateTexture(CU::Vector2<unsigned int> aSize, DXGI_FORMAT aFormat,const std::string& aName,D3D11_USAGE aUsage = D3D11_USAGE_DEFAULT);
	FullscreenTexture CreateTexture(ID3D11Texture2D* aTexture, bool aIsRenderTarget = true);
	FullscreenTexture CreateDepth(CU::Vector2<unsigned int> aSize, const std::string& aName);

private:
	DirectX11Framework* myFramework = nullptr;
};

