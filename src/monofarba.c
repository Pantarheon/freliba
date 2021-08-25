/*
	monofarba.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of monofarba in Koliba.

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
	KOLIBA_FLUT		fLut;
	double			primary, secondary, efficacy;
	size_t			count;
	KOLIBA_FLAGS	fflags;
	unsigned char	changed;
	unsigned char	flags;
	unsigned char	srgb;
} colors_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Mono Farba";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 10;
	info->explanation		= "Adjust the saturation of the farba.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(colors_instance),1)) != NULL) {
		instance->count		= (size_t)width * (size_t)height;

		instance->primary	= 1.0;
		instance->secondary	= 0.0;
		instance->efficacy	= 1.0;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating FLUT.
		// We start with it set true because our FLUT
		// is still not initialized.
		instance->changed	= 1;

		instance->flags		= KOLIBA_SLUTRED;
		instance->srgb		= 1;
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
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to red.";
			break;
		case 1:
			info->name			= "Green";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to green.";
			break;
		case 2:
			info->name			= "Blue";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to blue.";
			break;
		case 3:
			info->name			= "Cyan";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to cyan.";
			break;
		case 4:
			info->name			= "Magenta";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to magenta.";
			break;
		case 5:
			info->name			= "Yellow";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Apply primary saturation to yellow.";
			break;
		case 6:
			info->name			= "Primary Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Applied to channels selected above.";
			break;
		case 7:
			info->name			= "Secondary Saturaion";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Applied to channels not selected above.";
			break;
		case 8:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Overall efficacy of the effect.";
			break;
		case 9:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTRED;
			else
				instance->flags			&= ~KOLIBA_SLUTRED;
			instance->changed			 = 1;
			break;
		case 1:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTGREEN;
			else
				instance->flags			&= ~KOLIBA_SLUTGREEN;
			instance->changed			 = 1;
			break;
		case 2:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTBLUE;
			else
				instance->flags			&= ~KOLIBA_SLUTBLUE;
			instance->changed			 = 1;
			break;
		case 3:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTCYAN;
			else
				instance->flags			&= ~KOLIBA_SLUTCYAN;
			instance->changed			 = 1;
			break;
		case 4:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTMAGENTA;
			else
				instance->flags			&= ~KOLIBA_SLUTMAGENTA;
			instance->changed			 = 1;
			break;
		case 5:
			if ((*(double *)param) >= 0.5)
				instance->flags			|= KOLIBA_SLUTYELLOW;
			else
				instance->flags			&= ~KOLIBA_SLUTYELLOW;
			instance->changed			 = 1;
			break;
		case 6:
			if (instance->primary		!= *(double *)param) {
				instance->primary		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 7:
			if (instance->secondary		!= *(double *)param) {
				instance->secondary		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 8:
			if (instance->efficacy		!= *(double *)param) {
				instance->efficacy		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 9:
				instance->srgb			 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param			= (instance->flags & KOLIBA_SLUTRED) ? 1.0 : 0.0;
			break;
		case 1:
			*(double *)param			= (instance->flags & KOLIBA_SLUTGREEN) ? 1.0 : 0.0;
			break;
		case 2:
			*(double *)param			= (instance->flags & KOLIBA_SLUTBLUE) ? 1.0 : 0.0;
			break;
		case 3:
			*(double *)param			= (instance->flags & KOLIBA_SLUTCYAN) ? 1.0 : 0.0;
			break;
		case 4:
			*(double *)param			= (instance->flags & KOLIBA_SLUTMAGENTA) ? 1.0 : 0.0;
			break;
		case 5:
			*(double *)param			= (instance->flags & KOLIBA_SLUTYELLOW) ? 1.0 : 0.0;
			break;
		case 6:
			*(double *)param				= instance->primary;
			break;
		case 7:
			*(double *)param				= instance->secondary;
			break;
		case 8:
			*(double *)param				= instance->efficacy;
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
			KOLIBA_MonoFarbaToFlut(&instance->fLut, NULL, instance->primary, instance->secondary, instance->flags);
			instance->fflags = KOLIBA_FlutFlags(KOLIBA_FlutEfficacy(&instance->fLut, &instance->fLut, instance->efficacy));
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
			KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->fflags, iconv, oconv)->a = inframe->a;
		}
	}
}
