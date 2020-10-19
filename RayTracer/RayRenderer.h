#pragma once
#include "Ray.hpp"
#include "Sphere.hpp"
#include "PlaneVolume.hpp"

#define V3D CommonUtilities::Vector3<double>
#define RAY CommonUtilities::Ray<double>

class RayRenderer
{
public:
	RayRenderer();
	void SetBoundingSize(double aSize);


	void DrawTestRay(RAY aRay);

	V4F operator()(RAY aRay) const;

	void AddModel(std::string aFilePath);

private:
	struct RayHit
	{
		bool myHit;

		V4F mySurfaceColor;
		V3D mySurfaceNormal;
		V3D myLocation;
		double mySurfaceDiffusíon;

		bool myIsFinal;
	};

	struct SphereObject
	{
		CommonUtilities::Sphere<double> mySphere;
		double myDiffusion;
		V4F	mySurfaceColor;
		double myOpacity;
	};

	struct PolyObject
	{
		typedef std::array<V3D, 3> TriType;
		std::vector<TriType>	myTris;
		double myDiffusion;
		V4F	mySurfaceColor;
		bool myIsLight;
	};


	V4F Evaluate(RAY aRay, int aDepth = 0) const;

	RayHit Cast(RAY aRay) const;
	RayHit CastAt(RAY aRay, PolyObject aObject, double& t) const;
	

	int myMaxDepth = 8;

	std::vector<SphereObject> mySpheres;
	std::vector<PolyObject> myPolyObjects;

	double myBoundingSize;
	CommonUtilities::PlaneVolume<double> myBoundingBox;
};