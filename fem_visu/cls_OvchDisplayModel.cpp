#include "cls_OvchDisplayModel.h"

#pragma warning(push, 1)
#include <QByteArray>
#include <QDebug>
#pragma warning(pop)

// ALIGNER = number of floats in one vertex
#define ALIGNER 6

cls_OvchDisplayModel::cls_OvchDisplayModel(void) :
	mNumOfVertices(0),
	mNumOfTriangles(0),
	mNumOfWires(0),
	mVertexAndColorData(NULL),
	mTriangleIndices(NULL),
	mWireIndices(NULL),
	mVandCdataUniqueColors(NULL)
{
}

cls_OvchDisplayModel::~cls_OvchDisplayModel(void)
{
	this->Deconstruct();
}

void cls_OvchDisplayModel::ConstructFromTFdata(float* p_data, unsigned int p_numOfPrimitives)
{
	// Clear all
	this->Deconstruct();

	// Copy new data
	mNumOfVertices = p_numOfPrimitives * 3;
	mVertexAndColorData = new stc_VandC[mNumOfVertices];
	for (unsigned int i=0; i<mNumOfVertices; i++) {
		mVertexAndColorData[i].v[0] = p_data[i*ALIGNER+0];
		mVertexAndColorData[i].v[1] = p_data[i*ALIGNER+1];
		mVertexAndColorData[i].v[2] = p_data[i*ALIGNER+2];
		mVertexAndColorData[i].c[0] = p_data[i*ALIGNER+3];
		mVertexAndColorData[i].c[1] = p_data[i*ALIGNER+4];
		mVertexAndColorData[i].c[2] = p_data[i*ALIGNER+5];
	}

	mNumOfTriangles = p_numOfPrimitives;
	mTriangleIndices = new unsigned int[mNumOfTriangles*3];
	for (unsigned int i=0; i<mNumOfTriangles*3; i++)
		mTriangleIndices[i] = i;
}

void cls_OvchDisplayModel::AppendFromTFdata(float* p_data, unsigned int p_numOfPrimitives)
{
	// Extend the memory for the new data keeping existing data
	stc_VandC* v_newVertexANDcolorData = new stc_VandC[mNumOfVertices + p_numOfPrimitives*3];
	memcpy(v_newVertexANDcolorData, mVertexAndColorData, mNumOfVertices*sizeof(stc_VandC));
	delete [] mVertexAndColorData;
	mVertexAndColorData = v_newVertexANDcolorData;

	// Copy new data
	for (unsigned int i=0; i<p_numOfPrimitives*3; i++) {
		mVertexAndColorData[mNumOfVertices+i].v[0] = p_data[i*ALIGNER+0];
		mVertexAndColorData[mNumOfVertices+i].v[1] = p_data[i*ALIGNER+1];
		mVertexAndColorData[mNumOfVertices+i].v[2] = p_data[i*ALIGNER+2];
		mVertexAndColorData[mNumOfVertices+i].c[0] = p_data[i*ALIGNER+3];
		mVertexAndColorData[mNumOfVertices+i].c[1] = p_data[i*ALIGNER+4];
		mVertexAndColorData[mNumOfVertices+i].c[2] = p_data[i*ALIGNER+5];
	}
	mNumOfVertices += p_numOfPrimitives*3;

	// Extend the memory for the new data keeping existing data
	unsigned int* v_newTriangleIndices = new unsigned int[(mNumOfTriangles + p_numOfPrimitives)*3];
	memcpy(v_newTriangleIndices, mTriangleIndices, mNumOfTriangles*3*sizeof(unsigned int));
	delete [] mTriangleIndices;
	mTriangleIndices = v_newTriangleIndices;

	// Copy new data
	for (unsigned int i=0; i<p_numOfPrimitives*3; i++)
		mTriangleIndices[mNumOfTriangles*3+i] = mNumOfTriangles*3+i;

	mNumOfTriangles += p_numOfPrimitives;
}

void cls_OvchDisplayModel::Deconstruct()
{
	if (mVertexAndColorData) delete [] mVertexAndColorData;
	if (mTriangleIndices) delete [] mTriangleIndices;
	if (mWireIndices) delete [] mWireIndices;
	if (mVandCdataUniqueColors) delete [] mVandCdataUniqueColors;

	mVertexAndColorData = NULL;
	mTriangleIndices = NULL;
	mWireIndices = NULL;
	mVandCdataUniqueColors = NULL;

	mNumOfVertices = 0;
	mNumOfTriangles = 0;
	mNumOfWires = 0;
}

void cls_OvchDisplayModel::LeaveTriangles(QSet<unsigned int> p_listOfTrianglesToLeave)
{
	//TODO check that changes are needed at all!
	unsigned int v_newNumOfTriangles;
	v_newNumOfTriangles = p_listOfTrianglesToLeave.count();


/* This is an obsolete code. Keep it for some time.
	// ----------- Vertices and color -----------

	// Choose which data to save and copy it into temporary container
	stc_VandC* v_newVertexANDcolorData = new stc_VandC[v_newNumOfTriangles*3];
	unsigned int j=0;
	foreach(const unsigned int &value, p_listOfTrianglesToLeave) {
		v_newVertexANDcolorData[j+0] = mVertexAndColorData[value*3+0];
		v_newVertexANDcolorData[j+1] = mVertexAndColorData[value*3+1];
		v_newVertexANDcolorData[j+2] = mVertexAndColorData[value*3+2];
		j += 3;
	}

	// Clear old data and set new data as current
	delete [] mVertexAndColorData;
	mVertexAndColorData = v_newVertexANDcolorData;
	mNumOfVertices = v_newNumOfTriangles*3;

	// ----------- Triangles -----------

	unsigned int* v_newTriangleIndices = new unsigned int[v_newNumOfTriangles*3];
	for (unsigned int i=0; i<v_newNumOfTriangles*3; i++)
		v_newTriangleIndices[i] = i;

	// Clear old data and set new data as current
	delete [] mTriangleIndices;
	mTriangleIndices = v_newTriangleIndices;
	mNumOfTriangles = v_newNumOfTriangles;
*/

	unsigned int* v_newTriangleIndices = new unsigned int[v_newNumOfTriangles*3];

	unsigned int j=0;
	foreach(const unsigned int &value, p_listOfTrianglesToLeave) {
		v_newTriangleIndices[j*3+0] = mTriangleIndices[value*3+0];
		v_newTriangleIndices[j*3+1] = mTriangleIndices[value*3+1];
		v_newTriangleIndices[j*3+2] = mTriangleIndices[value*3+2];
		j++;
	}

	// Clear old data and set new data as current
	delete [] mTriangleIndices;
	mTriangleIndices = v_newTriangleIndices;
	mNumOfTriangles = v_newNumOfTriangles;

}

void cls_OvchDisplayModel::SetVertexAndColorData(stc_VandC* p_VandCdata, unsigned int p_numOfVertices)
{
	if (mVertexAndColorData) delete [] mVertexAndColorData;
	mVertexAndColorData = new stc_VandC[p_numOfVertices];
	memcpy(mVertexAndColorData, p_VandCdata, p_numOfVertices*sizeof(stc_VandC));
	mNumOfVertices = p_numOfVertices;
}

void cls_OvchDisplayModel::SetTriangleIndicesData(unsigned int* p_triangleIndicesData, unsigned int p_numOfTriangles)
{
	if (mTriangleIndices) delete [] mTriangleIndices;
	mTriangleIndices = new unsigned int[p_numOfTriangles*3];
	memcpy(mTriangleIndices, p_triangleIndicesData, p_numOfTriangles*3*sizeof(unsigned int));
	mNumOfTriangles = p_numOfTriangles;
}

void cls_OvchDisplayModel::SetWireIndicesData(unsigned int* p_wireIndicesData, unsigned int p_numOfWires)
{
	if (mWireIndices) delete [] mWireIndices;
	mWireIndices = new unsigned int[p_numOfWires*2];
	memcpy(mWireIndices, p_wireIndicesData, p_numOfWires*2*sizeof(unsigned int));
	mNumOfWires = p_numOfWires;
}

void cls_OvchDisplayModel::SendToGPUvAndC(GLuint p_VAO, GLuint p_VBO, bool p_uniqueColor) const
{
	stc_VandC* v_VandCdataPointer;
	if (p_uniqueColor) v_VandCdataPointer = mVandCdataUniqueColors;
	else v_VandCdataPointer = mVertexAndColorData;

	glBindVertexArray(p_VAO);
	{
		glBindBuffer(GL_ARRAY_BUFFER, p_VBO);
		glBufferData(GL_ARRAY_BUFFER, mNumOfVertices*sizeof(stc_VandC), v_VandCdataPointer, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(stc_VandC), (void*)offsetof(stc_VandC, v));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(stc_VandC), (void*)offsetof(stc_VandC, c));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
	glBindVertexArray(0);
}

void cls_OvchDisplayModel::SendToGPUtriangles(GLuint p_IBO) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumOfTriangles*3*sizeof(unsigned int), mTriangleIndices, GL_STATIC_DRAW);
}

void cls_OvchDisplayModel::SendToGPUwires(GLuint p_IBO) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumOfWires*2*sizeof(unsigned int), mWireIndices, GL_STATIC_DRAW);
}

void cls_OvchDisplayModel::SendToGPUFull(GLuint p_VAO, GLuint p_VBO, GLuint p_IBOtr, GLuint p_IBPwire, bool p_uniqueColor) const
{
	this->SendToGPUvAndC(p_VAO, p_VBO, p_uniqueColor);
	this->SendToGPUtriangles(p_IBOtr);
	this->SendToGPUwires(p_IBPwire);
}

void cls_OvchDisplayModel::DrawTriangles(GLuint p_program, GLuint p_vao, GLuint p_ibo) const
{
	glUseProgram(p_program);
	glBindVertexArray(p_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_ibo);
	glDrawElements(GL_TRIANGLES, mNumOfTriangles*3, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
	glUseProgram(0);
}

void cls_OvchDisplayModel::DrawWires(GLuint p_program, GLuint p_vao, GLuint p_ibo) const
{
	glUseProgram(p_program);
	glBindVertexArray(p_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_ibo);
	glDrawElements(GL_LINES, mNumOfWires*2, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
	glUseProgram(0);
}

// TODO fix - fixed?
void cls_OvchDisplayModel::PrepareUniqueColors()
{
	if (mVandCdataUniqueColors) delete [] mVandCdataUniqueColors;

	mVandCdataUniqueColors = new stc_VandC[mNumOfVertices];
	memcpy(mVandCdataUniqueColors, mVertexAndColorData, mNumOfVertices*sizeof(stc_VandC));

	for (unsigned int i=0; i<mNumOfTriangles; i++) {
		IntToColor(i, &mVandCdataUniqueColors[mTriangleIndices[i*3+0]]);
	}
}

/*
void cls_OvchDisplayModel::ExportGCDGR(QString p_filename) const
{
	QByteArray ba;
	char* char_filename;
	ba = p_filename.toLatin1();
	char_filename = ba.data();

	unsigned int v_desiredFileSize = 0;

	v_desiredFileSize += 3 * sizeof(unsigned int);						// mNumOfVertices, mNumOfTriangles, mNumOfWires
	v_desiredFileSize += mNumOfVertices * sizeof(stc_VandC);			// mVertexAndColorData
	v_desiredFileSize += mNumOfTriangles*3 * sizeof(unsigned int);		// mTriangleIndices
	v_desiredFileSize += mNumOfWires*2 * sizeof(unsigned int);			// mWireIndices
	v_desiredFileSize += sizeof(glm::vec3);								// mBScenter
	v_desiredFileSize += sizeof(float);									// mBSradius

	HANDLE hfile = CreateFile(char_filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE) {
		qDebug() << "Failed to open file.";
		return;
	}

	HANDLE hfilemap = CreateFileMapping(hfile, NULL, PAGE_READWRITE, 0, v_desiredFileSize, NULL);

	if (hfilemap == NULL) {
		qDebug() << "Failed to open file map.";
		CloseHandle(hfile);
		return;
	}

	PBYTE pbFile = (PBYTE) MapViewOfFile(hfilemap, FILE_MAP_WRITE, 0, 0, v_desiredFileSize);

	unsigned int v_shift = 0;

	memcpy(pbFile+v_shift, &mNumOfVertices, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, &mNumOfTriangles, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, &mNumOfWires, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);

	memcpy(pbFile+v_shift, mVertexAndColorData, mNumOfVertices * sizeof(stc_VandC));
	v_shift += mNumOfVertices * sizeof(stc_VandC);
	memcpy(pbFile+v_shift, mTriangleIndices, mNumOfTriangles*3 * sizeof(unsigned int));
	v_shift += mNumOfTriangles*3 * sizeof(unsigned int);
	memcpy(pbFile+v_shift, mWireIndices, mNumOfWires*2 * sizeof(unsigned int));
	v_shift += mNumOfWires*2 * sizeof(unsigned int);

	memcpy(pbFile+v_shift, &mBScenter, sizeof(glm::vec3));
	v_shift += sizeof(glm::vec3);
	memcpy(pbFile+v_shift, &mBSradius, sizeof(float));
	//v_shift += sizeof(float);

	CloseHandle(hfilemap); 
	CloseHandle(hfile);
}
*/

/*
unsigned int cls_OvchDisplayModel::ImportGCDGR(QString p_filename)
{
	if (!p_filename.endsWith(".gcdgr")) {
		qDebug() << "Wrong file type. GCDGR expected.";
		return 1;		// fail
	}

	QByteArray ba;
	char* char_filename;
	ba = p_filename.toLatin1();
	char_filename = ba.data();
*/
//	HANDLE hfile = CreateFile(char_filename, GENERIC_READ /* | GENERIC_WRITE */, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
/*
	if (hfile == INVALID_HANDLE_VALUE) {
		qDebug() << "Failed to open input data file.";
		return 1;		// fail
	}

	HANDLE hfilemap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);

	if (hfilemap != NULL) {

		PBYTE pbFile = (PBYTE) MapViewOfFile(hfilemap, FILE_MAP_READ, 0, 0, 0);

		unsigned int v_shift = 0;

		// Number of vertices, triangles and wires
		memcpy(&mNumOfVertices, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);
		memcpy(&mNumOfTriangles, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);
		memcpy(&mNumOfWires, pbFile+v_shift,sizeof(unsigned int));
		v_shift += sizeof(unsigned int);


		// vertices, colors, triangles and wires
		if (mVertexAndColorData) delete [] mVertexAndColorData;
		mVertexAndColorData = new stc_VandC[mNumOfVertices];
		memcpy(mVertexAndColorData, pbFile+v_shift, mNumOfVertices * sizeof(stc_VandC));
		v_shift += mNumOfVertices * sizeof(stc_VandC);

		if (mTriangleIndices) delete [] mTriangleIndices;
		mTriangleIndices = new unsigned int [mNumOfTriangles*3];
		memcpy(mTriangleIndices, pbFile+v_shift, mNumOfTriangles*3 * sizeof(unsigned int));
		v_shift += mNumOfTriangles*3 * sizeof(unsigned int);

		if (mWireIndices) delete [] mWireIndices;
		mWireIndices = new unsigned int[mNumOfWires*2];
		memcpy(mWireIndices, pbFile+v_shift, mNumOfWires*2 * sizeof(unsigned int));
		v_shift += mNumOfWires*2 * sizeof(unsigned int);


		// Bounding sphere center and radius
		memcpy(&mBScenter, pbFile+v_shift, sizeof(glm::vec3));
		v_shift += sizeof(glm::vec3);
		memcpy(&mBSradius, pbFile+v_shift, sizeof(float));
		v_shift += sizeof(float);

		// Finalize
		CloseHandle(hfilemap); 
		CloseHandle(hfile);

		return 0;		// success
	} else {
		qDebug() << "Failed to open input data file." << endl;
		return 1;		// fail
	}
}
*/

void cls_OvchDisplayModel::InitAABB(void)
{
	mAABB[0] = mVertexAndColorData[0].v[0];		// min X
	mAABB[1] = mVertexAndColorData[0].v[0];		// max X
	mAABB[2] = mVertexAndColorData[0].v[1];		// min Y
	mAABB[3] = mVertexAndColorData[0].v[1];		// max Y
	mAABB[4] = mVertexAndColorData[0].v[2];		// min Z
	mAABB[5] = mVertexAndColorData[0].v[2];		// max Z

	for (unsigned int i=1; i<mNumOfVertices; i++) {
		if (mVertexAndColorData[i].v[0] < mAABB[0]) mAABB[0] = mVertexAndColorData[i].v[0];		// min X
		if (mVertexAndColorData[i].v[0] > mAABB[1]) mAABB[1] = mVertexAndColorData[i].v[0];		// max X
		if (mVertexAndColorData[i].v[1] < mAABB[2]) mAABB[2] = mVertexAndColorData[i].v[1];		// min Y
		if (mVertexAndColorData[i].v[1] > mAABB[3]) mAABB[3] = mVertexAndColorData[i].v[1];		// max Y
		if (mVertexAndColorData[i].v[2] < mAABB[4]) mAABB[4] = mVertexAndColorData[i].v[2];		// min Z
		if (mVertexAndColorData[i].v[2] > mAABB[5]) mAABB[5] = mVertexAndColorData[i].v[2];		// max Z
	}
}
