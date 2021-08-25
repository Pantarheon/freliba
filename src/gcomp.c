/*
	gcomp.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the gray complement effect in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
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
/* End of frei0r.h extract */

typedef	struct _gcomp_instance {
	KOLIBA_FLUT			fLut;
	double				dchannel;
	KOLIBA_FLAGS		flags;
	size_t				count;
	unsigned int		channel;
	unsigned char		srgb;
	unsigned char		changed;
} gcomp_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Gray Complement";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 2;
	info->explanation		= "Use for color channel and its gray complement.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(gcomp_instance))) != NULL) {
		instance->count					= (size_t)width * (size_t)height;

		// We will use the default (Rec. 2020) chroma model.

		instance->dchannel					= 0.0;
		instance->channel					= 0;
		instance->srgb						= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
		instance->changed					= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Channel";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red = 0, Green = 0.5, Blue = 1.";
			break;
		case 1:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double d;
	unsigned int b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->dchannel	!= *(double *)param) {
				instance->dchannel	 = *(double *)param;
				d					 = instance->dchannel * 3.0;
				instance->channel	 = (d <= 1.0) ? 0 : (d <= 2.0) ? 1 : 2;
				instance->changed	 = 1;
			}
			break;
		case 1:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->srgb		!= b) {
				instance->srgb		 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param		= instance->dchannel;
			break;
		case 1:
			*(double *)param		= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		KOLIBA_MATRIX matrix;
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			instance->flags = KOLIBA_FlutFlags(KOLIBA_ConvertMatrixToFlut(&instance->fLut, KOLIBA_GrayComplementMatrix(&matrix, &KOLIBA_Rec2020, instance->channel)));
			instance->changed	= 0;
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
			KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
		}
	}
}
