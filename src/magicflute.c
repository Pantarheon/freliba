/*
	magicflute.c

	Copyright 2021 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of a sequence of two LUTs in Koliba. It needs
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

typedef	struct _magicflute_instance {
	KOLIBA_VERTICES vertices[2];
	KOLIBA_SLUT 	sLut[2];
	KOLIBA_FFLUT	ffLut[2];
	KOLIBA_FLUT		fLut[2];
	double			lut;
	double			magenta;
	double			imp;
	double			angle;
	double			atmo;
	double			fx;
	double			efficacy[2];
	size_t			count;
	unsigned char	inverse[2];
	unsigned char	srgb;
	unsigned char	changed[2];
} magicflute_instance, *f0r_instance_t;

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
	info->name				= "Koliba Magic Flute";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 1;
	info->num_params		= 11;
	info->explanation		= "Crimsonite and Color Roller.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(magicflute_instance))) != NULL) {
		// We set these values once and keep them that way for the
		// life of this instance.
		KOLIBA_SlutToVertices(instance->vertices, instance->sLut);
		KOLIBA_SlutToVertices(instance->vertices+1, instance->sLut+1);
		instance->ffLut[0].fLut	= instance->fLut;
		instance->ffLut[1].fLut	= instance->fLut+1;
		instance->count				= (size_t)width * (size_t)height;

		// We set the initial values for these parameters,
		// but allow the user to change them.
		instance->lut				= 0.5;
		instance->magenta			= 1.0;

		instance->imp				= 0.5;
		instance->angle				= 30.0/360.0;	// Must be * 360
		instance->atmo				= 0.0;	// Must be * 360
		instance->fx				= 0.0;

		// We start with full efficacy for both Luts.
		instance->efficacy[0]		= 1.0;
		instance->inverse[0]		= 0;
		instance->efficacy[1]		= 1.0;
		instance->inverse[1]		= 0;
		instance->srgb				= 1;

		// Set these to 1, whenever a parameter changes.
		// Set them back to 0 after recalculating sLuts.
		// We start with them set true because our luts
		// are still not initialized.
		instance->changed[0]		= 1;
		instance->changed[1]		= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		// These control the first LUT.
		case 0:
			info->name			= "Crimsonite Effect Type";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The relative contribution of the crimsonite input LUTs.";
			break;
		case 1:
			info->name			= "Crimsonite Demag";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Reduce the effect of the crimsonite magenta color.";
			break;
		case 2:
			info->name			= "Crimsonite Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the crimsonite effect.";
			break;
		case 3:
			info->name			= "Crimsonite Inverse";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Inverts the crimsonite efficacy.";
			break;

		// These control the second LUT.
		case 4:
			info->name			= "Color Roller Input Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The contrast of the color-roller input LUT";
			break;
		case 5:
			info->name			= "Color Roller Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the color rotation angle.";
			break;
		case 6:
			info->name			= "Color Roller Atmosphere";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the atmosphere LUT.";
			break;
		case 7:
			info->name			= "Color Roller Extreme";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how extreme the color-roller effect is.";
			break;
		case 8:
			info->name			= "Color Roller Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the color-roller effect.";
			break;
		case 9:
			info->name			= "Color Roller Inverse";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Inverts the color-roller efficacy.";
			break;

		// This one affects both LUTs.
		case 10:
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
			if (instance->lut			!= *(double *)param) {
				instance->lut			 = *(double *)param;
				instance->changed[0]	 = 1;
			}
			break;
		case 1:
			if (instance->magenta		!= *(double *)param) {
				instance->magenta		 = *(double *)param;
				instance->changed[0]	 = 1;
			}
			break;
		case 2:
			if (instance->efficacy[0]	!= *(double *)param) {
				instance->efficacy[0]	 = *(double *)param;
				instance->changed[0] 	 = 1;
			}
			break;
		case 3:
			d							 = *(double *)param;
			b							 = (d >= 0.5);
			if (instance->inverse[0]	!= b) {
				instance->inverse[0]	 = b;
				instance->changed[0] 	 = 1;
			}
			break;
		case 4:
			if (instance->imp			!= *(double *)param) {
				instance->imp			 = *(double *)param;
				instance->changed[1]	 = 1;
			}
			break;
		case 5:
			if (instance->angle			!= *(double *)param) {
				instance->angle			 = *(double *)param;
				instance->changed[1]	 = 1;
			}
			break;
		case 6:
			if (instance->atmo			!= *(double *)param) {
				instance->atmo			 = *(double *)param;
				instance->changed[1]	 = 1;
			}
			break;
		case 7:
			if (instance->fx			!= *(double *)param) {
				instance->fx			 = *(double *)param;
				instance->changed[1]	 = 1;
			}
			break;
		case 8:
			if (instance->efficacy[1]	!= *(double *)param) {
				instance->efficacy[1]	 = *(double *)param;
				instance->changed[1] 	 = 1;
			}
			break;
		case 9:
			d							 = *(double *)param;
			b							 = (d >= 0.5);
			if (instance->inverse[1]	!= b) {
				instance->inverse[1]	 = b;
				instance->changed[1] 	 = 1;
			}
			break;
		case 10:
			d							 = *(double *)param;
			b							 = (d >= 0.5);
			if (instance->srgb			!= b) {
				instance->srgb			 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param			 = instance->lut;
			break;
		case 1:
			*(double *)param			 = instance->magenta;
			break;
		case 2:
			*(double *)param			 = instance->efficacy[0];
			break;
		case 3:
			*(double *)param			 = (double)instance->inverse[0];
			break;
		case 4:
			*(double *)param			 = instance->imp;
			break;
		case 5:
			*(double *)param			 = instance->angle;
			break;
		case 6:
			*(double *)param			 = instance->atmo;
			break;
		case 7:
			*(double *)param			 = instance->fx;
			break;
		case 8:
			*(double *)param			 = instance->efficacy[1];
			break;
		case 9:
			*(double *)param			 = instance->inverse[1];
			break;
		case 10:
			*(double *)param			 = (double)instance->srgb;
			break;
	}
}


void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		// Changed or not, if both efficacies are 0, just do a quick copy
		// of all input pixels to corresponding output pixels.
		if ((instance->efficacy[0] == 0.0) && (instance->efficacy[1] == 0.0))
			memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->changed[0]) {
				KOLIBA_InterpolateSluts(&instance->sLut[0], sLut[0], instance->lut, sLut[1]);
				KOLIBA_Interpolate((double *)&instance->sLut[0].magenta, (double *)vrt, instance->magenta, (double *)&instance->sLut[0].magenta, 3);
				KOLIBA_SlutEfficacy(&instance->sLut[0], &instance->sLut[0], (instance->inverse[0]) ? -instance->efficacy[0] : instance->efficacy[0]);
				instance->ffLut[0].flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut[0], &instance->vertices[0]));
				instance->changed[0]	 = 0;
			}

			if (instance->changed[1]) {
				KOLIBA_ColorRoller(&instance->sLut[1], instance->imp, instance->angle * 360.0, instance->atmo * 360.0, instance->fx, (instance->inverse[1]) ? -instance->efficacy[1] : instance->efficacy[1]);
				instance->ffLut[1].flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut[1], &instance->vertices[1]));

				// Only the last LUT in the chain of LUTs is scaled.
				KOLIBA_ScaleFlut(&instance->fLut[1], &instance->fLut[1], 255.0);
				instance->changed[1]	 = 0;
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
				KOLIBA_ScaledPolyRgba8Pixel(outframe, inframe, instance->ffLut, 2, iconv, oconv)->a = inframe->a;
			}
		}
	}
}


