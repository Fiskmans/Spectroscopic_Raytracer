#pragma once
#include <array>
class PixelShader;

class DirectX11Framework;
class FullscreenRenderer
{
public:
	enum class Shader
	{
		COPY,
		MERGE,
		COUNT
	};

public:
	FullscreenRenderer();
	~FullscreenRenderer() = default;

	bool Init(DirectX11Framework* aFramework);

	void Render(Shader aEffect);
	void Render(PixelShader* aShader);

private:
	struct ID3D11DeviceContext* myContext = nullptr;
	class VertexShader* myVertexShader = nullptr;
	std::array<class PixelShader*, static_cast<int>(Shader::COUNT)> myPixelShaders;

};