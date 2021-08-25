/*
	tint.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the manipulation of tint in Koliba. It needs to be
	linked dynamically using the -lkoliba switch in Unix
	and its derivatives, or koliba.lib in Windows.
*/

#define	KOLIBCALLS
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
#define F0R_PARAM_COLOR     2
/* End of frei0r.h extract */

typedef	struct _tint_instance {
	KOLIBA_VERTEX	tint;
	KOLIBA_FLUT		fLut;
	double			tinge, light, saturation;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	flut;
	unsigned char	invert;
	unsigned char	srgb;
	unsigned char	changed;
} tint_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Tint";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 9;
	info->explanation		= "Experiment with the tint.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(tint_instance), 1)) != NULL) {
		instance->count			= width * height;

		instance->saturation	= 1.0;
		instance->tint.g		= 0.5;
		instance->tint.b		= 1.0;
		instance->tinge			= 0.25;
		instance->light			= 0.25;
		instance->srgb			= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and flags.
		instance->changed		= 1;
		instance->flags			= KOLIBA_AllFlutFlags;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Cyan";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Cyan Tint.";
			break;
		case 1:
			info->name			= "Magenta";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Magenta Tint.";
			break;
		case 2:
			info->name			= "Yellow";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Yellow Tint.";
			break;
		case 3:
			info->name			= "Tinge";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How strong/subtle the tint is.";
			break;
		case 4:
			info->name			= "Light";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Brightness of tinted light.";
			break;
		case 5:
			info->name			= "Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Saturation of the original colors (not the tint).";
			break;
		case 6:
			info->name			= "Invert Light";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the tint of the light.";
			break;
		case 7:
			info->name			= "Flut Direct";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use the fLut directly.";
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
			if (instance->tint.r		!= *(double *)param) {
				instance->tint.r		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->tint.g		!= *(double *)param) {
				instance->tint.g		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->tint.b		!= *(double *)param) {
				instance->tint.b		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->tinge			!= *(double *)param) {
				instance->tinge			 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 4:
			if (instance->light			!= *(double *)param) {
				instance->light			 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 5:
			if (instance->saturation	!= *(double *)param) {
				instance->saturation	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 6:
			b = (*(double *)param >= 0.5);
			if (instance->invert		!= b) {
				instance->invert		 = b;
				instance->changed		 = 1;
			}
			break;
		case 7:
			b = (*(double *)param >= 0.5);
			if (instance->flut			!= b) {
				instance->flut			 = b;
				instance->changed		 = 1;
			}
			break;
		case 8:
			b = (*(double *)param >= 0.5);
			if (instance->srgb			!= b) {
				instance->srgb			 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param	= instance->tint.r;
			break;
		case 1:
			*(double *)param	= instance->tint.g;
			break;
		case 2:
			*(double *)param	= instance->tint.b;
			break;
		case 3:
			*(double *)param	= instance->tinge;
			break;
		case 4:
			*(double *)param	= instance->light;
			break;
		case 5:
			*(double *)param	= instance->saturation;
			break;
		case 6:
			*(double *)param	= (double)instance->invert;
			break;
		case 7:
			*(double *)param	= (double)instance->flut;
			break;
		case 8:
			*(double *)param	= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			instance->flags		= KOLIBA_FlutFlags(KOLIBA_TintToFlut(&instance->fLut, &instance->tint, instance->saturation, instance->tinge, instance->light, instance->invert, instance->flut, NULL));
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
