/*
	colors.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the traditional color correction in Koliba, that is
	adjusting the lift, gamma, gain and offset, in that
	order. Of those four, the lift, the gain, and the
	offset could be all accomplished by multiplying their
	respective matrices. Unfortunately, the gamma is a
	non-linear operation, so it cannot be accomplished
	by using a traditional 3x4 matrix.

	So, we apply a lift matrix (converted to a fLut, as
	we do with everything in Koliba), then the gama as
	a separate non-linear function, and then a combined
	lift and gain matrix (fLut).

	It needs to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
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
/* End of frei0r.h extract */

typedef	struct _colors_instance {
	KOLIBA_FLUT		fLut[2];
	KOLIBA_FFLUT	ffLut[2];
	KOLIBA_XYZ		lift, gamma, gain, offset;
	size_t			count;
	unsigned char	changed;
	unsigned char	srgb;
} colors_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Colors";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 13;
	info->explanation		= "Adjust the colors.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(colors_instance),1)) != NULL) {
		instance->count			= (size_t)width * (size_t)height;

		instance->ffLut[0].fLut	= &instance->fLut[0];
		instance->ffLut[1].fLut	= &instance->fLut[1];

		instance->gain.x		= 1.0;
		instance->gain.y		= 1.0;
		instance->gain.z		= 1.0;

		// We keep a separate variable for the angle because some
		// frei0r hosts only allow it to be in the 0-1 range, while
		// Koliba expects the angle in degrees.
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
			info->name			= "Red Lift";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings red closer to white.";
			break;
		case 1:
			info->name			= "Green Lift";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings green closer to white.";
			break;
		case 2:
			info->name			= "Blue Lift";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings blue closer to white.";
			break;
		case 3:
			info->name			= "Red Gamma";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts red gamma (power curve).";
			break;
		case 4:
			info->name			= "Green Gamma";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts green gamma (power curve).";
			break;
		case 5:
			info->name			= "Blue Gamma";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Adjusts red gamma (power curve).";
			break;
		case 6:
			info->name			= "Red Gain";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings red closer to, or farther from, black.";
			break;
		case 7:
			info->name			= "Green Gain";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings green closer to, or farther from, black.";
			break;
		case 8:
			info->name			= "Blue Gain";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brings blue closer to, or farther from, black.";
			break;
		case 9:
			info->name			= "Red Offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Shifts red up or down.";
			break;
		case 10:
			info->name			= "Green Offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Shifts green up or down.";
			break;
		case 11:
			info->name			= "Blue Offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Shifts blue up or down.";
			break;
		case 12:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->lift.x			!= *(double *)param) {
				instance->lift.x			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 1:
			if (instance->lift.y			!= *(double *)param) {
				instance->lift.y			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 2:
			if (instance->lift.z			!= *(double *)param) {
				instance->lift.z			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 3:
			if (instance->gamma.x			!= *(double *)param) {
				instance->gamma.x			 = *(double *)param;
			}
			break;
		case 4:
			if (instance->gamma.y			!= *(double *)param) {
				instance->gamma.y			 = *(double *)param;
			}
			break;
		case 5:
			if (instance->gamma.z			!= *(double *)param) {
				instance->gamma.z			 = *(double *)param;
			}
			break;
		case 6:
			if (instance->gain.x			!= *(double *)param) {
				instance->gain.x			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 7:
			if (instance->gain.y			!= *(double *)param) {
				instance->gain.y			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 8:
			if (instance->gain.z			!= *(double *)param) {
				instance->gain.y			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 9:
			if (instance->offset.x			!= *(double *)param) {
				instance->offset.x			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 10:
			if (instance->offset.y			!= *(double *)param) {
				instance->offset.y			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 11:
			if (instance->offset.z			!= *(double *)param) {
				instance->offset.z			 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 12:
				instance->srgb				 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= instance->lift.x;
			break;
		case 1:
			*(double *)param				= instance->lift.y;
			break;
		case 2:
			*(double *)param				= instance->lift.z;
			break;
		case 3:
			*(double *)param				= instance->gamma.x;
			break;
		case 4:
			*(double *)param				= instance->gamma.y;
			break;
		case 5:
			*(double *)param				= instance->gamma.z;
			break;
		case 6:
			*(double *)param				= instance->gain.x;
			break;
		case 7:
			*(double *)param				= instance->gain.y;
			break;
		case 8:
			*(double *)param				= instance->gain.z;
			break;
		case 9:
			*(double *)param				= instance->offset.x;
			break;
		case 10:
			*(double *)param				= instance->offset.x;
			break;
		case 11:
			*(double *)param				= instance->offset.x;
			break;
		case 12:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	KOLIBA_MATRIX mat;
	KOLIBA_XYZ xyz;
	KOLIBA_EXTERNAL ext;

	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_MatrixLift(&mat, NULL, (KOLIBA_VERTEX *)&instance->lift);
			KOLIBA_ConvertMatrixToFlut(&instance->fLut[0], &mat);
			instance->ffLut[0].flags = KOLIBA_FlutFlags(&instance->fLut[0]);
			KOLIBA_MatrixGain(&mat, NULL,(KOLIBA_VERTEX *)&instance->gain);
			mat.red.o	= instance->offset.x;
			mat.green.o	= instance->offset.y;
			mat.blue.o	= instance->offset.z;
			KOLIBA_ConvertMatrixToFlut(&instance->fLut[1], &mat);
			instance->ffLut[1].flags = KOLIBA_FlutFlags(&instance->fLut[1]);
			instance->changed	= 0;
		}

		if ((instance->gamma.x == 0.0) && (instance->gamma.y == 0.0) && (instance->gamma.z == 0.0))
			ext = NULL;
		else {
			KOLIBA_PrepareGammaParameters(&xyz, &instance->gamma);
			ext = KOLIBA_Gamma;
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
			KOLIBA_ExternalRgba8Pixel(outframe, inframe, instance->ffLut, 1, 1, ext, &xyz, iconv, oconv)->a = inframe->a;
		}
	}
}
