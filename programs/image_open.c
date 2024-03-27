/***************************************************************************
** intoPIX SA & Fraunhofer IIS (hereinafter the "Software Copyright       **
** Holder") hold or have the right to license copyright with respect to   **
** the accompanying software (hereinafter the "Software").                **
**                                                                        **
** Copyright License for Evaluation and Testing                           **
** --------------------------------------------                           **
**                                                                        **
** The Software Copyright Holder hereby grants, to any implementer of     **
** this ISO Standard, an irrevocable, non-exclusive, worldwide,           **
** royalty-free, sub-licensable copyright licence to prepare derivative   **
** works of (including translations, adaptations, alterations), the       **
** Software and reproduce, display, distribute and execute the Software   **
** and derivative works thereof, for the following limited purposes: (i)  **
** to evaluate the Software and any derivative works thereof for          **
** inclusion in its implementation of this ISO Standard, and (ii)         **
** to determine whether its implementation conforms with this ISO         **
** Standard.                                                              **
**                                                                        **
** The Software Copyright Holder represents and warrants that, to the     **
** best of its knowledge, it has the necessary copyright rights to        **
** license the Software pursuant to the terms and conditions set forth in **
** this option.                                                           **
**                                                                        **
** No patent licence is granted, nor is a patent licensing commitment     **
** made, by implication, estoppel or otherwise.                           **
**                                                                        **
** Disclaimer: Other than as expressly provided herein, (1) the Software  **
** is provided “AS IS” WITH NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING  **
** BUT NOT LIMITED TO, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A   **
** PARTICULAR PURPOSE AND NON-INFRINGMENT OF INTELLECTUAL PROPERTY RIGHTS **
** and (2) neither the Software Copyright Holder (or its affiliates) nor  **
** the ISO shall be held liable in any event for any damages whatsoever   **
** (including, without limitation, damages for loss of profits, business  **
** interruption, loss of information, or any other pecuniary loss)        **
** arising out of or related to the use of or inability to use the        **
** Software.”                                                             **
**                                                                        **
** RAND Copyright Licensing Commitment                                    **
** -----------------------------------                                    **
**                                                                        **
** IN THE EVENT YOU WISH TO INCLUDE THE SOFTWARE IN A CONFORMING          **
** IMPLEMENTATION OF THIS ISO STANDARD, PLEASE BE FURTHER ADVISED THAT:   **
**                                                                        **
** The Software Copyright Holder agrees to grant a copyright              **
** license on reasonable and non- discriminatory terms and conditions for **
** the purpose of including the Software in a conforming implementation   **
** of the ISO Standard. Negotiations with regard to the license are       **
** left to the parties concerned and are performed outside the ISO.       **
**                                                                        **
** No patent licence is granted, nor is a patent licensing commitment     **
** made, by implication, estoppel or otherwise.                           **
***************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image_open.h"
#include "converters/v210.h"
#include "converters/rgb16.h"
#include "converters/yuv16.h"
#include "converters/planar.h"
#include "converters/uyvy8.h"
#include "converters/argb.h"
#include "converters/mono.h"
#include "converters/ppm.h"
#include "converters/pgx.h"

static int str_endswith(const char* haystack, const char* needle)
{
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);
	if(nlen > hlen) return 0;
	return (strcmp(&haystack[hlen - nlen], needle)) == 0;
}

int image_open_auto(char* filename, xs_image_t* image, int width, int height, int depth)
{
	int ret = -1;
	image->width = width;
	image->height = height;
	image->depth = depth;
	if (str_endswith(filename, ".yuv16le") ||
		str_endswith(filename, ".yuv16"))
	{
		fprintf(stdout, "[image_open] yuv16 interleaved decoder little endian\n");
		ret = yuv16_decode(filename, image, 1);
	}
	else if (str_endswith(filename, ".yuv16be"))
	{
		fprintf(stdout, "[image_open] yuv16 interleaved decoder big endian\n");
		ret = yuv16_decode(filename, image, 0);
	}
	else if (str_endswith(filename, ".yuv") ||
			str_endswith(filename, ".yuv8p") ||
			str_endswith(filename, ".yuv16p") ||
			str_endswith(filename, ".yuv8_420p") ||
			str_endswith(filename, ".yuv16_420p") ||
			str_endswith(filename, ".yuv16_422p") ||
			str_endswith(filename, ".yuv16_444p"))
	{
		fprintf(stdout, "[image_open] yuv planar decoder (little endian)\n");
		ret = yuv_planar_decode(filename, image);
	}
	else if (str_endswith(filename, ".v210le") ||
		 	 str_endswith(filename, ".v210") ||
		 	 str_endswith(filename, ".yuv10le"))
	{
		fprintf(stdout, "[image_open] yuv10 decoder little endian\n");
		ret = v210_decode(filename, image, 1  );
	}
	else if (str_endswith(filename, ".v210be") ||
		 	 str_endswith(filename, ".yuv10be") ||
		 	 str_endswith(filename, ".yuv10"))
	{
		fprintf(stdout, "[image_open] yuv10 decoder big endian\n");
		ret = v210_decode(filename, image, 0  );
	}
	else if (str_endswith(filename, ".uyvy") ||
			 str_endswith(filename, ".uyvy8"))
	{
		fprintf(stdout, "[image_open] uyvy8 decoder\n");
		ret = uyvy8_decode(filename, image);
	}
	else if (str_endswith(filename, ".rgb16")   ||
			 str_endswith(filename, ".rgb16be") ||
		 	 str_endswith(filename, ".rgb16le"))
	{
		fprintf(stdout, "[image_open] Using raw rgb16 decoder\n");
		ret = rgb16_decode(filename, image, (!str_endswith(filename, "be")));
	}
	else if (str_endswith(filename, ".argb"))
	{
		fprintf(stdout, "[image_open] Using raw argb decoder\n");
		ret = argb_decode(filename, image);
	}
	else if (str_endswith(filename, ".mono8"))
	{
		fprintf(stdout, "[image_open] Using mono8 decoder\n");
		ret = raw_mono_decode(filename, image);
	}
    else if (str_endswith(filename, ".pgx"))
    {
        fprintf(stdout, "[image_open] Using pgx lib to parse input file\n");
        ret = pgx_decode(filename, image);
    }
	else if (str_endswith(filename, ".ppm") || str_endswith(filename, ".pgm"))
	{
		fprintf(stdout, "[image_open] Using ppm lib to parse input file\n");
		ret = ppm_decode(filename, image);
	}
	else
	{
		fprintf(stderr,"Please use ppm or yuv formats\n");
	}
	if (ret == 0)
	{
		fprintf(stdout, "[image_open] Image: %dx%d with bitdepth=%d\n", image->width, image->height, image->depth);
	}
	return ret;
}

int image_save_auto(char* filename, xs_image_t* image)
{
	int ret = -1;

	if (str_endswith(filename, ".yuv16le") ||
		str_endswith(filename, ".yuv16"))
	{
		ret = yuv16_encode(filename, image, 1);
	}
	else if (str_endswith(filename, ".yuv16be"))
	{
		ret = yuv16_encode(filename, image, 0);
	}
	else if(str_endswith(filename, ".yuv") ||
			str_endswith(filename, ".yuv8p") ||
			str_endswith(filename, ".yuv16p") ||
			str_endswith(filename, ".yuv8_420p") ||
			str_endswith(filename, ".yuv16_420p") ||
			str_endswith(filename, ".yuv16_422p") ||
			str_endswith(filename, ".yuv16_444p"))
	{
		int force_16bits = 0;
		if(str_endswith(filename, ".yuv16p") ||
		   str_endswith(filename, ".yuv16_420p") ||
		   str_endswith(filename, ".yuv16_422p") ||
		   str_endswith(filename, ".yuv16_444p"))
			force_16bits = 1;
		ret = planar_encode(filename, image, force_16bits);
	}
	else if (str_endswith(filename, ".v210le") ||
		 	 str_endswith(filename, ".v210") ||
		 	 str_endswith(filename, ".yuv10le"))
	{
		ret = v210_encode(filename, image, 1  );
	}
	else if (str_endswith(filename, ".v210be") ||
		 	 str_endswith(filename, ".yuv10be") ||
		 	 str_endswith(filename, ".yuv10"))
	{
		ret = v210_encode(filename, image, 0  );
	}
	else if (str_endswith(filename, ".uyvy") ||
			 str_endswith(filename, ".uyvy8"))
	{
		ret = uyvy8_encode(filename, image);
	}
	else if (str_endswith(filename, ".rgb16") ||
			 str_endswith(filename, ".rgb16be") ||
		 	 str_endswith(filename, ".rgb16le"))
	{
		ret = rgb16_encode(filename, image, !str_endswith(filename, "be"));
	}
	else if (str_endswith(filename, ".argb"))
	{
		ret = argb_encode(filename, image);
	}
	else if (str_endswith(filename, ".mono8"))
	{
		ret = raw_mono_encode(filename, image);
	}
	else if (str_endswith(filename, ".ppm") || str_endswith(filename, ".pgm"))
	{
		ret = ppm_encode(filename, image,0);
	}
    else if (str_endswith(filename, ".pgx"))
    {
        ret = pgx_encode(filename, image);
    }
	else
	{
		fprintf(stderr, "Error: Unknown output image format\n");
		return -1;
	}
	return ret;
}

// Just for nicer verbosity.
typedef struct image_format_t image_format_t;
struct image_format_t
{
	const char* const name;
	int ncomps;
	uint8_t sx[MAX_NCOMPS];
	uint8_t sy[MAX_NCOMPS];
	xs_cpih_t mct;
	xs_cfa_pattern_t cfa;
};

const image_format_t IMAGE_FORMATS[] =
{
	{ "MONO 4:0:0", 1, { 1, 0, 0, 0 }, { 1, 0, 0, 0 }, XS_CPIH_NONE, 0 },
	{ "4:4:4", 3, { 1, 1, 1, 0 }, { 1, 1, 1, 0 }, XS_CPIH_NONE, 0 },
	{ "RGB 4:4:4", 3, { 1, 1, 1, 0 }, { 1, 1, 1, 0 }, XS_CPIH_RCT, 0 },
	{ "RGBA 4:4:4:4", 4, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, XS_CPIH_RCT, 0 },
	{ "YUV 4:2:2", 3, { 1, 2, 2, 0 }, { 1, 1, 1, 0 }, XS_CPIH_NONE, 0 },
	{ "YUV 4:2:0", 3, { 1, 2, 2, 0 }, { 1, 2, 2, 0 }, XS_CPIH_NONE, 0 },
	{ "BAYER RGGB", 4, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, XS_CPIH_TETRIX, XS_CFA_RGGB },
	{ "BAYER BGGR", 4, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, XS_CPIH_TETRIX, XS_CFA_BGGR },
	{ "BAYER GRBG", 4, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, XS_CPIH_TETRIX, XS_CFA_GRBG },
	{ "BAYER GBRG", 4, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, XS_CPIH_TETRIX, XS_CFA_GBRG },
};
#define IMAGE_FORMATS_COUNT (sizeof(IMAGE_FORMATS) / sizeof(image_format_t))

const char* image_find_format_str(const xs_image_t* image, const xs_config_t* xs_config)
{
	assert(MAX_NCOMPS == 4);
	static const char CUSTOM[] = "CUSTOM";

	for (int i = 0; i < IMAGE_FORMATS_COUNT; ++i)
	{
		const image_format_t* e = &IMAGE_FORMATS[i];
		if (image->ncomps != e->ncomps) continue;
		if (image->sx[0] != e->sx[0] || image->sx[1] != e->sx[1] || image->sx[2] != e->sx[2] || image->sx[3] != e->sx[3]) continue;
		if (image->sy[0] != e->sy[0] || image->sy[1] != e->sy[1] || image->sy[2] != e->sy[2] || image->sy[3] != e->sy[3]) continue;
		if (xs_config->p.color_transform != e->mct) continue;
		if (xs_config->p.cfa_pattern != e->cfa) continue;
		return e->name;
	}
	return CUSTOM;
}
