/*
	solidcolor.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of fLut interpolation in Koliba. It needs to be
	linked dynamically using the -lkoliba switch in Unix
	and its derivatives, or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>
#include	<string.h>

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
#define F0R_PARAM_COLOR     2
/* End of frei0r.h extract */

typedef	struct _filter_instance {
	KOLIBA_FLUT		fLut;
	KOLIBA_CFLT		cFlt;
	size_t			count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
	unsigned char	copy;
} filter_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Solid Color";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 3;
	info->explanation		= "Fill the frame with a color.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(filter_instance))) != NULL) {

		instance->count				= (size_t)width * (size_t)height;
		instance->flags				= KOLIBA_RgbFlutFlags;

		// Start with something rusty.
		instance->cFlt.r			= 1.0/2.0;
		instance->cFlt.g			= 1.0/4.0;
		instance->cFlt.b			= 1.0/8.0;
		instance->cFlt.d			= 1.0;

		instance->srgb				= 0;
		instance->changed			= 1;
		instance->copy				= 0;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Color";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Pick a color, any color.";
			break;
		case 1:
			info->name			= "Color Density";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The density of the color (1.0 for a solid color).";
			break;
		case 2:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Accomodates sRGB model.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	unsigned char b;
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->cFlt, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->cFlt, (KOLIBA_PIXEL *)param, 1);
				instance->changed	 = 1;
			}
			break;
		case 1:
			if (instance->cFlt.d	!= *(double *)param) {
				instance->cFlt.d	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 2:
			instance->srgb			 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->cFlt, 1);
			break;
		case 1:
			*(double *)param = (double)instance->cFlt.d;
			break;
		case 2:
			*(double *)param = (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_ConvertRgbToFlut(&instance->fLut, (KOLIBA_RGB *)&instance->cFlt);
			KOLIBA_FlutEfficacy(&instance->fLut, &instance->fLut, instance->cFlt.d);
			instance->flags		= (instance->cFlt.d == 1.0) ? KOLIBA_RgbFlutFlags : KOLIBA_DefaultFlutFlags;
			instance->copy		= (instance->cFlt.d == 0.0);
			instance->changed	= 0;
		}

		if (instance->copy)
			memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->srgb) {
				iconv = KOLIBA_SrgbByteToLinear;
				oconv = KOLIBA_LinearByteToSrgb;
			}
			else {
				iconv = NULL;
				oconv = NULL;
			}

			for (; i; i--, inframe++, outframe++) {
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
