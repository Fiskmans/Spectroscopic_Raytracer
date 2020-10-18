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
	mySPP = 5;

	SplitSection(Section(0,0,myWidth,myHeight));

	size_t pixelCount = aWidth * aHeight;

	myCPUTexture = new V4F[pixelCount];
	for (size_t i = 0; i < pixelCount; i++)
	{
		myCPUTexture[i] = V4F(0,0,0,1);// V4F(((rand() % 100) / 100.0), ((rand() % 100) / 100.0), ((rand() % 100) / 100.0), 1.f);
	}

	size_t hardwareThreads = std::thread::hardware_concurrency();
	if (hardwareThreads < 4)
	{
		hardwareThreads = 4;
	}
	size_t renderThreads = hardwareThreads - 1;

	myKillSwitch = new bool[64*3] + 64; // guarantee killswich is on it's own cacheline on machines with cachelines < 64byte (most/all of them at time of writing)
	*myKillSwitch = false;

	myJobsSlots.reserve(renderThreads);
	for (size_t i = 0; i < renderThreads; i++)
	{
		myJobsSlots.emplace_back();
		myWorkers.emplace_back([this, i]()
			{
				NAMETHREAD((L"Ray render worker thread: " + std::to_wstring(i)).c_str());
				bool* killswitch = myKillSwitch;
				while (!*killswitch)
				{
					Result job;
					myJobsSlots[i].FetchJob(job);
					RenderPixel(job);
					myJobsSlots[i].GiveResult(job);
				}
			});
	}

}

RayScheduler::~RayScheduler()
{
	SAFE_RELEASE(myTargetTexture);
	SAFE_DELETE_ARRAY(myCPUTexture);
	*myKillSwitch = false;
	for (auto& i : myWorkers)
	{
		i.join();
	}
	delete[] (myKillSwitch - 64);
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
			int wanted = mySPP;
			if (ImGui::InputInt("Samples per pixel", &wanted))
			{
				if (wanted > 1)
				{
					mySPP = wanted;
					mySections.clear();
					SplitSection(Section(0, 0, myWidth, myHeight));
				}
			}

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
	for (auto& job : myJobsSlots)
	{
		Result res;
		if (job.FetchResult(res))
		{
			myCPUTexture[res.x + res.y * myWidth] = res.color;
		}
	}

	while (!myUnassigned.empty())
	{
		Result job = myUnassigned.back();
		myUnassigned.pop_back();
		if (!Schedule(job))
		{
			break;
		}
	}

	while (myUnassigned.empty() && !mySections.empty())
	{
		Section sec = mySections.back();
		mySections.pop_back();

		int width = (sec.z - sec.x);
		int heigth = (sec.w - sec.y);

		if (width * heigth > 16)
		{
			SplitSection(sec);
		}
		else
		{
			WorkSection(sec);
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

void RayScheduler::WorkSection(Section aSection)
{
	for (int x = aSection.x; x < aSection.z; x++)
	{
		for (int y = aSection.y; y < aSection.w; y++)
		{
			Result job;
			job.x = x;
			job.y = y;
			Schedule(job);
		}
	}
}

void RayScheduler::RenderPixel(Result& aResult)
{
	float screenX = (static_cast<float>(aResult.x) / static_cast<float>(myWidth));
	float screenY = (static_cast<float>(aResult.y) / static_cast<float>(myHeight));

	V3F from = myCamera->Unproject(V3F(screenX,screenY,0));
	V3F to = myCamera->Unproject(V3F(screenX, screenY, 1));
	CommonUtilities::Ray<double> ray;
	ray.InitWith2Points(CommonUtilities::Vector3<double>(from.x, from.y, from.z), CommonUtilities::Vector3<double>(to.x, to.y, to.z));


	aResult.color = V4F(0, 0, 0, 0);
	for (size_t i = 0; i < mySPP; i++)
	{
		aResult.color += myRenderer(ray) / float(mySPP);
	}
}

bool RayScheduler::Schedule(RayScheduler::Result aJob)
{
	for (auto& slot : myJobsSlots)
	{
		if (slot.GiveJob(aJob))
		{
			return true;
		}
	}
	myUnassigned.push_back(aJob);
	return false;
}

bool RayScheduler::Job::GiveJob(RayScheduler::Result& aJob)
{
	auto* currentStatus = myToStart.load();

	if (currentStatus)
	{
		return false;
	}
	myToWorkData = aJob;
	myToStart.store(&myToWorkData);
	return true;
}

void RayScheduler::Job::FetchJob(RayScheduler::Result& aJob)
{
	RayScheduler::Result* currentStatus;
	do
	{
		currentStatus = myToStart.load();
	} while (!currentStatus);

	aJob = *currentStatus;
	myToStart.store(nullptr);
}

void RayScheduler::Job::GiveResult(RayScheduler::Result& aResult)
{
	RayScheduler::Result* currentStatus;
	do
	{
		currentStatus = myReady.load();
	} while (currentStatus);

	myReadyData = aResult;
	myReady.store(&myReadyData);
}

bool RayScheduler::Job::FetchResult(RayScheduler::Result& aResult)
{
	auto* currentStatus = myReady.load();

	if (!currentStatus)
	{
		return false;
	}
	aResult = *currentStatus;
	myReady.store(nullptr);
	return true;
}
