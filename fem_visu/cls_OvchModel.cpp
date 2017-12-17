#include "cls_OvchModel.h"

#pragma warning(push, 1)
#include <QByteArray>
#include <QDebug>
#include <QList>
//#include "omp.h"
#include <iostream>
//#include <Windows.h>
#pragma warning(pop)

#include "cls_OvchTimer.h"
#include "cls_OvchDisplayModel.h"

//FIXME nasty
#define strtok_s strtok_r

std::pair<unsigned int, unsigned int> flip_pair(const std::pair<unsigned int, unsigned int> &p)
{
	return std::pair<unsigned int, unsigned int>(p.second, p.first);
}

std::multimap<unsigned int, unsigned int> flip_map(const std::map<unsigned int, unsigned int> &src)
{
	std::multimap<unsigned int, unsigned int> dst;
	std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()), flip_pair);
	return dst;
}

cls_OvchModel::cls_OvchModel(void):
	mTriangleColorFieldActive(false),
	mCurVertexColorField(0),
	mCurTriangleColorField(0),

	mVertexCoordinatesPre(NULL),
	mPerVertexColorFields(NULL),
	mPerVertexColorFieldsMin(NULL),
	mPerVertexColorFieldsMax(NULL),

	mElementIndices(NULL),
	mDomainID(NULL),

	mTriangleIndices(NULL),
	mPerTriangleColorFields(NULL),
	mPerTriangleColorFieldsMin(NULL),
	mPerTriangleColorFieldsMax(NULL),
	mElementID(NULL),

	mFacesPre(NULL),

	mVertexCoordinatesPost(NULL),
	mPerVertexColorFieldsPost(NULL),
	mFacesPost(NULL),

	mFaceWires(NULL),

	mDisplayModel(new cls_OvchDisplayModel())
{
}

cls_OvchModel::~cls_OvchModel(void)
{	
}

void cls_OvchModel::FreeMemory(void)
{
	if (mVertexCoordinatesPre) delete [] mVertexCoordinatesPre;
	if (mPerVertexColorFields) delete [] mPerVertexColorFields;
	if (mPerVertexColorFieldsMin) delete [] mPerVertexColorFieldsMin;
	if (mPerVertexColorFieldsMax) delete [] mPerVertexColorFieldsMax;

	if (mElementIndices) delete [] mElementIndices;
	if (mDomainID) delete [] mDomainID;

	if (mTriangleIndices) delete [] mTriangleIndices;
	if (mPerTriangleColorFields) delete [] mPerTriangleColorFields;
	if (mPerTriangleColorFieldsMin) delete [] mPerTriangleColorFieldsMin;
	if (mPerTriangleColorFieldsMax) delete [] mPerTriangleColorFieldsMax;
	if (mElementID) delete [] mElementID;

	for (unsigned int i=0; i<mNumOfFaces; i++) {
		if (mFacesPre[i]) delete [] mFacesPre[i];
	}
	if (mFacesPre) delete [] mFacesPre;

	if (mVertexCoordinatesPost) delete [] mVertexCoordinatesPost;
	if (mPerVertexColorFieldsPost) delete [] mPerVertexColorFieldsPost;
	for (unsigned int i=0; i<mNumOfFaces; i++) {
		if (mFacesPost[i]) delete [] mFacesPost[i];
	}
	if (mFacesPost) delete [] mFacesPost;

	for (unsigned int i=0; i<mNumOfFaces; i++) {
		if (mFaceWires[i]) delete [] mFaceWires[i];
	}
	if (mFaceWires) delete [] mFaceWires;

	if (mDisplayModel) delete mDisplayModel;
}

unsigned int cls_OvchModel::Import(QString p_filename)
{
	cls_OvchTimer v_timer;
	v_timer.Start();

	qDebug().nospace() << "\nReading data from file " << p_filename << "...";

	if (p_filename.endsWith(".aneu")) {
		mFormat = etnANEU;
		if (this->ReadANEU(p_filename) != 0) return 1;
		qDebug() << v_timer.Milestone()/1000.0 << "s";
		this->PostProcess();
	} else if (p_filename.endsWith(".neu")) {
		mFormat = etnNEU;
		if (this->ReadANEU(p_filename) != 0) return 1;
		qDebug() << v_timer.Milestone()/1000.0 << "s";
		this->PostProcess();
	}
	else if (p_filename.endsWith(".mv2")) {
		mFormat = etnMV2;
		if (this->ReadMV2(p_filename) != 0) return 1;
		qDebug() << v_timer.Milestone()/1000.0 << "s";
		this->PostProcess();
	} else if (p_filename.endsWith(".mv")) {
		mFormat = etnMV;
		if (this->ReadMV2(p_filename) != 0) return 1;
		qDebug() << v_timer.Milestone()/1000.0 << "s";
		this->PostProcess();
	}
/*	else if (p_filename.endsWith(".gcdgr")) {
		mFormat = etnGCDGR;
		if (this->ReadGCDGR(p_filename) != 0) return 1;
    } else if (p_filename.endsWith(".gcdgeo")) {
		if (this->ReadGCDGEO(p_filename) != 0) return 1;

		qDebug() << "Computing bounding sphere...";
		this->InitBoundingSphere();
		qDebug() << v_timer.Milestone()/1000.0 << "s";
	}
*/
	else {
		qDebug() << "Unsupported file format.";
		return 1;		// fail
	}

	qDebug() << "\nTotal time:" << v_timer.Stop()/1000.0 << "s";

	return 0;		// success
}

void cls_OvchModel::PrintModelInfo(void)
{
	qDebug() << "-------------------------------------------------------------";
	qDebug() << "\t" << "           vertices:" << mNumOfVerticesPre;
	qDebug() << "\t" << "           elements:" << mNumOfElements;
	qDebug() << "\t" << "          triangles:" << mNumOfTriangles;
	qDebug() << "\t" << "              faces:" << mNumOfFaces;
	qDebug() << "\t" << "  per-vertex fields:" << mNumOfPerVertexColorFields;
	qDebug() << "\t" << "per-triangle fields:" << mNumOfPerTriangleColorFields;
	qDebug() << "-------------------------------------------------------------";
}

void cls_OvchModel::PostProcess(void)
{
	cls_OvchTimer v_timer;
	v_timer.Start();

	qDebug() << "Preparing surface data...";
	this->PrepareSurfaceData();
	qDebug() << v_timer.Milestone()/1000.0 << "s";

	qDebug() << "Allocating memory...";

	// Prepare new list of vertices
	mNumOfVerticesPost = mNumOfVerticesPre;
	mVertexCoordinatesPost = new float[mNumOfVerticesPost*3];
	memcpy(mVertexCoordinatesPost, mVertexCoordinatesPre, mNumOfVerticesPost*3*sizeof(float));
	mPerVertexColorFieldsPost = new float[mNumOfVerticesPost*mNumOfPerVertexColorFields];
	memcpy(mPerVertexColorFieldsPost, mPerVertexColorFields, mNumOfVerticesPost*mNumOfPerVertexColorFields*sizeof(float));

	// Prepare mFacesPost
	mFacesPost = new unsigned int*[mNumOfFaces];
	for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
		unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		unsigned int v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();
		mFacesPost[v_faceIndex] = new unsigned int[v_numOfTriangInFace*4];
		memcpy(mFacesPost[v_faceIndex], mFacesPre[v_faceIndex], v_numOfTriangInFace*4*sizeof(unsigned int));
	}

	qDebug() << v_timer.Milestone()/1000.0 << "s";

	QMap<unsigned int, unsigned int> v_newVertexToOld;

	if (mNumOfFaces > 1) {
		qDebug().nospace() << "Separating faces(" << mNumOfFaces << ")...";
		this->SeparateFaces(v_newVertexToOld);
		this->IncludeNewVertices(v_newVertexToOld);
		v_newVertexToOld.clear();
		qDebug() << v_timer.Milestone()/1000.0 << "s";
	}

	//if (mFormat == etnMV2 || mFormat == etnMV)
	{
		qDebug() << "Building missing vertices...";
		mNewVertexID = mNumOfVerticesPost;		// atomic variable used inside this parallel loop
		//#pragma omp parallel for
		for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
			this->BuildMissingVerticesForFace(v_faceIndex, v_newVertexToOld);
		}
		this->IncludeNewVertices(v_newVertexToOld);
		v_newVertexToOld.clear();
		qDebug() << v_timer.Milestone()/1000.0 << "s";
	}

	if (mFormat != etnMV) {
		qDebug() << "Generating wireframe...";
		this->GenerateWireframe();
		qDebug() << v_timer.Milestone()/1000.0 << "s";
	}

	qDebug() << "Computing bounding sphere...";
	this->InitBoundingSphere();
	qDebug() << v_timer.Milestone()/1000.0 << "s";

	v_timer.Stop();
}

void cls_OvchModel::ReadLineSkipComment(char* o_destination, FILE* p_file)
{
	do {
		fgets(o_destination, 1024, p_file);
	} while (o_destination[0] == '#');
}

unsigned int cls_OvchModel::ReadMV2(QString p_filename)
{
	if (!p_filename.endsWith(".mv") && !p_filename.endsWith(".mv2")) {
		qDebug() << "Wrong file type. MV or MV2 expected.";
		return 1;		// fail
	}

	QByteArray ba;
	char* char_filename;
	ba = p_filename.toLatin1();
	char_filename = ba.data();

	FILE* v_inputDataFile;
	fopen_s(&v_inputDataFile, char_filename, "r");

	if (v_inputDataFile) {

		char v_txtBuf[1024];
		char* v_tok;
		char delimiters[] = " \t";
		char* context = NULL;

		// --------------------------------------------------------------------------------------

		// Read first section header
		ReadLineSkipComment(v_txtBuf, v_inputDataFile);

		v_tok = strtok_s(v_txtBuf, delimiters, &context);
		mNumOfVerticesPre = atoi(v_tok);//		qDebug() << "mNumOfVerticesPre =" << mNumOfVerticesPre;
		v_tok = strtok_s(NULL, delimiters, &context);
		unsigned int v_numOfCoordinates = atoi(v_tok);
		v_tok = strtok_s(NULL, delimiters, &context);
		mNumOfPerVertexColorFields = atoi(v_tok);

		QString* v_perVertexColorFieldNames = new QString[mNumOfPerVertexColorFields];
		for (unsigned int i=0; i<mNumOfPerVertexColorFields; i++) {
			v_tok = strtok_s(NULL, delimiters, &context);
			v_perVertexColorFieldNames[i] = QString(v_tok);
		}

		delete [] v_perVertexColorFieldNames;

		// Allocate memory
		mVertexCoordinatesPre = new float[mNumOfVerticesPre*v_numOfCoordinates];
		mPerVertexColorFields = new float[mNumOfVerticesPre*mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMin = new float[mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMax = new float[mNumOfPerVertexColorFields];

		// Read first section
		for (unsigned int i=0; i<mNumOfVerticesPre; i++) {
			fgets(v_txtBuf, 1024, v_inputDataFile);

			v_tok = strtok_s(v_txtBuf, delimiters, &context);	// Vertex ID - skip it

			for (unsigned int j=0; j<v_numOfCoordinates; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mVertexCoordinatesPre[i*v_numOfCoordinates + j] = (float)atof(v_tok);

				if (i==0) {
					mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// min
					mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// max
				} else {
					if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] < mAABB[j*2+0])
						mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// min
					if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] > mAABB[j*2+1])
						mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// max
				}
			}

			for (unsigned int j=0; j<mNumOfPerVertexColorFields; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mPerVertexColorFields[i*mNumOfPerVertexColorFields + j] = (float)atof(v_tok);

				// Store min and max values of each field
				if (i==0) {
					mPerVertexColorFieldsMin[j] = mPerVertexColorFields[i*mNumOfPerVertexColorFields + j];
					mPerVertexColorFieldsMax[j] = mPerVertexColorFields[i*mNumOfPerVertexColorFields + j];
				} else {
					if (mPerVertexColorFields[i*mNumOfPerVertexColorFields + j] < mPerVertexColorFieldsMin[j])
						mPerVertexColorFieldsMin[j] = mPerVertexColorFields[i*mNumOfPerVertexColorFields + j];
					if (mPerVertexColorFields[i*mNumOfPerVertexColorFields + j] > mPerVertexColorFieldsMax[j])
						mPerVertexColorFieldsMax[j] = mPerVertexColorFields[i*mNumOfPerVertexColorFields + j];
				}
			}
		}

		// --------------------------------------------------------------------------------------
		
		// Read second section header
		ReadLineSkipComment(v_txtBuf, v_inputDataFile);

		v_tok = strtok_s(v_txtBuf, delimiters, &context);
		mNumOfTriangles = atoi(v_tok);//		qDebug() << "mNumOfTriangles =" << mNumOfTriangles;
		v_tok = strtok_s(NULL, delimiters, &context);
		unsigned int v_numOfPerTriangleIndices = atoi(v_tok);

		//TODO - Check that 'BC_id' is present and store its position
		v_tok = strtok_s(NULL, delimiters, &context);
		mNumOfPerTriangleColorFields = atoi(v_tok);
		mBCidPosition = 0;

		QString* v_perTriangleColorFieldNames = new QString[mNumOfPerTriangleColorFields];
		for (unsigned int i=0; i<mNumOfPerTriangleColorFields; i++) {
			v_tok = strtok_s(NULL, delimiters, &context);
			v_perTriangleColorFieldNames[i] = QString(v_tok);
			if (v_perTriangleColorFieldNames[i] == QString("BC_id")) mBCidPosition = i;
		}

		delete [] v_perTriangleColorFieldNames;

		// Allocate memory
		mTriangleIndices = new unsigned int[mNumOfTriangles*v_numOfPerTriangleIndices];
		mPerTriangleColorFields = new float[mNumOfTriangles*mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMin = new float[mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMax = new float[mNumOfPerTriangleColorFields];

		// Read second section
		unsigned int v_faceID;
		QMap<unsigned int, unsigned int>::iterator v_triaglesPerFaceIterator;
		mNumOfFaces = 0;

		for (unsigned int i=0; i<mNumOfTriangles; i++) {
			fgets(v_txtBuf, 1024, v_inputDataFile);

			v_tok = strtok_s(v_txtBuf, delimiters, &context);	// Triangle ID - skip it

			for (unsigned int j=0; j<v_numOfPerTriangleIndices; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mTriangleIndices[i*v_numOfPerTriangleIndices + j] = atoi(v_tok) - 1;
			}

			for (unsigned int j=0; j<mNumOfPerTriangleColorFields; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j] = (float)atof(v_tok);
				if (j == mBCidPosition) {
					if (mFormat	== etnMV) {
						v_faceID = 0;
					} else {
						v_faceID = atoi(v_tok);
					}
					v_triaglesPerFaceIterator = mFaceIDtoNumOfTriangles.find(v_faceID);
					if (v_triaglesPerFaceIterator == mFaceIDtoNumOfTriangles.end()) {
						mFaceIDtoNumOfTriangles.insert(v_faceID, 1);
						mFaceIndexToID.insert(mNumOfFaces, v_faceID);
						mFaceIDtoIndex.insert(v_faceID, mNumOfFaces);
						mNumOfFaces++;
					} else {
						v_triaglesPerFaceIterator.value() += 1;
					}
				}

				// Store min and max values of each field
				if (i==0) {
					mPerTriangleColorFieldsMin[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j];
					mPerTriangleColorFieldsMax[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j];
				} else {
					if (mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j] < mPerTriangleColorFieldsMin[j])
						mPerTriangleColorFieldsMin[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j];
					if (mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j] > mPerTriangleColorFieldsMax[j])
						mPerTriangleColorFieldsMax[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields+j];
				}
			}
		}

		// --------------------------------------------------------------------------------------

		mNumOfElements = 0;
		mElementIndices = NULL;
		mDomainID = NULL;
		mElementID = NULL;

		// --------------------------------------------------------------------------------------

		return 0;		// success
	} else {
		qDebug() << "Failed to open input data file." << endl;
		return 1;		// fail
	}
}

unsigned int cls_OvchModel::ReadANEU(QString p_filename)
{
	if (!p_filename.endsWith(".neu") && !p_filename.endsWith(".aneu")) {
		qDebug() << "Wrong file type. NEU or ANEU expected.";
		return 1;		// fail
	}

	QByteArray ba;
	char* char_filename;
	ba = p_filename.toLatin1();
	char_filename = ba.data();

	FILE* v_inputDataFile;
	fopen_s(&v_inputDataFile, char_filename, "r");

	if (v_inputDataFile) {

		char v_txtBuf[1024];
		char* v_tok;
		char delimiters[] = " \t";
		char* context = NULL;

		// --------------------------------------------------------------------------------------

		// Read first section header
		ReadLineSkipComment(v_txtBuf, v_inputDataFile);
		v_tok = strtok_s(v_txtBuf, delimiters, &context);
		mNumOfVerticesPre = atoi(v_tok);//		qDebug() << "mNumOfVerticesPre =" << mNumOfVerticesPre;

		unsigned int v_numOfCoordinates = 3;
		mNumOfPerVertexColorFields = 1;// 0;

		// Allocate memory
		mVertexCoordinatesPre = new float[mNumOfVerticesPre*v_numOfCoordinates];

		mPerVertexColorFields = new float[mNumOfVerticesPre*mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMin = new float[mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMax = new float[mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMin[0] = 0.0f;
		mPerVertexColorFieldsMax[0] = 1.0f;
		for (unsigned int i=0; i<mNumOfVerticesPre; i++)
			mPerVertexColorFields[i] = (float)rand()/(float)RAND_MAX;
/*
		mPerVertexColorFields = NULL;
		mPerVertexColorFieldsMin = NULL;
		mPerVertexColorFieldsMax = NULL;
*/
		// Read first section
		for (unsigned int i=0; i<mNumOfVerticesPre; i++) {
			fgets(v_txtBuf, 1024, v_inputDataFile);

			unsigned int j = 0;
			v_tok = strtok_s(v_txtBuf, delimiters, &context);
			mVertexCoordinatesPre[i*v_numOfCoordinates + j] = (float)atof(v_tok);

			if (i==0) {
				mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// min
				mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// max
			} else {
				if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] < mAABB[j*2+0])
					mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// min
				if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] > mAABB[j*2+1])
					mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// max
			}

			for (j=1; j<v_numOfCoordinates; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mVertexCoordinatesPre[i*v_numOfCoordinates + j] = (float)atof(v_tok);

				if (i==0) {
					mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// min
					mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];		// max
				} else {
					if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] < mAABB[j*2+0])
						mAABB[j*2+0] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// min
					if (mVertexCoordinatesPre[i*v_numOfCoordinates + j] > mAABB[j*2+1])
						mAABB[j*2+1] = mVertexCoordinatesPre[i*v_numOfCoordinates + j];	// max
				}
			}
		}

		// --------------------------------------------------------------------------------------

		// Read second section header
		ReadLineSkipComment(v_txtBuf, v_inputDataFile);
		v_tok = strtok_s(v_txtBuf, delimiters, &context);
		mNumOfElements = atoi(v_tok);//		qDebug() << "mNumOfElements =" << mNumOfElements;

		// Allocate memory
		mElementIndices = new unsigned int[mNumOfElements * 4];
		mDomainID = new unsigned int[mNumOfElements];

		// Read second section
		for (unsigned int i=0; i<mNumOfElements; i++) {
			fgets(v_txtBuf, 1024, v_inputDataFile);

			v_tok = strtok_s(v_txtBuf, delimiters, &context);
			mDomainID[i] = atoi(v_tok);

			for (unsigned int j=0; j<4; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mElementIndices[i*4+j] = atoi(v_tok) - 1;
			}
		}

		// --------------------------------------------------------------------------------------

		// Read third section header
		ReadLineSkipComment(v_txtBuf, v_inputDataFile);
		v_tok = strtok_s(v_txtBuf, delimiters, &context);
		mNumOfTriangles = atoi(v_tok);//		qDebug() << "mNumOfTriangles =" << mNumOfTriangles;

		unsigned int v_numOfPerTriangleIndices = 3;
		mNumOfPerTriangleColorFields = 1; 
		mBCidPosition = 0;

		// Allocate memory
		mTriangleIndices = new unsigned int[mNumOfTriangles*v_numOfPerTriangleIndices];
		mPerTriangleColorFields = new float[mNumOfTriangles*mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMin = new float[mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMax = new float[mNumOfPerTriangleColorFields];

		if (mFormat == etnANEU) mElementID = new unsigned int[mNumOfTriangles];
		else mElementID = NULL;

		// Read third section
		unsigned int v_faceID;
		QMap<unsigned int, unsigned int>::iterator v_triaglesPerFaceIterator;
		mNumOfFaces = 0;

		for (unsigned int i=0; i<mNumOfTriangles; i++) {
			fgets(v_txtBuf, 1024, v_inputDataFile);

			v_tok = strtok_s(v_txtBuf, delimiters, &context);

			v_faceID = atoi(v_tok);
			mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + mBCidPosition] = (float)v_faceID;		//TODO fix cast

			// Store min and max values of each field
			unsigned int j=0;
			if (i==0) {
				mPerTriangleColorFieldsMin[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j];
				mPerTriangleColorFieldsMax[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j];
			} else {
				if (mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j] < mPerTriangleColorFieldsMin[j])
					mPerTriangleColorFieldsMin[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j];
				if (mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j] > mPerTriangleColorFieldsMax[j])
					mPerTriangleColorFieldsMax[j] = mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + j];
			}

			v_triaglesPerFaceIterator = mFaceIDtoNumOfTriangles.find(v_faceID);
			if (v_triaglesPerFaceIterator == mFaceIDtoNumOfTriangles.end()) {
				mFaceIDtoNumOfTriangles.insert(v_faceID, 1);
				mFaceIndexToID.insert(mNumOfFaces, v_faceID);
				mFaceIDtoIndex.insert(v_faceID, mNumOfFaces);
				mNumOfFaces++;
			} else {
				v_triaglesPerFaceIterator.value() += 1;
			}

			for (unsigned int j=0; j<v_numOfPerTriangleIndices; j++) {
				v_tok = strtok_s(NULL, delimiters, &context);
				mTriangleIndices[i*v_numOfPerTriangleIndices + j] = atoi(v_tok) - 1;
			}

			if (mFormat == etnANEU) {
				v_tok = strtok_s(NULL, delimiters, &context);
				if (v_tok == NULL) mElementID[i] = 0;
				else mElementID[i] = atoi(v_tok) - 1;
			}
		}

		// --------------------------------------------------------------------------------------

		return 0;		// success
	} else {
		qDebug() << "Failed to open input data file." << endl;
		return 1;		// fail
	}
}

/*
unsigned int cls_OvchModel::ReadGCDGR(QString p_filename)
{
	mFormat = etnGCDGR;

	return mDisplayModel->ImportGCDGR(p_filename);
}
*/

/*
unsigned int cls_OvchModel::ReadGCDGEO(QString p_filename)
{
	if (!p_filename.endsWith(".gcdgeo")) {
		qDebug() << "Wrong file type. GCDGEO expected.";
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

		// ----------------------------------------

		memcpy(&mFormat, pbFile+v_shift, sizeof(enu_GeometryFormat));
		v_shift += sizeof(enu_GeometryFormat);

		// First section
		memcpy(&mNumOfVerticesPre, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);
		memcpy(&mNumOfPerVertexColorFields, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);

		mVertexCoordinatesPre = new float[mNumOfVerticesPre*3];
		mPerVertexColorFields = new float[mNumOfVerticesPre*mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMin = new float[mNumOfPerVertexColorFields];
		mPerVertexColorFieldsMax = new float[mNumOfPerVertexColorFields];

		memcpy(mVertexCoordinatesPre, pbFile+v_shift, mNumOfVerticesPre*3 * sizeof(float));
		v_shift += mNumOfVerticesPre*3 * sizeof(float);
		memcpy(mPerVertexColorFields, pbFile+v_shift, mNumOfVerticesPre*mNumOfPerVertexColorFields * sizeof(float));
		v_shift += mNumOfVerticesPre*mNumOfPerVertexColorFields * sizeof(float);
		memcpy(mPerVertexColorFieldsMin, pbFile+v_shift, mNumOfPerVertexColorFields * sizeof(float));
		v_shift += mNumOfPerVertexColorFields * sizeof(float);
		memcpy(mPerVertexColorFieldsMax, pbFile+v_shift, mNumOfPerVertexColorFields * sizeof(float));
		v_shift += mNumOfPerVertexColorFields * sizeof(float);


		// Second section
		memcpy(&mNumOfElements, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);

		mElementIndices = new unsigned int[mNumOfElements*4];
		mDomainID = new unsigned int[mNumOfElements];

		memcpy(mElementIndices, pbFile+v_shift, mNumOfElements*4 * sizeof(unsigned int));
		v_shift += mNumOfElements*4 * sizeof(unsigned int);
		memcpy(mDomainID, pbFile+v_shift, mNumOfElements * sizeof(unsigned int));
		v_shift += mNumOfElements * sizeof(unsigned int);

		// Third section
		memcpy(&mNumOfTriangles, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);
		memcpy(&mNumOfPerTriangleColorFields, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);
		memcpy(&mBCidPosition, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);

		mTriangleIndices = new unsigned int[mNumOfTriangles*3];
		mPerTriangleColorFields = new float[mNumOfTriangles*mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMin = new float[mNumOfPerTriangleColorFields];
		mPerTriangleColorFieldsMax = new float[mNumOfPerTriangleColorFields];
		if (mFormat == etnANEU) mElementID = new unsigned int[mNumOfTriangles];

		memcpy(mTriangleIndices, pbFile+v_shift, mNumOfTriangles*3 * sizeof(unsigned int));
		v_shift += mNumOfTriangles*3 * sizeof(unsigned int);
		memcpy(mPerTriangleColorFields, pbFile+v_shift, mNumOfTriangles*mNumOfPerTriangleColorFields * sizeof(float));
		v_shift += mNumOfTriangles*mNumOfPerTriangleColorFields * sizeof(float);
		memcpy(mPerTriangleColorFieldsMin, pbFile+v_shift, mNumOfPerTriangleColorFields * sizeof(float));
		v_shift += mNumOfPerTriangleColorFields * sizeof(float);
		memcpy(mPerTriangleColorFieldsMax, pbFile+v_shift, mNumOfPerTriangleColorFields * sizeof(float));
		v_shift += mNumOfPerTriangleColorFields * sizeof(float);
		if (mFormat == etnANEU) {
			memcpy(mElementID, pbFile+v_shift, mNumOfTriangles * sizeof(unsigned int));
			v_shift += mNumOfTriangles * sizeof(unsigned int);
		}

		// Axis-aligned bounding box
		memcpy(mAABB, pbFile+v_shift, 6 * sizeof(float));
		v_shift += 6 * sizeof(float);

		// ------------------------------------------------------------------------------------------------------------

		// Bounding sphere
// 		memcpy(&mBScenter, pbFile+v_shift, sizeof(glm::vec3));
// 		v_shift += sizeof(glm::vec3);
// 		memcpy(&mBSradius, pbFile+v_shift, sizeof(float));
// 		v_shift += sizeof(float);

		memcpy(&mNumOfFaces, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);

		unsigned int* v_FaceIndexToID_asArray = new unsigned int[mNumOfFaces * 2];
		unsigned int* v_FaceIDtoIndex_asArray = new unsigned int[mNumOfFaces * 2];
		unsigned int* v_FaceIDtoNumOfTriangles_asArray = new unsigned int[mNumOfFaces * 2];

		memcpy(v_FaceIndexToID_asArray, pbFile+v_shift, mNumOfFaces*2 * sizeof(unsigned int));
		v_shift += mNumOfFaces*2 * sizeof(unsigned int);
		memcpy(v_FaceIDtoIndex_asArray, pbFile+v_shift, mNumOfFaces*2 * sizeof(unsigned int));
		v_shift += mNumOfFaces*2 * sizeof(unsigned int);
		memcpy(v_FaceIDtoNumOfTriangles_asArray, pbFile+v_shift, mNumOfFaces*2 * sizeof(unsigned int));
		v_shift += mNumOfFaces*2 * sizeof(unsigned int);

		for (unsigned int i=0; i<mNumOfFaces; i++) {
			mFaceIndexToID.insert(v_FaceIndexToID_asArray[i*2+0], v_FaceIndexToID_asArray[i*2+1]);
			mFaceIDtoIndex.insert(v_FaceIDtoIndex_asArray[i*2+0], v_FaceIDtoIndex_asArray[i*2+1]);
			mFaceIDtoNumOfTriangles.insert(v_FaceIDtoNumOfTriangles_asArray[i*2+0], v_FaceIDtoNumOfTriangles_asArray[i*2+1]);
		}

		// ------------------------------------------------------------------------------------------------------------

		memcpy(&mNumOfVerticesPost, pbFile+v_shift, sizeof(unsigned int));
		v_shift += sizeof(unsigned int);

		mVertexCoordinatesPost = new float[mNumOfVerticesPost*3];
		mPerVertexColorFieldsPost = new float[mNumOfVerticesPost*mNumOfPerVertexColorFields];

		memcpy(mVertexCoordinatesPost, pbFile+v_shift, mNumOfVerticesPost*3 * sizeof(float));
		v_shift += mNumOfVerticesPost*3 * sizeof(float);
		memcpy(mPerVertexColorFieldsPost, pbFile+v_shift, mNumOfVerticesPost*mNumOfPerVertexColorFields * sizeof(float));
		v_shift += mNumOfVerticesPost*mNumOfPerVertexColorFields * sizeof(float);

		mFacesPost = new unsigned int*[mNumOfFaces];
		for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
			unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
			unsigned int v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();
			mFacesPost[v_faceIndex] = new unsigned int[v_numOfTriangInFace*4];
			memcpy(mFacesPost[v_faceIndex], pbFile+v_shift, v_numOfTriangInFace*4 * sizeof(unsigned int));
			v_shift += v_numOfTriangInFace*4 * sizeof(unsigned int);
		}

		// Wireframe
		if (mFormat != etnMV) {
			unsigned int* v_FaceIDtoNumOfWires_asArray = new unsigned int[mNumOfFaces * 2];
			memcpy(v_FaceIDtoNumOfWires_asArray, pbFile+v_shift, mNumOfFaces*2 * sizeof(unsigned int));
			v_shift += mNumOfFaces*2 * sizeof(unsigned int);
			for (unsigned int i=0; i<mNumOfFaces; i++) {
				mFaceIDtoNumOfWires.insert(v_FaceIDtoNumOfWires_asArray[i*2+0], v_FaceIDtoNumOfWires_asArray[i*2+1]);
			}

			mFaceWires = new unsigned int*[mNumOfFaces];
			for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
				unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
				unsigned int v_numOfWiresInFace = mFaceIDtoNumOfWires.find(v_faceID).value();
				mFaceWires[v_faceIndex] = new unsigned int[v_numOfWiresInFace*2];
				memcpy(mFaceWires[v_faceIndex], pbFile+v_shift, v_numOfWiresInFace*2 * sizeof(unsigned int));
				v_shift += v_numOfWiresInFace*2 * sizeof(unsigned int);
			}
		}

		// ------------------------------------------------------------------------------------------------------------

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

void cls_OvchModel::PrepareSurfaceData(void)
{
	unsigned int v_faceIndex;
	unsigned int v_faceID;
	QMap<unsigned int, unsigned int>::iterator v_curNumOfTriangles;

	mFacesPre = new unsigned int*[mNumOfFaces];
	for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
		v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID);
		mFacesPre[v_faceIndex] = new unsigned int[v_curNumOfTriangles.value()*4];
		v_curNumOfTriangles.value() = 0;	// Reset. Later will be counted again
	}

	unsigned int* v_curFace;
	for (unsigned int i=0; i<mNumOfTriangles; i++) {
		if (mFormat == etnMV) {
			v_faceID = 0;
		} else {
			v_faceID = (unsigned int)mPerTriangleColorFields[i*mNumOfPerTriangleColorFields + mBCidPosition];		//TODO fix cast
		}

		v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID);
		v_faceIndex = mFaceIDtoIndex.find(v_faceID).value();
		v_curFace = mFacesPre[v_faceIndex];

		v_curFace[(v_curNumOfTriangles.value())*4+0] = mTriangleIndices[i*3+0];
		v_curFace[(v_curNumOfTriangles.value())*4+1] = mTriangleIndices[i*3+1];
		v_curFace[(v_curNumOfTriangles.value())*4+2] = mTriangleIndices[i*3+2];
		v_curFace[(v_curNumOfTriangles.value())*4+3] = i;

		v_curNumOfTriangles.value() += 1;	// Counted here
	}
}

void cls_OvchModel::GenerateWireframe(void)
{
    linePair** v_rawWires = new linePair*[mNumOfFaces];
	linePair* v_curRawWires;

	unsigned int v_faceIndex;
	unsigned int v_faceID;
	unsigned int v_numOfTriangInFace;
	unsigned int v_numOfWiresInFace;
	unsigned int* v_curFaceAdress;

	// Allocate memory for the edges
	mFaceWires = new unsigned int*[mNumOfFaces];

	for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++)
	{
		v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();
		v_numOfWiresInFace = v_numOfTriangInFace*3;
		v_curFaceAdress = mFacesPre[v_faceIndex];

		v_rawWires[v_faceIndex] = new linePair[v_numOfWiresInFace];
		v_curRawWires = v_rawWires[v_faceIndex];

		// Fill the raw unprocessed array of wires
		// Make first index less than the second
		for (unsigned int i=0; i<v_numOfTriangInFace; i++) {
			if (v_curFaceAdress[i*4+0] < v_curFaceAdress[i*4+1]) {
				  v_curRawWires[i*3+0].first =  v_curFaceAdress[i*4+0];
				  v_curRawWires[i*3+0].second = v_curFaceAdress[i*4+1];
			} else {
				  v_curRawWires[i*3+0].first =  v_curFaceAdress[i*4+1];
				  v_curRawWires[i*3+0].second = v_curFaceAdress[i*4+0];
			}
			if (v_curFaceAdress[i*4+1] < v_curFaceAdress[i*4+2]) {
				  v_curRawWires[i*3+1].first =  v_curFaceAdress[i*4+1];
				  v_curRawWires[i*3+1].second = v_curFaceAdress[i*4+2];
			} else {
				  v_curRawWires[i*3+1].first =  v_curFaceAdress[i*4+2];
				  v_curRawWires[i*3+1].second = v_curFaceAdress[i*4+1];
			}
			if (v_curFaceAdress[i*4+2] < v_curFaceAdress[i*4+0]) {
				  v_curRawWires[i*3+2].first =  v_curFaceAdress[i*4+2];
				  v_curRawWires[i*3+2].second = v_curFaceAdress[i*4+0];
			} else {
				  v_curRawWires[i*3+2].first =  v_curFaceAdress[i*4+0];
				  v_curRawWires[i*3+2].second = v_curFaceAdress[i*4+2];
			}
		}

		// Sort all the raw wires
		std::sort (v_curRawWires, v_curRawWires+v_numOfWiresInFace, stc_sort_pred());

		// Allocate memory for temporary edges and fill it with raw pairs for further processing
		linePair* v_faceEdgesTmp = new linePair[v_numOfWiresInFace];
		memcpy(v_faceEdgesTmp, v_curRawWires, v_numOfWiresInFace*sizeof(linePair));

		// Remove duplicating pairs (both elements) and count the number of wires left
		unsigned int v_newSize = v_numOfWiresInFace;
		for (unsigned int i=0; i<v_numOfWiresInFace; i++) {
			if (v_faceEdgesTmp[i] == v_faceEdgesTmp[i+1]) {
				v_faceEdgesTmp[i].first = 0;
				v_faceEdgesTmp[i].second = 0;
				v_faceEdgesTmp[i+1].first = 0;
				v_faceEdgesTmp[i+1].second = 0;
				v_newSize -= 2;
			}
		}

		// Store the number of the wires per face in the map
		mFaceIDtoNumOfWires.insert(v_faceID, v_newSize);

		// Allocate memory for edges of the current face
		mFaceWires[v_faceIndex] = new unsigned int[2*v_newSize];

		// Fill the final per-face array with sensible data from the big array with holes
		unsigned int j=0;
		for (unsigned int i=0; i<v_numOfWiresInFace; i++) {
			if (v_faceEdgesTmp[i].first != 0 || v_faceEdgesTmp[i].second != 0) {
				mFaceWires[v_faceIndex][j*2+0] = v_faceEdgesTmp[i].first;
				mFaceWires[v_faceIndex][j*2+1] = v_faceEdgesTmp[i].second;
				j++;
			}
		}

		delete [] v_faceEdgesTmp;
	}

}

void cls_OvchModel::SeparateFaces(QMap<unsigned int, unsigned int>& o_newVertexToOld)
{
	QSet<unsigned int>* v_verticesUsedPerFace = new QSet<unsigned int>[mNumOfFaces];

	unsigned int v_faceIndex1;
	unsigned int v_faceIndex2;
	unsigned int v_faceID;
	unsigned int v_numOfTriangInFace;

	// Prepare v_verticesUsedPerFace
	for (v_faceIndex1=0; v_faceIndex1<mNumOfFaces; v_faceIndex1++) {
		v_faceID = mFaceIndexToID.find(v_faceIndex1).value();
		v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();
		v_verticesUsedPerFace[v_faceIndex1].clear();
		for (unsigned int i=0; i<v_numOfTriangInFace; i++) {
			for (unsigned int v_curIndex=0; v_curIndex<3; v_curIndex++) {
				v_verticesUsedPerFace[v_faceIndex1].insert(mFacesPost[v_faceIndex1][i*4+v_curIndex]);
			}
		}
	}

	QSet<unsigned int> v_commonVertices;
	QSet<unsigned int>::iterator v_commonVerticesIterator;

	unsigned int v_percentageCounter = 0;
	unsigned int v_totalIterations = mNumOfFaces * (mNumOfFaces-1) / 2;

	QMap<unsigned int, unsigned int> v_oldVertexToNew;
	QMap<unsigned int, unsigned int>::iterator v_oldVertexToNewIterator;

	unsigned int v_curVertexID = mNumOfVerticesPost;

	for (v_faceIndex1=0; v_faceIndex1<mNumOfFaces; v_faceIndex1++)
	{
		for (v_faceIndex2=v_faceIndex1+1; v_faceIndex2<mNumOfFaces; v_faceIndex2++)
		{
			// Find intersection of the Ai and Aj - vertices in both faces
			v_commonVertices = v_verticesUsedPerFace[v_faceIndex1] & v_verticesUsedPerFace[v_faceIndex2];
			//qDebug() << "Faces" << v_faceIndex1 << "and" << v_faceIndex2 << ": " << v_commonVertices.count() << "common vertices";

			// Do anything only if there are any common vertices for the current pair of faces
			if (v_commonVertices.count() > 0) {

				// Duplicate the vertices which are on both faces
				for (v_commonVerticesIterator = v_commonVertices.begin();
					v_commonVerticesIterator != v_commonVertices.end(); ++v_commonVerticesIterator) {

					o_newVertexToOld.insert(v_curVertexID, *v_commonVerticesIterator);
					v_oldVertexToNew.insert(*v_commonVerticesIterator, v_curVertexID);
					v_curVertexID++;
				}
				
				// Substitute the old vertex index with the new
				v_faceID = mFaceIndexToID.find(v_faceIndex1).value();
				v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();

				for (unsigned int i=0; i<v_numOfTriangInFace; i++) {
					for (unsigned int j=0; j<3; j++) {
						v_oldVertexToNewIterator = v_oldVertexToNew.find(mFacesPost[v_faceIndex1][i*4+j]);
						if (v_oldVertexToNewIterator != v_oldVertexToNew.end())
							mFacesPost[v_faceIndex1][i*4+j] = v_oldVertexToNewIterator.value();
					}
				}

				v_oldVertexToNew.clear();

				// Update the list of vertices used by the current face
				v_verticesUsedPerFace[v_faceIndex1].clear();
				for (unsigned int i=0; i<v_numOfTriangInFace; i++) {
					for (unsigned int j=0; j<3; j++) {
						v_verticesUsedPerFace[v_faceIndex1].insert(mFacesPost[v_faceIndex1][i*4+j]);
					}
				}

			}

			v_percentageCounter++;
			std::cout << (100*v_percentageCounter/v_totalIterations) << "%\r";
		}
	}

	/*
	// Debug printout
	// ---------------------------------------------------------------------------
	for (v_faceIndex1=0; v_faceIndex1<mNumOfFaces; v_faceIndex1++) {
		v_faceID = mFaceIndexToID.find(v_faceIndex1).value();
		v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();
		v_verticesUsedPerFace[v_faceIndex1].clear();
		for (unsigned int i=0; i<v_numOfTriangInFace; i++) {
			for (unsigned int j=0; j<3; j++)
				v_verticesUsedPerFace[v_faceIndex1].insert(mFacesPost[v_faceIndex1][i*4+j]);
		}
	}

	for (v_faceIndex1=0; v_faceIndex1<mNumOfFaces-1; v_faceIndex1++) {
		for (v_faceIndex2=v_faceIndex1+1; v_faceIndex2<mNumOfFaces; v_faceIndex2++) {
			v_commonVertices = v_verticesUsedPerFace[v_faceIndex1] & v_verticesUsedPerFace[v_faceIndex2];
			qDebug() << "Faces" << v_faceIndex1 << "and" << v_faceIndex2 << ": " << v_commonVertices.count() << "common vertices";
		}
	}
	//*/
	
	delete [] v_verticesUsedPerFace;
}

void cls_OvchModel::IncludeNewVertices(QMap<unsigned int, unsigned int> p_newVertexToOld)
{
	// Skip operation if there are no new vertices in the parameter map
	if (p_newVertexToOld.count() == 0) return;

	// Define new size
	unsigned int v_newNumOfVerticesPost = mNumOfVerticesPost + p_newVertexToOld.count();

	// Allocate container of new size
	float* v_newVertexCoordinatesPost = new float[v_newNumOfVerticesPost*3];
	float* v_newPerVertexColorFieldsPost = new float[v_newNumOfVerticesPost*mNumOfPerVertexColorFields];
	// Copy existing data
	memcpy(v_newVertexCoordinatesPost, mVertexCoordinatesPost, mNumOfVerticesPost*3 * sizeof(float));
	memcpy(v_newPerVertexColorFieldsPost, mPerVertexColorFieldsPost, mNumOfVerticesPost*mNumOfPerVertexColorFields * sizeof(float));

	// Duplicate vertices according to the map p_newVertexToOld
	QMap<unsigned int, unsigned int>::iterator v_newVerticesIterator;
	for (v_newVerticesIterator = p_newVertexToOld.begin(); v_newVerticesIterator != p_newVertexToOld.end(); ++v_newVerticesIterator)
	{
		// key - new vertex index, value - old vertex index

		// coordinates
		v_newVertexCoordinatesPost[v_newVerticesIterator.key()*3 + 0] = v_newVertexCoordinatesPost[v_newVerticesIterator.value()*3 + 0];
		v_newVertexCoordinatesPost[v_newVerticesIterator.key()*3 + 1] = v_newVertexCoordinatesPost[v_newVerticesIterator.value()*3 + 1];
		v_newVertexCoordinatesPost[v_newVerticesIterator.key()*3 + 2] = v_newVertexCoordinatesPost[v_newVerticesIterator.value()*3 + 2];

		// per-vertex color fields
		for (unsigned int v_curColorFieldIndex=0; v_curColorFieldIndex<mNumOfPerVertexColorFields; v_curColorFieldIndex++) {
			v_newPerVertexColorFieldsPost[v_newVerticesIterator.key()*mNumOfPerVertexColorFields+v_curColorFieldIndex] =
				mPerVertexColorFieldsPost[v_newVerticesIterator.value()*mNumOfPerVertexColorFields+v_curColorFieldIndex];
		}
	}

	// Clear old memory
	delete [] mVertexCoordinatesPost;
	delete [] mPerVertexColorFieldsPost;
	// Substitute the old pointer with the new
	mVertexCoordinatesPost = v_newVertexCoordinatesPost;
	mPerVertexColorFieldsPost = v_newPerVertexColorFieldsPost;

	qDebug() << "\tAdded" << v_newNumOfVerticesPost-mNumOfVerticesPost << "new vertices.";

	// Apply new size
	mNumOfVerticesPost = v_newNumOfVerticesPost;
}

void cls_OvchModel::BuildMissingVerticesForFace(unsigned int p_faceIndex, QMap<unsigned int, unsigned int>& o_newVertexToOld)
{
	unsigned int v_faceID = mFaceIndexToID.find(p_faceIndex).value();
	unsigned int v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();

	qDebug().nospace() << "\tFace " << p_faceIndex+1 << " of " << mNumOfFaces << ". " << v_numOfTriangInFace << " triangles.";

	unsigned int* v_curFaceAdress = mFacesPost[p_faceIndex];

	std::map<unsigned int, unsigned int> v_weights;
	std::map<unsigned int, unsigned int>::iterator v_weightsIterator;
	std::map<unsigned int, unsigned int> v_counters;
	std::map<unsigned int, unsigned int>::iterator v_countersIterator;
	QMap<unsigned int, unsigned int*> v_neighbors;

	unsigned int v_curVertexIndex;

	// Compute weights
	for (unsigned int v_triangleIndex=0; v_triangleIndex<v_numOfTriangInFace; v_triangleIndex++) {
		for (unsigned int v_vertexIndex=0; v_vertexIndex<3; v_vertexIndex++) {
			v_curVertexIndex = v_curFaceAdress[v_triangleIndex*4 + v_vertexIndex];
			v_weightsIterator = v_weights.find(v_curVertexIndex);
			if (v_weightsIterator == v_weights.end()) {
				v_weights.insert(std::pair<unsigned int, unsigned int>(v_curVertexIndex, 1));
				v_counters.insert(std::pair<unsigned int, unsigned int>(v_curVertexIndex, 0));
			} else {
				v_weightsIterator->second += 1;
			}
		}
	}

	// Allocate neighbors
	for (v_weightsIterator = v_weights.begin(); v_weightsIterator != v_weights.end(); ++v_weightsIterator) {
		v_neighbors.insert(v_weightsIterator->first, new unsigned int[v_weightsIterator->second]);
	}

	// v_curFaceAdress:
	// [v1, v2, v3, t][v1, v2, v3, t][v1, v2, v3, t]...[v1, v2, v3, t]
	// v1, v2, v3 - vertex index in mVertexCordinatesPost array
	// t - triangle ID in mTriangleIndices array

	unsigned int* v_curNeighborsAdress;
	// Fill neighbors
	for (unsigned int v_triangleIndex=0; v_triangleIndex<v_numOfTriangInFace; v_triangleIndex++) {
		for (unsigned int v_vertexIndex=0; v_vertexIndex<3; v_vertexIndex++) {
			v_curVertexIndex = v_curFaceAdress[v_triangleIndex*4 + v_vertexIndex];
			v_countersIterator = v_counters.find(v_curVertexIndex);
			v_curNeighborsAdress = v_neighbors.find(v_curVertexIndex).value();
			v_curNeighborsAdress[v_countersIterator->second] = v_triangleIndex;
			v_countersIterator->second += 1;
		}
	}

	std::multimap<unsigned int, unsigned int> v_sortedWeights = flip_map(v_weights);
	std::multimap<unsigned int, unsigned int>::iterator v_sortedWeightsIterator;

	// List of the occupied vertex indices
	QList<unsigned int> v_occupiedVertexIndices;

	// List of fixed triangles
	QList<unsigned int> v_fixedTriangles;

	bool v_processedTriangle;
	unsigned int v_tmpValue;
	//////unsigned int v_curVertexID = mNumOfVerticesPost + o_newVertexToOld.count();

	for (v_sortedWeightsIterator = v_sortedWeights.begin();
		v_sortedWeightsIterator != v_sortedWeights.end(); ++v_sortedWeightsIterator) {

		v_curNeighborsAdress  = v_neighbors.find(v_sortedWeightsIterator->second).value();
		// weight = v_sortedWeightsIterator->first
		// vertex = v_sortedWeightsIterator->second
		// neighbors = v_curNeighborsAdress
		// triangle ID = v_curNeighborsAdress[v_triangleIndex]
		for (unsigned int v_neighborID=0; v_neighborID<v_sortedWeightsIterator->first; v_neighborID++) {

			unsigned int v_triangle = v_curNeighborsAdress[v_neighborID];
	
			if (!v_fixedTriangles.contains(v_triangle))
			{
				v_processedTriangle = false;

				for (unsigned int checkedIndex = 0; checkedIndex<3; checkedIndex++) {
					if (!v_occupiedVertexIndices.contains(v_curFaceAdress[v_triangle*4+checkedIndex])) {
						// the checkedIndex is free - use it

						if (checkedIndex != 0) {
							// swap circularly
							if (checkedIndex == 1) {
								v_tmpValue = v_curFaceAdress[v_triangle*4+0];
								v_curFaceAdress[v_triangle*4+0] = v_curFaceAdress[v_triangle*4+1];
								v_curFaceAdress[v_triangle*4+1] = v_curFaceAdress[v_triangle*4+2];
								v_curFaceAdress[v_triangle*4+2] = v_tmpValue;
							} else {
								v_tmpValue = v_curFaceAdress[v_triangle*4+2];
								v_curFaceAdress[v_triangle*4+2] = v_curFaceAdress[v_triangle*4+1];
								v_curFaceAdress[v_triangle*4+1] = v_curFaceAdress[v_triangle*4+0];
								v_curFaceAdress[v_triangle*4+0] = v_tmpValue;
							}
						}

						v_occupiedVertexIndices.append(v_curFaceAdress[v_triangle*4+0]);
						v_processedTriangle = true;
						break;
					}
				}
				if (!v_processedTriangle) {

					int v_curVertexID = mNewVertexID.fetchAndAddRelaxed(1);
					o_newVertexToOld.insert(v_curVertexID, v_curFaceAdress[v_triangle*4+0]);

					// Set the triangle index to the new vertex
					v_curFaceAdress[v_triangle*4+0] = v_curVertexID;

					//////v_curVertexID++;
				}
				v_fixedTriangles.append(v_triangle);
			}
		}
	}
}

void cls_OvchModel::BuildDisplayModelFull(void)
{
	if (mFormat != etnGCDGR) {
		this->BuildDisplayModelVandC();
		this->BuildDisplayModelTriangles();
		this->BuildDisplayModelWires();
	}

	mDisplayModel->InitAABB();
}

void cls_OvchModel::BuildDisplayModelVandC(void)
{
	if (mFormat == etnGCDGR) return;

	unsigned int v_faceIndex;
	unsigned int v_faceID;
	unsigned int v_curNumOfTriangles;
	unsigned int v_curVertex;

	stc_VandC* v_GPUvertexANDcolorData = new stc_VandC[mNumOfVerticesPost];

	for (unsigned int i=0; i<mNumOfVerticesPost; i++) {
		v_GPUvertexANDcolorData[i].v[0] = mVertexCoordinatesPost[i*3+0];
		v_GPUvertexANDcolorData[i].v[1] = mVertexCoordinatesPost[i*3+1];
		v_GPUvertexANDcolorData[i].v[2] = mVertexCoordinatesPost[i*3+2];
	}

	if (mTriangleColorFieldActive) {
		// TRIANGLE

		for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
			v_faceID = mFaceIndexToID.find(v_faceIndex).value();
			v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID).value();
			for (unsigned int v_curTriangle=0; v_curTriangle<v_curNumOfTriangles; v_curTriangle++) {
				for (unsigned int j=0; j<1; j++) {		//TODO !!! avoid setting color for _each_ vertex
					v_curVertex = mFacesPost[v_faceIndex][v_curTriangle*4+j];

					//if (mFormat == etnMV2 || mFormat == etnMV)
					{
						unsigned int v_TriangleIndexInmTriangleIndices = mFacesPost[v_faceIndex][v_curTriangle*4+3];
						ValueToColor(
							mPerTriangleColorFields[v_TriangleIndexInmTriangleIndices * mNumOfPerTriangleColorFields + mCurTriangleColorField],
							mPerTriangleColorFieldsMin[mCurTriangleColorField],
							mPerTriangleColorFieldsMax[mCurTriangleColorField],
							&v_GPUvertexANDcolorData[v_curVertex]);
					}/*
					else {
						this->ValueToColor((float)v_faceIndex, 0.0f, (float)(mNumOfFaces), &mGPUvertexANDcolorData[v_curVertex]);
					}*/
				}
			}
		}
	} else {
		// VERTEX
		for (unsigned int i=0; i<mNumOfVerticesPost; i++) {
			ValueToColor(mPerVertexColorFieldsPost[i*mNumOfPerVertexColorFields + mCurVertexColorField],
				mPerVertexColorFieldsMin[mCurVertexColorField],
				mPerVertexColorFieldsMax[mCurVertexColorField],
				&v_GPUvertexANDcolorData[i]);

		}
	}

	mDisplayModel->SetVertexAndColorData(v_GPUvertexANDcolorData, mNumOfVerticesPost);

	delete [] v_GPUvertexANDcolorData;
}

void cls_OvchModel::BuildDisplayModelTriangles(void)
{
	unsigned int v_faceIndex;
	unsigned int v_faceID;
	unsigned int v_curNumOfTriangles;

	// First zero and then count the number of triangles in all the faces
	unsigned int v_GPUnumOfTriangles = 0;
	for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++)
	{
		v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID).value();
		v_GPUnumOfTriangles += v_curNumOfTriangles;
	}

	// Allocate data for the triangles indices to be sent to GPU
	unsigned int* v_GPUtriangleIndices = new unsigned int[v_GPUnumOfTriangles*3];

	// Copy all the blocks from each face into a common data container
	unsigned int v_counter = 0;
	for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++)
	{
		v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID).value();

		for (unsigned int j=0; j<v_curNumOfTriangles; j++) {
			v_GPUtriangleIndices[v_counter*3 + j*3+0] = mFacesPost[v_faceIndex][j*4+0];
			v_GPUtriangleIndices[v_counter*3 + j*3+1] = mFacesPost[v_faceIndex][j*4+1];
			v_GPUtriangleIndices[v_counter*3 + j*3+2] = mFacesPost[v_faceIndex][j*4+2];
		}

		v_counter += v_curNumOfTriangles;
	}

	mDisplayModel->SetTriangleIndicesData(v_GPUtriangleIndices, v_GPUnumOfTriangles);

	delete [] v_GPUtriangleIndices;
}

void cls_OvchModel::BuildDisplayModelWires(void)
{
	unsigned int v_faceIndex;
	unsigned int v_faceID;

	unsigned int v_GPUnumOfWires = 0;
	unsigned int* v_GPUwireIndices = NULL;

	if (mFormat != etnMV) {
		//TODO get mGPUnumOfWires in a more efficient way
		unsigned int v_curNumOfWires;
		v_GPUnumOfWires = 0;
		for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++)
		{
			v_faceID = mFaceIndexToID.find(v_faceIndex).value();
			v_curNumOfWires = mFaceIDtoNumOfWires.find(v_faceID).value();
			v_GPUnumOfWires += v_curNumOfWires;
		}

		// Allocate data for the wires' indices to be sent to GPU
		v_GPUwireIndices = new unsigned int[v_GPUnumOfWires*2];

		unsigned int j=0;
		for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++)
		{
			v_faceID = mFaceIndexToID.find(v_faceIndex).value();
			v_curNumOfWires = mFaceIDtoNumOfWires.find(v_faceID).value();
			memcpy(v_GPUwireIndices+j, mFaceWires[v_faceIndex], v_curNumOfWires*2 * sizeof(unsigned int));
			j += v_curNumOfWires*2;
		}
	}

	if (v_GPUnumOfWires != 0) {
		mDisplayModel->SetWireIndicesData(v_GPUwireIndices, v_GPUnumOfWires);
	}

	if (v_GPUwireIndices) delete [] v_GPUwireIndices;
}

void cls_OvchModel::PrepareDataForGPU_uniqueColors(void)
{
//TODO implement
/*
	if (mGPUvertexANDcolorData_uniqueColors) delete [] mGPUvertexANDcolorData_uniqueColors;
	mGPUvertexANDcolorData_uniqueColors = new stc_VandC[mNumOfVerticesPost];

	for (unsigned int i=0; i<mNumOfVerticesPost; i++) {
		mGPUvertexANDcolorData_uniqueColors[i].v[0] = mVertexCoordinatesPost[i*3+0];
		mGPUvertexANDcolorData_uniqueColors[i].v[1] = mVertexCoordinatesPost[i*3+1];
		mGPUvertexANDcolorData_uniqueColors[i].v[2] = mVertexCoordinatesPost[i*3+2];
	}

	unsigned int v_faceIndex;
	unsigned int v_faceID;
	unsigned int v_curNumOfTriangles;
	unsigned int v_curTriangle;
	unsigned int v_curVertex;

	unsigned int v_curUniqueColor = 0;

	for (v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
		v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID).value();

		for (v_curTriangle=0; v_curTriangle<v_curNumOfTriangles; v_curTriangle++) {

			v_curVertex = mFacesPost[v_faceIndex][v_curTriangle*4+0];
			IntToColor(v_curUniqueColor, &mGPUvertexANDcolorData_uniqueColors[v_curVertex]);

			v_curUniqueColor++;
		}
	}
*/
}

void cls_OvchModel::HighlightTriangle(unsigned int /* p_triangleID */)
{
	//TODO implement
/*
	unsigned int v_curVertex;
	unsigned int v_curTriangleID = 0;

	for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
		unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		unsigned int v_curNumOfTriangles = mFaceIDtoNumOfTriangles.find(v_faceID).value();

		for (unsigned int v_curTriangleInFace=0; v_curTriangleInFace<v_curNumOfTriangles; v_curTriangleInFace++) {

			if (v_curTriangleID == p_triangleID) {
				v_curVertex = mFacesPost[v_faceIndex][v_curTriangleInFace*4+0];
				mGPUvertexANDcolorData[v_curVertex].c[0] = 0.0;
				mGPUvertexANDcolorData[v_curVertex].c[1] = 1.0;
				mGPUvertexANDcolorData[v_curVertex].c[2] = 0.0;
			}
			v_curTriangleID++;
		}
	}
*/
}

/*
void cls_OvchModel::ExportGraphics(QString p_filename)
{
	qDebug() << "Exporting graphics-only model...";

	cls_OvchTimer v_timer;
	v_timer.Start();

	mDisplayModel->ExportGCDGR(p_filename);

	qDebug() << v_timer.Stop()/1000.0 << "s";
}
*/
/*
void cls_OvchModel::ExportFull(QString p_filename)
{
	if (mFormat == etnGCDGR) {
		qDebug() << "Impossible to export full model from graphics-only model.";
		return;
	}

	qDebug() << "Exporting full model...";

	cls_OvchTimer v_timer;
	v_timer.Start();

	// Prepare filename
	QByteArray ba;
	char* char_filename;
	ba = p_filename.toLatin1();
	char_filename = ba.data();

	// Translate maps into arrays
	unsigned int* v_FaceIndexToID_asArray = new unsigned int[mNumOfFaces * 2];
	unsigned int* v_FaceIDtoIndex_asArray = new unsigned int[mNumOfFaces * 2];
	unsigned int* v_FaceIDtoNumOfTriangles_asArray = new unsigned int[mNumOfFaces * 2];
	unsigned int* v_FaceIDtoNumOfWires_asArray = new unsigned int[mNumOfFaces * 2];

	unsigned int j;
	QMap<unsigned int, unsigned int>::iterator v_iterator;

	j = 0;
	for (v_iterator = mFaceIndexToID.begin();
		v_iterator != mFaceIndexToID.end(); ++v_iterator) {
		v_FaceIndexToID_asArray[j*2+0] = v_iterator.key();
		v_FaceIndexToID_asArray[j*2+1] = v_iterator.value();
		j++;
	}

	j = 0;
	for (v_iterator = mFaceIDtoIndex.begin();
		v_iterator != mFaceIDtoIndex.end(); ++v_iterator) {
		v_FaceIDtoIndex_asArray[j*2+0] = v_iterator.key();
		v_FaceIDtoIndex_asArray[j*2+1] = v_iterator.value();
		j++;
	}

	j = 0;
	for (v_iterator = mFaceIDtoNumOfTriangles.begin();
		v_iterator != mFaceIDtoNumOfTriangles.end(); ++v_iterator) {
		v_FaceIDtoNumOfTriangles_asArray[j*2+0] = v_iterator.key();
		v_FaceIDtoNumOfTriangles_asArray[j*2+1] = v_iterator.value();
		j++;
	}

	j = 0;
	for (v_iterator = mFaceIDtoNumOfWires.begin();
		v_iterator != mFaceIDtoNumOfWires.end(); ++v_iterator) {
		v_FaceIDtoNumOfWires_asArray[j*2+0] = v_iterator.key();
		v_FaceIDtoNumOfWires_asArray[j*2+1] = v_iterator.value();
		j++;
	}



	// Compute required disk space
	unsigned int v_desiredFileSize = 0;

	v_desiredFileSize += sizeof(enu_GeometryFormat);			// mFormat
	
	// ------------------------------------------------------------------------------------------------------------

	// First section
	v_desiredFileSize += 2 * sizeof(unsigned int);				// mNumOfVerticesPre, mNumOfPerVertexColorFields

	v_desiredFileSize += mNumOfVerticesPre*3 * sizeof(float);							// mVertexCoordinatesPre
	v_desiredFileSize += mNumOfVerticesPre*mNumOfPerVertexColorFields * sizeof(float);	// mPerVertexColorFields
	v_desiredFileSize += mNumOfPerVertexColorFields * sizeof(float);					// mPerVertexColorFieldsMin
	v_desiredFileSize += mNumOfPerVertexColorFields * sizeof(float);					// mPerVertexColorFieldsMax


	// Second section
	v_desiredFileSize += sizeof(unsigned int);							// mNumOfElements
	v_desiredFileSize += mNumOfElements*4 * sizeof(unsigned int);		// mElementIndices
	v_desiredFileSize += mNumOfElements * sizeof(unsigned int);			// mDomainID


	// Third section
	v_desiredFileSize += 3 * sizeof(unsigned int);						// mNumOfTriangles, mNumOfPerTriangleColorFields, mBCidPosition

	v_desiredFileSize += mNumOfTriangles*3 * sizeof(unsigned int);			// mTriangleIndices
	v_desiredFileSize += mNumOfTriangles*mNumOfPerTriangleColorFields * sizeof(float);
	v_desiredFileSize += mNumOfPerTriangleColorFields * sizeof(float);		// mPerTriangleColorFieldsMin
	v_desiredFileSize += mNumOfPerTriangleColorFields * sizeof(float);		// mPerTriangleColorFieldsMax

	if (mFormat == etnANEU) v_desiredFileSize += mNumOfTriangles * sizeof(unsigned int);			// mElementID

	// Axis-aligned bounding box
	v_desiredFileSize += 6 * sizeof(float);									// mAABB

	// ------------------------------------------------------------------------------------------------------------

	// Bounding sphere
// 	v_desiredFileSize += sizeof(glm::vec3);									// mBScenter
// 	v_desiredFileSize += sizeof(float);										// mBSradius

	v_desiredFileSize += sizeof(unsigned int);								// mNumOfFaces
	v_desiredFileSize += 3 * mNumOfFaces*2 * sizeof(unsigned int);			// mFaceIndexToID, mFaceIDtoIndex, mFaceIDtoNumOfTriangles

	// ------------------------------------------------------------------------------------------------------------

	v_desiredFileSize += sizeof(unsigned int);								// mNumOfVerticesPost
	v_desiredFileSize += mNumOfVerticesPost*3 * sizeof(float);				// mVertexCoordinatesPost
	v_desiredFileSize += mNumOfVerticesPost * mNumOfPerVertexColorFields * sizeof(float);	// mPerVertexColorFieldsPost

	for (v_iterator = mFaceIDtoNumOfTriangles.begin();
		v_iterator != mFaceIDtoNumOfTriangles.end(); ++v_iterator) {
		v_desiredFileSize += v_iterator.value()*4 * sizeof(unsigned int);		// mFacesPost
	}

	// Wireframe
	if (mFormat != etnMV) {
		v_desiredFileSize += 2*mNumOfFaces * sizeof(unsigned int);					// mFaceIDtoNumOfWires

		for (v_iterator = mFaceIDtoNumOfWires.begin();
			v_iterator != mFaceIDtoNumOfWires.end(); ++v_iterator) {
				v_desiredFileSize += v_iterator.value()*2 * sizeof(unsigned int);	// mFaceEdges
		}
	}

	// ------------------------------------------------------------------------------------------------------------

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

	// ------------------------------------------------------------------------------------------------------------

	unsigned int v_shift = 0;

	memcpy(pbFile+v_shift, &mFormat, sizeof(enu_GeometryFormat));
	v_shift += sizeof(enu_GeometryFormat);

	// First section
	memcpy(pbFile+v_shift, &mNumOfVerticesPre, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, &mNumOfPerVertexColorFields, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);

	memcpy(pbFile+v_shift, mVertexCoordinatesPre, mNumOfVerticesPre*3 * sizeof(float));
	v_shift += mNumOfVerticesPre*3 * sizeof(float);
	memcpy(pbFile+v_shift, mPerVertexColorFields, mNumOfVerticesPre*mNumOfPerVertexColorFields * sizeof(float));
	v_shift += mNumOfVerticesPre*mNumOfPerVertexColorFields * sizeof(float);
	memcpy(pbFile+v_shift, mPerVertexColorFieldsMin, mNumOfPerVertexColorFields * sizeof(float));
	v_shift += mNumOfPerVertexColorFields * sizeof(float);
	memcpy(pbFile+v_shift, mPerVertexColorFieldsMax, mNumOfPerVertexColorFields * sizeof(float));
	v_shift += mNumOfPerVertexColorFields * sizeof(float);


	// Second section
	memcpy(pbFile+v_shift, &mNumOfElements, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);

	memcpy(pbFile+v_shift, mElementIndices, mNumOfElements*4 * sizeof(unsigned int));
	v_shift += mNumOfElements*4 * sizeof(unsigned int);
	memcpy(pbFile+v_shift, mDomainID, mNumOfElements * sizeof(unsigned int));
	v_shift += mNumOfElements * sizeof(unsigned int);

	// Third section
	memcpy(pbFile+v_shift, &mNumOfTriangles, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, &mNumOfPerTriangleColorFields, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, &mBCidPosition, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);

	memcpy(pbFile+v_shift, mTriangleIndices, mNumOfTriangles*3 * sizeof(unsigned int));
	v_shift += mNumOfTriangles*3 * sizeof(unsigned int);
	memcpy(pbFile+v_shift, mPerTriangleColorFields, mNumOfTriangles*mNumOfPerTriangleColorFields * sizeof(float));
	v_shift += mNumOfTriangles*mNumOfPerTriangleColorFields * sizeof(float);
	memcpy(pbFile+v_shift, mPerTriangleColorFieldsMin, mNumOfPerTriangleColorFields * sizeof(float));
	v_shift += mNumOfPerTriangleColorFields * sizeof(float);
	memcpy(pbFile+v_shift, mPerTriangleColorFieldsMax, mNumOfPerTriangleColorFields * sizeof(float));
	v_shift += mNumOfPerTriangleColorFields * sizeof(float);
	if (mFormat == etnANEU) {
		memcpy(pbFile+v_shift, mElementID, mNumOfTriangles * sizeof(unsigned int));
		v_shift += mNumOfTriangles * sizeof(unsigned int);
	}

	// Axis-aligned bounding box
	memcpy(pbFile+v_shift, mAABB, 6 * sizeof(float));
	v_shift += 6 * sizeof(float);

	// ------------------------------------------------------------------------------------------------------------

	// Bounding sphere
// 	memcpy(pbFile+v_shift, &mBScenter, sizeof(glm::vec3));
// 	v_shift += sizeof(glm::vec3);
// 	memcpy(pbFile+v_shift, &mBSradius, sizeof(float));
// 	v_shift += sizeof(float);

	memcpy(pbFile+v_shift, &mNumOfFaces, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);

	memcpy(pbFile+v_shift, v_FaceIndexToID_asArray, mNumOfFaces*2 * sizeof(unsigned int));
	v_shift += mNumOfFaces*2 * sizeof(unsigned int);
	memcpy(pbFile+v_shift, v_FaceIDtoIndex_asArray, mNumOfFaces*2 * sizeof(unsigned int));
	v_shift += mNumOfFaces*2 * sizeof(unsigned int);
	memcpy(pbFile+v_shift, v_FaceIDtoNumOfTriangles_asArray, mNumOfFaces*2 * sizeof(unsigned int));
	v_shift += mNumOfFaces*2 * sizeof(unsigned int);

	// ------------------------------------------------------------------------------------------------------------

	memcpy(pbFile+v_shift, &mNumOfVerticesPost, sizeof(unsigned int));
	v_shift += sizeof(unsigned int);
	memcpy(pbFile+v_shift, mVertexCoordinatesPost, mNumOfVerticesPost*3 * sizeof(float));
	v_shift += mNumOfVerticesPost*3 * sizeof(float);
	memcpy(pbFile+v_shift, mPerVertexColorFieldsPost, mNumOfVerticesPost*mNumOfPerVertexColorFields * sizeof(float));
	v_shift += mNumOfVerticesPost*mNumOfPerVertexColorFields * sizeof(float);

	for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
		unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
		unsigned int v_numOfTriangInFace = mFaceIDtoNumOfTriangles.find(v_faceID).value();

		memcpy(pbFile+v_shift, mFacesPost[v_faceIndex], v_numOfTriangInFace*4 * sizeof(unsigned int));
		v_shift += v_numOfTriangInFace*4 * sizeof(unsigned int);
	}

	if (mFormat != etnMV) {
		memcpy(pbFile+v_shift, v_FaceIDtoNumOfWires_asArray, mNumOfFaces*2 * sizeof(unsigned int));
		v_shift += mNumOfFaces*2 * sizeof(unsigned int);

		for (unsigned int v_faceIndex=0; v_faceIndex<mNumOfFaces; v_faceIndex++) {
			unsigned int v_faceID = mFaceIndexToID.find(v_faceIndex).value();
			unsigned int v_numOfWiresInFace = mFaceIDtoNumOfWires.find(v_faceID).value();

			memcpy(pbFile+v_shift, mFaceWires[v_faceIndex], v_numOfWiresInFace*2 * sizeof(unsigned int));
			v_shift += v_numOfWiresInFace*2 * sizeof(unsigned int);
		}
	}
	// ------------------------------------------------------------------------------------------------------------

	CloseHandle(hfilemap); 
	CloseHandle(hfile);

	qDebug() << v_timer.Stop()/1000.0 << "s";

}
*/

signed int cls_OvchModel::SetCurVertexColorFieldIndex(signed int p_newColorFieldsIndex)
{
	if (p_newColorFieldsIndex >= 0 && p_newColorFieldsIndex < (signed int)mNumOfPerVertexColorFields) {
		mCurVertexColorField = p_newColorFieldsIndex;
	}
	return mCurVertexColorField;
}

signed int cls_OvchModel::DecrCurVertexColorFieldIndex(void)
{
	if (mCurVertexColorField-1 >= 0 && mCurVertexColorField-1 < (signed int)mNumOfPerVertexColorFields) {
		mCurVertexColorField -= 1;
	}
	return mCurVertexColorField;
}

signed int cls_OvchModel::IncrCurVertexColorFieldIndex(void)
{
	if (mCurVertexColorField+1 >= 0 && mCurVertexColorField+1 < (signed int)mNumOfPerVertexColorFields) {
		mCurVertexColorField += 1;
	}
	return mCurVertexColorField;
}

signed int cls_OvchModel::SetCurTriangleColorFieldIndex(signed int p_newColorFieldsIndex)
{
	if (p_newColorFieldsIndex >= 0 && p_newColorFieldsIndex < (signed int)mNumOfPerTriangleColorFields) {
		mCurTriangleColorField = p_newColorFieldsIndex;
	}
	return mCurTriangleColorField;
}

signed int cls_OvchModel::DecrCurTriangleColorFieldIndex(void)
{
	if (mCurTriangleColorField-1 >= 0 && mCurTriangleColorField-1 < (signed int)mNumOfPerTriangleColorFields) {
		mCurTriangleColorField -= 1;
	}
	return mCurTriangleColorField;
}

signed int cls_OvchModel::IncrCurTriangleColorFieldIndex(void)
{
	if (mCurTriangleColorField+1 >= 0 && mCurTriangleColorField+1 < (signed int)mNumOfPerTriangleColorFields) {
		mCurTriangleColorField += 1;
	}
	return mCurTriangleColorField;
}

// Compute the bounding sphere for the model and
// put the center coordinates into 'coords' and the radius into 'r'
// 'coords' and 'r' have to be allocated beforehand.
// A very rough algorithm of Ritter's bounding sphere. need check
// TODO fix!!!
void cls_OvchModel::InitBoundingSphere()
{
	unsigned int i;
	bool v_finished;

	float v_curR;
	glm::vec3 v_curC;
	glm::vec3 v_p;
	glm::vec3 v_D;

	// Set initial approximation of bounding sphere
	// using the very first point p and the most distant point p2 from it
	v_p = glm::vec3(mVertexCoordinatesPre[0*3+0], mVertexCoordinatesPre[0*3+1], mVertexCoordinatesPre[0*3+2]);
	glm::vec3 v_p2;
	float maxDist=0.0f;
	int maxIndex=0;
	for (i=1; i<mNumOfVerticesPre; i++) {
		v_p2 = glm::vec3(mVertexCoordinatesPre[i*3+0], mVertexCoordinatesPre[i*3+1], mVertexCoordinatesPre[i*3+2]);
		if (glm::distance(v_p, v_p2) > maxDist) {
			maxIndex = i;
			maxDist = glm::distance(v_p, v_p2);
		}
	}
	v_p2 = glm::vec3(mVertexCoordinatesPre[maxIndex*3+0], mVertexCoordinatesPre[maxIndex*3+1], mVertexCoordinatesPre[maxIndex*3+2]);
	v_curC = (v_p+v_p2)/2.0f;
	v_curR = glm::distance(v_curC, v_p);

	unsigned int v_numOfIterationDone=0;

	// Loop
	do {
		v_finished = true;

		// Search for a point p outside current bounding sphere
		for (i=0; i<mNumOfVerticesPre; i++) {
			v_p = glm::vec3(mVertexCoordinatesPre[i*3+0], mVertexCoordinatesPre[i*3+1], mVertexCoordinatesPre[i*3+2]);
			if (glm::distance(v_curC, v_p) > v_curR) {
				v_finished = false;
				break;
			}
		}

		// Set new bounding sphere according to the point found
		if (!v_finished) {
			v_D = v_curC + glm::normalize(v_curC-v_p)*v_curR;
			v_curC = (v_D+v_p)/2.0f;
			v_curR = glm::distance(v_curC, v_p);
		}

		if (v_numOfIterationDone++ > 10000) v_finished = true;
	} while (!v_finished);

	// Return computed values
	mDisplayModel->SetBScenter(v_curC); // mBScenter = v_curC;
	mDisplayModel->SetBSradius(v_curR); // mBSradius = v_curR;
}

void cls_OvchModel::GetBoundingSphere(glm::vec3 &o_center, float &o_radius)
{
	o_center = mDisplayModel->GetBScenter();
	o_radius = mDisplayModel->GetBSradius();
}
