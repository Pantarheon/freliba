/*
	ranges.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating the use
	of selective SLUT copying in Koliba. It needs to be linked
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

typedef	struct _ranges_instance {
	KOLIBA_RGB		from, to;
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT		fLut;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	changed;
	unsigned char	svit;
	unsigned char	srgb;
	unsigned char	copy;
} ranges_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Ranges";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 13;
	info->explanation		= "Set the ranges of the farba and the svit.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(ranges_instance), 1)) != NULL) {
		KOLIBA_ResetSlut(&instance->sLut);

		instance->count	= width * height;
		instance->to.r	= 1.0;
		instance->to.g  = 1.0;
		instance->to.b	= 1.0;
		instance->srgb	= 1;
		instance->copy	= 1;

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
			info->name			= "Bottom Farba Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of red in the farba.";
			break;
		case 1:
			info->name			= "Bottom Farba Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of green in the farba.";
			break;
		case 2:
			info->name			= "Bottom Farba Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of blue in the farba.";
			break;
		case 3:
			info->name			= "Top Farba Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of red in the farba.";
			break;
		case 4:
			info->name			= "Top Farba Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of green in the farba.";
			break;
		case 5:
			info->name			= "Top Farba Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of blue in the farba.";
			break;
		case 6:
			info->name			= "Bottom Svit Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of red in the svit.";
			break;
		case 7:
			info->name			= "Bottom Svit Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of green in the svit.";
			break;
		case 8:
			info->name			= "Bottom Svit Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The smallest value of blue in the svit.";
			break;
		case 9:
			info->name			= "Top Svit Red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of red in the svit.";
			break;
		case 10:
			info->name			= "Top Svit Green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of green in the svit.";
			break;
		case 11:
			info->name			= "Top Svit Blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The largest value of blue in the svit.";
			break;
		case 12:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Accomodates sRGB model.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->from.r		!= *(double *)param) {
				instance->from.r		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 1:
			if (instance->from.g		!= *(double *)param) {
				instance->from.g		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 2:
			if (instance->from.b		!= *(double *)param) {
				instance->from.b		 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 3:
			if (instance->to.r			!= *(double *)param) {
				instance->to.r			 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 4:
			if (instance->to.g			!= *(double *)param) {
				instance->to.g			 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 5:
			if (instance->to.b			!= *(double *)param) {
				instance->to.b			 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 6:
			if (instance->sLut.black.r	!= *(double *)param) {
				instance->sLut.black.r	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 7:
			if (instance->sLut.black.g	!= *(double *)param) {
				instance->sLut.black.g	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 8:
			if (instance->sLut.black.b	!= *(double *)param) {
				instance->sLut.black.b	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 9:
			if (instance->sLut.white.r	!= *(double *)param) {
				instance->sLut.white.r	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 10:
			if (instance->sLut.white.g	!= *(double *)param) {
				instance->sLut.white.g	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 11:
			if (instance->sLut.white.b	!= *(double *)param) {
				instance->sLut.white.b	 = *(double *)param;
				instance->changed		 = 1;
			}
			break;
		case 12:
			instance->srgb			 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->from.r;
			break;
		case 1:
			*(double *)param = (double)instance->from.g;
			break;
		case 2:
			*(double *)param = (double)instance->from.b;
			break;
		case 3:
			*(double *)param = (double)instance->to.r;
			break;
		case 4:
			*(double *)param = (double)instance->to.g;
			break;
		case 5:
			*(double *)param = (double)instance->to.b;
			break;
		case 6:
			*(double *)param = (double)instance->sLut.black.r;
			break;
		case 7:
			*(double *)param = (double)instance->sLut.black.g;
			break;
		case 8:
			*(double *)param = (double)instance->sLut.black.b;
			break;
		case 9:
			*(double *)param = (double)instance->sLut.white.r;
			break;
		case 10:
			*(double *)param = (double)instance->sLut.white.g;
			break;
		case 11:
			*(double *)param = (double)instance->sLut.white.b;
			break;
		case 12:
			*(double *)param = (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			instance->copy	= KOLIBA_IsIdentityFlut(KOLIBA_ConvertSlutToFlut(&instance->fLut, KOLIBA_FarbaRange(&instance->vert, &instance->from, &instance->to)));
			instance->flags	= KOLIBA_FlutFlags(&instance->fLut);
		}

		if (instance->copy)
			memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
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
