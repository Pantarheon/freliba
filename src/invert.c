/*
	invert.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	inverting sLut vertices in Koliba.

	It needs to be linked dynamically using the -lkoliba switch
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

typedef	struct _invert_instance {
	KOLIBA_FLUT		fLut;
	KOLIBA_SLUT		sLut;
	KOLIBA_EFFILUT	eLut;
	KOLIBA_VERTICES	vertices;
	KOLIBA_FLAGS	flags;
	size_t			count;
	unsigned char	changed;
	unsigned char	srgb;
	unsigned char	iflags;
} invert_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Invert";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 17;
	info->explanation		= "Selectively invert vertices.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(invert_instance),1)) != NULL) {
		instance->count		= (size_t)width * (size_t)height;

		KOLIBA_SlutToVertices(&instance->vertices, &instance->sLut);
		KOLIBA_SetEfficacies(&instance->eLut, 1.0);

		instance->srgb		= 1;
		instance->iflags	= KOLIBA_SLUTALL;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
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
			info->name			= "Invert Black";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the black vertex.";
			break;
		case 1:
			info->name			= "Black Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the black vertex.";
			break;
		case 2:
			info->name			= "Invert White";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the white vertex.";
			break;
		case 3:
			info->name			= "White Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the white vertex.";
			break;
		case 4:
			info->name			= "Invert Red";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the red vertex.";
			break;
		case 5:
			info->name			= "Red Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the red vertex.";
			break;
		case 6:
			info->name			= "Invert Green";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the green vertex.";
			break;
		case 7:
			info->name			= "Green Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the green vertex.";
			break;
		case 8:
			info->name			= "Invert Blue";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the blue vertex.";
			break;
		case 9:
			info->name			= "Blue Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the blue vertex.";
			break;
		case 10:
			info->name			= "Invert Cyan";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the cyan vertex.";
			break;
		case 11:
			info->name			= "Cyan Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the cyan vertex.";
			break;
		case 12:
			info->name			= "Invert Magenta";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the magenta vertex.";
			break;
		case 13:
			info->name			= "Magenta Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the magenta vertex.";
			break;
		case 14:
			info->name			= "Invert Yellow";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Invert the yellow vertex.";
			break;
		case 15:
			info->name			= "Yellow Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to invert the yellow vertex.";
			break;
		case 16:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTBLACK;
			else instance->iflags &= ~KOLIBA_SLUTBLACK;
			instance->changed = 1;
			break;
		case 1:
			instance->eLut.black = *(double *)param;
			instance->changed = 1;
			break;
		case 2:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTWHITE;
			else instance->iflags &= ~KOLIBA_SLUTWHITE;
			instance->changed = 1;
			break;
		case 3:
			instance->eLut.white = *(double *)param;
			instance->changed = 1;
			break;
		case 4:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTRED;
			else instance->iflags &= ~KOLIBA_SLUTRED;
			instance->changed = 1;
			break;
		case 5:
			instance->eLut.red = *(double *)param;
			instance->changed = 1;
			break;
		case 6:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTGREEN;
			else instance->iflags &= ~KOLIBA_SLUTGREEN;
			instance->changed = 1;
			break;
		case 7:
			instance->eLut.green = *(double *)param;
			instance->changed = 1;
			break;
		case 8:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTBLUE;
			else instance->iflags &= ~KOLIBA_SLUTBLUE;
			instance->changed = 1;
			break;
		case 9:
			instance->eLut.blue = *(double *)param;
			instance->changed = 1;
			break;
		case 10:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTCYAN;
			else instance->iflags &= ~KOLIBA_SLUTCYAN;
			instance->changed = 1;
			break;
		case 11:
			instance->eLut.cyan = *(double *)param;
			instance->changed = 1;
			break;
		case 12:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTMAGENTA;
			else instance->iflags &= ~KOLIBA_SLUTMAGENTA;
			instance->changed = 1;
			break;
		case 13:
			instance->eLut.magenta = *(double *)param;
			instance->changed = 1;
			break;
		case 14:
			if (*(double *)param > 0.5) instance->iflags |= KOLIBA_SLUTYELLOW;
			else instance->iflags &= ~KOLIBA_SLUTYELLOW;
			instance->changed = 1;
			break;
		case 15:
			instance->eLut.yellow = *(double *)param;
			instance->changed = 1;
			break;
		case 16:
				instance->srgb = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTBLACK) != 0);
			break;
		case 1:
			*(double *)param = instance->eLut.black;
			break;
		case 2:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTWHITE) != 0);
			break;
		case 3:
			*(double *)param = instance->eLut.white;
			break;
		case 4:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTRED) != 0);
			break;
		case 5:
			*(double *)param = instance->eLut.red;
			break;
		case 6:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTGREEN) != 0);
			break;
		case 7:
			*(double *)param = instance->eLut.green;
			break;
		case 8:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTBLUE) != 0);
			break;
		case 9:
			*(double *)param = instance->eLut.blue;
			break;
		case 10:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTCYAN) != 0);
			break;
		case 11:
			*(double *)param = instance->eLut.cyan;
			break;
		case 12:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTMAGENTA) != 0);
			break;
		case 13:
			*(double *)param = instance->eLut.magenta;
			break;
		case 14:
			*(double *)param = (double)((instance->iflags & KOLIBA_SLUTYELLOW) != 0);
			break;
		case 15:
			*(double *)param = instance->eLut.yellow;
			break;
		case 16:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->changed) {
			KOLIBA_ApplyEfficacies(&instance->sLut, KOLIBA_InvertSlutVertices(KOLIBA_ResetSlut(&instance->sLut), instance->iflags), &instance->eLut, &KOLIBA_IdentitySlut);
			instance->flags = KOLIBA_FlutFlags(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vertices));
			instance->changed = 0;
		}

		if (KOLIBA_IsIdentityFlut(&instance->fLut)) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
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
