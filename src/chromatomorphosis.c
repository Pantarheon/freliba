/*
	chromatomorphosis.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of mallets in Koliba. It needs to be linked
	dynamically using the -lkoliba switch in Unix and its
	derivatives, or koliba.lib in Windows.
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
#define F0R_PARAM_COLOR     2
/* End of frei0r.h extract */

typedef	struct _chromatomorphosis_instance {
	KOLIBA_MALLET	mallet[2];
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT		fLut;
	double			con[2];
	size_t			count;
	KOLIBA_FLAGS	flags;
	unsigned char	invert;
	unsigned char	srgb;
	unsigned char	changed;
} chromatomorphosis_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Chromatomorphosis";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 3;
	info->explanation		= "Complete overhaul of the farba in the image.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(chromatomorphosis_instance))) != NULL) {
		KOLIBA_InitializeMallet(instance->mallet, KOLIBA_SLUTPRIMARY);
		KOLIBA_InitializeMallet(instance->mallet+1, KOLIBA_SLUTSECONDARY);

		instance->count					= (size_t)width * (size_t)height;
		instance->mallet[0].center.r	= 0.186529;
		instance->mallet[0].center.g	= 0.135684;
		instance->mallet[0].center.b	= 0.008110;
		instance->mallet[0].adjustment	= 0.292517;
		instance->mallet[1].center.r	= 0.792746;
		instance->mallet[1].center.g	= 0.019148;
		instance->mallet[1].center.b	= 0.019148;
		instance->mallet[1].adjustment	= 0.442177;
		instance->mallet[1].gain		= -1.0;
		instance->con[0]				= 1.0;
		instance->con[1]				= 1.0;
		instance->srgb					= 1;
		instance->invert				= 0;
		instance->changed				= 1;

		// We only need to initialize the pointers to the vertices once
		// because their addresses within an instance never change.
		KOLIBA_SlutToVertices(&instance->vert, &instance->sLut);
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Primary Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Primary color contrast adjustment.";
			break;
		case 1:
			info->name			= "Secondary Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Secondary color contrast adjustment.";
			break;
		case 2:
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
			if (instance->con[0]				!= *(double *)param) {
				instance->con[0]				 = *(double *)param;
				instance->mallet[0].adjustment	 = 0.292517 * (*(double *)param);
				instance->changed				 = 1;
			}
			break;
		case 1:
			if (instance->con[1]				!= *(double *)param) {
				instance->con[1]				 = *(double *)param;
				instance->mallet[1].adjustment	 = 0.442177 * (*(double *)param);
				instance->changed				 = 1;
			}
			break;
		case 2:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->con[0];
			break;
		case 1:
			*(double *)param = (double)instance->con[1];
			break;
		case 2:
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
			KOLIBA_ConvertMalletsToSlut(&instance->sLut, NULL, instance->mallet, NULL, 2);
			KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vert);
			instance->flags		= KOLIBA_FlutFlags(&instance->fLut);
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
