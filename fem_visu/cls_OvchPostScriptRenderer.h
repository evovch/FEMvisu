#pragma once

#pragma warning(push, 1)
#include <QString>
#pragma warning(pop)

class cls_OvchDisplayModel;

class PointToDraw
{
public:
	// X, Y, Z, R, G, B
	float mData[6];

	PointToDraw(void)
	{}
	PointToDraw (float inX, float inY, float inZ, float inR, float inG, float inB) {
		mData[0]=inX; mData[1]=inY; mData[2]=inZ; mData[3]=inR; mData[4]=inG; mData[5]=inB;
	}
	PointToDraw(float* p_data) {
		for (unsigned int i=0; i<6; i++) mData[i] = p_data[i];
	}
	void Construct(float* p_data) {
		for (unsigned int i=0; i<6; i++) mData[i] = p_data[i];
	}
	bool operator< (const PointToDraw& op2) const {
		return (this->mData[2] < op2.mData[2]);
	}
	PointToDraw& operator=(PointToDraw& arg) {
		for (unsigned int i=0; i<6; i++) this->mData[i] = arg.mData[i];
		return *this;
	}
};

class LineToDraw
{
public:
	PointToDraw mPoint1;
	PointToDraw mPoint2;

	LineToDraw(void)
	{}
	LineToDraw (PointToDraw p_point1, PointToDraw p_point2) :
		mPoint1(p_point1), mPoint2(p_point2)
	{}
	void Construct(PointToDraw p_point1, PointToDraw p_point2) {
		mPoint1 = p_point1;	mPoint2 = p_point2;
	}
	bool operator< (const LineToDraw& op2) const {
		return (op2.mPoint1 < mPoint1);
	}
};

class cls_OvchPostScriptRenderer
{
private:

public:
	cls_OvchPostScriptRenderer(void);
	~cls_OvchPostScriptRenderer(void);

	void Export(cls_OvchDisplayModel* p_model, const QString& p_filename);
/*
	void ExportPSMeshLines(float* p_rawData, unsigned int p_size, const QString& p_filename);
	void WritePSLines(LineToDraw* p_sortedData, unsigned int p_numOfLines, const QString& p_filename);
*/
	void ExportPSMeshTriangles(float* p_rawData, unsigned int p_size, const QString& p_filename);
	void WritePSTriangles(LineToDraw* p_sortedData, unsigned int p_numOfTriangles, const QString& p_filename);
};
