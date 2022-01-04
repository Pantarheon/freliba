/*
	palette.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the palette effects in Koliba. It needs
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
#define F0R_PARAM_COLOR     2
/* End of frei0r.h extract */

typedef	struct _palette_instance {
	KOLIBA_SLUT		sLut;
	KOLIBA_PALETTE	palette;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT	fLut;
	unsigned int	count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
	unsigned char	copy;
} palette_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Palette";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 19;
	info->explanation		= "Change the pallete of the image.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(palette_instance))) != NULL) {
		instance->count		= width * height;
		
		// We start with a default palette that does nothing.
		KOLIBA_ResetPalette(&instance->palette);

		// We only need to initialize the pointers to the vertices once
		// because their addresses within an instance never change.
		KOLIBA_SlutToVertices(&instance->vert, &instance->sLut);

		// As much as it pains me, we should probably start
		// assuming we are being used in an sRGB environment.
		instance->srgb		= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		instance->changed	= 0;

		// We start with all defaults, so we can just copy
		// the input to the output for now. Thanks to that,
		// we do not have to reset the SLUT, the FLUT, or
		// assign an initial value to the FLAGS.
		instance->copy		= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Black Svit";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the shadows.";
			break;
		case 1:
			info->name			= "Black Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the black svit.";
			break;
		case 2:
			info->name			= "White Svit";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the highlights.";
			break;
		case 3:
			info->name			= "White Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the white svit.";
			break;
		case 4:
			info->name			= "Red Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the red ink.";
			break;
		case 5:
			info->name			= "Red Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the red farba.";
			break;
		case 6:
			info->name			= "Green Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the green ink.";
			break;
		case 7:
			info->name			= "Green Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the green farba.";
			break;
		case 8:
			info->name			= "Blue Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the blue ink.";
			break;
		case 9:
			info->name			= "Blue Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the blue farba.";
			break;
		case 10:
			info->name			= "Cyan Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the cyan ink.";
			break;
		case 11:
			info->name			= "Cyan Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the cyan farba.";
			break;
		case 12:
			info->name			= "Magenta Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the magenta ink.";
			break;
		case 13:
			info->name			= "Magenta Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the magenta farba.";
			break;
		case 14:
			info->name			= "Yellow Farba";
			info->type			= F0R_PARAM_COLOR;
			info->explanation	= "Controls the yellow ink.";
			break;
		case 15:
			info->name			= "Yellow Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Controls the efficacy of the yellow farba.";
			break;
		case 16:
			info->name			= "Palette Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the overall strength vs. subtlety of the effect.";
			break;
		case 17:
			info->name			= "Erythropy";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Determines whether to apply erythropy.";
			break;
		case 18:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use this if running in an sRGB environment.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, const f0r_param_t param, int param_index) {
	double d;
	unsigned char b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Black, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Black, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 1:
			if (instance->palette.Black.efficacy		!= *(double *)param) {
				instance->palette.Black.efficacy		 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 2:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.White, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.White, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 3:
			if (instance->palette.White.efficacy		!= *(double *)param) {
				instance->palette.White.efficacy		 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 4:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Red, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Red, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 5:
			if (instance->palette.Red.efficacy			!= *(double *)param) {
				instance->palette.Red.efficacy			 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 6:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Green, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Green, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 7:
			if (instance->palette.Green.efficacy		!= *(double *)param) {
				instance->palette.Green.efficacy		 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 8:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Blue, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Blue, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 9:
			if (instance->palette.Blue.efficacy			!= *(double *)param) {
				instance->palette.Blue.efficacy			 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 10:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Cyan, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Cyan, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 11:
			if (instance->palette.Cyan.efficacy			!= *(double *)param) {
				instance->palette.Cyan.efficacy			 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 12:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Magenta, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Magenta, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 13:
			if (instance->palette.Magenta.efficacy		!= *(double *)param) {
				instance->palette.Magenta.efficacy		 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 14:
			if (!KOLIBA_PixelIsVertex((KOLIBA_VERTEX *)&instance->palette.Yellow, (KOLIBA_PIXEL *)param)) {
				KOLIBA_PixelToVertex((KOLIBA_VERTEX *)&instance->palette.Yellow, (KOLIBA_PIXEL *)param, 1);
				instance->changed						 = 1;
			}
			break;
		case 15:
			if (instance->palette.Yellow.efficacy		!= *(double *)param) {
				instance->palette.Yellow.efficacy		 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 16:
			if (instance->palette.efficacy				!= *(double *)param) {
				instance->palette.efficacy				 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 17:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->palette.erythropy				!= b) {
				instance->palette.erythropy				 = b;
				instance->changed				 		 = 1;
			}
			break;
		case 18:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->srgb							!= b) {
				instance->srgb							 = b;
			}
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Black, 1);
			break;
		case 1:
			*(double *)param				= instance->palette.Black.efficacy;
			break;
		case 2:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.White, 1);
			break;
		case 3:
			*(double *)param				= instance->palette.White.efficacy;
			break;
		case 4:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Red, 1);
			break;
		case 5:
			*(double *)param				= instance->palette.Red.efficacy;
			break;
		case 6:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Green, 1);
			break;
		case 7:
			*(double *)param				= instance->palette.Green.efficacy;
			break;
		case 8:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Blue, 1);
			break;
		case 9:
			*(double *)param				= instance->palette.Blue.efficacy;
			break;
		case 10:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Cyan, 1);
			break;
		case 11:
			*(double *)param				= instance->palette.Cyan.efficacy;
			break;
		case 12:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Magenta, 1);
			break;
		case 13:
			*(double *)param				= instance->palette.Magenta.efficacy;
			break;
		case 14:
			KOLIBA_VertexToPixel((KOLIBA_PIXEL *)param, (KOLIBA_VERTEX *)&instance->palette.Yellow, 1);
			break;
		case 15:
			*(double *)param				= instance->palette.Yellow.efficacy;
			break;
		case 16:
			*(double *)param				= instance->palette.efficacy;
			break;
		case 17:
			*(double *)param				= (double)instance->palette.erythropy;
			break;
		case 18:
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
			KOLIBA_ConvertPaletteToSlut(&instance->sLut, &instance->palette);
			instance->copy		= KOLIBA_IsIdentityFlut(KOLIBA_ConvertSlutToFlut(&instance->fLut, &instance->vert));
			instance->flags		= KOLIBA_FlutFlags(&instance->fLut);
			instance->changed	= 0;
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
