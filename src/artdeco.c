#define  KOLIBCALLS
#include <koliba.h>
#include <stdlib.h>
#include <string.h>

/*
	artdeco.c

	Copyright 2019 G. Adam Stanislav
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of Lumidux in Koliba. It needs to be
	linked dynamically using the -lkoliba switch in Unix
	and its derivatives, or koliba.lib in Windows.

	An Art Deco effect.
*/

#define	adFlags	0xFFFFFF

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

typedef	struct _artdeco_instance {
	KOLIBA_LDX		ldx;
	KOLIBA_FLUT		fLut;
	double			efficacy;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	post;
	unsigned char	mask;
	unsigned char	srgb;
	unsigned char	changed;
} artdeco_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;


static const unsigned long long uLut[] = {

	0xBFE71717DF19D66B, 0xBFDC9C9D5A187A4A, 0xBFD050507A6BD6E9,
	0x3FF71717DF19D66B, 0x3FEC9C9D5A187A4A, 0x3FE050507A6BD6E9,
	0x3FEF3F401C4FC1E0, 0x3FF2B2B3461309C8, 0x3FE67676EA42289A,
	0x3FF2B2B3461309C8, 0x3FE67676EA42289A, 0x3FEF3F401C4FC1E0,
	0xBFF25253543AEAB8, 0xBFEBDBDD76683C29, 0xBFE5B5B70691EA79,
	0xBFF25253543AEAB8, 0xBFEBDBDD76683C2A, 0xBFE5B5B70691EA79,
	0xBFF25253543AEAB8, 0xBFEBDBDD76683C2A, 0xBFE5B5B70691EA78,
	0x3FF9191AB8E8EA3A, 0x3FF4B4B61FE21D96, 0x3FF1A1A2E7F6F4BE
};
static const KOLIBA_FLUT *fArtDeco = (KOLIBA_FLUT *)uLut;

static const KOLIBA_LDX ldxArtDeco = {
	-1, 0, 3, 0, 0, 0, 3, 0,
	0.000000, 1.000000,
	0.000000, 1.000000
};

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Art Deco";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 4;
	info->explanation		= "An Art Deco effect.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(1, sizeof(artdeco_instance))) != NULL) {

		instance->count		= width * height;
		instance->efficacy	= 1.0;
		instance->flags		= adFlags;
		instance->srgb		= 1;

		memcpy(&instance->fLut, fArtDeco, sizeof(KOLIBA_FLUT));
		memcpy(&instance->ldx, &ldxArtDeco, sizeof(KOLIBA_LDX));
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
			info->explanation	= "The strength of the effect.";
			break;
		case 1:
			info->name			= "Post-processing";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Uses post-processed chrominance (or saturation).";
			break;
		case 2:
			info->name			= "Show mask";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Shows a mask instead of colors.";
			break;
		case 3:
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
			if (instance->efficacy	!= *(double *)param) {
				instance->efficacy	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			b						 = (*(double *)(param) > 0.5);
			if (instance->post		!= b) {
				instance->post		 = b;
				instance->changed	 = 1;
			}
			break;
		case 2:
			b						 = (*(double *)(param) > 0.5);
			if (instance->mask		!= b) {
				instance->mask		 = b;
				instance->changed	 = 1;
			}
			break;
		case 3:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->efficacy;
			break;
		case 1:
			*(double *)param = (double)instance->post;
			break;
		case 2:
			*(double *)param = (double)instance->mask;
			break;
		case 3:
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
			instance->flags = KOLIBA_FlutFlags(KOLIBA_FlutEfficacy(&instance->fLut, fArtDeco, instance->efficacy));
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
			KOLIBA_LumiduxRgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, &instance->ldx, NULL, iconv, oconv)->a = inframe->a;
		}
	}
}


