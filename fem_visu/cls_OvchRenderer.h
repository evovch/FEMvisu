#pragma once

#pragma warning(push, 1)
#include <QString>
#include <QBitArray>
#include "GL/glew.h"
#include <vector>
#pragma warning(pop)

class cls_OvchCamera;
class cls_OvchModel;
class cls_OvchOffscreenRenderer;
class cls_OvchPostScriptRenderer;
class cls_OvchModelProcessor;

class cls_OvchRenderer
{
	friend class cls_OvchCamera;
	friend class cls_OvchOffscreenRenderer;

// public methods
public:

	cls_OvchRenderer();
	~cls_OvchRenderer(void);

	// Singleton object instance finder
	static cls_OvchRenderer* Instance(void);

	void SetModel(cls_OvchModel* p_model);

	void InitProgs(void);
	void InitBuffers(void);

	static void CreateProg(GLuint p_program, const std::vector<GLuint>& p_shaderList);
	static GLuint CreateShader(GLenum p_eShaderType, const QString& p_strShaderFile);

	// Set the size of the screen
	void SetScreenSize(int p_winW, int p_winH);

	// Send model data to GPU
	void SendModelToGPU(void) const;
	void UpdateVandCOnGPU(void);

	// Display method
	void Display() const;

	// Get the singleton camera object
	cls_OvchCamera* GetCamera(void) const { return mCamera; }

	// Get the singleton model object
	cls_OvchModel* GetModel(void) const { return mModel; }

	// Get the singleton offscreen renderer object
	cls_OvchOffscreenRenderer* GetOffscreenRenderer(void) const { return mOffscreenRenderer; }

	// Get singleton model processor object
	cls_OvchModelProcessor* GetModelProcessor(void) const { return mModelProcessor; }

	QBitArray* GetVisMode(void) { return &mVisMode; }

	void SwitchSectioning(void);
	void SwitchWireframe(void);


	void RenderPNG(QString p_filename);
	void RenderPostScript(QString p_filename) const;

	int GetWinW(void) { return mWinW; }
	int GetWinH(void) { return mWinH; }
	int GetMinWinDim(void) { return (mWinH < mWinW) ? mWinH : mWinW; }
	float GetSphR(void) { return (mCentralCircleK * this->GetMinWinDim() / 2.0f); }

	void GeometryClick(int p_curScreenX, int p_curScreenY);

// private data members
private:

	static cls_OvchRenderer* mInstance;

	// Model object
	cls_OvchModel* mModel;

	// Camera object
	cls_OvchCamera* mCamera;

	// Offscreen renderer object
	cls_OvchOffscreenRenderer* mOffscreenRenderer;

	// PostScript renderer object
	cls_OvchPostScriptRenderer* mPostScriptRenderer;

	// Model processor object
	cls_OvchModelProcessor* mModelProcessor;

	// -----------------------------------------------

	QBitArray mVisMode;
	QBitArray mVisModeMask;

	// -----------------------------------------------

	// Window size
	int mWinW;
	int mWinH;

	// Central circle
	float mCentralCircleK;

	// -----------------------------------------------
	// OpenGL-related

	// Program objects
	GLuint mShadingDrawProgram;
	GLuint mWireframeDrawProgram;

	// Vertex array objects, vertex buffer objects and index buffer objects
	GLuint mVAOshading;
	GLuint mVBOshading;
	GLuint mIBOshading;
	GLuint mIBOwire;

	// Uniform variables for matrices
	GLuint mMVPshadingUniform;
	GLuint mMVPwireUniform;

	// Transform feedback buffer object
	GLuint mTFBO;

};
