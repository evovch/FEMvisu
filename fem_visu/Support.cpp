#include "Support.h"

#include <assert.h>
#include <cerrno>

// Transform the value of some field into the color
// Blue is minimum, green is 1/2 and red is maximum
void ValueToColor(float p_inVal, float p_inValMin, float p_inValMax, stc_VandC* o_destination)
{
	float v_x = (p_inVal - p_inValMin) / (p_inValMax - p_inValMin);

	if (v_x >= 0.0f && v_x <= 0.5f) {
		o_destination->c[0] = 0.0f;
		o_destination->c[1] = 2.0f * v_x;
		o_destination->c[2] = 1.0f - 2.0f * v_x;
	} else {
		if (v_x <= 1.0f) {
			o_destination->c[0] = -1.0f + 2.0f * v_x;
			o_destination->c[1] = 2.0f - 2.0f * v_x;
			o_destination->c[2] = 0.0f;
		}
	}
}

// Transform unique number of the triangle into unique color
// Color is coded by 3 floats, each [0.0; 1.0]
void IntToColor(unsigned int p_inVal, stc_VandC* o_destination)
{
	unsigned int v_R, v_G, v_B;

	v_R = (p_inVal & 0x000000FF) >> 0;
	v_G = (p_inVal & 0x0000FF00) >> 8;
	v_B = (p_inVal & 0x00FF0000) >> 16;

	o_destination->c[0] = (float)v_R / 255.0f;
	o_destination->c[1] = (float)v_G / 255.0f;
	o_destination->c[2] = (float)v_B / 255.0f;
}

// Transform unique color into unique number of the triangle
// Color is coded by 3 floats, each [0.0; 1.0]
unsigned int ColorToInt(stc_VandC* p_source)
{
	unsigned int v_R, v_G, v_B;

	v_R = (unsigned int)(p_source->c[0] * 255.0f);
	v_G = (unsigned int)(p_source->c[1] * 255.0f);
	v_B = (unsigned int)(p_source->c[2] * 255.0f);

	return v_R + v_G*256 + v_B*256*256;
}

// Transform unique color into unique number of the triangle
// Color is coded by 3 bytes, each [0; 255]
unsigned int PixelColorToInt(unsigned char* p_source)
{
	return p_source[0] + p_source[1]*256 + p_source[2]*256*256;
}

//TODO test under Windows
// Thx https://stackoverflow.com/questions/1513209/is-there-a-way-to-use-fopen-s-with-gcc-or-at-least-create-a-define-about-it
int fopen_s(FILE **f, const char *name, const char *mode) {
	int ret = 0;
	assert(f);
	*f = fopen(name, mode);
	/* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
	if (!*f) {
		ret = errno;
	}
	return ret;
}
