#include "stdafx.h"
#include "RayRenderer.h"
#include "Intersection.hpp"
#include "DebugDrawer.h"
#include "Random.h"
#include "AssImp\mesh.h"
#include "AssImp\scene.h"
#include "AssImp\cimport.h"
#include "AssImp\postprocess.h"

//https://learn.foundry.com/modo/content/help/pages/shading_lighting/shader_items/refract_index.html

RayRenderer::RayRenderer()
{
	//{ // middle
	//	SphereObject sphere;
	//	sphere.mySphere.InitWithCenterAndRadius({ 0,0,0 }, 50.0);
	//	sphere.mySurfaceColor = V4F(1, 1, 1, 1);
	//	sphere.myDiffusion = 0.03;
	//
	//	mySpheres.push_back(sphere);
	//}
	{ // front top right
		SphereObject sphere;
		sphere.mySphere.InitWithCenterAndRadius({ 30,30,-70 }, 20.0);
		sphere.mySurfaceColor = V4F(0.9, 0.8, 1, 1);
		sphere.myDiffusion = 0.0;

		mySpheres.push_back(sphere);
	}


	{ // bottom left corner
		PolyObject poly;
		poly.myTris.push_back({ V3D(-20, -140, 90), V3D(-140, -20, 90), V3D(-140, -140, 0) });
		poly.mySurfaceColor = V4F(1, 0.8, 0.7, 1);
		poly.myReflectivity = 0.2;
		poly.myIsLight = false;

		myPolyObjects.push_back(poly);
	}

	{ // light 1
		PolyObject poly;
		poly.myTris.push_back({ V3D(-20, 140, 90), V3D(-140, 140, 90), V3D(-140, 140, 0) });
		poly.myTris.push_back({ V3D(-20, 140, 90), V3D(-20, 140, 0), V3D(-140, 140, 0) });
		poly.mySurfaceColor = V4F(1, 1, 1, 1);
		poly.myReflectivity = 0.0;
		poly.myIsLight = true;

		myPolyObjects.push_back(poly);
	}

	{// wall 1
		PolyObject poly;
		poly.myTris.push_back({ V3D(-140, 140, 90), V3D(-140, 140, -90), V3D(-140, -140, -90) });
		poly.myTris.push_back({ V3D(-140, 140, 90), V3D(-140, -140, -90), V3D(-140, -140, 90) });
		poly.myTris.push_back({ V3D(-20, -140, 90), V3D(-20, -20, 90), V3D(-140, -20, 90) });
		poly.myTris.push_back({ V3D(-20, 140, 90), V3D(-140, -20, 90), V3D(-20, -20, 90) });
		poly.myTris.push_back({ V3D(-20, 140, 90), V3D(-140, 140, 90), V3D(-140, -20, 90) });
		poly.myTris.push_back({ V3D(-20, 140, 90), V3D(-20, -140, 90), V3D(140, -140, 140) });
		poly.myTris.push_back({ V3D(140, 140, 140), V3D(-20, 140, 90), V3D(140, -140, 140) });
		poly.mySurfaceColor = V4F(0.8, 0.8, 0.8, 1);
		poly.myReflectivity = 0.05;
		poly.myIsLight = false;

		myPolyObjects.push_back(poly);
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
	for (size_t i = 0; i < myMaxDepth; i++)
	{
		if (hitResult.myHit)
		{
			DebugDrawer::GetInstance().DrawArrow(current.Position(), hitResult.myLocation);
			if (hitResult.myIsFinal)
			{
				DebugDrawer::GetInstance().DrawCross(hitResult.myLocation, 30);
				break;
			}

			V3D dir;
			if (Tools::RandomNormalized() < hitResult.mySurfaceReflectivity)
			{
				dir = current.Direction().Reflected(hitResult.mySurfaceNormal);
			}
			else
			{
				dir = Tools::RandomDirection();
				if (dir.Dot(hitResult.mySurfaceNormal) < 0)
				{
					dir = -dir;
				}
			}
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

void RayRenderer::AddModel(std::string aFilePath)
{
	const aiScene* scene = NULL;

	scene = aiImportFile(aFilePath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

	if (!scene)
	{
		SYSERROR(aiGetErrorString(), aFilePath);
		return;
	}

	std::deque<const aiNode*> nodeStack;
	nodeStack.push_back(scene->mRootNode);
	while (!nodeStack.empty())
	{
		const aiNode* current = nodeStack.back();
		nodeStack.pop_back();

		for (size_t i = 0; i < current->mNumChildren; i++)
		{
			nodeStack.push_back(current->mChildren[i]);
		}

		aiMatrix4x4 transform;
		{
			std::stack<const aiNode*> parentStack;
			const aiNode* parentSeeker = current->mParent;
			while (parentSeeker)
			{
				parentStack.push(parentSeeker);
				parentSeeker = parentSeeker->mParent;
			}
			while (!parentStack.empty())
			{
				const aiNode* p = parentStack.top();
				parentStack.pop();
				transform *= p->mTransformation;
			}
			transform *= current->mTransformation;
		}

		for (size_t i = 0; i < current->mNumMeshes; i++)
		{
			PolyObject obj;
			const aiMesh* mesh = scene->mMeshes[current->mMeshes[i]];

			std::vector<V3D> vertexes;
			for (size_t i = 0; i < mesh->mNumVertices; i++)
			{
				aiVector3D vec = mesh->mVertices[i];
				vec *= transform;
				vertexes.push_back(V3D(vec.x, vec.y, vec.z));
			}

			for (size_t n = 0; n < mesh->mNumFaces; n++)
			{
				const aiFace* face = mesh->mFaces + n;
				if (face->mNumIndices < 3)
				{
					continue;
				}
				PolyObject::TriType tri;

				tri[0] = vertexes[face->mIndices[0]];
				for (size_t i = 1; i < face->mNumIndices-1; i++)
				{
					tri[1] = vertexes[face->mIndices[i]];
					tri[2] = vertexes[face->mIndices[i+1]];
					obj.myTris.push_back(tri);
				}
			}
			obj.myReflectivity = 0.1;
			obj.myIsLight = false;
			obj.mySurfaceColor = V4F(0.6, 0.6, 0.6, 1);
			myPolyObjects.push_back(obj);
		}
	}

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


	if (Tools::RandomNormalized() < hitResult.mySurfaceReflectivity)
	{
		V3D dir = aRay.Direction().Reflected(hitResult.mySurfaceNormal);

		RAY sampleRay;
		V3D sampleDir = dir + V3D(Tools::RandomDirection()) * hitResult.mySurfaceReflectivity;
		sampleRay.InitWithOriginAndDirection(hitResult.myLocation + sampleDir * STANDARDMARG, sampleDir);

		Color *= Evaluate(sampleRay, aDepth + 1);
	}
	else
	{
		RAY sampleRay;
		V3D sampleDir = Tools::RandomDirection();
		if (sampleDir.Dot(hitResult.mySurfaceNormal) < 0)
		{
			sampleDir = -sampleDir;
		}
		sampleRay.InitWithOriginAndDirection(hitResult.myLocation + sampleDir * STANDARDMARG, sampleDir);

		Color *= hitResult.mySurfaceColor * Evaluate(sampleRay, aDepth + 1);
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
				result.mySurfaceReflectivity = sphere.myDiffusion;
			}
		}
	}
	for (auto& poly : myPolyObjects)
	{
		RayHit hit = CastAt(aRay,poly,t);
		if (t > STANDARDMARG && t < closestT)
		{
			result = hit;
			closestT = t;
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

					result.mySurfaceReflectivity = 0;
				}
			}
		}
	}


	return result;
}

RayRenderer::RayHit RayRenderer::CastAt(RAY aRay, PolyObject aObject,double& t) const
{
	RayHit result;
	result.myHit = false;
	double closestT = _HUGE_ENUF;
	for (auto& tri : aObject.myTris)
	{
		CommonUtilities::Plane<double> plane(tri[0], tri[1], tri[2]);
		V3D intersect;
		if (CommonUtilities::IntersectionPlaneRay(plane, aRay, intersect))
		{
			std::array<V3D, 3> crosses;
			for (size_t i = 0; i < tri.size(); i++)
			{
				V3D toCorner = intersect - tri[i];
				V3D toNext = tri[i] - tri[(i + 1) % tri.size()];

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
					result.myIsFinal = aObject.myIsLight;
					result.myLocation = intersect;
					result.mySurfaceNormal = plane.Normal();
					result.mySurfaceColor = aObject.mySurfaceColor;
					result.mySurfaceReflectivity = aObject.myReflectivity;
				}
			}
		}
	}
	return result;
}
