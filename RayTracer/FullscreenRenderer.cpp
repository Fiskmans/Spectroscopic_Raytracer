#include "stdafx.h"
#include "FullscreenRenderer.h"
#include "DirectX11Framework.h"
#include <d3d11.h>
#include "ShaderCompiler.h"

FullscreenRenderer::FullscreenRenderer()
{
	for (size_t i = 0; i < static_cast<size_t>(Shader::COUNT); i++)
	{
		myPixelShaders[i] = nullptr;
	}
}

bool FullscreenRenderer::Init(DirectX11Framework* aFramework)
{
	if (!aFramework)
	{
		SYSCRASH("FullscreenRenderer got no framework to work on!!.!1!");
		return false;
	}
	myContext = aFramework->GetContext();
	if (!myContext)
	{
		SYSCRASH("Got Invalid framework in fullscreen renderer");
		return false;
	}

	ID3D11Device* device = aFramework->GetDevice();

	std::vector<char> blob;
	myVertexShader = GetVertexShader(device,"Data/Shaders/Fullscreen/VertexShader.hlsl",blob);
	if (!myVertexShader)
	{
		SYSERROR("Could not compile fullscreen vertex shader","");
		return false;
	}

	std::array<std::string, static_cast<int>(Shader::COUNT)> filePaths;
	filePaths[static_cast<int>(Shader::MERGE)] = "Data/Shaders/Fullscreen/Merge.hlsl";
	filePaths[static_cast<int>(Shader::COPY)] = "Data/Shaders/Fullscreen/Copy.hlsl";

	for (size_t i = 0; i < filePaths.size(); i++)
	{
		myPixelShaders[i] = GetPixelShader(device, filePaths[i]);
		if(!myPixelShaders[i])
		{
			SYSERROR("could not compile fullscreenshader ",filePaths[i]);
			return false;
		}
	}



	return true;
}

void FullscreenRenderer::Render(Shader aEffect)
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	myContext->GSSetShader(nullptr, nullptr, 0);

	myContext->VSSetShader(*myVertexShader, nullptr, 0);
	myContext->PSSetShader(*myPixelShaders[static_cast<int>(aEffect)], nullptr, 0);

	myContext->Draw(3,0);
}

void FullscreenRenderer::Render(PixelShader* aShader)
{
	myContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myContext->IASetInputLayout(nullptr);
	myContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	myContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	myContext->GSSetShader(nullptr, nullptr, 0);

	myContext->VSSetShader(*myVertexShader, nullptr, 0);
	myContext->PSSetShader(*aShader, nullptr, 0);

	myContext->Draw(3, 0);
}
