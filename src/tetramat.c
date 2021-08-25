/*
	tetramat.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the TetraMatrix use in Koliba.

	It needs to be linked dynamically using the -lkoliba switch
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

typedef	struct _tetramat_instance {
	KOLIBA_FLUT		fLut;
	KOLIBA_CHROMAT 	y;
	KOLIBA_CHROMA	r, g, b;
	size_t			count;
	unsigned char	changed;
	unsigned char	srgb;
	unsigned char	copy;
} tetramat_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Tetramat";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 17;
	info->explanation		= "color overall, and separately by channel.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(tetramat_instance),1)) != NULL) {
		instance->count	= (size_t)width * (size_t)height;

		KOLIBA_ResetChromaticMatrix(&instance->y, &KOLIBA_Rec2020);
		KOLIBA_ResetChroma(&instance->r);
		KOLIBA_ResetChroma(&instance->g);
		KOLIBA_ResetChroma(&instance->b);
		instance->srgb	= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
		instance->copy	= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Overall Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to rotate all channels.";
			break;
		case 1:
			info->name			= "Overall Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The saturation of all channels.";
			break;
		case 2:
			info->name			= "Overall Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The black level of all channels.";
			break;
		case 3:
			info->name			= "Overall White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The white level of all channels.";
			break;
		case 4:
			info->name			= "Red Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red channel angle adjustment.";
			break;
		case 5:
			info->name			= "Red Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red channel saturation adjustment.";
			break;
		case 6:
			info->name			= "Red Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red channel black level adjustment.";
			break;
		case 7:
			info->name			= "Red White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red channel white level adjustment.";
			break;
		case 8:
			info->name			= "Green Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Green channel angle adjustment.";
			break;
		case 9:
			info->name			= "Green Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Green channel saturation adjustment.";
			break;
		case 10:
			info->name			= "Green Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Green channel black level adjustment.";
			break;
		case 11:
			info->name			= "Green White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Green channel white level adjustment.";
			break;
		case 12:
			info->name			= "Blue Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Blue channel angle adjustment.";
			break;
		case 13:
			info->name			= "Blue Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Blue channel saturation adjustment.";
			break;
		case 14:
			info->name			= "Blue Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Blue channel black level adjustment.";
			break;
		case 15:
			info->name			= "Blue White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Blue channel white level adjustment.";
			break;
		case 16:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double a;
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			a = (*(double *)param)*360.0;
			if (instance->y.chroma.angle		!= a) {
				instance->y.chroma.angle		 = a;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->y.chroma.saturation	!= *(double *)param) {
				instance->y.chroma.saturation	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->y.chroma.black		!= *(double *)param) {
				instance->y.chroma.black		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->y.chroma.white		!= *(double *)param) {
				instance->y.chroma.white		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 4:
			a = (*(double *)param)*360.0;
			if (instance->r.angle		!= a) {
				instance->r.angle		 = a;
				instance->changed		 = 1;
			}
			break;
		case 5:
			if (instance->r.saturation	!= *(double *)param) {
				instance->r.saturation	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 6:
			if (instance->r.black		!= *(double *)param) {
				instance->r.black		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 7:
			if (instance->r.white		!= *(double *)param) {
				instance->r.white		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 8:
			a = (*(double *)param)*360.0;
			if (instance->g.angle		!= a) {
				instance->g.angle		 = a;
				instance->changed		 = 1;
			}
			break;
		case 9:
			if (instance->g.saturation	!= *(double *)param) {
				instance->g.saturation	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 10:
			if (instance->g.black		!= *(double *)param) {
				instance->g.black		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 11:
			if (instance->g.white		!= *(double *)param) {
				instance->g.white		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 12:
			a = (*(double *)param)*360.0;
			if (instance->b.angle		!= a) {
				instance->b.angle		 = a;
				instance->changed		 = 1;
			}
			break;
		case 13:
			if (instance->b.saturation	!= *(double *)param) {
				instance->b.saturation	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 14:
			if (instance->b.black		!= *(double *)param) {
				instance->b.black		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 15:
			if (instance->b.white		!= *(double *)param) {
				instance->b.white		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 16:
				instance->srgb				 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= instance->y.chroma.angle/360.0;
			break;
		case 1:
			*(double *)param				= instance->y.chroma.saturation;
			break;
		case 2:
			*(double *)param				= instance->y.chroma.black;
			break;
		case 3:
			*(double *)param				= instance->y.chroma.white;
			break;
		case 4:
			*(double *)param				= instance->r.angle/360.0;
			break;
		case 5:
			*(double *)param				= instance->r.saturation;
			break;
		case 6:
			*(double *)param				= instance->r.black;
			break;
		case 7:
			*(double *)param				= instance->r.white;
			break;
		case 8:
			*(double *)param				= instance->g.angle/360.0;
			break;
		case 9:
			*(double *)param				= instance->g.saturation;
			break;
		case 10:
			*(double *)param				= instance->g.black;
			break;
		case 11:
			*(double *)param				= instance->g.white;
			break;
		case 12:
			*(double *)param				= instance->r.angle/360.0;
			break;
		case 13:
			*(double *)param				= instance->r.saturation;
			break;
		case 14:
			*(double *)param				= instance->r.black;
			break;
		case 15:
			*(double *)param				= instance->r.white;
			break;
		case 16:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_MATRIX mat;

			KOLIBA_TetraMat(&mat, &instance->y, &instance->r, &instance->g, &instance->b);
			instance->copy		= KOLIBA_IsIdentityFlut(KOLIBA_ConvertMatrixToFlut(&instance->fLut, &mat));
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
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, KOLIBA_MatrixFlutFlags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
