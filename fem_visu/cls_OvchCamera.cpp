#include "cls_OvchCamera.h"

#pragma warning(push, 1)
#include "GL/glew.h"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cmath>
#pragma warning(pop)

#include "cls_OvchRenderer.h"
#include "cls_OvchModelProcessor.h"

cls_OvchCamera::cls_OvchCamera(glm::vec3 p_center, float p_radius) :
	mDefCenter(p_center), mDefRadius(p_radius)
{
	this->Reset();
}

cls_OvchCamera::~cls_OvchCamera(void)
{
}

void cls_OvchCamera::Reset()
{
	this->SetLookPt(mDefCenter);
	this->SetDist(mDefRadius*2.0f);

	mIsPerspective = true;
	this->SetFrAngle(45.0f);
	this->SetFrNearWidth(mDefRadius*1.2f);

	this->SetParallelBoxSize(mDefRadius*1.2f);
	this->SetParallelBoxDepth(mDefRadius*1.2f);

	mQua = glm::quat();

	this->SendCamToGPU();
}

void cls_OvchCamera::Pan(float /*p_curScreenX*/, float /*p_curScreenY*/,
						 float /*p_startScreenX*/, float /*p_startScreenY*/,
						 glm::vec3 /*p_startLookPt*/)
{
	//TODO implement
}

void cls_OvchCamera::Zoom(float p_curScreenY, float p_startScreenY, float p_startFrAngle, float p_startPBS)
{
	// Perspective
	float v_newAngle = p_startFrAngle - (p_curScreenY-p_startScreenY)*0.2f;
	if (v_newAngle > 0.0f && v_newAngle < 180.0f) {
		this->SetFrAngle(v_newAngle);
	}

	// Parallel
	// PBS - parallel box size
	float v_newPBS = p_startPBS - (p_curScreenY-p_startScreenY)*0.1f;
	this->SetParallelBoxSize(v_newPBS);

	this->SendCamToGPU();
}

void cls_OvchCamera::Rotate(glm::vec3 p_curLocalDir, glm::vec3 p_startLocalDir)
{
	glm::vec3 v_perpLocalDir = glm::normalize(glm::cross(p_startLocalDir, p_curLocalDir));
	v_perpLocalDir = v_perpLocalDir * this->GetRotM();
	this->mQua = glm::rotate(mQua, glm::angle(p_curLocalDir, p_startLocalDir), v_perpLocalDir);

	this->SendCamToGPU();
}

void cls_OvchCamera::Center(float /*p_curScreenX*/, float /*p_curScreenY*/)
{
	//TODO implement
}

glm::mat3 cls_OvchCamera::GetRotM(void) const
{
	glm::mat3 v_M = glm::mat3_cast(mQua);
	return v_M;
}

glm::mat4 cls_OvchCamera::GetModelToCamera(void) const
{
	// Form a model-to-camera matrix from the quaternion and shift vector
	glm::mat3 v_modelToCamera3x3 = this->GetRotM();
	glm::vec3 v_shift(0.0f, 0.0f, -(this->GetDist()));
	glm::vec3 v_gshift = v_shift * v_modelToCamera3x3;

	glm::mat4 v_modelToCamera;
	v_modelToCamera[0].x = v_modelToCamera3x3[0].x;
	v_modelToCamera[0].y = v_modelToCamera3x3[0].y;
	v_modelToCamera[0].z = v_modelToCamera3x3[0].z;
	v_modelToCamera[1].x = v_modelToCamera3x3[1].x;
	v_modelToCamera[1].y = v_modelToCamera3x3[1].y;
	v_modelToCamera[1].z = v_modelToCamera3x3[1].z;
	v_modelToCamera[2].x = v_modelToCamera3x3[2].x;
	v_modelToCamera[2].y = v_modelToCamera3x3[2].y;
	v_modelToCamera[2].z = v_modelToCamera3x3[2].z;
	v_modelToCamera[3].w = 1.0f;

	v_modelToCamera = glm::translate(v_modelToCamera, -(this->GetLookPt()) + v_gshift);

	return v_modelToCamera;
}

glm::mat4 cls_OvchCamera::GetCameraToClip(void) const
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	// Form a camera-to-clip matrix from the frustum
	float aspectRat = (float)v_renderer->GetWinW() / (float)v_renderer->GetWinH();
	glm::mat4 cameraToClip;
	if (mIsPerspective) {
		cameraToClip = glm::infinitePerspective(this->GetFrAngle(), aspectRat, this->GetDist() - this->GetFrNearWidth());
	} else {
		float paralS = this->GetParallelBoxSize();
		float paralD = this->GetParallelBoxDepth();
		cameraToClip = glm::ortho(-paralS*aspectRat, paralS*aspectRat, -paralS, paralS, this->GetDist()-paralD, this->GetDist()+paralD);
	}

	return cameraToClip;
}

glm::mat4 cls_OvchCamera::GetMVP(void) const
{
	glm::mat4 v_modelToCamera = this->GetModelToCamera();
	glm::mat4 v_cameraToClip = this->GetCameraToClip();

	// Form a final matrix
	glm::mat4 v_MVP = v_cameraToClip * v_modelToCamera;

	return v_MVP;
}

glm::vec3 cls_OvchCamera::GetViewerPoint(void) const
{
	glm::vec3 v_lookDirInCamSpace(0.0f, 0.0f, 1.0f);
	glm::vec3 v_lookDirInModelSpace;

	v_lookDirInModelSpace = glm::inverse(this->GetRotM()) * v_lookDirInCamSpace;

	return (this->GetLookPt() + this->GetDist() * v_lookDirInModelSpace);
}

void cls_OvchCamera::SendCamToGPU(void) const
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	glm::mat4 v_MVP = this->GetMVP();

	// Send the matrix to the GPU
	glUseProgram(v_renderer->mShadingDrawProgram);
	glUniformMatrix4fv(v_renderer->mMVPshadingUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(v_renderer->mWireframeDrawProgram);
	glUniformMatrix4fv(v_renderer->mMVPwireUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(0);

	glUseProgram(v_renderer->GetModelProcessor()->mSectionProgram);
	glUniformMatrix4fv(v_renderer->GetModelProcessor()->mMVPsectionUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(v_renderer->GetModelProcessor()->mCutterProgram);
	glUniformMatrix4fv(v_renderer->GetModelProcessor()->mMVPcutterUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(v_renderer->GetModelProcessor()->mSpaceTransferProgram);
	glUniformMatrix4fv(v_renderer->GetModelProcessor()->mMVPspaceTransferUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(0);
}
