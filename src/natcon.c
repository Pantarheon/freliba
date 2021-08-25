/*
	natcon.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of mallets in Koliba. It needs to be linked
	dynamically using the -lkoliba switch in Unix and its
	derivatives, or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>

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

typedef	struct _natcon_instance {
	KOLIBA_MALLET	mallet[2];
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT	fLut;
	double			natcon[2];
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	invert;
	unsigned char	srgb;
	unsigned char	changed;
} natcon_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Natural Contrast";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 5;
	info->explanation		= "Control the natural contrast of the svit and the farba.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(natcon_instance))) != NULL) {
		KOLIBA_InitializeMallet(instance->mallet, KOLIBA_SLUTSVIT);
		KOLIBA_InitializeMallet(instance->mallet+1, KOLIBA_SLUTFARBA);

		instance->count				= width * height;
		instance->flags				= ~0;
		instance->natcon[0]			= 0.125;
		instance->natcon[1]			= 0.75;
		instance->srgb				= 1;
		instance->invert			= 0;
		instance->changed			= 1;

		// We only need to initialize the pointers to the vertices once
		// because their addresses within an instance never change.
		KOLIBA_SlutToVertices(&instance->vert, &instance->sLut);
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Natural Svit Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of svit vertices.";
			break;
		case 1:
			info->name			= "Decrease Svit";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Decrease svit contrast if checked, increase if not.";
			break;
		case 2:
			info->name			= "Natural Farba Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of farba vertices.";
			break;
		case 3:
			info->name			= "Decrease Farba";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Decrease farba contrast if checked, increase if not.";
			break;
		case 4:
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
			if (instance->natcon[0]		!= *(double *)param) {
				instance->natcon[0]		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			b							 = (*(double *)param >= 0.5) ? 1 : 0;
			if ((instance->invert & 1)	!= b) {
				instance->invert			^= 1;
				instance->changed			 = 1;
			}
			break;
		case 2:
			if (instance->natcon[1]		!= *(double *)param) {
				instance->natcon[1]		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			b							 = (*(double *)param >= 0.5) ? 2 : 0;
			if ((instance->invert & 2)	!= b) {
				instance->invert		^= 2;
				instance->changed		 = 1;
			}
			break;
		case 4:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->natcon[0];
			break;
		case 1:
			*(double *)param = (double)(instance->invert & 1);
			break;
		case 2:
			*(double *)param = (double)instance->natcon[1];
			break;
		case 3:
			*(double *)param = (double)((instance->invert & 2) >> 1);
			break;
		case 4:
			*(double *)param = (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			instance->mallet[0].natcon = (instance->invert & 1) ? -instance->natcon[0] : instance->natcon[0];
			instance->mallet[1].natcon = (instance->invert & 2) ? -instance->natcon[1] : instance->natcon[1];
			KOLIBA_ConvertMalletsToSlut(&instance->sLut, NULL, instance->mallet, NULL, 2);
			KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vert);
			instance->flags		= KOLIBA_FlutFlags(&instance->fLut);
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
