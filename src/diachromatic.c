/*
	diachromatic.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the diachromatic effects in Koliba. It needs
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

typedef	struct _diachromatic_instance {
	KOLIBA_DIACHROMA	diachroma;
	KOLIBA_FLUT			fLut;
	double				angle[3];	// In turns
	double				rotation;	// In turns
	size_t				count;
	unsigned char		normalize[4];
	unsigned char		srgb;
	unsigned char		changed;
} diachromatic_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Diachromatic";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 22;
	info->explanation		= "Create a diachromatic image.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(diachromatic_instance))) != NULL) {
		instance->count					= (size_t)width * (size_t)height;

		// We will use the default (Rec. 2020) chroma model.
		// We only need to do this once.
		KOLIBA_SetChromaModel(&instance->diachroma.model, NULL);

		// We start with default chromas.
		KOLIBA_ResetChroma((KOLIBA_CHROMA *)&instance->diachroma.chr[0]);
		KOLIBA_ResetChroma((KOLIBA_CHROMA *)&instance->diachroma.chr[1]);
		KOLIBA_ResetChroma((KOLIBA_CHROMA *)&instance->diachroma.chr[2]);

		// We keep separate variables for the angles because some
		// frei0r hosts only allow it to be in the 0-1 range, while
		// Koliba expects the angles in degrees.
		instance->angle[0]					= 0.1;
		instance->diachroma.chr[0].angle	= 0.1 * 360.0;
		instance->angle[1]					= 0.75;
		instance->diachroma.chr[1].angle	= 0.75 * 360.0;
		instance->angle[2]					= 0.35;
		instance->diachroma.chr[2].angle	= 0.35 * 360.0;
		instance->rotation					= 0.0;

		// We start with full efficacy, and normalized.
		instance->diachroma.efficacy		= 1.0;
		instance->normalize[3]				= 1;
		instance->srgb						= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		// We start with it set true because our matrix
		// is still not initialized.
		instance->changed					= 1;
	}
	return instance;
}

void f0r_destruct(f0r_instance_t instance) {
	if (instance != NULL) free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
	switch (param_index) {
		case 0:
			info->name			= "Red Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the chroma rotation angle of the red channel.";
			break;
		case 1:
			info->name			= "Green Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the chroma rotation angle of the green channel.";
			break;
		case 2:
			info->name			= "Blue Angle";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the chroma rotation angle of the blue channel.";
			break;
		case 3:
			info->name			= "Red Magnitude";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how strong the red channel rotation effect is.";
			break;
		case 4:
			info->name			= "Green Magnitude";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how strong the green channel rotation effect is.";
			break;
		case 5:
			info->name			= "Blue Magnitude";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines how strong the blue channel rotation effect is.";
			break;
		case 6:
			info->name			= "Red Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the saturation of the red channel.";
			break;
		case 7:
			info->name			= "Green Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the saturation of the green channel.";
			break;
		case 8:
			info->name			= "Blue Saturation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the saturation of the blue channel.";
			break;
		case 9:
			info->name			= "Red Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the shadows of the red channel.";
			break;
		case 10:
			info->name			= "Green Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the shadows of the green channel.";
			break;
		case 11:
			info->name			= "Blue Black";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the shadows of the blue channel.";
			break;
		case 12:
			info->name			= "Red White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the highlights of the red channel.";
			break;
		case 13:
			info->name			= "Green White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the highlights of the green channel.";
			break;
		case 14:
			info->name			= "Blue White";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets the highlights of the blue channel.";
			break;
		case 15:
			info->name			= "Rotation";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Overall rotation angle.";
			break;
		case 16:
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Determines the strength vs. subtlety of the effect.";
			break;
		case 17:
			info->name			= "Normalize Red Channel";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "This may help keep the brightness. Or not.";
			break;
		case 18:
			info->name			= "Normalize Green Channel";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "This may help keep the brightness. Or not.";
			break;
		case 19:
			info->name			= "Normalize Blue Channel";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "This may help keep the brightness. Or not.";
			break;
		case 20:
			info->name			= "Normalize All Channels";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "This may help keep the brightness. Or not.";
			break;
		case 21:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	double d;
	unsigned int b;

	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->angle[0]						!= *(double *)param) {
				instance->angle[0]						 = *(double *)param;
				instance->diachroma.chr[0].angle		 = *(double *)param * 360.0;
				instance->changed						 = 1;
			}
			break;
		case 1:
			if (instance->angle[1]						!= *(double *)param) {
				instance->angle[1]						 = *(double *)param;
				instance->diachroma.chr[1].angle		 = *(double *)param * 360.0;
				instance->changed						 = 1;
			}
			break;
		case 2:
			if (instance->angle[2]						!= *(double *)param) {
				instance->angle[2]						 = *(double *)param;
				instance->diachroma.chr[2].angle		 = *(double *)param * 360.0;
				instance->changed						 = 1;
			}
			break;
		case 3:
			if (instance->diachroma.chr[0].magnitude	!= *(double *)param) {
				instance->diachroma.chr[0].magnitude	 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 4:
			if (instance->diachroma.chr[1].magnitude	!= *(double *)param) {
				instance->diachroma.chr[1].magnitude	 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 5:
			if (instance->diachroma.chr[2].magnitude	!= *(double *)param) {
				instance->diachroma.chr[2].magnitude	 = *(double *)param;
				instance->changed						 = 1;
			}
			break;
		case 6:
			if (instance->diachroma.chr[0].saturation	!= *(double *)param) {
				instance->diachroma.chr[0].saturation	 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 7:
			if (instance->diachroma.chr[1].saturation	!= *(double *)param) {
				instance->diachroma.chr[1].saturation	 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 8:
			if (instance->diachroma.chr[2].saturation	!= *(double *)param) {
				instance->diachroma.chr[2].saturation	 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 9:
			if (instance->diachroma.chr[0].black		!= *(double *)param) {
				instance->diachroma.chr[0].black		 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 10:
			if (instance->diachroma.chr[1].black		!= *(double *)param) {
				instance->diachroma.chr[1].black		 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 11:
			if (instance->diachroma.chr[2].black		!= *(double *)param) {
				instance->diachroma.chr[2].black		 = *(double *)param;
				instance->changed	 					 = 1;
			}
			break;
		case 12:
			if (instance->diachroma.chr[0].white		!= *(double *)param) {
				instance->diachroma.chr[0].white		 = *(double *)param;
				instance->changed			 			 = 1;
			}
			break;
		case 13:
			if (instance->diachroma.chr[1].white		!= *(double *)param) {
				instance->diachroma.chr[1].white		 = *(double *)param;
				instance->changed			 			 = 1;
			}
			break;
		case 14:
			if (instance->diachroma.chr[2].white		!= *(double *)param) {
				instance->diachroma.chr[2].white		 = *(double *)param;
				instance->changed			 			 = 1;
			}
			break;
		case 15:
			if (instance->rotation						!= *(double *)param) {
				instance->rotation						 = *(double *)param;
				instance->diachroma.rotation			 = *(double *)param * 360.0;
				instance->changed						 = 1;
			}
			break;
		case 16:
			if (instance->diachroma.efficacy			!= *(double *)param) {
				instance->diachroma.efficacy			 = *(double *)param;
				instance->changed				 		 = 1;
			}
			break;
		case 17:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->normalize[0]					!= b) {
				instance->normalize[0]					 = b;
				instance->changed				 		 = 1;
			}
			break;
		case 18:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->normalize[1]					!= b) {
				instance->normalize[1]					 = b;
				instance->changed				 		 = 1;
			}
			break;
		case 19:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->normalize[2]					!= b) {
				instance->normalize[2]					 = b;
				instance->changed				 		 = 1;
			}
			break;
		case 20:
			d											 = *(double *)param;
			b											 = (d >= 0.5);
			if (instance->normalize[3]					!= b) {
				instance->normalize[3]					 = b;
				instance->changed				 		 = 1;
			}
			break;
		case 21:
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
		case 1:
		case 2:
			*(double *)param				= instance->angle[param_index];
			break;
		case 3:
		case 4:
		case 5:
			*(double *)param				= instance->diachroma.chr[param_index-3].magnitude;
			break;
		case 6:
		case 7:
		case 8:
			*(double *)param				= instance->diachroma.chr[param_index-6].saturation;
			break;
		case 9:
		case 10:
		case 11:
			*(double *)param				= instance->diachroma.chr[param_index-9].black;
			break;
		case 12:
		case 13:
		case 14:
			*(double *)param				= instance->diachroma.chr[param_index-12].white;
			break;
		case 15:
			*(double *)param				= instance->rotation;
			break;
		case 16:
			*(double *)param				= instance->diachroma.efficacy;
			break;
		case 17:
		case 18:
		case 19:
		case 20:
			*(double *)param				= (double)instance->normalize[param_index-17];
			break;
		case 21:
			*(double *)param				= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->diachroma.efficacy == 0.0) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->changed) {
				unsigned int n;

				if (instance->normalize[3]) n = NORMALIZE_ALL;
				else n = instance->normalize[0] | (instance->normalize[1] << 1) | (instance->normalize[2] << 2);
				KOLIBA_ConvertDiachromaticMatrixToFlut(&instance->fLut, &instance->diachroma, n);
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

		for (; i; i--, inframe++, outframe++) {
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, KOLIBA_MatrixFlutFlags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}
