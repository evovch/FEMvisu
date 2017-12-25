#include "cls_OvchRenderer.h"

#pragma warning(push, 1)
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include "glm/glm.hpp"
#pragma warning(pop)

#include "cls_OvchModel.h"
#include "cls_OvchCamera.h"
#include "cls_OvchTimer.h"
#include "cls_OvchOffscreenRenderer.h"
#include "cls_OvchPostScriptRenderer.h"
#include "cls_OvchModelProcessor.h"
#include "cls_OvchDisplayModel.h"

//#define CATCHERROR assert(glGetError() == GL_NO_ERROR)

cls_OvchRenderer* cls_OvchRenderer::mInstance = 0;

cls_OvchRenderer::cls_OvchRenderer()
{
	// Init program and shaders
	this->InitProgs();

	// Init buffers
	this->InitBuffers();

	// Init culling		//TODO enable/disable
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);

	// Init depth
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0f);

	// Set the first vertex of the triangle as the vertex
	// holding the color for the whole triangle for flat shading rendering
	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);


	// Init visualization mode.
	mVisMode.resize(4);
	mVisModeMask.resize(4);
	mVisMode.setBit(0);		mVisMode.setBit(1);			mVisMode.setBit(2);			mVisMode.setBit(3);
	mVisModeMask.setBit(0);	mVisModeMask.clearBit(1);	mVisModeMask.clearBit(2);	mVisModeMask.setBit(3);
	

	// Zero the camera pointer
	mCamera = NULL;

	mOffscreenRenderer = new cls_OvchOffscreenRenderer();

	mPostScriptRenderer = new cls_OvchPostScriptRenderer();

	mModelProcessor = new cls_OvchModelProcessor();

	// Init parameters for the planar draw - needed when rotating and tilting.
	mCentralCircleK = 0.8f;

	// Save singleton object
	mInstance = this;
}

cls_OvchRenderer::~cls_OvchRenderer(void)
{
	//TODO implement all the cleaning of the OpenGL stuff.

	if (mCamera) delete mCamera;
	if (mOffscreenRenderer) delete mOffscreenRenderer;
}

cls_OvchRenderer* cls_OvchRenderer::Instance(void)
{
	if (!mInstance) qDebug() << "No instance of OvchRenderer yet!";
	return mInstance;
}

void cls_OvchRenderer::InitProgs(void)
{
	// Create program and shaders

	// ------------------------------ Shading draw program ------------------------------
	mShadingDrawProgram = glCreateProgram();
	std::vector<GLuint> v_shaderList;
	v_shaderList.push_back(CreateShader(GL_VERTEX_SHADER, "shaders/vertSh_passthrough.vp"));
	v_shaderList.push_back(CreateShader(GL_GEOMETRY_SHADER, "shaders/geomSh_shading.gp"));
	v_shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, "shaders/frSh_flat.fp"));

	CreateProg(mShadingDrawProgram, v_shaderList);

	// Cleanup
	std::for_each(v_shaderList.begin(), v_shaderList.end(), glDeleteShader);

	// Connect uniform variables
	mMVPshadingUniform = glGetUniformLocation(mShadingDrawProgram, "MVP");

	// ------------------------------ Wireframe draw program ------------------------------
	mWireframeDrawProgram = glCreateProgram();
	std::vector<GLuint> shaderList2;
	shaderList2.push_back(CreateShader(GL_VERTEX_SHADER, "shaders/vertSh_wire.vp"));
	shaderList2.push_back(CreateShader(GL_GEOMETRY_SHADER, "shaders/geomSh_wire.gp"));
	shaderList2.push_back(CreateShader(GL_FRAGMENT_SHADER, "shaders/frSh_flat.fp"));

	CreateProg(mWireframeDrawProgram, shaderList2);

	// Cleanup
	std::for_each(shaderList2.begin(), shaderList2.end(), glDeleteShader);

	// Connect uniform variables
	mMVPwireUniform = glGetUniformLocation(mWireframeDrawProgram, "MVP");
}

void cls_OvchRenderer::InitBuffers(void)
{
	glGenVertexArrays(1, &mVAOshading);
	glGenBuffers(1, &mVBOshading);
	glGenBuffers(1, &mIBOshading);
	glGenBuffers(1, &mIBOwire);
	glGenBuffers(1, &mTFBO);
}

void cls_OvchRenderer::CreateProg(GLuint p_program, const std::vector<GLuint>& p_shaderList)
{
	// (Create the program outside)
	// Here - attach the shaders, link.

	for (size_t iLoop = 0; iLoop < p_shaderList.size(); iLoop++)
		glAttachShader(p_program, p_shaderList[iLoop]);

	glLinkProgram(p_program);

	GLint v_status;
	glGetProgramiv (p_program, GL_LINK_STATUS, &v_status);
	if (v_status == GL_FALSE) {
		GLint v_infoLogLength;
		glGetProgramiv(p_program, GL_INFO_LOG_LENGTH, &v_infoLogLength);

		GLchar *v_strInfoLog = new GLchar[v_infoLogLength + 1];
		glGetProgramInfoLog(p_program, v_infoLogLength, NULL, v_strInfoLog);
		qDebug() << "Linker failure:" << v_strInfoLog << endl;
		delete[] v_strInfoLog;
	}

	// Cleanup
	for (size_t iLoop = 0; iLoop < p_shaderList.size(); iLoop++)
		glDetachShader(p_program, p_shaderList[iLoop]);
}

GLuint cls_OvchRenderer::CreateShader(GLenum p_eShaderType, const QString& p_strShaderFile)
{
	// Open and read input file with shader code
	QFile v_shaderFile(p_strShaderFile);
	QString v_fullShaderCode;
	QByteArray ba;
	char* v_cstrFullShaderCode;

	if (!v_shaderFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Error opening shader file:" << p_strShaderFile << endl;
	} else {
		QTextStream v_inStream(&v_shaderFile);
		v_fullShaderCode = v_inStream.readAll();

		ba = v_fullShaderCode.toLatin1();
		v_cstrFullShaderCode = ba.data();
	}

	// Debug output
	//qDebug() << "---------------------------------------------------------------------";
	//qDebug() << cstrFullShaderCode;
	//qDebug() << "---------------------------------------------------------------------";

	// Prepare and compile shader
	GLuint v_shader = glCreateShader(p_eShaderType);
	glShaderSource(v_shader, 1, (const GLchar**)&v_cstrFullShaderCode, NULL);
	glCompileShader(v_shader);

	// Get diagnostics info and print it in case of error
	GLint v_status;
	glGetShaderiv(v_shader, GL_COMPILE_STATUS, &v_status);

	QString v_strShaderType;
	switch(p_eShaderType) {
	case GL_VERTEX_SHADER: v_strShaderType = "vertex"; break;
	case GL_GEOMETRY_SHADER: v_strShaderType = "geometry"; break;
	case GL_FRAGMENT_SHADER: v_strShaderType = "fragment"; break;
	}

	if (v_status == GL_FALSE) {
		GLint v_infoLogLength;
		glGetShaderiv(v_shader, GL_INFO_LOG_LENGTH, &v_infoLogLength);

		GLchar *v_strInfoLog = new GLchar[v_infoLogLength + 1];
		glGetShaderInfoLog(v_shader, v_infoLogLength, NULL, v_strInfoLog);

		qDebug().nospace() << "Compile failure in " << v_strShaderType.toStdString().c_str()  << " shader from " << p_strShaderFile << ": " << endl << v_strInfoLog;
		delete[] v_strInfoLog;
	} else {
		qDebug().nospace() << "Successfully compiled " << v_strShaderType.toStdString().c_str() << " shader from " << p_strShaderFile << ".";
	}

	// Finalize
	v_shaderFile.close();
	return v_shader;
}

void cls_OvchRenderer::SetModel(cls_OvchModel* p_model)
{
	mModel = p_model;

	// Init camera position according to the model
	glm::vec3 v_center; float v_radius;
	mModel->GetBoundingSphere(v_center, v_radius);

	// TODO check
	if (mCamera) delete mCamera;

	mCamera = new cls_OvchCamera(v_center, v_radius);
}

void cls_OvchRenderer::SetScreenSize(int p_winW, int p_winH)
{
	//qDebug() << "OvchRenderer::SetScreenSize";
	mWinW = p_winW;
	mWinH = p_winH;
	mCamera->SendCamToGPU();
	glViewport(0, 0, (GLsizei)mWinW, (GLsizei)mWinH);
}

void cls_OvchRenderer::SendModelToGPU(void) const
{
	mModel->GetDisplayModel()->SendToGPUFull(mVAOshading, mVBOshading, mIBOshading, mIBOwire, false);
	mModelProcessor->SendFullDataToGPU(mModel);
}

void cls_OvchRenderer::UpdateVandCOnGPU(void)
{
	mModel->GetDisplayModel()->SendToGPUvAndC(mVAOshading, mVBOshading, false);
	mModelProcessor->SendVandCdataToGPU(mModel->GetDisplayModel(), false);
}

void cls_OvchRenderer::Display() const
{
	//qDebug() << "OvchRenderer::Display";
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QBitArray v_visModeFull = mVisMode & mVisModeMask;

	if (v_visModeFull.testBit(0)) {
		mModel->GetDisplayModel()->DrawTriangles(mShadingDrawProgram, mVAOshading, mIBOshading);
	}

	if (v_visModeFull.testBit(1) && !v_visModeFull.testBit(2)) {
		mModelProcessor->Display(mModel, true, false, false);
	}

	if (v_visModeFull.testBit(2) && !v_visModeFull.testBit(1)) {
		mModelProcessor->Display(mModel, false, true, false);
	}

	if (v_visModeFull.testBit(1) && v_visModeFull.testBit(2)) {
		mModelProcessor->Display(mModel, true, true, false);
	}

	if (v_visModeFull.testBit(3)) {
		mModel->GetDisplayModel()->DrawWires(mWireframeDrawProgram, mVAOshading, mIBOwire);
	}
}

void cls_OvchRenderer::SwitchSectioning(void)
{
	mVisModeMask.toggleBit(0);
	mVisModeMask.toggleBit(1);
	mVisModeMask.toggleBit(2);
}

void cls_OvchRenderer::SwitchWireframe(void)
{
	mVisModeMask.toggleBit(3);
}

void cls_OvchRenderer::RenderPNG(QString p_filename)
{
	qDebug() << "Rendering PNG...";
	cls_OvchTimer v_timer;
	v_timer.Start();

	QBitArray v_visModeFull = mVisMode & mVisModeMask;

	cls_OvchDisplayModel* v_cutModel = new cls_OvchDisplayModel();

	mModel->BuildDisplayModelFull();

	if (v_visModeFull.testBit(1) && !v_visModeFull.testBit(2)) {
		mModelProcessor->ProcessCutter(mModel->GetDisplayModel(), v_cutModel, false);
	} else if (v_visModeFull.testBit(2) && !v_visModeFull.testBit(1)) {
		mModelProcessor->ProcessSection(mModel, v_cutModel, false);
	} else if (v_visModeFull.testBit(1) && v_visModeFull.testBit(2)) {
		mModelProcessor->ProcessSectionAndCutter(mModel, v_cutModel);
	} else if (v_visModeFull.testBit(0) && !v_visModeFull.testBit(1) && !v_visModeFull.testBit(2)) { //full
		*v_cutModel = *mModel->GetDisplayModel();
	}	

	mModelProcessor->SendVandCdataToGPU(v_cutModel, false);
	mModelProcessor->SendTriangleDataToGPU(v_cutModel);
	mModelProcessor->SendCamToGPU(mCamera);

	mOffscreenRenderer->RenderModelToBuffer(v_cutModel);
	mOffscreenRenderer->WritePNGfile(p_filename);

	delete v_cutModel;

	this->SendModelToGPU();
	mModelProcessor->SendFullDataToGPU(mModel);

	qDebug() << v_timer.Stop()/1000.0 << "s";
}

void cls_OvchRenderer::RenderPostScript(QString p_filename) const
{
	if (mModel->mFormat == etnGCDGR) {
		qDebug() << "Impossible to export PostScript from graphics-only model.";
		return;
	}

	qDebug() << "Rendering PostScript...";
	cls_OvchTimer v_timer;
	v_timer.Start();

	QBitArray v_visModeFull = mVisMode & mVisModeMask;

	cls_OvchDisplayModel* v_cutModel = new cls_OvchDisplayModel();
	cls_OvchDisplayModel* v_cutModel2 = new cls_OvchDisplayModel();
	QSet<unsigned int> v_usedTriangles;

	mModel->BuildDisplayModelFull();

	float v_z = mModelProcessor->GetSectionOrigin().z;
	//for (v_z = 0.5f; v_z < 3.0f; v_z += 0.5f)
	{
		//mModelProcessor->SetSectionParams(glm::vec3(0.0, 0.0, v_z), glm::vec3(0.0, 0.0, 1.0));

		if (v_visModeFull.testBit(1) && !v_visModeFull.testBit(2)) {
			mModelProcessor->ProcessCutter(mModel->GetDisplayModel(), v_cutModel, false);
		} else if (v_visModeFull.testBit(2) && !v_visModeFull.testBit(1)) {
			mModelProcessor->ProcessSection(mModel, v_cutModel, false);
		} else if (v_visModeFull.testBit(1) && v_visModeFull.testBit(2)) {
			mModelProcessor->ProcessSectionAndCutter(mModel, v_cutModel);
		} else if (v_visModeFull.testBit(0) && !v_visModeFull.testBit(1) && !v_visModeFull.testBit(2)) { //full
			*v_cutModel = *mModel->GetDisplayModel();
		}		

		v_cutModel->PrepareUniqueColors();
		mModelProcessor->SendVandCdataToGPU(v_cutModel, true);
		mModelProcessor->SendTriangleDataToGPU(v_cutModel);
		mModelProcessor->SendCamToGPU(mCamera);
		mOffscreenRenderer->RenderModelToBuffer(v_cutModel);
		//mOffscreenRenderer->WritePNGfile(p_filename + QString::number(v_z) + "__.png");

		v_usedTriangles.clear();
		mOffscreenRenderer->FetchVisibleTriangles(v_usedTriangles);
		v_cutModel->LeaveTriangles(v_usedTriangles);

		mModelProcessor->ProcessSpaceTransfer(v_cutModel, v_cutModel2);

		mPostScriptRenderer->Export(v_cutModel2, p_filename + "_z=" + QString::number(v_z) + ".ps");
	}

	delete v_cutModel;
	delete v_cutModel2;

	this->SendModelToGPU();
	mModelProcessor->SendFullDataToGPU(mModel);

	qDebug() << v_timer.Stop()/1000.0 << "s";
}

void cls_OvchRenderer::GeometryClick(int /*p_curScreenX*/, int /*p_curScreenY*/)
{
/*
	mModel->PrepareDataForGPU_uniqueColors();

	glBindVertexArray(mVAOaux);
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBOaux);
		glBufferData(GL_ARRAY_BUFFER, (mModel->GetGPUnumOfVertices())*sizeof(stc_VandC), mModel->mGPUvertexANDcolorData_uniqueColors, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(stc_VandC), (void*)offsetof(stc_VandC, v));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(stc_VandC), (void*)offsetof(stc_VandC, c));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
	glBindVertexArray(0);

	unsigned int v_triangleID;
	v_triangleID = mOffscreenRenderer->GetPickedTriangleID(p_curScreenX, p_curScreenY);

	mModel->HighlightTriangle(v_triangleID);
	this->UpdateVandCOnGPU();
*/
}
