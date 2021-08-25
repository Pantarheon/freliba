#define  KOLIBCALLS
#include <koliba.h>
#include <stdlib.h>
#include <string.h>

/*
	chromozone.c

	Copyright 2019 G. Adam Stanislav
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of Lumidux in Koliba. It needs to be
	linked dynamically using the -lkoliba switch in Unix
	and its derivatives, or koliba.lib in Windows.

	Increase the contrast of high-chrominance pixels.

	The LUT (uLut, fChromozone) increases the contrast
	of all pixels, but then the Lumidux (ldxChromozone)
	interpolates each modified pixel with the original
	pixel based on the chrominance of the original pixel
	(though we can change it from chrominance to saturation,
	from the original pixel to the modified one, we can
	change the strength of the contrast, and we can output
	a mask instead of the resultant pixels).
*/

#define	czFlags	0x88F

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

typedef	struct _chrzone_instance {
	KOLIBA_LDX		ldx;
	KOLIBA_FLUT		fLut;
	double			contrast;
	unsigned int	count;
	unsigned char	sat;
	unsigned char	post;
	unsigned char	mask;
	unsigned char	srgb;
	unsigned char	changed;
} chrzone_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

static const unsigned long long uLut[] = {

	0xBF747AF68220DFD3, 0xBF747AF68220DFD3, 0xBF747AF68220DFD3,
	0x3FFA33D3C77CE768, 0x0, 0x0,
	0x0, 0x3FFA33D3C77CE768, 0x0,
	0x0, 0x0, 0x3FFA33D3C77CE768,
	0x0, 0x0, 0x0,
	0x0, 0x0, 0x0,
	0x0, 0x0, 0x0,
	0x0, 0x0, 0x0
};
static const KOLIBA_FLUT *fChromozone = (KOLIBA_FLUT *)uLut;

static const KOLIBA_LDX ldxChromozone = {
	-1, 0, 3, 0, 0, 0, 3, 1,
	0.000000, 1.000000,
	0.000000, 1.000000
};

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Chromozone";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 7;
	info->explanation		= "Adjust the contrast based on chrominance.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(1, sizeof(chrzone_instance))) != NULL) {

		instance->count		= width * height;
		instance->contrast	= 1.0;
		instance->srgb		= 1;

		memcpy(&instance->fLut, fChromozone, sizeof(KOLIBA_FLUT));
		memcpy(&instance->ldx, &ldxChromozone, sizeof(KOLIBA_LDX));
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Contrast level of high-chrominance pixels.";
			break;
		case 1:
			info->name			= "Threshold A";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Effect threshold.";
			break;
		case 2:
			info->name			= "Threshold B";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Effect threshold.";
			break;
		case 3:
			info->name			= "Saturation";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Uses saturation instead of chrominance.";
			break;
		case 4:
			info->name			= "Post-processing";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Uses post-processed chrominance (or saturation).";
			break;
		case 5:
			info->name			= "Show mask";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Shows a mask instead of colors.";
			break;
		case 6:
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
			if (instance->contrast	!= *(double *)param) {
				instance->contrast	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			instance->ldx.slow		 = *(double *)param;
			break;
		case 2:
			instance->ldx.shigh		 = *(double *)param;
			break;
		case 3:
			b						 = (*(double *)(param) > 0.5);
			if (instance->sat		!= b) {
				instance->sat		 = b;
				instance->changed	 = 1;
			}
			break;
		case 4:
			b						 = (*(double *)(param) > 0.5);
			if (instance->post		!= b) {
				instance->post		 = b;
				instance->changed	 = 1;
			}
			break;
		case 5:
			b						 = (*(double *)(param) > 0.5);
			if (instance->mask		!= b) {
				instance->mask		 = b;
				instance->changed	 = 1;
			}
			break;
		case 6:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->contrast;
			break;
		case 1:
			*(double *)param = (double)instance->ldx.slow;
			break;
		case 2:
			*(double *)param = (double)instance->ldx.shigh;
			break;
		case 3:
			*(double *)param = (double)instance->sat;
			break;
		case 4:
			*(double *)param = (double)instance->post;
			break;
		case 5:
			*(double *)param = (double)instance->mask;
			break;
		case 6:
			*(double *)param = (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_FlutEfficacy(&instance->fLut, fChromozone, instance->contrast);
			instance->ldx.schroma	= (!instance->sat);
			instance->ldx.sbase		= instance->post;
			instance->ldx.mask		= instance->mask;
			instance->changed		= 0;
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
			KOLIBA_LumiduxRgba8Pixel(outframe, inframe, &instance->fLut, czFlags, &instance->ldx, NULL, iconv, oconv)->a = inframe->a;
		}
	}
}


