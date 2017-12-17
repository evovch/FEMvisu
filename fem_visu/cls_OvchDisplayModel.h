#pragma once

#pragma warning(push, 1)
#include <QSet>
#include <QString>
#include <QDebug>
#include "glm/glm.hpp"
//#include <Windows.h>
#pragma warning(pop)

#include "Support.h"

class cls_OvchDisplayModel
{
public:

	cls_OvchDisplayModel(void);
	~cls_OvchDisplayModel(void);

	cls_OvchDisplayModel& operator=(cls_OvchDisplayModel& arg) {

		//TODO does it call for a constructor?????
		///////////////////////////////////////
		// keep this code for safety
		this->mVertexAndColorData = NULL;
		this->mTriangleIndices = NULL;
		this->mWireIndices = NULL;
		this->mVandCdataUniqueColors = NULL;
		///////////////////////////////////////

		this->SetVertexAndColorData(arg.mVertexAndColorData, arg.mNumOfVertices);
		this->SetTriangleIndicesData(arg.mTriangleIndices, arg.mNumOfTriangles);
		this->SetWireIndicesData(arg.mWireIndices, arg.mNumOfWires);

		this->mBScenter = arg.mBScenter;
		this->mBSradius = arg.mBSradius;

		return *this;
	}

	// p_data format (TF - transform feedback):
	// ALIGNER = number of floats in one vertex (here 6)
	// vertex:  xyzrgb  \
	// vertex:  xyzrgb   | triangle
	// vertex:  xyzrgb  /
	// vertex:  xyzrgb  \
	// vertex:  xyzrgb   | triangle
	// vertex:  xyzrgb  /
	// ...
	// vertex:  xyzrgb  \
	// vertex:  xyzrgb   | triangle
	// vertex:  xyzrgb  /

	// Construct the display-model from the unordered array of vertices
	void ConstructFromTFdata(float* p_data, unsigned int p_numOfPrimitives);
	// Append the triangles to the existing geometry ob the current display-model
	void AppendFromTFdata(float* p_data, unsigned int p_numOfPrimitives);
	// Free all the memory
	void Deconstruct();

	// Filter current geometry leaving only listed ones
	void LeaveTriangles(QSet<unsigned int> p_listOfTrianglesToLeave);

	// Partial constructors
	// Data is copied from the original array
	void SetVertexAndColorData(stc_VandC* p_VandCdata, unsigned int p_numOfVertices);
	void SetTriangleIndicesData(unsigned int* p_triangleIndicesData, unsigned int p_numOfTriangles);
	void SetWireIndicesData(unsigned int* p_wireIndicesData, unsigned int p_numOfWires);

	// Send the display-model to the GPU using given OpenGL object
	void SendToGPUvAndC(GLuint p_VAO, GLuint p_VBO, bool p_uniqueColor = false) const;
	void SendToGPUtriangles(GLuint p_IBO) const;
	void SendToGPUwires(GLuint p_IBO) const;
	// Simply call SendToGPUvAndC(), SendToGPUtriangles(), SendToGPUwires() one after another
	void SendToGPUFull(GLuint p_VAO, GLuint p_VBO, GLuint p_IBOtr, GLuint p_IBPwire, bool p_uniqueColor = false) const;

	// Evident aliases
	void SendToGPUvAndCNormal(GLuint p_VAO, GLuint p_VBO) const { this->SendToGPUvAndC(p_VAO, p_VBO, false); }
	void SendToGPUvAndCUniqueColors(GLuint p_VAO, GLuint p_VBO) const { this->SendToGPUvAndC(p_VAO, p_VBO, true); }

	void DrawTriangles(GLuint p_program, GLuint p_vao, GLuint p_ibo) const;
	void DrawWires(GLuint p_program, GLuint p_vao, GLuint p_ibo) const;

	void PrepareUniqueColors();

 /*
	void ExportGCDGR(QString p_filename) const;
	unsigned int ImportGCDGR(QString p_filename);
*/

	// Getters
	unsigned int GetNumOfVertices() const { return mNumOfVertices; }
	unsigned int GetNumOfTriangles() const { return mNumOfTriangles; }
	unsigned int GetNumOfWires() const { return mNumOfWires; }
	stc_VandC* GetVandCdata() const { return mVertexAndColorData; }
	unsigned int* GetTriangleIndices() const { return mTriangleIndices; }
	unsigned int* GetWireIndices() const { return mWireIndices; }

	// Getters/setters for the bounding sphere
	glm::vec3 GetBScenter(void) const { return mBScenter; }
	float GetBSradius(void) const { return mBSradius; }
	void SetBScenter(glm::vec3 p_center) { mBScenter = p_center; }
	void SetBSradius(float p_radius) { mBSradius = p_radius; }

	// Axis-aligned bounding box
	void InitAABB(void);
	float* GetAABB(void) { return mAABB; }

private:

	stc_VandC* mVandCdataUniqueColors;	// [mNumOfVertices]

	float mAABB[6];

private:

	unsigned int mNumOfVertices;
	unsigned int mNumOfTriangles;
	unsigned int mNumOfWires;

	stc_VandC* mVertexAndColorData;		// [mNumOfVertices]
	unsigned int* mTriangleIndices;		// [mNumOfTriangles*3]
	unsigned int* mWireIndices;			// [mNumOfWires*2]

	// Bounding sphere
	glm::vec3 mBScenter;
	float mBSradius;

};
