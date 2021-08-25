/*
	vampyrectomy.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of the using fLuts in Koliba. It needs
	to be linked dynamically using the -lkoliba switch
	in Unix and its derivatives, or koliba.lib in Windows.
*/

#define	KOLIBCALLS
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

typedef	struct _vampyrectomy_instance {
	KOLIBA_FLUT		fLut;
	double			efficacy;
	size_t			count;
	KOLIBA_FLAGS	flags;
	unsigned char	srgb;
	unsigned char	changed;
} vampyrectomy_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

// Strangely, the C language has no direct way of assigning doubles
// directly by entering their binary/hexadecimal values, so we have to
// lie to the compiler and pretend this is an array of unsigned long long
// integers, and later down in the code we have to typecast it as the
// KOLIBA_FLUT structure of 24 doubles that it actually is.
//
// The usual alternative would be to convert these values from binary
// to decimal and type the decimal values and have the compiler convert
// them back to binary. But that way we would lose some precision both
// when converting them to decimal and when the compiler would be converting
// them back to binary. Which is unacceptable.
static const unsigned long long iLut[8][3] = {
        { 0xBFCBDA429201160C, 0xBFC2277D0B50C4B0, 0xBFB7B71F898B9BC8 },
        { 0x3FF59ACF764D0386, 0x3FEC4C7566BC5456, 0x3FDC0964EE18B3E2 },
        { 0xBFE18F6C01342D31, 0x3FB82E1EC9E5ABD8, 0x3F5A31CEFB002300 },
        { 0x3FE7816CA2753200, 0x3FCD9AC7592EBA67, 0x3FB0B48C4CB4AE78 },
        { 0xBFC2A4933CC35188, 0xBFD979BAEA8019B4, 0x3FD064A309E04DB8 },
        { 0xC000472216B71C49, 0xBFFB5D4DB9B419D6, 0xBFE02F5A07AD040E },
        { 0x3FF46A95C655B49C, 0xBFBF715A1C414E76, 0xBFF268803DECD7F9 },
        { 0x3FF340FEE2ED9E61, 0x40038E27810AE273, 0x3FFD7A2FE42B0897 }
};

typedef	void	*f0r_param_t;

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba Vampyrectomy";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 2;
	info->explanation		= "Emulate vampire vision.";
}

int f0r_init() {return 1;}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(vampyrectomy_instance))) != NULL) {
		instance->count		= (size_t)width * (size_t)height;

		// We start with full efficacy.
		instance->efficacy	= 1.0;
		instance->srgb		= 1;

		// Set this to 1, whenever a parameter changes.
		// Set it back to 0 after recalculating sLut and fn.
		instance->changed	= 1;
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
			info->name			= "Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets how strong or subtle the effect is to be.";
			break;
		case 1:
			info->name			= "sRGB";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Use with the sRGB color space.";
			break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch (param_index) {
		case 0:
			if (instance->efficacy	!= *(double *)param) {
				instance->efficacy	 = *(double *)param;
				instance->changed	 = 1;
			}
			break;
		case 1:
			instance->srgb			 = ((*(double *)param) >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param		= instance->efficacy;
			break;
		case 1:
			*(double *)param		= (double)instance->srgb;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const KOLIBA_RGBA8PIXEL *inframe, KOLIBA_RGBA8PIXEL *outframe) {
	if ((instance != NULL) && (inframe != NULL) && (outframe != NULL)) {
		size_t i = instance->count;
		const double *iconv;
		const unsigned char *oconv;

		if (instance->efficacy == 0.0) memcpy(outframe, inframe, i*sizeof(KOLIBA_RGBA8PIXEL));
		else {
			if (instance->changed) {
				KOLIBA_FlutEfficacy(&instance->fLut, (KOLIBA_FLUT *)&iLut, instance->efficacy);
				instance->flags = KOLIBA_FlutFlags(&instance->fLut);
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
				KOLIBA_Rgba8Pixel(outframe, inframe, &instance->fLut, instance->flags, iconv, oconv)->a = inframe->a;
			}
		}
	}
}

