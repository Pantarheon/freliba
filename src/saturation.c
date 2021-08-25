/*
	saturation.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of gray fLuts in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
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

typedef	struct _saturation_instance {
	KOLIBA_RGB		model;
	double			saturation;
	KOLIBA_FLUT		fLut;
	size_t			count;
	unsigned char	changed;
	unsigned char	invert;
	unsigned char	srgb;
	unsigned char	copy;
} saturation_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Saturation";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 6;
	info->explanation		= "Customized saturation.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(saturation_instance))) != NULL) {
		instance->count			= (size_t)width * (size_t)height;

		// We will use the default (Rec. 2020) chroma model.
		// We only need to do this once.
		KOLIBA_SetChromaModel(&instance->model, NULL);

		instance->saturation	= 0.0;
		instance->invert		= 0;
		instance->srgb			= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fLut.
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
			info->name			= "Red Weight";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the weight of the red channel.";
			break;
		case 1:
			info->name			= "Green weight";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the weight of the green channel.";
			break;
		case 2:
			info->name			= "Blue weight";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the weight of the blue channel.";
			break;
		case 3:
			info->name			= "Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the overall saturation.";
			break;
		case 4:
			info->name			= "Invert";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Inverts the saturation.";
			break;
		case 5:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	unsigned char b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->model.r		!= *(double *)param) {
				instance->model.r		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->model.g		!= *(double *)param) {
				instance->model.g		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->model.b		!= *(double *)param) {
				instance->model.b		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->saturation	!= *(double *)param) {
				instance->saturation	 = *(double *)param;
				instance->changed	 	 = 1;
			}
			break;
		case 4:
			b							 = ((*(double *)param) >= 0.5);
			if (instance->invert		!= b) {
				instance->invert		 = b;
				instance->changed	 	 = 1;
			}
			break;
		case 5:
			b								 = ((*(double *)param) >= 0.5);
			if (instance->srgb				!= b) {
				instance->srgb				 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param	= instance->model.r;
			break;
		case 1:
			*(double *)param	= instance->model.g;
			break;
		case 2:
			*(double *)param	= instance->model.b;
			break;
		case 3:
			*(double *)param	= instance->saturation;
			break;
		case 4:
			*(double *)param	= (double)instance->invert;
			break;
		case 5:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	KOLIBA_FLUT fLut;

	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_ConvertGrayToFlut(&fLut, &instance->model);
			KOLIBA_InterpolateFluts(&instance->fLut, &KOLIBA_IdentityFlut, (instance->invert)? -instance->saturation : instance->saturation, &fLut);
			instance->copy	= ((instance->invert == 0) && (instance->saturation == 1.0));
			instance->changed	= 0;
		}

		if (instance->copy != 0) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
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
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, KOLIBA_GrayFlutFlags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
