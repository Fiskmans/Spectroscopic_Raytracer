#include "stdafx.h"
#include "RayScheduler.h"
#include "Camera.h"

RayScheduler::RayScheduler(size_t aWidth, size_t aHeight, ID3D11Texture2D* aTargetTexture, Camera* aCamera, ID3D11DeviceContext* aContext, RayRenderer aRenderFunction)
{
	myWidth = aWidth;
	myHeight = aHeight;
	myTargetTexture = aTargetTexture;
	myTargetTexture->AddRef();
	myCamera = aCamera;
	myContext = aContext;
	myRenderer = aRenderFunction;

	SplitSection(Section(0,0,myWidth,myHeight));

	size_t pixelCount = aWidth * aHeight;

	myCPUTexture = new V4F[pixelCount];
	for (size_t i = 0; i < pixelCount; i++)
	{
		myCPUTexture[i] = V4F(((rand() % 100) / 100.0), ((rand() % 100) / 100.0), ((rand() % 100) / 100.0), 1.f);
	}
}

RayScheduler::~RayScheduler()
{
	SAFE_RELEASE(myTargetTexture);
	SAFE_DELETE_ARRAY(myCPUTexture);
}

void RayScheduler::PushToHardware()
{
	D3D11_TEXTURE2D_DESC desc;
	myTargetTexture->GetDesc(&desc);
	size_t pixelCount = desc.Width * desc.Height;
	size_t totalSize = pixelCount * sizeof(V4F);

	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT result = myContext->Map(myTargetTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(result))
	{
		char* readHead = reinterpret_cast<char*>(myCPUTexture);
		size_t rowSize = desc.Width * sizeof(V4F);
		for (size_t i = 0; i < desc.Height; i++)
		{
			memcpy(reinterpret_cast<char*>(map.pData) + (rowSize * i), readHead, rowSize);
			readHead += rowSize;
		}

		myContext->Unmap(myTargetTexture, 0);
	}
}

void RayScheduler::Imgui()
{
	WindowControl::Window("Scheduler", [this]()
		{
			if (ImGui::TreeNode("sections"))
			{
				size_t count = 0;
				for (auto& i : mySections)
				{
					ImGui::InputInt4(std::to_string(count).c_str(), &i.x);
					count++;
				}
				ImGui::TreePop();
			}
		});

	float xscale = 800.f / 816.f;
	float yscale = 600.f / 640.f;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	for (auto& i : mySections)
	{
		drawList->AddRect(ImVec2(i.x* xscale, i.y*yscale), ImVec2(i.z* xscale, i.w * yscale), ImColor(255, 0, 0,80));
	}

	if (ImGui::GetIO().KeyAlt)
	{
		float screenX = (static_cast<float>(ImGui::GetIO().MousePos.x / xscale) / static_cast<float>(myWidth));
		float screenY = (static_cast<float>(ImGui::GetIO().MousePos.y / yscale) / static_cast<float>(myHeight));

		V3F from = myCamera->Unproject(V3F(screenX, screenY, 0));
		V3F to = myCamera->Unproject(V3F(screenX, screenY, 1));
		CommonUtilities::Ray<double> ray;
		ray.InitWith2Points(CommonUtilities::Vector3<double>(from.x, from.y, from.z), CommonUtilities::Vector3<double>(to.x, to.y, to.z));

		myRenderer.DrawTestRay(ray);
	}

}


void RayScheduler::Update()
{
	for (size_t i = 0; i < myPending.size(); i++)
	{
		if (myPending[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			Result res = myPending[i].get();
			myCPUTexture[res.x + res.y * myWidth] = res.color;
			myPending.erase(myPending.begin() + i);
		}
	}

	while (myPending.size() < 64 && !mySections.empty())
	{
		Section sec = mySections.back();
		mySections.pop_back();

		int width = (sec.z - sec.x);
		int heigth = (sec.w - sec.y);

		if (width * heigth > 64)
		{
			SplitSection(sec);
		}
		else
		{
			for (int x = sec.x; x < sec.z; x++)
			{
				for (int y = sec.y; y < sec.w; y++)
				{
					myPending.push_back(std::async(std::bind(&RayScheduler::RenderPixel, this, x, y)));
				}
			}
		}
	}
}

void RayScheduler::SplitSection(Section aSection)
{
	int width = (aSection.z - aSection.x)/2;
	int heigth = (aSection.w - aSection.y)/2;


	mySections.push_back({ aSection.x,aSection.y,aSection.x + width,aSection.y + heigth });
	mySections.push_back({ aSection.x,aSection.y+heigth,aSection.x + width,aSection.w});
	mySections.push_back({ aSection.x+width,aSection.y,aSection.z,aSection.y + heigth });
	mySections.push_back({ aSection.x+width,aSection.y+heigth,aSection.z,aSection.w });
}

RayScheduler::Result RayScheduler::RenderPixel(int x, int y)
{
	NAMETHREAD((L"RayRender x:" + std::to_wstring(x)  + L" y:" + std::to_wstring(y)).c_str());

	float screenX = (static_cast<float>(x) / static_cast<float>(myWidth));
	float screenY = (static_cast<float>(y) / static_cast<float>(myHeight));

	V3F from = myCamera->Unproject(V3F(screenX,screenY,0));
	V3F to = myCamera->Unproject(V3F(screenX, screenY, 1));
	CommonUtilities::Ray<double> ray;
	ray.InitWith2Points(CommonUtilities::Vector3<double>(from.x, from.y, from.z), CommonUtilities::Vector3<double>(to.x, to.y, to.z));

	Result res;
	res.x = x;
	res.y = y;
	res.color = myRenderer(ray);

	return res;
}
