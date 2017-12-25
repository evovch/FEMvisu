#pragma once

#pragma warning(push, 1)
#include <QSet>
#include <QString>
#include "GL/glew.h"
#pragma warning(pop)

class cls_OvchDisplayModel;

class cls_OvchOffscreenRenderer
{
public:
	cls_OvchOffscreenRenderer(void);
	cls_OvchOffscreenRenderer(GLsizei p_width, GLsizei p_height);
	~cls_OvchOffscreenRenderer(void);

	void Resize(GLsizei p_width, GLsizei p_height);

	void RenderModelToBuffer(cls_OvchDisplayModel* p_model);

	void FetchVisibleTriangles(QSet<unsigned int>& o_usedTriangles);
	unsigned int GetPickedTriangleID(int p_curScreenX, int p_curScreenY);
//private:
	void Construct(void);
	void WritePNGfile(const QString& p_filename);

private:
	// Size of the output picture
	GLsizei mPicWidth;
	GLsizei mPicHeight;

	// Container for pixels received from GPU
	GLubyte* mPixels;

	// Renderbuffer objects for color and depth and framebuffer object
	GLuint mRBOcolor;
	GLuint mRBOdepth;
	GLuint mFBO;

};
