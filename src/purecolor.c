/*
	purecolor.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	how to convert an algorithm into a look-up 	table
	of differing x, y, and z dimensions, and converting
	it to a series of FLUTs as needed on the fly.

	We will use the KOLIBA_PureColor function to compute
	the values for the FLUTs. The function has an optional
	parameter with separate efficacies for each color
	channel. That means we need to recalculate the fLuts
	every time the user picks a different efficacy. Because
	we use the Koliba library function to calculate the fLuts
	on the fly, and the function calculates them when the
	corresponding flag equals zero, we will simply fill the
	falgs array with zeros whenever such a change happens.

	It needs to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>
#include	<string.h>

// We define the x, y, z dimensions here, so we can test
// this with a variety of them.

#define	XDIM	32
#define	YDIM	32
#define	ZDIM	32

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

// We need an array listing our dimensions.
// Since it does not change, we make it both
// static and const.
static const unsigned int dim[3] = {XDIM, YDIM, ZDIM};

typedef	struct _purecol_instance {
	KOLIBA_FLUT		fLut[XDIM*YDIM*ZDIM];
	KOLIBA_RGB		impurities;
	size_t			count;
	KOLIBA_FLAGS	flags[XDIM*YDIM*ZDIM];
	unsigned char	changed;
	unsigned char	srgb;
} purecol_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Pure Color";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 4;
	info->explanation		= "The pure-color effect.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(purecol_instance),1)) != NULL) {
		instance->count			= (size_t)width * (size_t)height;
		instance->srgb			= 1;

		// We do not need to set the value of "changed" because it is 0,
		// which is what calloc() set everything to. Same for the flags.
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Red Impurity";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adds red impurity.";
			break;
		case 1:
			info->name			= "Green Impurity";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adds green impurity.";
			break;
		case 2:
			info->name			= "Blue Impurity";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adds blue impurity.";
			break;
		case 3:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
				instance->impurities.r		 = *(double *)param;
				instance->changed			 = 1;
				break;
		case 1:
				instance->impurities.g		 = *(double *)param;
				instance->changed			 = 1;
				break;
		case 2:
				instance->impurities.b		 = *(double *)param;
				instance->changed			 = 1;
				break;
		case 3:
				instance->srgb				 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= (double)instance->impurities.r;
			break;
		case 1:
			*(double *)param				= (double)instance->impurities.g;
			break;
		case 2:
			*(double *)param				= (double)instance->impurities.b;
			break;
		case 3:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	KOLIBA_XYZ xyz;
	unsigned int ind[3];
	int index;

	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed != 0) {
			memset(instance->flags, 0, sizeof(unsigned int)*XDIM*YDIM*ZDIM);
			instance->changed = 0;
		}

		if (instance->srgb) {
			iconv = KOLIBA_SrgbByteToLinear;
			oconv = KOLIBA_LinearByteToSrgb;
		}
		else {
			iconv = NULL;
			oconv = NULL;
		}

		for (i = instance->count; i; i--, inframe++, outframe++) {
			KOLIBA_FlyRgba8Pixel(outframe, inframe, instance->fLut, instance->flags, dim, KOLIBA_PureColor, &instance->impurities, iconv, oconv)->a = inframe->a;
		}
	}
}
