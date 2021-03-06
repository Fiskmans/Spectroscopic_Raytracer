#pragma once
#include "Matrix3x3.hpp"
#include "Matrix4x4.hpp"
#include "Vector2.hpp"
#include "PlaneVolume.hpp"

namespace CommonUtilities
{
	template<class T>
	class Vector3;
}

class Camera
{
public:
	enum class CameraType
	{
		Orthographic
	};

	Camera();

	bool Init(float aFoV, CommonUtilities::Vector2<float> aResolution, float aNear, float aFar);
	bool Init(CommonUtilities::Vector3<float> aBoundingBox);
	void SetTransform(CommonUtilities::Vector3<float> aPosition, CommonUtilities::Vector3<float> aRotation);
	void SetTransform(M44F aTransform);
	void SetRotation(CommonUtilities::Vector3<float> aRotation);
	void SetRotation(CommonUtilities::Matrix3x3<float> aRotationMatrix);
	void SetPosition(CommonUtilities::Vector3<float> aPosition);
	void Move(CommonUtilities::Vector3<float> aMovement);
	void Rotate(CommonUtilities::Vector3<float> aRotation);
	void RotateWorldSpace(CommonUtilities::Vector3<float> aRotation);
	void SetFov(const float aFov, bool aSetAsSecondary = false);
	void SetResolution(const CommonUtilities::Vector2<float>& aResolution);
	float GetFoV() const;
	V3F Unproject(V3F aProjectedPoint) const;

	
	template<class T>
	void LookAt(T* aTarget);
	void LookAt(CommonUtilities::Vector3<float> aTarget);


	CommonUtilities::Matrix4x4<float> GetTransform() const;
	CommonUtilities::Matrix4x4<float> GetProjection(bool aWantsSecondary) const;

	CommonUtilities::Vector3<float> GetForward() const;
	CommonUtilities::Vector3<float> GetFlatForward() const;
	CommonUtilities::Vector3<float> GetUp() const;
	CommonUtilities::Vector3<float> GetRight() const;

	bool IsInView(const CommonUtilities::Vector3<float>& aPosition) const;

	CommonUtilities::Vector3<float> GetPosition() const;

	CommonUtilities::PlaneVolume<float> GenerateFrustum() const;

private:
	CommonUtilities::Matrix4x4<float> myTransform;
	CommonUtilities::Matrix4x4<float> myProjection;
	CommonUtilities::Matrix4x4<float> myProjection2;

	CommonUtilities::Vector3<float> myOrthoBounds;
	CommonUtilities::Vector2<float> myResolution;
	float myNearPlane;
	float myFarPlane;
	float myFOV;
	float myFov2;
	bool myHasSecondaryFov;
	bool myIsOrthogonal;

};

template<class T>
inline void Camera::LookAt(T* aTarget)
{
	LookAt(aTarget->GetPosition());
}
