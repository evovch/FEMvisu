#pragma once

#pragma warning(push, 1)
#include <QAtomicInt>
#include <QMap>
#include <QSet>
#include <QString>
#include "glm/glm.hpp"
#pragma warning(pop)

#include "Support.h"

class cls_OvchDisplayModel;

typedef std::pair<unsigned int, unsigned int> linePair;

enum enu_GeometryFormat {
	etnMV, etnMV2, etnNEU, etnANEU, etnGCDGR
};

struct stc_sort_pred {
	bool operator()(const linePair &left, linePair &right) {
		return ((left.first < right.first) || 
			((left.first == right.first) && (left.second < right.second)));
	}
};

struct stc_unique_pred {
	bool operator()(const linePair &left, linePair &right) {
		return ((left.first == right.first) && (left.second == right.second));
	}
};

struct stc_sort_pred2 {
	bool operator()(const std::pair<unsigned int, unsigned int> &left,
						std::pair<unsigned int, unsigned int> &right) {
		return (left.second > right.second);
	}
};

class cls_OvchModel
{

// public methods
public:

	cls_OvchModel(void);
	~cls_OvchModel(void);

	void FreeMemory(void);

	unsigned int Import(QString p_filename);

	void PrintModelInfo(void);

	void BuildDisplayModelFull(void);
	void BuildDisplayModelVandC(void);
	void BuildDisplayModelTriangles(void);
	void BuildDisplayModelWires(void);

	void PrepareDataForGPU_uniqueColors(void);

	void HighlightTriangle(unsigned int p_triangleID);

/*
	void ExportGraphics(QString p_filename);
    void ExportFull(QString p_filename);
*/

	void SetField(bool p_field) { mTriangleColorFieldActive = p_field; }
	signed int SetCurVertexColorFieldIndex(signed int p_newColorFieldsIndex);
	signed int DecrCurVertexColorFieldIndex(void);
	signed int IncrCurVertexColorFieldIndex(void);
	signed int SetCurTriangleColorFieldIndex(signed int p_newColorFieldsIndex);
	signed int DecrCurTriangleColorFieldIndex(void);
	signed int IncrCurTriangleColorFieldIndex(void);

	// Get axis-aligned bounding box.
	float* GetAABB(void) { return mAABB; }

	// Compute bounding sphere parameters and store them in the mBS*** members
	void InitBoundingSphere();

	// Get the bounding sphere parameters from the previously initialized mBS*** members
	void GetBoundingSphere(glm::vec3 &o_center, float &o_radius);

	unsigned int GetNumOfElements(void) const { return mNumOfElements; }
	unsigned int* GetElements(void) const { return mElementIndices; }

	unsigned int GetGPUnumOfVertices(void) const { return mNumOfVerticesPost; }

	cls_OvchDisplayModel* GetDisplayModel(void) const { return mDisplayModel; }

	// private methods
private:

	static void ReadLineSkipComment(char* o_destination, FILE* p_file);

	unsigned int ReadMV2(QString p_filename);
	unsigned int ReadANEU(QString p_filename);
    /*unsigned int ReadGCDGR(QString p_filename);
    unsigned int ReadGCDGEO(QString p_filename);*/
	void PostProcess(void);
	
	void PrepareSurfaceData(void);
	void SeparateFaces(QMap<unsigned int, unsigned int>& o_newVertexToOld);
	void BuildMissingVerticesForFace(unsigned int p_faceIndex, QMap<unsigned int, unsigned int>& o_newVertexToOld);
	void GenerateWireframe(void);
	void IncludeNewVertices(QMap<unsigned int, unsigned int> p_newVertexToOld);

// This data is filled from the file or computed on the fly during the first stage
public:
	enu_GeometryFormat mFormat;

// private data members
private:
	// First section
	unsigned int mNumOfVerticesPre;
	unsigned int mNumOfPerVertexColorFields;

	float* mVertexCoordinatesPre;				// [mNumOfVerticesPre * v_numOfCoordinates] where v_numOfCoordinates=3
	float* mPerVertexColorFields;				// [mNumOfVerticesPre * mNumOfPerVertexColorFields]
	float* mPerVertexColorFieldsMin;			// [mNumOfPerVertexColorFields]
	float* mPerVertexColorFieldsMax;			// [mNumOfPerVertexColorFields]

	// Second section
	unsigned int mNumOfElements;

	unsigned int* mElementIndices;				// [mNumOfElements * 4]
	unsigned int* mDomainID;					// [mNumOfElements]

	// Third section
	unsigned int mNumOfTriangles;
	unsigned int mNumOfPerTriangleColorFields;	// Including BC_id having mBCidPosition index in the array
	unsigned int mBCidPosition;

	unsigned int* mTriangleIndices;				// [mNumOfTriangles * v_numOfPerTriangleIndices] where v_numOfPerTriangleIndices=3
	float* mPerTriangleColorFields;				// [mNumOfTriangles * mNumOfPerTriangleColorFields]
	float* mPerTriangleColorFieldsMin;			// [mNumOfPerTriangleColorFields]
	float* mPerTriangleColorFieldsMax;			// [mNumOfPerTriangleColorFields]
	unsigned int* mElementID;					// [mNumOfTriangles]

	// Axis-aligned bounding box
	// Xmin, Xmax, Ymin, Ymax, Zmin, Zmax
	float mAABB[6];

// This data is computed afterwards from the data from the first section

	// Bounding sphere
// 	glm::vec3 mBScenter;
// 	float mBSradius;

	// Filled during PrepareSurfaceData()
	unsigned int mNumOfFaces;
	// key - subsequent integer, value - face ID
	QMap<unsigned int, unsigned int> mFaceIndexToID;				// [mNumOfFaces]
	// key - face ID, value - subsequent integer
	QMap<unsigned int, unsigned int> mFaceIDtoIndex;				// [mNumOfFaces]
	// key - face ID, value - number of triangles in the face
	QMap<unsigned int, unsigned int> mFaceIDtoNumOfTriangles;		// [mNumOfFaces]
	// key - subsequent integer, value - list of triangle indices
	unsigned int** mFacesPre;										// [mNumOfFaces][4*mFaceIDtoNumOfTriangles(mFaceIndexToID(index))]

// This is the section where the processed vertices and triangles grouped in faces are stored

	unsigned int mNumOfVerticesPost;
	float* mVertexCoordinatesPost;			// [mNumOfVerticesPost*3]
	float* mPerVertexColorFieldsPost;		// [mNumOfVerticesPost * mNumOfPerVertexColorFields]
	unsigned int** mFacesPost;				// [mNumOfFaces][4*mFaceIDtoNumOfTriangles(mFaceIndexToID(index))]

	QAtomicInt mNewVertexID;

	// Wireframe
	// key - face ID, value - number of wires in the face
	QMap<unsigned int, unsigned int> mFaceIDtoNumOfWires;		// [mNumOfFaces]
	// index - subsequent integer, value - array of pairs
	unsigned int** mFaceWires;									// [mNumOfFaces][2*mFaceIDtoNumOfWires(mFaceIndexToID(index))]

// This data is set from outside and define which color field to use

	bool mTriangleColorFieldActive;
	signed int mCurVertexColorField;
	signed int mCurTriangleColorField;

//public:

	// This data is computed from all the data above and used to be sent to GPU
	cls_OvchDisplayModel* mDisplayModel;

};
