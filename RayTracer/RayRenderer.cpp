#include "stdafx.h"
#include "RayRenderer.h"
#include "Intersection.hpp"
#include "DebugDrawer.h"
#include "Random.h"


RayRenderer::RayRenderer()
{
	{
		SphereObject sphere;
		sphere.mySphere.InitWithCenterAndRadius({ 0,0,0 }, 50.0);
		sphere.mySurfaceColor = V4F(1, 1, 1, 1);
		sphere.myDiffusion = 0.03;

		mySpheres.push_back(sphere);
	}
	{
		SphereObject sphere;
		sphere.mySphere.InitWithCenterAndRadius({ 30,30,-70 }, 20.0);
		sphere.mySurfaceColor = V4F(0.9, 0.8, 1, 1);
		sphere.myDiffusion = 0.0;

		mySpheres.push_back(sphere);
	}


	{
		TriObject tri;
		tri.myCorners[0] = V3D(-20, -150, 90);
		tri.myCorners[1] = V3D(-150, -20, 90);
		tri.myCorners[2] = V3D(-150, -150, 0);
		tri.mySurfaceColor = V4F(1, 0.8, 0.7, 1);
		tri.myDiffusion = 0.2;
		tri.myIsLight = false;

		myTris.push_back(tri);
	}

	{
		TriObject tri;
		tri.myCorners[0] = V3D(-20, 140, 90);
		tri.myCorners[1] = V3D(-140, 140, 90);
		tri.myCorners[2] = V3D(-140, 140, 0);
		tri.mySurfaceColor = V4F(1, 1, 1, 1);
		tri.myDiffusion = 0.0;
		tri.myIsLight = true;

		myTris.push_back(tri);
	}
	{
		TriObject tri;
		tri.myCorners[0] = V3D(-20, 140, 90);
		tri.myCorners[1] = V3D(-20, 140, 0);
		tri.myCorners[2] = V3D(-140, 140, 0);
		tri.mySurfaceColor = V4F(1, 1, 1, 1);
		tri.myDiffusion = 0.0;
		tri.myIsLight = true;

		myTris.push_back(tri);
	}

	{
		TriObject tri;
		tri.myCorners[0] = V3D(-140, 140, 90);
		tri.myCorners[1] = V3D(-140, 140, -90);
		tri.myCorners[2] = V3D(-140, -140, -90);
		tri.mySurfaceColor = V4F(0.8, 0.8, 0.8, 1);
		tri.myDiffusion = 0.05;
		tri.myIsLight = false;

		myTris.push_back(tri);
	}
	{
		TriObject tri;
		tri.myCorners[0] = V3D(-140, 140, 90);
		tri.myCorners[1] = V3D(-140, -140, -90);
		tri.myCorners[2] = V3D(-140, -140, 90);
		tri.mySurfaceColor = V4F(0.8, 0.8, 0.8, 1);
		tri.myDiffusion = 0.05;
		tri.myIsLight = false;

		myTris.push_back(tri);
	}
}

void RayRenderer::SetBoundingSize(double aSize)
{
	myBoundingSize = aSize;

	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(0, 0, aSize), V3D(0, 0, -1)));
	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(0, 0, -aSize), V3D(0, 0, 1)));
	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(0, aSize, 0), V3D(0, -1, 0)));
	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(0, -aSize, 0), V3D(0, 1, 0)));
	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(aSize, 0, 0), V3D(-1, 0, 0)));
	myBoundingBox.AddPlane(CommonUtilities::Plane<double>(V3D(-aSize, 0, 0), V3D(1, 0, 0)));
}

void RayRenderer::DrawTestRay(RAY aRay)
{
	DebugDrawer::GetInstance().SetColor(Tools::RandomRange(V4F(0.2,0.2,0.2,1),V4F(1,1,1,1)));

	RAY current = aRay;
	RayHit hitResult = Cast(current);
	while (true)
	{
		if (hitResult.myHit)
		{
			DebugDrawer::GetInstance().DrawArrow(current.Position(), hitResult.myLocation);
			if (hitResult.myIsFinal)
			{
				DebugDrawer::GetInstance().DrawCross(hitResult.myLocation, 30);
				break;
			}

			V3D dir = current.Direction().Reflected(hitResult.mySurfaceNormal);
			current.InitWithOriginAndDirection(hitResult.myLocation + dir * STANDARDMARG, dir);

			hitResult = Cast(current);
		}
		else
		{
			DebugDrawer::GetInstance().DrawDirection(current.Position(), current.Direction(),40);
			break;
		}
	}
}

V4F RayRenderer::operator()(RAY aRay) const
{
	return Evaluate(aRay);
}

V4F RayRenderer::Evaluate(RAY aRay, int aDepth) const
{
	RayHit hitResult = Cast(aRay);

	if (!hitResult.myHit)
	{
		return V4F(0, 0, 0, 0);
	}
	V4F Color = V4F(1, 1, 1, 1);

	if (hitResult.myIsFinal || aDepth > myMaxDepth)
	{
		return hitResult.mySurfaceColor;
	}

	Color *= hitResult.mySurfaceColor;
	{
		V3D dir = aRay.Direction().Reflected(hitResult.mySurfaceNormal);
		std::vector<V4F> samples;
		double area = hitResult.mySurfaceDiffusíon * hitResult.mySurfaceDiffusíon * 4.0 * PI;
		double maxArea = 4.0 * PI;
		int sampleCount = MAX(1, (area / maxArea) * myMaxSamples);
		if (aDepth > 0)
		{
			sampleCount = MAX(1, sampleCount / 8);
		}

		for (size_t i = 0; i < sampleCount; i++)
		{
			RAY sampleRay;
			V3D sampleDir = dir + V3D(Tools::RandomDirection()) * hitResult.mySurfaceDiffusíon;
			sampleRay.InitWithOriginAndDirection(hitResult.myLocation + sampleDir * STANDARDMARG, sampleDir);

			samples.push_back(Evaluate(sampleRay, aDepth + 1));
		}
		float totalValue = 0;
		for (auto& sample : samples)
		{
			totalValue += sample.w;
		}
		V4F sampleColor = V4F(0, 0, 0, 0);
		for (auto& sample : samples)
		{
			sampleColor += sample * (sample.w / totalValue);
		}
		sampleColor.w = 1;
		Color *= sampleColor;
	}

	return Color;
}

RayRenderer::RayHit RayRenderer::Cast(RAY aRay) const
{
	RayHit result;
	result.myHit = false;
	double t;
	double closestT = _HUGE_ENUF;

	for (auto& sphere : mySpheres)
	{
		if (CommonUtilities::IntersectionSphereRay(sphere.mySphere, aRay, t))
		{
			if (t > STANDARDMARG && t < closestT)
			{
				closestT = t;
				result.myHit = true;
				result.myIsFinal = false;
				result.myLocation = aRay.Position() + aRay.Direction() * t;
				result.mySurfaceNormal = (result.myLocation - sphere.mySphere.myPosition).GetNormalized();
				result.mySurfaceColor = sphere.mySurfaceColor;
				result.mySurfaceDiffusíon = sphere.myDiffusion;
			}
		}
	}
	for (auto& tri : myTris)
	{
		CommonUtilities::Plane<double> plane(tri.myCorners[0], tri.myCorners[1], tri.myCorners[2]);
		V3D intersect;
		if (CommonUtilities::IntersectionPlaneRay(plane,aRay,intersect))
		{
			std::array<V3D, 3> crosses;
			for (size_t i = 0; i < tri.myCorners.size(); i++)
			{
				V3D toCorner = intersect - tri.myCorners[i];
				V3D toNext = tri.myCorners[i] - tri.myCorners[(i + 1) % tri.myCorners.size()];

				crosses[i] = toCorner.Cross(toNext).GetNormalized();
			}

			if (crosses[0].Dot(crosses[1]) > 0 &&
				crosses[1].Dot(crosses[2]) > 0 &&
				crosses[2].Dot(crosses[0]) > 0)
			{
				t = (intersect - aRay.Position()).Dot(aRay.Direction());
				if (t < closestT)
				{
					closestT = t;
					result.myHit = true;
					result.myIsFinal = tri.myIsLight;
					result.myLocation = intersect;
					result.mySurfaceNormal = plane.Normal();
					result.mySurfaceColor = tri.mySurfaceColor;
					result.mySurfaceDiffusíon = tri.myDiffusion;
				}
			}
		}
	}

	for (auto& side : myBoundingBox.Planes())
	{
		V3D intersect;

		if (CommonUtilities::IntersectionPlaneRay(side, aRay, intersect))
		{
			if (abs(intersect.x) <= myBoundingSize + STANDARDMARG &&
				abs(intersect.y) <= myBoundingSize + STANDARDMARG &&
				abs(intersect.z) <= myBoundingSize + STANDARDMARG)
			{
				t = (intersect - aRay.Position()).Dot(aRay.Direction());
				if (t < closestT)
				{
					closestT = t;
					result.myHit = true;
					result.myIsFinal = true;
					result.myLocation = intersect;
					result.mySurfaceNormal = V3D(0, 0, 0);
					char parity = 0;
					parity ^= static_cast<int>((abs(intersect.x) + 15) / 30) & 0x1;
					parity ^= static_cast<int>((abs(intersect.y) + 15) / 30) & 0x1;
					parity ^= static_cast<int>((abs(intersect.z) + 15) / 30) & 0x1;



					result.mySurfaceColor = V4F((intersect.x + myBoundingSize) / (myBoundingSize * 2),
						(intersect.y + myBoundingSize) / (myBoundingSize * 2),
						(intersect.z + myBoundingSize) / (myBoundingSize * 2), 1);
					result.mySurfaceColor = LERP(result.mySurfaceColor, V4F(1, 1, 1, 1), 0.7f);

					result.mySurfaceColor *= ((parity == 0) ? V4F(1, 1, 1, 1) : V4F(0.6, 0.6, 0.6, 1));

					result.mySurfaceDiffusíon = 0;
				}
			}
		}
	}


	return result;
}
