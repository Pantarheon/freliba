/*
	lut.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	how to convert a triple math formula into a look-up
	table of differing x, y, and z dimensions, and
	converting it to a series of FLUTs as needed on the
	fly. This is doing it the hard way, but sometimes
	that is the right way (even if I do not care for
	large LUTs).

	We will use the sample KOLIBA_MakeVertex function to
	compute the values for the FLUTs. The function expects
	us to pass a pointer to a pi/2 variable as its parameter.
	For that, we can use KOLIBA_PiDiv2, which comes with
	the Koliba library.

	It needs to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>
#include	<math.h>

// We define the x, y, z dimensions here, so we can test
// this with a variety of them.

#define	XDIM	8
#define	YDIM	4
#define	ZDIM	2

/* From frei0r.h, which may or may not be on your system */
typedef struct f0r_plugin_info
{
  const char* name;
  const char* author;
  int plugin_type;
  int color_model;
  int frei0r_version;
  int major_version;
  int minor_version;
  int num_params;
  const char* explanation;
} f0r_plugin_info_t;

typedef struct f0r_param_info
{
  const char* name;
  int type;
  const char* explanation;
} f0r_param_info_t;

#define FREI0R_MAJOR_VERSION 1
#define F0R_PLUGIN_TYPE_FILTER 0
#define F0R_COLOR_MODEL_RGBA8888 1
#define	F0R_PARAM_BOOL	0
#define F0R_PARAM_DOUBLE    1
/* End of frei0r.h extract */

// The math converted to our dimensions will give
// us the same result no matter how many times we
// run the conversion. So we only need to do it
// once regardless of how many instances the host
// may request. As such, we can use global tables.

// We need an array listing our dimensions.
static const unsigned int dim[3] = {XDIM, YDIM, ZDIM};
// This table will contain the FLUTs, one
// for each i,j,k combination. That is huge!
static KOLIBA_FLUT fLut[XDIM*YDIM*ZDIM];
// Finally, we need the flags for each FLUT.
// Initially we set them all to zero, which
// will make it possible for us to know whether
// any FLUT has already been calculated.
static KOLIBA_FLAGS flags[XDIM*YDIM*ZDIM] = {0};

typedef	struct _lut_instance {
	size_t			count;
	unsigned char	srgb;
} lut_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba LUT Test";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 1;
	info->explanation		= "Test LUT functionality.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(lut_instance),1)) != NULL) {
		instance->count			= (size_t)width * (size_t)height;
		instance->srgb			= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
				instance->srgb				 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	unsigned int ind[3];
	int index;

	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->srgb) {
			iconv = KOLIBA_SrgbByteToLinear;
			oconv = KOLIBA_LinearByteToSrgb;
		}
		else {
			iconv = NULL;
			oconv = NULL;
		}

		for (i = instance->count; i; i--, inframe++, outframe++) {
			// This will will out the fLut array and the flags array on the go
			// as needed, and apply the effect on the fly.
			KOLIBA_FlyRgba8Pixel(outframe, inframe, fLut, flags, dim, KOLIBA_MakeVertex, &KOLIBA_PiDiv2, iconv, oconv)->a = inframe->a;
		}
	}
}
