/*
	colordials.c

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

typedef	struct _color_instance {
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT	fLut;
	KOLIBA_EFFILUT	gains, lifts;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
} color_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

KOLIBA_SLUT gLut = {	// gains
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0},
	{0.0, 0.0, 0.0}
};

KOLIBA_SLUT lLut = {	// lifts
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0}
};

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Color Dials";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 9;
	info->explanation		= "Dial the efficacy of each color vertex separately.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(color_instance))) != NULL) {

		instance->count				= width * height;
		instance->flags				= ~0;
		KOLIBA_SetEfficacies(&instance->gains, 1.0);
		KOLIBA_SetEfficacies(&instance->lifts, 1.0);
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
			info->name			= "Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the vertex vertex.";
			break;
		case 1:
			info->name			= "White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the white vertex.";
			break;
		case 2:
			info->name			= "Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the red vertex.";
			break;
		case 3:
			info->name			= "Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the green vertex.";
			break;
		case 4:
			info->name			= "Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the blue vertex.";
			break;
		case 5:
			info->name			= "Cyan";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the cyan vertex.";
			break;
		case 6:
			info->name			= "Magenta";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the magenta vertex.";
			break;
		case 7:
			info->name			= "Yellow";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts the yellow vertex.";
			break;
		case 8:
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
			if (instance->lifts.black	!= *(double *)param) {
				instance->lifts.black	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->gains.white	!= *(double *)param) {
				instance->gains.white	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->gains.red		!= *(double *)param) {
				instance->gains.red		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->gains.green	!= *(double *)param) {
				instance->gains.green	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 4:
			if (instance->gains.blue	!= *(double *)param) {
				instance->gains.blue	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 5:
			if (instance->lifts.cyan	!= *(double *)param) {
				instance->lifts.cyan	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 6:
			if (instance->lifts.magenta	!= *(double *)param) {
				instance->lifts.magenta	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 7:
			if (instance->lifts.yellow	!= *(double *)param) {
				instance->lifts.yellow	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 8:
			instance->srgb			 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->lifts.black;
			break;
		case 1:
			*(double *)param = (double)instance->gains.white;
			break;
		case 2:
			*(double *)param = (double)instance->gains.red;
			break;
		case 3:
			*(double *)param = (double)instance->gains.green;
			break;
		case 4:
			*(double *)param = (double)instance->gains.blue;
			break;
		case 5:
			*(double *)param = (double)instance->lifts.cyan;
			break;
		case 6:
			*(double *)param = (double)instance->lifts.magenta;
			break;
		case 7:
			*(double *)param = (double)instance->lifts.yellow;
			break;
		case 8:
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
			KOLIBA_ApplyEfficacies(&instance->sLut, &KOLIBA_IdentitySlut, &instance->lifts, &lLut);
			KOLIBA_ApplyEfficacies(&instance->sLut, &instance->sLut, &instance->gains, &gLut);
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
