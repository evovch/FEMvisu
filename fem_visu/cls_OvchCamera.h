#pragma once

#pragma warning(push, 1)
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#pragma warning(pop)

class cls_OvchCamera
{
public:
	cls_OvchCamera(glm::vec3 p_center, float p_radius);
	~cls_OvchCamera(void);

	// Camera-to-clip-----------------------------------------------------

	// Switch between parallel and perspective
	void SwitchProjType(void) { mIsPerspective = !mIsPerspective; }
	// Setters/getters for parameters
	float GetFrAngle(void) const { return mFrustumAngle; }
	float GetFrNearWidth(void) const { return mFrustumZNearWidth; }
	float GetParallelBoxSize(void) const { return mParallelBoxSize; }
	float GetParallelBoxDepth(void) const { return mParallelBoxDepth; }
	void SetFrAngle(float p_newAngle) { mFrustumAngle = p_newAngle; }
	void SetFrNearWidth(float p_newNearWidth) { mFrustumZNearWidth = p_newNearWidth; }
	void SetParallelBoxSize(float p_newParallelBoxSize) { mParallelBoxSize = p_newParallelBoxSize; }
	void SetParallelBoxDepth(float p_newParallelBoxDepth) { mParallelBoxDepth = p_newParallelBoxDepth; }

	// Model-to-camera-----------------------------------------------------

	// Setters/getters
	float GetDist(void) const { return mDist; }
	glm::vec3 GetLookPt(void) const { return mLookPt; }
	void SetDist(float p_newDist) { mDist = p_newDist; }
	void SetLookPt(glm::vec3 p_newLookPt) { mLookPt = p_newLookPt; }

	// Common-----------------------------------------------------

	// Manipulation methods
	void Reset();
	void Pan(float /*p_curScreenX*/, float /*p_curScreenY*/, float /*p_startScreenX*/, float /*p_startScreenY*/, glm::vec3 /*p_startLookPt*/);
	void Zoom(float p_curScreenY, float p_startScreenY, float p_startFrAngle, float p_startPBS);
	void Rotate(glm::vec3 p_curLocalDir, glm::vec3 p_startLocalDir);
	void Center(float /*p_curScreenX*/, float /*p_curScreenY*/);

	// Getters for the matrices:
	// 3x3 rotation matrix
	glm::mat3 GetRotM(void) const;
	// Model-to-camera matrix
	glm::mat4 GetModelToCamera(void) const;
	// Camera-to-clip matrix
	glm::mat4 GetCameraToClip(void) const;
	// Full MVP matrix (which is Camera-to-clip * Model-to-camera)
	glm::mat4 GetMVP(void) const;

	// Get the viewer standing point
	glm::vec3 GetViewerPoint(void) const;

	// Send camera to GPU
	void SendCamToGPU(void) const;

private:
	// Default values - initialized once during construction, constant through time, used for Reset()
	glm::vec3 mDefCenter;
	float mDefRadius;

	// Camera-to-clip-----------------------------------------------------

	// Current projection mode - perspective or parallel
	bool mIsPerspective;
	// Perspective viewing frustum parameters
	float mFrustumAngle;
	float mFrustumZNearWidth;
	// Parallel projections parameters
	float mParallelBoxSize;
	float mParallelBoxDepth;

	// Model-to-camera-----------------------------------------------------

	// Look point
	glm::vec3 mLookPt;
	// Distance to the looking point along the looking direction - defines the camera point
	float mDist;

public:
	// Quaternion, defining the looking direction
	glm::quat mQua;
};
