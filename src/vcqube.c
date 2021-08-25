/*
	vcqube.c

	Copyright 2019 G. Adam Stanislav.
	All rights reserved.

	http://www.pantarheon.org

	This is a frei0r-compatible plug-in, demonstrating
	the use of fLuttter and chaining in Koliba. It also
	illustrates how changing the FLUT flags affects the
	result of an effect. It needs to be linked dynamically
	using the -lkoliba switch in Unix and its derivatives,
	or koliba.lib in Windows.
*/

#include	<koliba.h>
#include	<stdlib.h>
#include	<stdint.h>

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

typedef	struct _vcchain_instance {
	KOLIBA_MALLET	mallet[2];
	KOLIBA_SLUT		sLut;
	KOLIBA_VERTICES	vert;
	KOLIBA_FLUT		fLut[2];
	KOLIBA_FFLUT	fChain[3];
	double			con[2];
	double			efficacy;
	size_t			count;
	unsigned char	srgb;
	unsigned char	matricize;
	unsigned char	changed;
} vcchain_instance, *f0r_instance_t;

typedef	void	*f0r_param_t;

static const uint64_t iLut[8][3] = {
        { 0xBFCBDA429201160C, 0xBFC2277D0B50C4B0, 0xBFB7B71F898B9BC8 },
        { 0x3FF59ACF764D0386, 0x3FEC4C7566BC5456, 0x3FDC0964EE18B3E2 },
        { 0xBFE18F6C01342D31, 0x3FB82E1EC9E5ABD8, 0x3F5A31CEFB002300 },
        { 0x3FE7816CA2753200, 0x3FCD9AC7592EBA67, 0x3FB0B48C4CB4AE78 },
        { 0xBFC2A4933CC35188, 0xBFD979BAEA8019B4, 0x3FD064A309E04DB8 },
        { 0xC000472216B71C49, 0xBFFB5D4DB9B419D6, 0xBFE02F5A07AD040E },
        { 0x3FF46A95C655B49C, 0xBFBF715A1C414E76, 0xBFF268803DECD7F9 },
        { 0x3FF340FEE2ED9E61, 0x40038E27810AE273, 0x3FFD7A2FE42B0897 }
};

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
	info->name				= "Koliba VC Qube";
	info->author			= "G. Adam Stanislav";
	info->plugin_type		= F0R_PLUGIN_TYPE_FILTER;
	info->color_model		= F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version	= FREI0R_MAJOR_VERSION;
	info->major_version		= 1;
	info->minor_version		= 0;
	info->num_params		= 5;
	info->explanation		= "Qubes Vampyrectomy and Chromatomorphosis.";
}

int f0r_init() {
	return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
	f0r_instance_t	instance;

	if ((instance = malloc(sizeof(vcchain_instance))) != NULL) {
		KOLIBA_InitializeMallet(instance->mallet, KOLIBA_SLUTPRIMARY);
		KOLIBA_InitializeMallet(instance->mallet+1, KOLIBA_SLUTSECONDARY);

		instance->count					= (size_t)width * (size_t)height;
		instance->fChain[0].fLut		= (KOLIBA_FLUT *)iLut;
		instance->fChain[0].flags		= KOLIBA_AllFlutFlags;
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
		instance->efficacy				= 1.0;
		instance->srgb					= 1;
		instance->matricize				= 0;
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
			info->name			= "Vampyrectomy Efficacy";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Sets how strong or subtle the vampyrectomy effect is to be.";
			break;
		case 1:
			info->name			= "Primary Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Primary color contrast adjustment.";
			break;
		case 2:
			info->name			= "Secondary Contrast";
			info->type			= F0R_PARAM_DOUBLE;
			info->explanation	= "Secondary color contrast adjustment.";
			break;
		case 3:
			info->name			= "Matricize";
			info->type			= F0R_PARAM_BOOL;
			info->explanation	= "Throws out non-matrix portions of the effect.";
			break;
		case 4:
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
			if (instance->efficacy				!= *(double *)param) {
				instance->efficacy				 = *(double *)param;
				instance->changed				 = 1;
			}
			break;
		case 1:
			if (instance->con[0]				!= *(double *)param) {
				instance->con[0]				 = *(double *)param;
				instance->mallet[0].adjustment	 = 0.292517 * (*(double *)param);
				instance->changed				 = 1;
			}
			break;
		case 2:
			if (instance->con[1]				!= *(double *)param) {
				instance->con[1]				 = *(double *)param;
				instance->mallet[1].adjustment	 = 0.442177 * (*(double *)param);
				instance->changed				 = 1;
			}
			break;
		case 3:
			if ((b = (*(double *)param >= 0.5)) != instance->matricize) {
				instance->matricize				 = b;
				instance->changed				 = 1;
			}
			break;
		case 4:
			instance->srgb						 = (*(double *)param >= 0.5);
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
	if ((instance != NULL) && (param != NULL)) switch(param_index) {
		case 0:
			*(double *)param = (double)instance->efficacy;
			break;
		case 1:
			*(double *)param = (double)instance->con[0];
			break;
		case 2:
			*(double *)param = (double)instance->con[1];
			break;
		case 3:
			*(double *)param = (double)instance->matricize;
			break;
		case 4:
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
			KOLIBA_FlutEfficacy(&(instance->fLut[0]), (KOLIBA_FLUT *)&iLut, instance->efficacy);
			KOLIBA_ConvertSlutToFlut(&(instance->fLut[1]), &instance->vert);
			KOLIBA_Flutter(&(instance->fLut[0]), &(instance->fLut[1]), &(instance->fLut[0]));
			instance->fChain[1].fLut = &(instance->fLut[1]);
			instance->fChain[2].fLut = &instance->fLut[0];
			if (instance->matricize == 0) {
				instance->fChain[0].flags	= KOLIBA_AllFlutFlags;
				instance->fChain[1].flags	= KOLIBA_FlutFlags(instance->fChain[1].fLut);
				instance->fChain[2].flags	= KOLIBA_FlutFlags(instance->fChain[2].fLut);
			}
			else {
				instance->fChain[0].flags	= KOLIBA_MatrixFlutFlags;
				instance->fChain[1].flags	= KOLIBA_MatrixFlutFlags;
				instance->fChain[2].flags	= KOLIBA_MatrixFlutFlags;
			}
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
			KOLIBA_PolyRgba8Pixel(outframe, inframe, instance->fChain, 3, iconv, oconv)->a = inframe->a;
		}
	}
}
