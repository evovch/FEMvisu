#pragma once

#pragma warning(push, 1)
#include "GL/glew.h"
#include "glm/glm.hpp"
#pragma warning(pop)

class cls_OvchCamera;
class cls_OvchModel;
class cls_OvchDisplayModel;

class cls_OvchModelProcessor
{

	friend class cls_OvchCamera;

// public methods
public:

	cls_OvchModelProcessor(void);
	~cls_OvchModelProcessor(void);

	// The mDisplayModel of the p_model must be built before calling any of the methods here

	// Build the section of the given geometry. Put the result into the output display-model object.
	void ProcessSection(const cls_OvchModel* p_model, cls_OvchDisplayModel* o_result, bool p_append = false) const;
	// Cut the given geometry and put left triangles into the output display-model object.
	void ProcessCutter(const cls_OvchDisplayModel* p_dispmodel, cls_OvchDisplayModel* o_result, bool p_append = false) const;
	// Simply call ProcessSection() and ProcessCutter() one after another and put results into one output display-model object.
	void ProcessSectionAndCutter(const cls_OvchModel* p_model, cls_OvchDisplayModel* o_result) const;
	// Move the given object from its model space into clip space. Put the result into the output display-model object.
	void ProcessSpaceTransfer(const cls_OvchDisplayModel* p_dispmodel, cls_OvchDisplayModel* o_result) const;

	// Getters/setters for the section parameters
	glm::vec3 GetSectionOrigin(void) const { return mSectionOrigin; }
	glm::vec3 GetSectionNorm(void) const { return mSectionNorm; }
	void SetSectionOrigin(glm::vec3 p_origin) { mSectionOrigin = p_origin; }
	void SetSectionNorm(glm::vec3 p_norm) { mSectionNorm = p_norm; }

	void MoveSectionOrigin(float p_shift, glm::vec3 p_startCoords);
	// Rotate the section normal
	void RotateSectionNorm(glm::vec3 p_curLocalDir, glm::vec3 p_startLocalDir);

	// Set the section parameters and send them to GPU
	void SetSectionParams(glm::vec3 p_origin, glm::vec3 p_norm);
	// Send the section parameters to GPU
	void SendSectionParamsToGPU(void) const;

	// Send the given camera to GPU
	void SendCamToGPU(cls_OvchCamera* p_camera) const;

	// Draw given model using the specified programs.
	// This method differs from the Process*** method in the following:
	// rasterization is ON here, transform feedback is not executed - 
	// we do not get any output object, but render pixels into the current OpenGL context
	void Display(const cls_OvchModel* p_model, bool p_toDrawCutter, bool p_toDrawSection, bool p_toDrawSpaceTransfer) const;
	void Display(const cls_OvchDisplayModel* p_model, bool p_toDrawCutter, bool p_toDrawSpaceTransfer) const;

	// Send vertices and colors of the given display-model to GPU
	void SendVandCdataToGPU(const cls_OvchDisplayModel* p_model, bool p_uniqueColor = false) const;
	// Send triangles of the given display-model to GPU
	void SendTriangleDataToGPU(const cls_OvchDisplayModel* p_model) const;
	// Send elements of the given model to GPU
	void SendElementsDataToGPU(const cls_OvchModel* p_model) const;
	// Simply call SendVandC-, SendTriangle-, SendElements-DataToGPU methods.
	void SendFullDataToGPU(const cls_OvchModel* p_model) const;

// private data members
private:

	// Section parameters
	glm::vec3 mSectionOrigin;
	glm::vec3 mSectionNorm;

	// Program objects
	GLuint mCutterProgram;
	GLuint mSectionProgram;
	GLuint mSpaceTransferProgram;

	// Uniform variables
	GLuint mMVPcutterUniform;
	GLuint mOriginCutterUniform;
	GLuint mNormCutterUniform;

	GLuint mMVPsectionUniform;
	GLuint mOriginSectionUniform;
	GLuint mNormSectionUniform;

	GLuint mMVPspaceTransferUniform;

	// Vertex array object
	GLuint mVAO;
	// Vertex buffer object
	GLuint mVBO;
	// Index buffer object for section program
	GLuint mIBOelements;
	// Index buffer object for cutter and space transfer program
	GLuint mIBOtriangles;

	// Transform feedback buffer object
	GLuint mTFBO;

	// Transform feedback query object
	GLuint mQuery;
};
