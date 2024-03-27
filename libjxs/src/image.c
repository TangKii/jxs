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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libjxs.h"

bool xs_allocate_image(xs_image_t* im, const bool set_zero)
{
	for (int c = 0; c < im->ncomps; c++)
	{
		assert(im->comps_array[c] == NULL);
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		im->comps_array[c] = (xs_data_in_t*)malloc(sample_count * sizeof(xs_data_in_t));
		if (im->comps_array[c] == NULL)
		{
			return false;
		}
		if (set_zero)
		{
			memset(im->comps_array[c], 0, sample_count * sizeof(xs_data_in_t));
		}
	}
	return true;
}

void xs_free_image(xs_image_t* im)
{
	for (int c = 0; c < im->ncomps; c++)
	{
		if (im->comps_array[c])
		{
			free(im->comps_array[c]);
		}
		im->comps_array[c] = NULL;
	}
}

//int xs_image_dump(xs_image_t* im, char* filename)
//{
//	//int min_val;
//	//int max_val;
//
//	FILE* f_out = fopen(filename, "wb");
//	if (f_out == NULL)
//	{
//		return -1;
//	}
//
//	//min_val = ~(1 << 31);
//	//max_val = (1 << 31);
//	//for (int c = 0; c < im->ncomps; ++c)
//	//{
//	//	for (i = 0; i < im->w[c] * im->h[c]; i++)
//	//	{
//	//		if (im->comps_array[comp][i] < min_val)
//	//			min_val = im->comps_array[comp][i];
//	//		if (im->comps_array[c][i] > max_val)
//	//			max_val = im->comps_array[c][i];
//	//	}
//	//}
//
//	//if (max_val == min_val)
//	//{
//	//	min_val = 0;
//	//	max_val = 255;
//	//}
//	//const bool use16 = max_val > 255;
//	const bool use16 = im->depth > 8;
//
//	//#define SCALE_TO_8BITS(v) (((v-min_val) * 256)/(max_val - min_val))
//
//	for (int c = 0; c < im->ncomps; ++c)
//	{
//		for (int i = 0; i < im->w[c] * im->h[c]; i++)
//		{
//			//const xs_data_in_t v = im->comps_array[comp][i] - min_val;
//			const xs_data_in_t v = im->comps_array[c][i];
//			putc(v & 0xff, f_out);
//			if (use16)
//			{
//				putc((v >> 8) & 0xff, f_out);
//			}
//		}
//	}
//	fclose(f_out);
//	return 0;
//}
