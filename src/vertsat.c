/*
	vertsat.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of sLut interpolation in Koliba. It needs to be
	linked dynamically using the -lkoliba switch in Unix
	and its derivatives, or koliba.lib in Windows.
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
	KOLIBA_SLUT		sLut;
	KOLIBA_EFFILUT	eLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT	fLut;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
} natcon_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

static KOLIBA_SLUT gLut;	// We will fill it with grays.

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Vertex Saturation";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 7;
	info->explanation		= "Control the saturation of individual sLut vertices.";
}

int f0r_init() {
	// This is only needed once regardless of how many instances we have.
	KOLIBA_ConvertGrayToSlut(&gLut, NULL);
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(natcon_instance))) != NULL) {

		KOLIBA_SetEfficacies(&instance->eLut, 1.0);
		instance->count				= width * height;
		instance->flags				= ~0;
		instance->srgb				= 1;
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
			info->name			= "Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the red vertex.";
			break;
		case 1:
			info->name			= "Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the green vertex.";
			break;
		case 2:
			info->name			= "Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the blue vertex.";
			break;
		case 3:
			info->name			= "Cyan";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the cyan vertex.";
			break;
		case 4:
			info->name			= "Magenta";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the magenta vertex.";
			break;
		case 5:
			info->name			= "Yellow";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Natural contrast of the yellow vertex.";
			break;
		case 6:
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
			if (instance->eLut.red		!= *(double *)param) {
				instance->eLut.red		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->eLut.green	!= *(double *)param) {
				instance->eLut.green	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->eLut.blue		!= *(double *)param) {
				instance->eLut.blue		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->eLut.cyan		!= *(double *)param) {
				instance->eLut.cyan		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 4:
			if (instance->eLut.magenta	!= *(double *)param) {
				instance->eLut.magenta	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 5:
			if (instance->eLut.yellow	!= *(double *)param) {
				instance->eLut.yellow	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 6:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->eLut.red;
			break;
		case 1:
			*(double *)param = (double)instance->eLut.green;
			break;
		case 2:
			*(double *)param = (double)instance->eLut.blue;
			break;
		case 3:
			*(double *)param = (double)instance->eLut.cyan;
			break;
		case 4:
			*(double *)param = (double)instance->eLut.magenta;
			break;
		case 5:
			*(double *)param = (double)instance->eLut.yellow;
			break;
		case 6:
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
			KOLIBA_ApplyEfficacies(&instance->sLut, &KOLIBA_IdentitySlut, &instance->eLut, &gLut);
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
