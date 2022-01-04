/*
	crimsonite.c

	Copyright 2021 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the interpolation of LUTs in Koliba. It needs
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
/* End of frei0r.h extract */

typedef	struct _crimsonite_instance {
	KOLIBA_VERTICES vertices;
	KOLIBA_SLUT 	sLut;
	KOLIBA_FLUT		fLut;
	KOLIBA_FLAGS	flags;
	double			lut;
	double			magenta;
	double			efficacy;
	size_t			count;
	unsigned char	inverse;
	unsigned char	srgb;
	unsigned char	changed;
} crimsonite_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

const uint64_t eLut_D08[8][3] = {	/* redmane-eddy.sLut */
	{ 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xBFD4BE8A472F053B, 0xBFCDB537B6DB6DB4, 0x3FF343EB1AF05397 },
	{ 0xBFCDB537B6DB6DB4, 0x3FF343EB1AF05397, 0xBFD4BE8A472F053B },
	{ 0xBFCA1F58D7829CBA, 0x3FF52FA291CBC150, 0x3FF3B6A6F6DB6DB9 },
	{ 0x3FF343EB1AF05397, 0xBFD4BE8A472F0539, 0xBFCDB537B6DB6DB6 },
	{ 0x3FF3B6A6F6DB6DB8, 0xBFCA1F58D7829CBD, 0x3FF52FA291CBC152 },
	{ 0x3FF52FA291CBC14E, 0x3FF3B6A6F6DB6DB6, 0xBFCA1F58D7829CBC },
	{ 0x3FF0000000000000, 0x3FF0000000000000, 0x3FF0000000000000 }
};

const uint64_t eLut_768B[8][3] = {	/* Crimsonite.sLut */
	{ 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xBFD03616BC687D65, 0xBFBE6CCAB6DB6DB7, 0x3FF252FA294E5E0C },
	{ 0xBFBE6CCAB6DB6DB8, 0x3FF252FA294E5E0B, 0xBFD03616BC687D63 },
	{ 0xBFC297D14A72F059, 0x3FF40D85AF1A1F5A, 0x3FF1E6CCAB6DB6DE },
	{ 0x3FF252FA294E5E0C, 0xBFD03616BC687D63, 0xBFBE6CCAB6DB6DB7 },
	{ 0x3FF1E6CCAB6DB6DB, 0xBFC297D14A72F059, 0x3FF40D85AF1A1F58 },
	{ 0x3FF40D85AF1A1F59, 0x3FF1E6CCAB6DB6DB, 0xBFC297D14A72F059 },
	{ 0x3FF0000000000000, 0x3FF0000000000000, 0x3FF0000000000000 }
};

const uint64_t vrt[] = {
	0x3FEB1B1B1FFFFFFE,
	0x3FE4B4B4C0000000,
	0x3FE6363640000000
};

KOLIBA_SLUT *sLut[2] = { (KOLIBA_SLUT *)&eLut_D08, (KOLIBA_SLUT *)&eLut_768B };

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Crimsonite";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 5;
	info->explanation		= "Skin Crimsonite.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(crimsonite_instance))) != NULL) {
		KOLIBA_SlutToVertices(&instance->vertices, &instance->sLut);
		instance->count			= (size_t)width * (size_t)height;

		instance->lut			= 0.5;
		instance->magenta		= 0.5;

		// We start with full efficacy.
		instance->efficacy		= 1.0;
		instance->inverse		= 0;
		instance->srgb			= 1;

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
			info->name			= "Effect Type";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The relative contribution of the input LUTs.";
			break;
		case 1:
			info->name			= "Demag";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Reduce the effect of the magenta color.";
			break;
		case 2:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the effect.";
			break;
		case 3:
			info->name			= "Inverse";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Inverts the efficacy.";
			break;
		case 4:
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
			if (instance->lut		!= *(double *)param) {
				instance->lut		 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			if (instance->magenta	!= *(double *)param) {
				instance->magenta	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 2:
			if (instance->efficacy	!= *(double *)param) {
				instance->efficacy	 = *(double *)param;
				instance->changed 	 = 1;
			}
			break;
		case 3:
			d						 = *(double *)param;
			b						 = (d >= 0.5);
			if (instance->inverse	!= b) {
				instance->inverse	 = b;
				instance->changed 	 = 1;
			}
			break;
		case 4:
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
			*(double *)param		 = instance->lut;
			break;
		case 1:
			*(double *)param		 = instance->magenta;
			break;
		case 2:
			*(double *)param		 = instance->efficacy;
			break;
		case 3:
			*(double *)param		 = (double)instance->inverse;
			break;
		case 4:
			*(double *)param		 = (double)instance->srgb;
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
			if (instance->changed) {
				KOLIBA_InterpolateSluts(&instance->sLut, sLut[0], instance->lut, sLut[1]);
				KOLIBA_Interpolate((double *)&instance->sLut.Magenta, (double *)vrt, instance->magenta, (double *)&instance->sLut.Magenta, 3);
				KOLIBA_SlutEfficacy(&instance->sLut, &instance->sLut, (instance->inverse) ? -instance->efficacy : instance->efficacy);
				instance->flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vertices));
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

		for (; i; i--, inframe++, outframe++) {
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}


