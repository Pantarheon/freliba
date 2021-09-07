/*
	colorroller.c

	Copyright 2021 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the ColorRoller effect in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.

	It also demonstrates the use of fast but low quality
	preview, as well as the use of scaled FLUT introduced
	in libkoliba v.0.4.
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

typedef	struct _colorroller_instance {
	KOLIBA_VERTICES 	vertices;
	KOLIBA_SLUT 		sLut;
	KOLIBA_FLUT			fLut;
	KOLIBA_FLAGS		flags;
	KOLIBA_RGBA8PIXEL	palette[256];
	double				imp;
	double				angle;
	double				atmo;
	double				fx;
	double				efficacy;
	size_t				count;
	unsigned char		inverse;
	unsigned char		srgb;
	unsigned char		preview;
	unsigned char		changed;
} colorroller_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Color Roller";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 1;
	info->num_params		= 8;
	info->explanation		= "Create a color-rolled image.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(colorroller_instance))) != NULL) {
		KOLIBA_SlutToVertices(&instance->vertices, &instance->sLut);
		instance->count			= (size_t)width * (size_t)height;

		instance->imp			= 0.5;
		instance->angle			= 0.1;	// Must be * 360
		instance->atmo			= 0.0;	// Must be * 360
		instance->fx			= 0.0;

		// We start with full efficacy.
		instance->efficacy		= 1.0;
		instance->inverse		= 0;
		instance->srgb			= 1;
		instance->preview		= 0;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
		instance->changed		= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Input Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The contrast of the input LUT";
			break;
		case 1:
			info->name			= "Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the color rotation angle.";
			break;
		case 2:
			info->name			= "Atmosphere";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the atmosphere LUT.";
			break;
		case 3:
			info->name			= "Extreme";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how extreme the effect is.";
			break;
		case 4:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the effect.";
			break;
		case 5:
			info->name			= "Inverse";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Inverts the efficacy.";
			break;
		case 6:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
		case 7:
			info->name			= "Quick Preview";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Show quick low quality preview. Turn off before rendering.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double d;
	unsigned int b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->imp		!= *(double *)param) {
				instance->imp		 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			if (instance->angle		!= *(double *)param) {
				instance->angle		 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 2:
			if (instance->atmo		!= *(double *)param) {
				instance->atmo		 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 3:
			if (instance->fx		!= *(double *)param) {
				instance->fx		 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 4:
			if (instance->efficacy	!= *(double *)param) {
				instance->efficacy	 = *(double *)param;
				instance->changed 	 = 1;
			}
			break;
		case 5:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->inverse	!= b) {
				instance->inverse	 = b;
				instance->changed 	 = 1;
			}
			break;
		case 6:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->srgb		!= b) {
				instance->srgb		 = b;
			}
			break;
		case 7:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->preview	!= b) {
				instance->preview	 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param		 = instance->imp;
			break;
		case 1:
			*(double *)param		 = instance->angle;
			break;
		case 2:
			*(double *)param		 = instance->atmo;
			break;
		case 3:
			*(double *)param		 = instance->fx;
			break;
		case 4:
			*(double *)param		 = instance->efficacy;
			break;
		case 5:
			*(double *)param		 = instance->inverse;
			break;
		case 6:
			*(double *)param		 = (double)instance->srgb;
			break;
		case 7:
			*(double *)param		 = (double)instance->preview;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->efficacy == 0.0) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->srgb) {
				iconv = KOLIBA_SrgbByteToLinear;
				oconv = KOLIBA_LinearByteToSrgb;
			}
			else {
				iconv = NULL;
				oconv = NULL;
			}

			if (instance->changed) {
				KOLIBA_ColorRoller(&instance->sLut, instance->imp, instance->angle * 360.0, instance->atmo * 360.0, instance->fx, (instance->inverse) ? -instance->efficacy : instance->efficacy);
				instance->flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vertices));
				KOLIBA_ScaleFlut(&instance->fLut, &instance->fLut, 255.0);
				KOLIBA_SlutToRgba8Palette(instance->palette, &instance->sLut, iconv, oconv);
				instance->changed	= 0;
			}

			if (instance->preview) KOLIBA_PaletteToRgba8Alpha(outframe, inframe, instance->palette, i);
			else for (; i; i--, inframe++, outframe++) {
				KOLIBA_ScaledRgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
