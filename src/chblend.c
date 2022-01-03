/*
	chblend.c

	Copyright 2019-2022 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the Channel Blend use in Koliba.

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

typedef	struct _chblend_instance {
	KOLIBA_FLUT		fLut;
	KOLIBA_CHANNELBLEND blend;
	size_t			count;
	unsigned char	changed;
	unsigned char	srgb;
} chblend_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Channel Blend";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 18;
	info->explanation		= "Adjust the channel blend.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = calloc(sizeof(chblend_instance),1)) != NULL) {
		instance->count		= (size_t)width * (size_t)height;

		KOLIBA_ResetChannelBlend(&instance->blend);
		instance->srgb		= 1;

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
			info->name			= "Red red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much red to mix in.";
			break;
		case 1:
			info->name			= "Red green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much green to mix in.";
			break;
		case 2:
			info->name			= "Red blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much blue to mix in.";
			break;
		case 3:
			info->name			= "Red offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to add to the mix.";
			break;
		case 4:
			info->name			= "Normalize red";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Normalize the values to add up to 1.";
			break;
		case 5:
			info->name			= "Green red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much red to mix in.";
			break;
		case 6:
			info->name			= "Green green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much green to mix in.";
			break;
		case 7:
			info->name			= "Green blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much blue to mix in.";
			break;
		case 8:
			info->name			= "Green offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to add to the mix.";
			break;
		case 9:
			info->name			= "Normalize green";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Normalize the values to add up to 1.";
			break;
		case 10:
			info->name			= "Blue red";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much red to mix in.";
			break;
		case 11:
			info->name			= "Blue green";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much green to mix in.";
			break;
		case 12:
			info->name			= "Blue blue";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much blue to mix in.";
			break;
		case 13:
			info->name			= "Blue offset";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "How much to add to the mix.";
			break;
		case 14:
			info->name			= "Normalize blue";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Normalize the values to add up to 1.";
			break;
		case 15:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "The strength of the effect.";
			break;
		case 16:
			info->name			= "Normalize all";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Normalize all channels.";
			break;
		case 17:
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
			if (instance->blend.mat.Red.r	!= *(double *)param) {
				instance->blend.mat.Red.r	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 1:
			if (instance->blend.mat.Red.g	!= *(double *)param) {
				instance->blend.mat.Red.g	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 2:
			if (instance->blend.mat.Red.b	!= *(double *)param) {
				instance->blend.mat.Red.b	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 3:
			if (instance->blend.mat.Red.o	!= *(double *)param) {
				instance->blend.mat.Red.o	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 4:
			b								 = (*(double *)param >= 0.5);
			if (instance->blend.nr			!= b) {
				instance->blend.nr			 = b;
				instance->changed			 = 1;
			}
			break;
		case 5:
			if (instance->blend.mat.Green.r	!= *(double *)param) {
				instance->blend.mat.Green.r	 = *(double *)param;
				instance->changed			 = 1;
			}
		case 6:
			if (instance->blend.mat.Green.g	!= *(double *)param) {
				instance->blend.mat.Green.g	 = *(double *)param;
				instance->changed			 = 1;
			}
		case 7:
			if (instance->blend.mat.Green.b	!= *(double *)param) {
				instance->blend.mat.Green.b	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 8:
			if (instance->blend.mat.Green.o	!= *(double *)param) {
				instance->blend.mat.Green.o	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 9:
			b								 = (*(double *)param >= 0.5);
			if (instance->blend.ng			!= b) {
				instance->blend.ng			 = b;
				instance->changed			 = 1;
			}
			break;
		case 10:
			if (instance->blend.mat.Blue.r	!= *(double *)param) {
				instance->blend.mat.Blue.r	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 11:
			if (instance->blend.mat.Blue.g	!= *(double *)param) {
				instance->blend.mat.Blue.g	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 12:
			if (instance->blend.mat.Blue.b	!= *(double *)param) {
				instance->blend.mat.Blue.b	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 13:
			if (instance->blend.mat.Blue.o	!= *(double *)param) {
				instance->blend.mat.Blue.o	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 14:
			b								 = (*(double *)param >= 0.5);
			if (instance->blend.nb			!= b) {
				instance->blend.nb			 = b;
				instance->changed			 = 1;
			}
			break;
		case 15:
			if (instance->blend.efficacy	!= *(double *)param) {
				instance->blend.efficacy	 = *(double *)param;
				instance->changed			 = 1;
			}
			break;
		case 16:
			b								 = (*(double *)param >= 0.5);
			if (instance->blend.na			!= b) {
				instance->blend.na			 = b;
				instance->changed			 = 1;
			}
			break;
		case 17:
				instance->srgb				 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param				= instance->blend.mat.Red.r;
			break;
		case 1:
			*(double *)param				= instance->blend.mat.Red.g;
			break;
		case 2:
			*(double *)param				= instance->blend.mat.Red.b;
			break;
		case 3:
			*(double *)param				= instance->blend.mat.Red.o;
			break;
		case 4:
			*(double *)param				= instance->blend.nr;
			break;
		case 5:
			*(double *)param				= instance->blend.mat.Green.r;
			break;
		case 6:
			*(double *)param				= instance->blend.mat.Green.g;
			break;
		case 7:
			*(double *)param				= instance->blend.mat.Green.b;
			break;
		case 8:
			*(double *)param				= instance->blend.mat.Green.o;
			break;
		case 9:
			*(double *)param				= instance->blend.ng;
			break;
		case 10:
			*(double *)param				= instance->blend.mat.Blue.r;
			break;
		case 11:
			*(double *)param				= instance->blend.mat.Blue.g;
			break;
		case 12:
			*(double *)param				= instance->blend.mat.Blue.b;
			break;
		case 13:
			*(double *)param				= instance->blend.mat.Blue.o;
			break;
		case 14:
			*(double *)param				= instance->blend.nb;
			break;
		case 15:
			*(double *)param				= instance->blend.efficacy;
			break;
		case 16:
			*(double *)param				= instance->blend.na;
			break;
		case 17:
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
			KOLIBA_ConvertChannelBlendToFlut(&instance->fLut, &instance->blend);
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
			KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, KOLIBA_MatrixFlutFlags, iconv, oconv)->a = inframe->a;
		}
	}
}
