/*
	warm-and-cold.c

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

typedef	struct _warmandcold_instance {
	KOLIBA_CHROMAT	warm;
	KOLIBA_CHROMA	cold;
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT		fLut;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	changed[2];
	unsigned char	srgb;
	unsigned char	copy;
} warmandcold_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Warm and Cold";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 9;
	info->explanation		= "Control warm and cold colors separately.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(warmandcold_instance))) != NULL) {
		KOLIBA_ResetSlut(&instance->sLut);

		instance->count			= width * height;
		instance->changed[0]	= 0;
		instance->changed[1]	= 0;
		instance->srgb			= 1;
		instance->copy			= 1;

		KOLIBA_ResetChroma(&instance->cold);
		KOLIBA_ResetChromaticMatrix(&instance->warm, &KOLIBA_Rec2020);

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
			info->name			= "Warm Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The angle of the warm farba.";
			break;
		case 1:
			info->name			= "Warm Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The saturation of the warm farba.";
			break;
		case 2:
			info->name			= "Warm Black Level";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The black level of the warm farba.";
			break;
		case 3:
			info->name			= "Warm White Level";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The white level of the warm farba.";
			break;
		case 4:
			info->name			= "Cold Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The angle of the cold farba.";
			break;
		case 5:
			info->name			= "Cold Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The saturation of the cold farba.";
			break;
		case 6:
			info->name			= "Cold Black Level";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The black level of the cold farba.";
			break;
		case 7:
			info->name			= "Cold White Level";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The white level of the cold farba.";
			break;
		case 8:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Accomodates sRGB model.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double d;
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
		d = (*(double *)param) * 360.0;
			if (instance->warm.chroma.angle			!= d) {
				instance->warm.chroma.angle			 = d;
				instance->changed[0]				 = 1;
			}
			break;
		case 1:
			if (instance->warm.chroma.saturation	!= *(double *)param) {
				instance->warm.chroma.saturation	 = *(double *)param;
				instance->changed[0]				 = 1;
			}
			break;
		case 2:
			if (instance->warm.chroma.black			!= *(double *)param) {
				instance->warm.chroma.black			 = *(double *)param;
				instance->changed[0]				 = 1;
			}
			break;
		case 3:
			if (instance->warm.chroma.white			!= *(double *)param) {
				instance->warm.chroma.white			 = *(double *)param;
				instance->changed[0]				 = 1;
			}
			break;
		case 4:
		d = (*(double *)param) * 360.0;
			if (instance->cold.angle				!= d) {
				instance->cold.angle				 = d;
				instance->changed[1]				 = 1;
			}
			break;
		case 5:
			if (instance->cold.saturation			!= *(double *)param) {
				instance->cold.saturation			 = *(double *)param;
				instance->changed[1]				 = 1;
			}
			break;
		case 6:
			if (instance->cold.black				!= *(double *)param) {
				instance->cold.black				 = *(double *)param;
				instance->changed[1]				 = 1;
			}
			break;
		case 7:
			if (instance->cold.white				!= *(double *)param) {
				instance->cold.white				 = *(double *)param;
				instance->changed[1]				 = 1;
			}
			break;
		case 8:
			instance->srgb				 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->warm.chroma.angle;
			break;
		case 1:
			*(double *)param = (double)instance->warm.chroma.saturation;
			break;
		case 2:
			*(double *)param = (double)instance->warm.chroma.black;
			break;
		case 3:
			*(double *)param = (double)instance->warm.chroma.white;
			break;
		case 4:
			*(double *)param = (double)instance->cold.angle;
			break;
		case 5:
			*(double *)param = (double)instance->cold.saturation;
			break;
		case 6:
			*(double *)param = (double)instance->cold.black;
			break;
		case 7:
			*(double *)param = (double)instance->cold.white;
			break;
		case 8:
			*(double *)param = (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if ((instance->changed[0]) || (instance->changed[1])) {
			KOLIBA_SLUT sl;

			if (instance->changed[0]) {
				KOLIBA_ConvertChromatToSlut(&sl, &instance->warm);
				KOLIBA_SlutSelect(&instance->sLut, &sl, KOLIBA_Fundmal(KOLIBA_MalletWarm));
				instance->changed[0] = 0;
			}

			if (instance->changed[1]) {
				KOLIBA_ConvertChromaMatrixToSlut(&sl, &instance->cold, &instance->warm.model);
				KOLIBA_SlutSelect(&instance->sLut, &sl, KOLIBA_FundamentalMalletFlags[KOLIBA_MalletCold]);
				instance->changed[1] = 0;
			}

			instance->copy	= KOLIBA_IsIdentityFlut(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vert));
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
