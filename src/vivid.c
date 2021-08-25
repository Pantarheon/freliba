/*
	vivid.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating the
	use of pure erythropy in Koliba. It needs to be linked
	dynamically using the -lkoliba switch in Unix and its
	derivatives, or koliba.lib in Windows.
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
#define F0R_PARAM_COLOR     2
/* End of frei0r.h extract */

typedef	struct _vivid_instance {
	KOLIBA_FLUT		fLut;
	double			efficacy;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
	unsigned char	copy;
} vivid_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

static const KOLIBA_VERTEX red = {
	0.470588,
	0.325490,
	0.294118
};

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Vivid Colors";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 2;
	info->explanation		= "Make the farba more (or less) vivid.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(vivid_instance))) != NULL) {
		instance->count		= width * height;

		instance->efficacy	= 0.428571;
		instance->srgb		= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and flags.
		instance->changed	= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How strong/subtle the effect is.";
			break;
		case 1:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Accomodates sRGB model";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->efficacy	!= *(double *)param) {
				instance->efficacy	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			instance->srgb			 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param	= instance->efficacy;
			break;
		case 1:
			*(double *)param	= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			if ((instance->copy = (instance->efficacy == 0.0)) == 0) {
				KOLIBA_VERTEX vertex;

				instance->flags = KOLIBA_FlutFlags(KOLIBA_ApplyPureErythropy(&instance->fLut, (KOLIBA_VERTEX *)KOLIBA_Interpolate(
					(double *)&vertex,
					(double *)&red,
					-instance->efficacy,
					(double *)&KOLIBA_IdentitySlut.red,
					sizeof(KOLIBA_VERTEX)/sizeof(double)
				)));
			}
			instance->changed = 0;
		}

		if (instance->copy) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
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
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
