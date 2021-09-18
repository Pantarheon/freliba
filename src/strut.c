/*
	strut.c

	Copyright 2021 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the strutted ring in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdint.h>

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
#define	F0R_PARAM_COLOR		2
/* End of frei0r.h extract */

typedef	struct _struttedring_instance {
	KOLIBA_VERTICES vertices;
	KOLIBA_SLUT 	sLut;
	KOLIBA_FLUT		fLut;
	KOLIBA_VERTEX	vertex;
	KOLIBA_FLAGS	flags;
	KOLIBA_Pluts	plut;
	double			ring;
	double			strut;
	double			efficacy;
	size_t			count;
	unsigned char	secondary;
	unsigned char	inverse;
	unsigned char	srgb;
	unsigned char	changed;
} struttedring_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Strutted Ring";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 7;
	info->explanation		= "Strutted Ring.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(struttedring_instance))) != NULL) {
		// We set these values once and keep them that way for the
		// life of this instance.
		KOLIBA_SlutToVertices(&instance->vertices, &instance->sLut);
		instance->count				= (size_t)width * (size_t)height;

		// We set the initial values for these parameters,
		// but allow the user to change them.
		instance->ring			= 0.0;
		instance->secondary		= 0;
		instance->vertex.r		= 1.0;
		instance->vertex.g		= 0.0;
		instance->vertex.b		= 0.0;
		instance->strut			= 0.5;
		instance->efficacy		= 1.0;
		instance->inverse		= 1;
		instance->srgb			= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLuts.
		// We start with it set true because our lut
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
			info->name			= "Color";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "The color modifier.";
			break;
		case 1:
			info->name			= "Ring";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "0 = red/cyan, 0.5 = green/magenta, 1 = blue/yellow.";
			break;
		case 2:
			info->name			= "Secondary";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use a secondary color ring.";
			break;
		case 3:
			info->name			= "Strut";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to strut the ring vertex.";
			break;
		case 4:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Effect efficacy.";
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
	}
}


void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double d;
	unsigned int b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->vertex, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->vertex, (KOLIBA_PIXEL *)param, 1);
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->ring		!= *(double *)param) {
				instance->ring		 = *(double *)param;
				instance->changed 	 = 1;
			}
			break;
		case 2:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->secondary	!= b) {
				instance->secondary	 = b;
				instance->changed	 = 1;
			}
			break;
		case 3:
			if (instance->strut		!= *(double *)param) {
				instance->strut		 = *(double *)param;
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
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->vertex, 1);
			break;
		case 1:
			*(double *)param			 = instance->ring;
			break;
		case 2:
			*(double *)param			 = (double)instance->secondary;
			break;
		case 3:
			*(double *)param			 = instance->strut;
			break;
		case 4:
			*(double *)param			 = instance->efficacy;
			break;
		case 5:
			*(double *)param			 = (double)instance->inverse;
			break;
		case 6:
			*(double *)param			 = (double)instance->srgb;
			break;
	}
}


void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		// Changed or not, if the efficacy is 0, just do a quick copy
		// of all input pixels to corresponding output pixels.
		if (instance->efficacy == 0.0) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->changed) {
				instance->plut = (instance->secondary) ? (instance->ring <= 1.0/3.0) ? KOLIBA_PlutCyan : (instance->ring <= 2.0/3.0) ? KOLIBA_PlutMagenta : KOLIBA_PlutYellow : (instance->ring <= 1.0/3.0) ? KOLIBA_PlutRed : (instance->ring <= 2.0/3.0) ? KOLIBA_PlutGreen : KOLIBA_PlutBlue;
				KOLIBA_SlutEfficacy(&instance->sLut, KOLIBA_ApplyStrutRing(&instance->sLut, &instance->vertex, instance->plut, instance->strut), (instance->inverse) ? -instance->efficacy : instance->efficacy);
				instance->flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vertices));
				KOLIBA_ScaleFlut(&instance->fLut, &instance->fLut, 255.0);
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

			for (; i; i--, inframe++, outframe++) {
				KOLIBA_ScaledRgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}


