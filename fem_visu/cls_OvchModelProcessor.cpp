#include "cls_OvchModelProcessor.h"

#pragma warning(push, 1)
//#include <QDebug>
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#pragma warning(pop)

#include "cls_OvchRenderer.h"
#include "cls_OvchCamera.h"
#include "cls_OvchModel.h"
#include "cls_OvchDisplayModel.h"

cls_OvchModelProcessor::cls_OvchModelProcessor(void)
{
	// ------------------------------ Cutter program ------------------------------
	mCutterProgram = glCreateProgram();
	std::vector<GLuint> v_shaderList;
	v_shaderList.push_back(cls_OvchRenderer::CreateShader(GL_VERTEX_SHADER, "shaders/vertSh_passthrough.vp"));
	v_shaderList.push_back(cls_OvchRenderer::CreateShader(GL_GEOMETRY_SHADER, "shaders/geomSh_cutter.gp"));
	v_shaderList.push_back(cls_OvchRenderer::CreateShader(GL_FRAGMENT_SHADER, "shaders/frSh_flat.fp"));

	const char* varying_names[] = { "additional_Position", "additional_Color" };
	glTransformFeedbackVaryings(mCutterProgram, 2, varying_names, GL_INTERLEAVED_ATTRIBS);

	cls_OvchRenderer::CreateProg(mCutterProgram, v_shaderList);

	// Cleanup
	std::for_each(v_shaderList.begin(), v_shaderList.end(), glDeleteShader);

	// Connect uniform variables
	mMVPcutterUniform = glGetUniformLocation(mCutterProgram, "MVP");
	mOriginCutterUniform = glGetUniformLocation(mCutterProgram, "sectionOrigin");
	mNormCutterUniform = glGetUniformLocation(mCutterProgram, "sectionNorm");

	// ------------------------------ Section program ------------------------------
	mSectionProgram = glCreateProgram();
	std::vector<GLuint> shaderList2;
	shaderList2.push_back(cls_OvchRenderer::CreateShader(GL_VERTEX_SHADER, "shaders/vertSh_passthrough.vp"));
	shaderList2.push_back(cls_OvchRenderer::CreateShader(GL_GEOMETRY_SHADER, "shaders/geomSh_section.gp"));
	shaderList2.push_back(cls_OvchRenderer::CreateShader(GL_FRAGMENT_SHADER, "shaders/frSh_flat.fp"));

	const char* varying_names2[] = { "additional_Position", "additional_Color" };
	glTransformFeedbackVaryings(mSectionProgram, 2, varying_names2, GL_INTERLEAVED_ATTRIBS);

	cls_OvchRenderer::CreateProg(mSectionProgram, shaderList2);

	// Cleanup
	std::for_each(shaderList2.begin(), shaderList2.end(), glDeleteShader);

	// Connect uniform variables
	mMVPsectionUniform = glGetUniformLocation(mSectionProgram, "MVP");
	mOriginSectionUniform = glGetUniformLocation(mSectionProgram, "sectionOrigin");
	mNormSectionUniform = glGetUniformLocation(mSectionProgram, "sectionNorm");

	// ------------------------------ Space transfer program ------------------------------
	mSpaceTransferProgram = glCreateProgram();
	std::vector<GLuint> shaderList3;
	shaderList3.push_back(cls_OvchRenderer::CreateShader(GL_VERTEX_SHADER, "shaders/vertSh_passthrough.vp"));
	shaderList3.push_back(cls_OvchRenderer::CreateShader(GL_GEOMETRY_SHADER, "shaders/geomSh_spacetransfer.gp"));
	shaderList3.push_back(cls_OvchRenderer::CreateShader(GL_FRAGMENT_SHADER, "shaders/frSh_flat.fp"));

	const char* varying_names3[] = { "additional_Position", "additional_Color" };
	glTransformFeedbackVaryings(mSpaceTransferProgram, 2, varying_names3, GL_INTERLEAVED_ATTRIBS);

	cls_OvchRenderer::CreateProg(mSpaceTransferProgram, shaderList3);

	// Cleanup
	std::for_each(shaderList3.begin(), shaderList3.end(), glDeleteShader);

	// Connect uniform variables
	mMVPspaceTransferUniform = glGetUniformLocation(mSpaceTransferProgram, "MVP");

	// ------------------------------ Other staff ------------------------------
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mIBOelements);
	glGenBuffers(1, &mIBOtriangles);
	glGenBuffers(1, &mTFBO);
	glGenQueries(1, &mQuery);

	mSectionOrigin = glm::vec3(0.0, 0.0, 0.0);
	mSectionNorm = glm::vec3(0.0, 0.0, 1.0);
}

cls_OvchModelProcessor::~cls_OvchModelProcessor(void)
{
	glDeleteProgram(mCutterProgram);
	glDeleteProgram(mSectionProgram);
	glDeleteProgram(mSpaceTransferProgram);

	glDeleteVertexArrays(1, &mVAO);
	glDeleteBuffers(1, &mVBO);
	glDeleteBuffers(1, &mIBOelements);
	glDeleteBuffers(1, &mIBOtriangles);
	glDeleteBuffers(1, &mTFBO);
	glDeleteQueries(1, &mQuery);
}

void cls_OvchModelProcessor::SetSectionParams(glm::vec3 p_origin, glm::vec3 p_norm)
{
	this->SetSectionOrigin(p_origin);
	this->SetSectionNorm(p_norm);
	this->SendSectionParamsToGPU();
}

void cls_OvchModelProcessor::SendSectionParamsToGPU(void) const
{
	glUseProgram(mSectionProgram);
	glUniform3f(mOriginSectionUniform, mSectionOrigin.x, mSectionOrigin.y, mSectionOrigin.z);
	glUniform3f(mNormSectionUniform, mSectionNorm.x, mSectionNorm.y, mSectionNorm.z);
	//glUseProgram(0);
	glUseProgram(mCutterProgram);
	glUniform3f(mOriginCutterUniform, mSectionOrigin.x, mSectionOrigin.y, mSectionOrigin.z);
	glUniform3f(mNormCutterUniform, mSectionNorm.x, mSectionNorm.y, mSectionNorm.z);
	glUseProgram(0);
}

void cls_OvchModelProcessor::MoveSectionOrigin(float p_shift, glm::vec3 p_startCoords)
{
	mSectionOrigin = p_startCoords + p_shift * mSectionNorm;
	this->SendSectionParamsToGPU();
}

void cls_OvchModelProcessor::RotateSectionNorm(glm::vec3 p_curLocalDir, glm::vec3 p_startLocalDir)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	cls_OvchCamera* v_camera = v_renderer->GetCamera();

	glm::vec3 v_perpLocalDir = glm::normalize(glm::cross(p_startLocalDir, p_curLocalDir));
	v_perpLocalDir = v_perpLocalDir * v_camera->GetRotM();

	mSectionNorm = glm::rotate(mSectionNorm, glm::angle(p_curLocalDir, p_startLocalDir), v_perpLocalDir);

	this->SendSectionParamsToGPU();
}

void cls_OvchModelProcessor::SendVandCdataToGPU(const cls_OvchDisplayModel* p_model, bool p_uniqueColor /*= false*/) const
{
	p_model->SendToGPUvAndC(mVAO, mVBO, p_uniqueColor);
}

void cls_OvchModelProcessor::SendTriangleDataToGPU(const cls_OvchDisplayModel* p_model) const
{
	p_model->SendToGPUtriangles(mIBOtriangles);
}

void cls_OvchModelProcessor::SendElementsDataToGPU(const cls_OvchModel* p_model) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOelements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, p_model->GetNumOfElements()*4*sizeof(unsigned int), p_model->GetElements(), GL_STATIC_DRAW);
}

void cls_OvchModelProcessor::SendFullDataToGPU(const cls_OvchModel* p_model) const
{
	this->SendVandCdataToGPU(p_model->GetDisplayModel());
	this->SendTriangleDataToGPU(p_model->GetDisplayModel());
	this->SendElementsDataToGPU(p_model);
}

void cls_OvchModelProcessor::SendCamToGPU(cls_OvchCamera* p_camera) const
{
	glm::mat4 v_MVP = p_camera->GetMVP();

	glUseProgram(mSectionProgram);
	glUniformMatrix4fv(mMVPsectionUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	//glUseProgram(0);
	glUseProgram(mCutterProgram);
	glUniformMatrix4fv(mMVPcutterUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	//glUseProgram(0);
	glUseProgram(mSpaceTransferProgram);
	glUniformMatrix4fv(mMVPspaceTransferUniform, 1, GL_FALSE, glm::value_ptr(v_MVP));
	glUseProgram(0);
}

void cls_OvchModelProcessor::ProcessSection(const cls_OvchModel* p_model, cls_OvchDisplayModel* o_result, bool p_append /*= false*/) const
{
	this->SendVandCdataToGPU(p_model->GetDisplayModel());
	this->SendElementsDataToGPU(p_model);

	unsigned int v_expectedMaxDataSize;

	//TODO - test this cheating
	// Theoretically maximum possible number of triangles generated - twice more than number of elements
	// However this is only in theory. Practically _most_ of the elements are thrown away
	// because they are far away from the cutting plane and we cut only a very small number of elements.
	// I set a limit on the max size of the received data to approx 32Mb which correspond to
	// cutting a model of 200000 elements in which all the elements intersect the cutting plane.

	// if N is number of elements
	// N * 2 = max number of triangles
	// N * 2 * 3 = corresponding number of vertices
	// N * 2 * 3 * 6 = corresponding number of floats
	// N * 2 * 3 * 6 * 4 = corresponding number of bytes taking into account that sizeof(float)=4
	// Thus ((N * 2 * 3 * 6 * 4)/1024)/1024 is the size of the data bunch in Mb

	if (p_model->GetNumOfElements() <= 200000)
		v_expectedMaxDataSize = p_model->GetNumOfElements() * 2*3*6*sizeof(float);
	else
		v_expectedMaxDataSize = 200000 * 2*3*6*sizeof(float);

	glUseProgram(mSectionProgram);
	glEnable(GL_RASTERIZER_DISCARD);
	{
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, mTFBO);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, v_expectedMaxDataSize, NULL, GL_DYNAMIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTFBO);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mQuery);

		glBeginTransformFeedback(GL_TRIANGLES);
		{
			glBindVertexArray(mVAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOelements);

			glDrawElements(GL_LINES_ADJACENCY, p_model->GetNumOfElements()*4, GL_UNSIGNED_INT, NULL);

			glBindVertexArray(0);
		}
		glEndTransformFeedback();

		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	}
	glDisable(GL_RASTERIZER_DISCARD);
	glUseProgram(0);

	glFlush();

	// Extract the number of triangles written
	GLuint v_numOfPrimitivesWrittenSection;
	glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &v_numOfPrimitivesWrittenSection);
	//qDebug() << v_numOfPrimitivesWrittenSection << "primitives written.";

	// Allocate memory for received data
	float* v_receivedSecionData = new float[v_numOfPrimitivesWrittenSection*3*6];

	// Receive data
	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, v_numOfPrimitivesWrittenSection*3*6*sizeof(float), (GLvoid*)v_receivedSecionData);

	if (p_append) 
		o_result->AppendFromTFdata(v_receivedSecionData, v_numOfPrimitivesWrittenSection);
	else
		o_result->ConstructFromTFdata(v_receivedSecionData, v_numOfPrimitivesWrittenSection);

	delete [] v_receivedSecionData;
}


void cls_OvchModelProcessor::ProcessCutter(const cls_OvchDisplayModel* p_dispmodel, cls_OvchDisplayModel* o_result, bool p_append /*= false*/) const
{
	this->SendVandCdataToGPU(p_dispmodel);
	this->SendTriangleDataToGPU(p_dispmodel);

	unsigned int v_expectedMaxDataSize;

	//TODO
	// Maximum possible number of generated triangles is twice more than number of triangles in the original model.
	if (p_dispmodel->GetNumOfTriangles() <= 200000)
		v_expectedMaxDataSize = p_dispmodel->GetNumOfTriangles() * 2*3*6*sizeof(float);
	else
		v_expectedMaxDataSize = 200000 * 2*3*6*sizeof(float);

	glUseProgram(mCutterProgram);
	glEnable(GL_RASTERIZER_DISCARD);
	{
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, mTFBO);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, v_expectedMaxDataSize, NULL, GL_DYNAMIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTFBO);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mQuery);

		glBeginTransformFeedback(GL_TRIANGLES);
		{
			glBindVertexArray(mVAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

			glDrawElements(GL_TRIANGLES, p_dispmodel->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

			glBindVertexArray(0);
		}
		glEndTransformFeedback();

		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	}
	glDisable(GL_RASTERIZER_DISCARD);
	glUseProgram(0);

	// Extract the number of triangles written
	GLuint v_numOfPrimitivesWrittenCutter;
	glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &v_numOfPrimitivesWrittenCutter);
	//qDebug() << v_numOfPrimitivesWrittenCutter << "primitives written.";

	// Allocate memory for received data
	float* v_receivedCutterData = new float[v_numOfPrimitivesWrittenCutter*3*6];

	// Receive data
	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, v_numOfPrimitivesWrittenCutter*3*6*sizeof(float), (GLvoid*)v_receivedCutterData);

	// ---------------------------------------------------------------------------------------------------

	if (p_append)
		o_result->AppendFromTFdata(v_receivedCutterData, v_numOfPrimitivesWrittenCutter);
	else
		o_result->ConstructFromTFdata(v_receivedCutterData, v_numOfPrimitivesWrittenCutter);

	delete [] v_receivedCutterData;
}

void cls_OvchModelProcessor::ProcessSectionAndCutter(const cls_OvchModel* p_model, cls_OvchDisplayModel* o_result) const
{
	this->ProcessSection(p_model, o_result, false);
	this->ProcessCutter(p_model->GetDisplayModel(), o_result, true);
}

void cls_OvchModelProcessor::Display(const cls_OvchModel* p_model, bool p_toDrawCutter, bool p_toDrawSection, bool p_toDrawSpaceTransfer) const
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (p_toDrawCutter) {
		glUseProgram(mCutterProgram);
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

		glDrawElements(GL_TRIANGLES, p_model->GetDisplayModel()->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	if (p_toDrawSection) {
		glUseProgram(mSectionProgram);
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOelements);

		glDrawElements(GL_LINES_ADJACENCY, p_model->GetNumOfElements()*4, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	if (p_toDrawSpaceTransfer) {
		glUseProgram(mSpaceTransferProgram);
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

		glDrawElements(GL_TRIANGLES, p_model->GetDisplayModel()->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
		glUseProgram(0);
	}
}

void cls_OvchModelProcessor::Display(const cls_OvchDisplayModel* p_model, bool p_toDrawCutter, bool p_toDrawSpaceTransfer) const
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (p_toDrawCutter) {
		glUseProgram(mCutterProgram);
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

		glDrawElements(GL_TRIANGLES, p_model->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	if (p_toDrawSpaceTransfer) {
		glUseProgram(mSpaceTransferProgram);
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

		glDrawElements(GL_TRIANGLES, p_model->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);
		glUseProgram(0);
	}
}

void cls_OvchModelProcessor::ProcessSpaceTransfer(const cls_OvchDisplayModel* p_dispmodel, cls_OvchDisplayModel* o_result) const
{
	// Send camera (MVP matrix) to GPU
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	cls_OvchCamera* v_camera = v_renderer->GetCamera();
	this->SendCamToGPU(v_camera);

	// Send vertices, colors and triangles to GPU
	p_dispmodel->SendToGPUvAndCNormal(mVAO, mVBO);
	p_dispmodel->SendToGPUtriangles(mIBOtriangles);

	// Start transform feedback process of the 'space transform' program.
	// This program only changes the space from the model to clip.
	// Geometry in clip space is ready to be drawn.

	// Normally we expect the 'space transform' pipeline return as many triangles (primitives)
	// as have been sent to it.
	unsigned int v_expectedDataSize = p_dispmodel->GetNumOfTriangles() * 3*6*sizeof(float);

	glUseProgram(mSpaceTransferProgram);
	glEnable(GL_RASTERIZER_DISCARD);
	{
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, mTFBO);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, v_expectedDataSize, NULL, GL_DYNAMIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTFBO);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mQuery);

		glBeginTransformFeedback(GL_TRIANGLES);
		{
			glBindVertexArray(mVAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOtriangles);

			glDrawElements(GL_TRIANGLES, p_dispmodel->GetNumOfTriangles()*3, GL_UNSIGNED_INT, NULL);

			glBindVertexArray(0);
		}
		glEndTransformFeedback();

		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	}
	glDisable(GL_RASTERIZER_DISCARD);
	glUseProgram(0);

	// Extract the number of triangles written
	GLuint v_numOfPrimitivesWritten;
	glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &v_numOfPrimitivesWritten);
	//qDebug() << p_dispmodel->GetNumOfTriangles() << "primitives expected.";
	//qDebug() << v_numOfPrimitivesWritten << "primitives written.";

	// Allocate memory for received data
	float* v_receivedCutterData = new float[v_numOfPrimitivesWritten*3*6];

	// Receive data
	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, v_numOfPrimitivesWritten*3*6*sizeof(float), (GLvoid*)v_receivedCutterData);

	// Write output display model object
	o_result->ConstructFromTFdata(v_receivedCutterData, v_numOfPrimitivesWritten);

	// Cleanup
	delete [] v_receivedCutterData;
}
