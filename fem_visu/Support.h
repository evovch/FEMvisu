#pragma once

#pragma warning(push, 1)
#include "GL/glew.h"
#include <cstdio>
#pragma warning(pop)

struct stc_VandC {
	float v[3];
	float c[3];
};

void ValueToColor(float p_inVal, float p_inValMin, float p_inValMax, stc_VandC* o_destination);

void IntToColor(unsigned int p_inVal, stc_VandC* o_destination);

unsigned int ColorToInt(stc_VandC* p_source);

unsigned int PixelColorToInt(GLubyte* p_source);

int fopen_s(FILE **f, const char *name, const char *mode);
