#pragma once
#include <array>

struct ID3D11Device;

class RenderStateManager
{
public:
	enum class BlendState
	{
		Disable,
		AlphaBlend,
		AdditativeBlend,
		Count
	};
	enum class DepthStencilState
	{
		Default,
		ReadOnly,
		OnlyCovered,
		Count
	};
	enum class RasterizerState
	{
		Default,
		Wireframe,
		NoBackfaceCulling,
		CullFrontFacing,
		Count
	};
	enum class SamplerState
	{
		Trilinear,
		Point,
		Count
	};

public:
	RenderStateManager() = default;
	~RenderStateManager() = default;

	bool Init(class DirectX11Framework* aFramework);

	void SetBlendState(BlendState aState);
	void SetDepthStencilState(DepthStencilState aState);
	void SetRasterizerState(RasterizerState aState);
	void SetSamplerState(SamplerState aState);

	void SetAllStates(BlendState aBlendState = BlendState::Disable, DepthStencilState aDepthState = DepthStencilState::Default, RasterizerState aRasterizerState = RasterizerState::Default, SamplerState aSamplerState = SamplerState::Trilinear);

private:
	bool CreateBlendStates(ID3D11Device* aDevice);
	bool CreateDepthStencilStates(ID3D11Device* aDevice);
	bool CreateRasterizerStates(ID3D11Device* aDevice);
	bool CreateSamplerStates(ID3D11Device* aDevice);

	struct ID3D11DeviceContext* myContext;
	std::array<struct ID3D11BlendState*, static_cast<int>(BlendState::Count)> myBlendStates;
	std::array<struct ID3D11DepthStencilState*, static_cast<int>(DepthStencilState::Count)> myDepthStencilStates;
	std::array<struct ID3D11RasterizerState*, static_cast<int>(RasterizerState::Count)> myRasterizerStates;
	std::array<struct ID3D11SamplerState*, static_cast<int>(SamplerState::Count)> mySamplerStates;
};

