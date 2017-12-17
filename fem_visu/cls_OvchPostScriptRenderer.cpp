#include "cls_OvchPostScriptRenderer.h"

#pragma warning(push, 1)
#include <QByteArray>
#include <libps/pslib.h>
#pragma warning(pop)

#include "cls_OvchRenderer.h"
#include "cls_OvchDisplayModel.h"

#define ALIGNER 6

cls_OvchPostScriptRenderer::cls_OvchPostScriptRenderer(void)
{
}

cls_OvchPostScriptRenderer::~cls_OvchPostScriptRenderer(void)
{
}
/*
// Input data format:
// [XYZRGBA XYZRGBA][...][...][...]
// each of X, Y, Z, R, G, B, A is a float.
// each block is a pair of points - a line.
// size is a total number of blocks.
void cls_OvchPostScriptRenderer::ExportPSMeshLines(float* p_rawData, unsigned int p_size, const QString& p_filename)
{
	// Prepare data
	LineToDraw* psLineData = new LineToDraw[p_size];
	PointToDraw pnt1;
	PointToDraw pnt2;
	for (unsigned int i=0; i<p_size; i++) {
		pnt1 = PointToDraw(&p_rawData[i*14]);
		pnt2 = PointToDraw(&p_rawData[i*14+7]);
		psLineData[i] = LineToDraw(pnt1, pnt2);
	}

	// Sort data
	std::sort(psLineData, psLineData + p_size);

	// Write data
	WritePSLines(psLineData, p_size, p_filename);
}

void cls_OvchPostScriptRenderer::WritePSLines(LineToDraw* p_sortedData, unsigned int p_numOfLines, const QString& p_filename)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	unsigned int winW = v_renderer->GetWinW();
	unsigned int winH = v_renderer->GetWinH();

	QByteArray ba;
	char* cfilename;
	ba = p_filename.toAscii();
	cfilename = ba.data();

	// Init ps file
	PSDoc* ps;
	PS_boot();
	ps = PS_new();
	PS_open_file(ps, cfilename);

	PS_begin_page(ps, (float)winW, (float)winH);

	// Draw global frame
	PS_setlinewidth(ps, 0.1f);
	PS_setcolor(ps, "stroke", "rgb", 0.0, 0.0, 0.0, 0.0);
	PS_rect(ps, 0.0f, 0.0f, (float)winW, (float)winH);
	PS_stroke(ps);

	// -----------------------------------------------------------------------------

	// Move the coordinate system to the center
	PS_translate(ps, (float)winW/2.0f, (float)winH/2.0f);

	PS_setlinewidth(ps, 0.1f);

	float scalX = (float)winW/2.0f;
	float scalY = (float)winH/2.0f;

	for (unsigned int i=0; i<p_numOfLines; i++) {
		PS_setcolor(ps, "stroke", "rgb", p_sortedData[i].p1.data[3], p_sortedData[i].p1.data[4], p_sortedData[i].p1.data[5], 0.0);
		PS_moveto(ps, scalX*p_sortedData[i].p1.data[0],  scalY*p_sortedData[i].p1.data[1]);
		PS_lineto(ps, scalX*p_sortedData[i].p2.data[0],  scalY*p_sortedData[i].p2.data[1]);
		PS_stroke(ps);
	}

	// -----------------------------------------------------------------------------

	PS_end_page(ps);

	PS_close(ps);
	PS_delete(ps);
	PS_shutdown();
}
*/
// Input data format:
// [XYZRGB XYZRGB XYZRGB][...][...][...]
// each of X, Y, Z, R, G, B is a float.
// each block is a triplet of points - a triangle.
// size is a total number of blocks.
void cls_OvchPostScriptRenderer::ExportPSMeshTriangles(float* p_rawData, unsigned int p_size, const QString& p_filename)
{
	// Prepare data
	LineToDraw* psLineData = new LineToDraw[p_size*3];
	PointToDraw pnt1;
	PointToDraw pnt2;
	for (unsigned int i=0; i<p_size; i++) {
		pnt1.Construct(&p_rawData[i*3*ALIGNER + 0]);
		pnt2.Construct(&p_rawData[i*3*ALIGNER + ALIGNER]);
		psLineData[i*3+0].Construct(pnt1, pnt2);

		pnt1.Construct(&p_rawData[i*3*ALIGNER + ALIGNER]);
		pnt2.Construct(&p_rawData[i*3*ALIGNER + 2*ALIGNER]);
		psLineData[i*3+1].Construct(pnt1, pnt2);

		pnt1.Construct(&p_rawData[i*3*ALIGNER + 2*ALIGNER]);
		pnt2.Construct(&p_rawData[i*3*ALIGNER + 0]);
		psLineData[i*3+2].Construct(pnt1, pnt2);
	}

	// Draw filled triangles
	WritePSTriangles(psLineData, p_size, p_filename);

	// or

	// Sort data and draw lines
	//	std::sort(psLineData, psLineData + size*3);
	//	WritePSMesh(psLineData, size*3, filename);

	delete [] psLineData;
}


void cls_OvchPostScriptRenderer::WritePSTriangles(LineToDraw* p_sortedData, unsigned int p_numOfTriangles, const QString& p_filename)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	unsigned int winW = v_renderer->GetWinW();
	unsigned int winH = v_renderer->GetWinH();

	QByteArray ba;
	char* cfilename;
	ba = p_filename.toLatin1();
	cfilename = ba.data();

	// Init ps file
	PSDoc* ps;
	PS_boot();
	ps = PS_new();
	PS_open_file(ps, cfilename);

	PS_begin_page(ps, (float)winW, (float)winH);

	// Draw global frame
	PS_setlinewidth(ps, 0.1f);
	PS_setcolor(ps, "stroke", "rgb", 0.0f, 0.0f, 0.0f, 0.0f);
	PS_rect(ps, 0.0f, 0.0f, (float)winW, (float)winH);
	PS_stroke(ps);

	// -----------------------------------------------------------------------------

	// Move the coordinate system to the center
	PS_translate(ps, (float)winW/2.0f, (float)winH/2.0f);

	PS_setlinewidth(ps, 0.1f);

	float scalX = (float)winW/2.0f;
	float scalY = (float)winH/2.0f;

	PointToDraw p1, p2, p3;

	for (unsigned int i=0; i<p_numOfTriangles; i++) {

		p1 = p_sortedData[i*3].mPoint1;
		p2 = p_sortedData[i*3].mPoint2;
		p3 = p_sortedData[i*3+1].mPoint2;

		PS_setcolor(ps, "fill", "rgb", p1.mData[3], p1.mData[4], p1.mData[5], 0.0f);
		PS_moveto(ps, scalX * p1.mData[0],  scalY * p1.mData[1]);
		PS_lineto(ps, scalX * p2.mData[0],  scalY * p2.mData[1]);
		PS_lineto(ps, scalX * p3.mData[0],  scalY * p3.mData[1]);
		PS_closepath(ps);
		PS_fill_stroke(ps);
	}

	// -----------------------------------------------------------------------------

	PS_end_page(ps);

	PS_close(ps);
	PS_delete(ps);
	PS_shutdown();
}

void cls_OvchPostScriptRenderer::Export(cls_OvchDisplayModel* p_model, const QString& p_filename)
{
	unsigned int v_numOfTriangles = p_model->GetNumOfTriangles();

	stc_VandC* v_origData = p_model->GetVandCdata();
	float* v_data = new float[v_numOfTriangles * 3 * ALIGNER];
	
	for (unsigned int i=0; i<v_numOfTriangles*3; i++) {
		v_data[i*ALIGNER+0] = v_origData[i].v[0];
		v_data[i*ALIGNER+1] = v_origData[i].v[1];
		v_data[i*ALIGNER+2] = v_origData[i].v[2];
		v_data[i*ALIGNER+3] = v_origData[i].c[0];
		v_data[i*ALIGNER+4] = v_origData[i].c[1];
		v_data[i*ALIGNER+5] = v_origData[i].c[2];
	}

	this->ExportPSMeshTriangles(v_data, v_numOfTriangles, p_filename);

	delete [] v_data;
}
