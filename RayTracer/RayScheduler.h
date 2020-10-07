#pragma once
#include "Ray.hpp"
#include "RayRenderer.h"


class Camera;

class RayScheduler
{
public:
	RayScheduler(size_t aWidth, size_t aHeight,ID3D11Texture2D* aTargetTexture,Camera* aCamera,ID3D11DeviceContext* aContext, RayRenderer aRenderFunction);
	~RayScheduler();

	void PushToHardware();

	void Imgui();

	void Update();

private:

	typedef CommonUtilities::Vector4<int> Section;

	struct Result
	{
		int x;
		int y;
		V4F color;
	};

	void SplitSection(Section aSection);

	Result RenderPixel(int x, int y);

	RayRenderer myRenderer;

	std::vector<std::future<Result>> myPending;

	std::vector<Section> mySections;

	V4F* myCPUTexture;

	size_t myWidth;
	size_t myHeight;
	Camera* myCamera;

	ID3D11DeviceContext* myContext;
	ID3D11Texture2D* myTargetTexture;
};

