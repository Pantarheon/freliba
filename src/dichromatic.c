/*
	dichromatic.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the dichromatic effects in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
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

typedef	struct _dichromatic_instance {
	KOLIBA_MATRIX	matrix;
	KOLIBA_DICHROMA	dichroma;
	KOLIBA_FLUT		fLut;
	double			angle;		// In turns
	double			rotation;	// In turns
	double			dchannel;
	size_t			count;
	unsigned int	channel;
	unsigned int	normalize;
	unsigned char	srgb;
	unsigned char	changed;
} dichromatic_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Dichromatic";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 10;
	info->explanation		= "Create a dichromatic image.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(dichromatic_instance))) != NULL) {
		instance->count					= (size_t)width * (size_t)height;

		// We will use the default (Rec. 2020) chroma model.
		// We only need to do this once.
		KOLIBA_SetChromaModel(&instance->dichroma.chr.model, NULL);

		// We start with a default chroma.
		KOLIBA_ResetChroma((KOLIBA_CHROMA *)&instance->dichroma.chr.chroma.angle);

		// We keep separate variables for the angles because some
		// frei0r hosts only allow it to be in the 0-1 range, while
		// Koliba expects the angles in degrees.
		instance->angle					= 1.0;
		instance->dichroma.chr.chroma.angle	= instance->angle * 360.0;
		instance->rotation				= 0.0;

		// We start with full efficacy and the red channel.
		instance->dichroma.efficacy		= 1.0;
		instance->dchannel				= 0.0;
		instance->channel				= 0;
		instance->normalize				= 1;
		instance->srgb					= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
		instance->changed				= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the chroma rotation angle.";
			break;
		case 1:
			info->name			= "Magnitude";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how strong the chroma rotation effect is.";
			break;
		case 2:
			info->name			= "Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the saturation of the primary channel.";
			break;
		case 3:
			info->name			= "Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the shadows.";
			break;
		case 4:
			info->name			= "White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the highlights.";
			break;
		case 5:
			info->name			= "Rotation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Gray rotation angle.";
			break;
		case 6:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the effect.";
			break;
		case 7:
			info->name			= "Channel";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Red = 0, Green = 0.5, Blue = 1.";
			break;
		case 8:
			info->name			= "Normalize";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "This may help keep the brightness. Or not.";
			break;
		case 9:
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
			if (instance->angle						!= *(double *)param) {
				instance->angle						 = *(double *)param;
				instance->dichroma.chr.chroma.angle	 = *(double *)param * 360.0;
				instance->changed					 = 1;
			}
			break;
		case 1:
			if (instance->dichroma.chr.chroma.magnitude	!= *(double *)param) {
				instance->dichroma.chr.chroma.magnitude	 = *(double *)param;
				instance->changed					 = 1;
			}
			break;
		case 2:
			if (instance->dichroma.chr.chroma.saturation	!= *(double *)param) {
				instance->dichroma.chr.chroma.saturation	 = *(double *)param;
				instance->changed	 				 = 1;
			}
			break;
		case 3:
			if (instance->dichroma.chr.chroma.black	!= *(double *)param) {
				instance->dichroma.chr.chroma.black	 = *(double *)param;
				instance->changed	 				 = 1;
			}
			break;
		case 4:
			if (instance->dichroma.chr.chroma.white	!= *(double *)param) {
				instance->dichroma.chr.chroma.white	 = *(double *)param;
				instance->changed			 		 = 1;
			}
			break;
		case 5:
			if (instance->rotation					!= *(double *)param) {
				instance->rotation					 = *(double *)param;
				instance->dichroma.rotation	 		= *(double *)param * 360.0;
				instance->changed			 		= 1;
			}
			break;
		case 6:
			if (instance->dichroma.efficacy			!= *(double *)param) {
				instance->dichroma.efficacy			 = *(double *)param;
				instance->changed			 		 = 1;
			}
			break;
		case 7:
			if (instance->dchannel					!= *(double *)param) {
				instance->dchannel					 = *(double *)param;
				d									 = instance->dchannel * 3.0;
				instance->channel					 = (d <= 1.0) ? 0 : (d <= 2.0) ? 1 : 2;
				instance->changed			 		 = 1;
			}
			break;
		case 8:
			b										 = ((*(double *)param) >= 0.5);
			if (instance->normalize					!= b) {
				instance->normalize					 = b;
				instance->changed			 		 = 1;
			}
			break;
		case 9:
			b										 = ((*(double *)param) >= 0.5);
			if (instance->srgb						!= b) {
				instance->srgb						 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= instance->angle;
			break;
		case 1:
			*(double *)param				= instance->dichroma.chr.chroma.magnitude;
			break;
		case 2:
			*(double *)param				= instance->dichroma.chr.chroma.saturation;
			break;
		case 3:
			*(double *)param				= instance->dichroma.chr.chroma.black;
			break;
		case 4:
			*(double *)param				= instance->dichroma.chr.chroma.white;
			break;
		case 5:
			*(double *)param				= instance->rotation;
			break;
		case 6:
			*(double *)param				= instance->dichroma.efficacy;
			break;
		case 7:
			*(double *)param				= instance->dchannel;
			break;
		case 8:
			*(double *)param				= (double)instance->normalize;
			break;
		case 9:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_DichromaticMatrix(&instance->matrix, &instance->dichroma, instance->normalize, instance->channel);
			KOLIBA_ConvertMatrixToFlut(&instance->fLut, &instance->matrix);
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
			KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, KOLIBA_MatrixFlutFlags, iconv, oconv)->a = inframe->a;
		}
	}
}
