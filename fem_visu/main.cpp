#pragma warning(push, 1)
#include <QString>
#include "GL/glew.h"
#include "GL/glut.h"
#include "glm/glm.hpp"
#pragma warning(pop)

#include "cls_OvchRenderer.h"
#include "cls_OvchModel.h"
#include "cls_OvchCamera.h"
#include "cls_OvchOffscreenRenderer.h"
#include "cls_OvchModelProcessor.h"
#include "cls_OvchDisplayModel.h"

#define OFFSCREENK 2

// GLUT-related functions. Serve as an interface with the user, to be replaced by anything else (ie. QT)
void Display();
void Keyboard(unsigned char, int, int);
void SpecKeys(int, int, int);
void Mouse(int button, int state, int x, int y);
void Reshape(int w, int h);
void Idle();

// Mouse buttons states
bool glb_isMidPressed;
bool glb_isLeftPressed;
bool glb_isRightPressed;

enum action_type { ACT_NO_ACT, ACT_PAN, ACT_ZOOM, ACT_ROTATE, ACT_TILT, ACT_SECTIONMOVE, ACT_SECTIONROTATE, ACT_SECTIONTILT };
action_type currentAction;
void MotionMove(int x, int y);

// Camera-related
float glb_startxa, glb_startya;		// Panning, Zooming
glm::vec3 glb_startLocalDir;		// Rotating

glm::vec3 glb_startLookPt;			// Panning
float glb_startFrAngle;				// Zooming (perspective)
float glb_startPBS;					// Zooming (parallel), PBS - parallel box size

// Vertex color field or triangle color field
bool glb_triangleFieldActive;

// Section-related
glm::vec3 glb_startSectionCoord;
float glb_sectionK;

// Input data filename
QString glb_filename;

int main(int argc, char** argv)
{
	// Standard part
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	unsigned int v_winW = 600;
	unsigned int v_winH = 600;
	glutInitWindowSize(v_winW, v_winH);
	glutInitWindowPosition(60, 60);
	glutCreateWindow("OvchRenderer_subtests");
	glutDisplayFunc(Display);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecKeys);
	glutMouseFunc(Mouse);
	glutReshapeFunc(Reshape);
	glutIdleFunc(Idle);
	glewInit();

	glb_triangleFieldActive = false;
	cls_OvchModel* v_model1 = new cls_OvchModel;

	cls_OvchRenderer* v_renderer = new cls_OvchRenderer;

    //strcpy_s(glb_filename, 512, argv[1]);

    glb_filename = argv[1];

	if (v_model1->Import(glb_filename) == 0) {

		v_model1->BuildDisplayModelFull();
		
		v_renderer->SetModel(v_model1);
		v_renderer->SetScreenSize(v_winW, v_winH);
		v_renderer->GetOffscreenRenderer()->Resize(OFFSCREENK*v_winW, OFFSCREENK*v_winH);

		glb_sectionK = v_model1->GetDisplayModel()->GetAABB()[5] - v_model1->GetDisplayModel()->GetAABB()[4];
		glb_startSectionCoord = glm::vec3(
			(v_model1->GetDisplayModel()->GetAABB()[1] + v_model1->GetDisplayModel()->GetAABB()[0]) / 2.0f,
			(v_model1->GetDisplayModel()->GetAABB()[3] + v_model1->GetDisplayModel()->GetAABB()[2]) / 2.0f,
			(v_model1->GetDisplayModel()->GetAABB()[5] + v_model1->GetDisplayModel()->GetAABB()[4]) / 2.0f);
		v_renderer->GetModelProcessor()->SetSectionParams(glb_startSectionCoord, glm::vec3(0.0, 0.0, 1.0));

		v_renderer->SendModelToGPU();

		glutMainLoop();

		v_model1->FreeMemory();
		delete v_model1;
	}

	return 0;
}

void Display() 
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	v_renderer->Display();
	glutSwapBuffers();
}

void Keyboard(unsigned char key, int /*x*/, int /*y*/)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	switch (key) {
	case 'q':
	case 27: exit (0); break;
	case 'r': v_renderer->GetCamera()->Reset(); break;

	case 'z':
		glb_triangleFieldActive = !glb_triangleFieldActive;
		v_renderer->GetModel()->SetField(glb_triangleFieldActive);
		v_renderer->GetModel()->BuildDisplayModelVandC();
		v_renderer->UpdateVandCOnGPU();
		break;

	case 'x':
		v_renderer->SwitchSectioning();
		break;
	case 'w':
		v_renderer->SwitchWireframe();
		break;

	case 't':
		v_renderer->GetModelProcessor()->SetSectionNorm(glm::vec3(1.0, 0.0, 0.0));
		glb_startSectionCoord = glm::vec3(
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[1] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[0]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[3] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[2]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[5] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[4]) / 2.0f);
		v_renderer->GetModelProcessor()->SetSectionOrigin(glb_startSectionCoord);
		v_renderer->GetModelProcessor()->SendSectionParamsToGPU();
		break;
	case 'g':
		v_renderer->GetModelProcessor()->SetSectionNorm(glm::vec3(0.0, 1.0, 0.0));
		glb_startSectionCoord = glm::vec3(
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[1] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[0]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[3] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[2]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[5] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[4]) / 2.0f);
		v_renderer->GetModelProcessor()->SetSectionOrigin(glb_startSectionCoord);
		v_renderer->GetModelProcessor()->SendSectionParamsToGPU();
		break;
	case 'b':
		v_renderer->GetModelProcessor()->SetSectionNorm(glm::vec3(0.0, 0.0, 1.0));
		glb_startSectionCoord = glm::vec3(
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[1] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[0]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[3] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[2]) / 2.0f,
			(v_renderer->GetModel()->GetDisplayModel()->GetAABB()[5] + v_renderer->GetModel()->GetDisplayModel()->GetAABB()[4]) / 2.0f);
		v_renderer->GetModelProcessor()->SetSectionOrigin(glb_startSectionCoord);
		v_renderer->GetModelProcessor()->SendSectionParamsToGPU();
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (!glb_triangleFieldActive) v_renderer->GetModel()->SetCurVertexColorFieldIndex(key-'1');
		else v_renderer->GetModel()->SetCurTriangleColorFieldIndex(key-'1');
		v_renderer->GetModel()->BuildDisplayModelVandC();
		v_renderer->UpdateVandCOnGPU();
		break;
	case '0':
		if (!glb_triangleFieldActive) v_renderer->GetModel()->SetCurVertexColorFieldIndex(9);
		else v_renderer->GetModel()->SetCurTriangleColorFieldIndex(9);
		v_renderer->GetModel()->BuildDisplayModelVandC();
		v_renderer->UpdateVandCOnGPU();
		break;

	case '-':
		if (!glb_triangleFieldActive) v_renderer->GetModel()->DecrCurVertexColorFieldIndex();
		else v_renderer->GetModel()->DecrCurTriangleColorFieldIndex();
		v_renderer->GetModel()->BuildDisplayModelVandC();
		v_renderer->UpdateVandCOnGPU();
		break;
	case '+':
		if (!glb_triangleFieldActive) v_renderer->GetModel()->IncrCurVertexColorFieldIndex();
		else v_renderer->GetModel()->IncrCurTriangleColorFieldIndex();
		v_renderer->GetModel()->BuildDisplayModelVandC();
		v_renderer->UpdateVandCOnGPU();
		break;
	default: break;
	}

	glutPostRedisplay();
}

void SpecKeys(int key, int /*x*/, int /*y*/)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();

	QString v_outputFileName;

	switch (key) {
	case GLUT_KEY_F1:
		v_renderer->GetVisMode()->setBit(1);	v_renderer->GetVisMode()->setBit(2);
		break;
	case GLUT_KEY_F2:
		v_renderer->GetVisMode()->setBit(1);	v_renderer->GetVisMode()->clearBit(2);
		break;
	case GLUT_KEY_F3:
		v_renderer->GetVisMode()->clearBit(1);	v_renderer->GetVisMode()->setBit(2);
		break;

/*	case GLUT_KEY_F5:
		v_outputFileName = QString(glb_filename) + ".gcdgr";
		v_renderer->GetModel()->ExportGraphics(v_outputFileName);
		break;
	case GLUT_KEY_F6:
		v_outputFileName = QString(glb_filename) + ".gcdgeo";
		v_renderer->GetModel()->ExportFull(v_outputFileName);
		break;
*/
	case GLUT_KEY_F7:
		v_outputFileName = QString(glb_filename) + ".png";
		v_renderer->RenderPNG(v_outputFileName);
		break;
	case GLUT_KEY_F8:
		v_outputFileName = QString(glb_filename);// + ".ps";
		v_renderer->RenderPostScript(v_outputFileName);
		break;
	}

	glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	cls_OvchCamera* v_camera = v_renderer->GetCamera();

	float v_xa = (float)(x - v_renderer->GetWinW()/2);		// local coordinates (from screen center)
	float v_ya = -(float)(y - v_renderer->GetWinH()/2);

	// Button press
	if (state == GLUT_DOWN) {
		if (button == GLUT_MIDDLE_BUTTON) {
			glb_isMidPressed = true;
		} else if (button == GLUT_LEFT_BUTTON) {
			glb_isLeftPressed = true;

			if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {

				glb_startya = v_ya;
				glb_startFrAngle = v_camera->GetFrAngle();
				glb_startPBS = v_camera->GetParallelBoxSize();

				currentAction = ACT_ZOOM;
				glutMotionFunc(MotionMove);
/*
			} else if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {

				glb_startxa = v_xa; glb_startya = v_ya;
				glb_startLookPt = v_camera->GetLookPt();

				currentAction = ACT_PAN;
				glutMotionFunc(MotionMove);
*/

			} else if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {

				float curR = sqrt(v_xa*v_xa+v_ya*v_ya);
				float sphR = v_renderer->GetSphR();

				// Check: if we are inside the central circle area - rotate, otherwise - tilt
				if (curR > sphR) {
					glb_startLocalDir = glm::normalize(glm::vec3(v_xa, v_ya, 0.0f));
					currentAction = ACT_SECTIONTILT;
					glutMotionFunc(MotionMove);
				} else {
					float za = sqrt(sphR*sphR-v_xa*v_xa-v_ya*v_ya);
					glb_startLocalDir = glm::normalize(glm::vec3(v_xa, v_ya, za));

					currentAction = ACT_SECTIONROTATE;
					glutMotionFunc(MotionMove);
				}

			} else if (glutGetModifiers() == GLUT_ACTIVE_ALT) {

				glb_startya = v_ya;
				glb_startSectionCoord = v_renderer->GetModelProcessor()->GetSectionOrigin();

				currentAction = ACT_SECTIONMOVE;
				glutMotionFunc(MotionMove);

			} else {

				float curR = sqrt(v_xa*v_xa+v_ya*v_ya);
				float sphR = v_renderer->GetSphR();

				// Check: if we are inside the central circle area - rotate, otherwise - tilt
				if (curR > sphR) {
					glb_startLocalDir = glm::normalize(glm::vec3(v_xa, v_ya, 0.0f));
					currentAction = ACT_TILT;
					glutMotionFunc(MotionMove);
				} else {
					float za = sqrt(sphR*sphR-v_xa*v_xa-v_ya*v_ya);
					glb_startLocalDir = glm::normalize(glm::vec3(v_xa, v_ya, za));

					currentAction = ACT_ROTATE;
					glutMotionFunc(MotionMove);
				}

			}

		} else if (button == GLUT_RIGHT_BUTTON) {
			glb_isRightPressed = true;
		}
	}

	// Button release
	else {
		if (button == GLUT_MIDDLE_BUTTON) {
			glb_isMidPressed = false;
		} else if (button == GLUT_LEFT_BUTTON) {
			glb_isLeftPressed = false;

			currentAction = ACT_NO_ACT;
			glutMotionFunc(NULL);

		} else if (button == GLUT_RIGHT_BUTTON) {
			glb_isRightPressed = false;

			v_renderer->GeometryClick(x, y);
		}
	}

	glutPostRedisplay();
}

void Reshape(int w, int h)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	v_renderer->SetScreenSize(w, h);
	v_renderer->GetOffscreenRenderer()->Resize(OFFSCREENK*w, OFFSCREENK*h);
}

void Idle()
{
}

void MotionMove(int x, int y)
{
	cls_OvchRenderer* v_renderer = cls_OvchRenderer::Instance();
	cls_OvchCamera* v_camera = v_renderer->GetCamera();

	// Get local coordinates (from screen center)
	float xa = (float)(x - v_renderer->GetWinW()/2);
	float ya = -(float)(y - v_renderer->GetWinH()/2);

	float v_sphR = v_renderer->GetSphR();
	float v_za = sqrt(v_sphR*v_sphR-xa*xa-ya*ya);
	float v_za2;
    if (std::isnan(v_za)) v_za2 = 0.0f; // used to be _isnan
	else v_za2 = v_za;

	glm::vec3 v_localDir;

	switch (currentAction)
	{
	case ACT_PAN:
		v_camera->Pan(xa, ya, glb_startxa, glb_startya, glb_startLookPt);
		break;
	case ACT_ZOOM:
		v_camera->Zoom(ya, glb_startya, glb_startFrAngle, glb_startPBS); // PBS - parallel box size
		break;
	case ACT_ROTATE:
		v_localDir = glm::normalize(glm::vec3(xa, ya, v_za2));
		if (glb_startLocalDir != v_localDir) v_camera->Rotate(v_localDir, glb_startLocalDir);
		glb_startLocalDir = v_localDir;
		break;
	case ACT_TILT:
		v_localDir = glm::normalize(glm::vec3(xa, ya, 0.0f));
		if (glb_startLocalDir != v_localDir) v_camera->Rotate(v_localDir, glb_startLocalDir);
		glb_startLocalDir = v_localDir;
		break;

	case ACT_SECTIONMOVE:
		v_renderer->GetModelProcessor()->MoveSectionOrigin((ya-glb_startya)/v_renderer->GetWinH()*glb_sectionK, glb_startSectionCoord);
		break;
	case ACT_SECTIONROTATE:
		v_localDir = glm::normalize(glm::vec3(xa, ya, v_za2));
		if (glb_startLocalDir != v_localDir) v_renderer->GetModelProcessor()->RotateSectionNorm(v_localDir, glb_startLocalDir);
		glb_startLocalDir = v_localDir;
		break;
	case ACT_SECTIONTILT:
		v_localDir = glm::normalize(glm::vec3(xa, ya, 0.0f));
		if (glb_startLocalDir != v_localDir)v_renderer->GetModelProcessor()->RotateSectionNorm(v_localDir, glb_startLocalDir);
		glb_startLocalDir = v_localDir;
		break;

    case ACT_NO_ACT:
        break;
	}

	glutPostRedisplay();
}
