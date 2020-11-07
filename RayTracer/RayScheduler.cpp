#include "stdafx.h"
#include "RayScheduler.h"
#include "Camera.h"
#include "cachelineSize.h"


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

	RenderAll();

	size_t pixelCount = aWidth * aHeight;

	myCPUTexture = new V4F[pixelCount];
	mySamplesInPixel = new unsigned short[pixelCount];
	mysecondaryBuffer = new V4F[pixelCount];
	myPixelUncertanity = new float[pixelCount];

	myShowUncertenityIMGUI = false;
	myShowUncertenity = false;

	for (size_t i = 0; i < pixelCount; i++)
	{
		mySamplesInPixel[i] = 0;
		myPixelUncertanity[i] = 1.f;
	}

	size_t hardwareThreads = std::thread::hardware_concurrency();
	if (hardwareThreads < 4)
	{
		hardwareThreads = 4;
	}
	size_t renderThreads = hardwareThreads - 1;

	size_t cachelinesize = cache_line_size();
	if (cachelinesize == 0)
	{
		cachelinesize = 64;
	}

	myKillSwitch = new bool[cachelinesize * 2] + cachelinesize; // guarantee (ish) killswich is on it's own cacheline
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
	SAFE_DELETE_ARRAY(mysecondaryBuffer);
	SAFE_DELETE_ARRAY(myPixelUncertanity);
	*myKillSwitch = false;
	for (auto& i : myWorkers)
	{
		i.join();
	}
	size_t cachelinesize = cache_line_size();
	if (cachelinesize == 0)
	{
		cachelinesize = 64;
	}
	delete[] (myKillSwitch - cachelinesize);
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
		if (myShowUncertenity ^ myShowUncertenityIMGUI)
		{
			readHead = reinterpret_cast<char*>(mysecondaryBuffer);
		}
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

			if (ImGui::Button("Render All"))
			{
				RenderAll();
			}
			if (ImGui::Button("Render Worst"))
			{
				mySections.push_back(FindWorstSection());
			}

			int wanted = mySPP;
			if (ImGui::InputInt("Samples per pixel", &wanted))
			{
				if (wanted > 0)
				{
					mySPP = wanted;
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
			ImGui::Checkbox("Uncertanity",&myShowUncertenityIMGUI);
		});


	float xscale = myWidth / float(myWidth + 16); 
	float yscale = myHeight / float(myHeight + 40);

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

	if (ImGui::GetIO().KeyCtrl)
	{
		static ImVec2 start;
		if (ImGui::GetIO().MouseClicked[0])
		{
			start = ImGui::GetIO().MousePos;
		}
		if (ImGui::GetIO().MouseDown[0])
		{
			drawList->AddRect(start, ImGui::GetIO().MousePos, ImColor(0, 0, 0, 80));
		}
		if (ImGui::GetIO().MouseReleased[0])
		{
			ImVec2 end = ImGui::GetIO().MousePos;
			ImVec2 topleft = ImVec2(MIN(start.x, end.x), MIN(start.y, end.y));
			ImVec2 bottomRight = ImVec2(MAX(start.x, end.x), MAX(start.y, end.y));
			Section sec;
			sec.x = CLAMP(0,myWidth, topleft.x / xscale);
			sec.y = CLAMP(0,myHeight, topleft.y / yscale);
			sec.z = CLAMP(1,myWidth, bottomRight.x / xscale);
			sec.w = CLAMP(1,myHeight, bottomRight.y / yscale);
			mySections.push_back(sec);
		}
	}

	myShowUncertenity = ImGui::GetIO().KeyShift;
}

void RayScheduler::Update()
{
	for (auto& job : myJobsSlots)
	{
		Result res;
		if (job.FetchResult(res))
		{
			ApplyColor(res);
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

	if (mySections.empty())
	{
		mySections.push_back(FindWorstSection());
	}
}

RayScheduler::Section RayScheduler::FindWorstSection()
{
	const size_t size = 32;

	Section ret;
	float* tmp = new float[(myWidth- size) * (myHeight - size)];
	for (size_t y = 0; y < myHeight - size; y++)
	{
		for (size_t x = 0; x < myWidth - size; x++)
		{
			tmp[y * (myWidth - size) + x] = 0;
			for (size_t y1 = y; y1 < y + size; y1++)
			{
				for (size_t x1 = x; x1 < x + size; x1++)
				{
					tmp[y * (myWidth - size) + x] += myPixelUncertanity[y1 * myWidth + x1];
				}
			}
		}
	}
	size_t highestIndex = std::max_element(tmp, tmp + ((myWidth - size) * (myHeight - size))) - tmp;
	ret.x = highestIndex % (myWidth - size);
	ret.y = highestIndex / (myWidth - size);
	ret.z = ret.x + size;
	ret.w = ret.y + size;
	delete[] tmp;
	return ret;
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
			size_t maxSize = (1 << (sizeof(unsigned char) * CHAR_BIT)) - 1;
			for (size_t i = 0; i < mySPP / maxSize; i++)
			{
				job.samples = maxSize;
				Schedule(job);
			}

			job.samples = mySPP % maxSize;

			if (job.samples > 0)
			{
				Schedule(job);
			}
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
	for (size_t i = 0; i < aResult.samples; i++)
	{
		aResult.color += myRenderer(ray) / float(aResult.samples);
	}
}

void RayScheduler::ApplyColor(Result& aResult)
{
	size_t index = (aResult.y * myWidth + aResult.x);
	mySamplesInPixel[index] += aResult.samples;

	V4F newColor = LERP(myCPUTexture[index], aResult.color, float(aResult.samples) / mySamplesInPixel[index]);
	float change = newColor.Distance(myCPUTexture[index]);

	myPixelUncertanity[index] = LERP(change, myPixelUncertanity[index], std::pow(0.9,aResult.samples));
	myCPUTexture[index] = newColor;
	mysecondaryBuffer[index] = V4F(myPixelUncertanity[index], myPixelUncertanity[index], myPixelUncertanity[index], 1);
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

void RayScheduler::RenderAll()
{
	SplitSection(Section(0, 0, myWidth, myHeight));
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
