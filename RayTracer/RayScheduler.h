#pragma once
#include "Ray.hpp"
#include "RayRenderer.h"

#include <thread>
#include <atomic>

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
		V4F color;
		int x;
		int y;
		unsigned char samples;
	};

	class Job
	{
	public:
		Job() = default;
		Job(const Job& aOther)
		{
			assert(false);
		}

		bool GiveJob(RayScheduler::Result& aJob);
		void FetchJob(RayScheduler::Result& aJob);

		void GiveResult(RayScheduler::Result& aResult);
		bool FetchResult(RayScheduler::Result& aResult);

	private:

		std::atomic<RayScheduler::Result*> myReady;
		std::atomic<RayScheduler::Result*> myToStart;

		RayScheduler::Result myToWorkData;
		RayScheduler::Result myReadyData;
	};

	void SplitSection(Section aSection);

	void WorkSection(Section aSection);

	void RenderPixel(Result& aResult);

	void ApplyColor(Result& aResult);

	bool Schedule(Result aJob);

	void RenderAll();

	RayRenderer myRenderer;

	std::vector<Job> myJobsSlots;
	std::vector<std::thread> myWorkers;

	std::vector<Section> mySections;
	std::vector<RayScheduler::Result> myUnassigned;

	V4F* myCPUTexture;
	unsigned short* mySamplesInPixel;

	size_t myWidth;
	size_t myHeight;
	Camera* myCamera;
	bool* myKillSwitch;
	size_t mySPP;

	ID3D11DeviceContext* myContext;
	ID3D11Texture2D* myTargetTexture;
};

