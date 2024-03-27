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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v210.h"
#include "helpers.h"



int v210_decode(char* filename, xs_image_t* im, int little_endian)
{
	int shift_array_le[] = { 0,10,20 };
	int shift_array_be[] = { 22,12,2 };
	FILE* f_in;
	int file_len;
	uint32_t words[4];
	int color_idx = 0, comp_idx = 0, i = 0;
	int color_order[] = { 1,0,2,0 };
	int color_ptrs[] = { 0,0,0 };
	int expected_len;
	int must_swap = host_little_endian() ^ little_endian;
	int* shift_array = little_endian ? shift_array_le : shift_array_be;

	if ((im->width <= 0) || (im->height <= 0))
	{
		fprintf(stderr, "v210_decode error: unknown image width/height\n");
		return -1;
	}

	im->ncomps = 3;
	im->sx[0] = 1;
	im->sx[1] = im->sx[2] = 2;
	im->sy[0] = im->sy[1] = im->sy[2] = 1;
	if (im->depth == -1)
		im->depth = 10;

	if (!xs_allocate_image(im, false))
		return -1;

	f_in = fopen(filename, "rb");
	if (f_in == NULL)
		return -1;

	fseek(f_in, 0, SEEK_END);
	file_len = ftell(f_in);
	fseek(f_in, 0, SEEK_SET);

	expected_len = ((im->height * im->width * 8) + 2) / 3;

	if (file_len > expected_len)
		file_len = expected_len;

	while (file_len >= 4 * sizeof(uint32_t))
	{
		file_len -= sizeof(uint32_t) * (int)fread(words, sizeof(uint32_t), 4, f_in);
		for (i = 0; i < 4; i++)
		{
			if (must_swap)
				words[i] = SWAP(words[i]);

			for (comp_idx = 0; comp_idx < 3; comp_idx++)
			{
				int color = color_order[color_idx % 4];
				im->comps_array[color][color_ptrs[color]++] = (xs_data_in_t)((words[i] >> shift_array[comp_idx]) & 0x3ff);
				color_idx++;
			}
		}
	}
	fclose(f_in);

	return 0;
}

int v210_encode(char* filename, xs_image_t* im, int little_endian)
{
	FILE* f_out;
	int shift_array_le[] = { 0,10,20 };
	int shift_array_be[] = { 22,12,2 };
	int i = 0;
	int color_idx = 0, comp_idx = 0;
	int color_order[] = { 1,0,2,0 };
	int color_ptrs[] = { 0,0,0 };
	int must_swap = host_little_endian() ^ little_endian;
	int* shift_array = little_endian ? shift_array_le : shift_array_be;
	int file_len = ((im->height * im->width * 8) + 2) / 3;

	if (im->ncomps == 3)
	{
		if (im->sx[0] != 1 || im->sx[1] != 2 || im->sx[2] != 2 ||
			im->sy[0] != 1 || im->sy[1] != 1 || im->sy[2] != 1)
		{
			fprintf(stderr, "v210 only supports 422 image data\n");
			return -1;
		}
	}
	else
	{
		fprintf(stderr, "unsupported image layout, cannot save as v210\n");
		return -1;
	}

	f_out = fopen(filename, "wb");
	if (f_out == NULL)
		return -1;

	while (file_len > 0)
	{
		for (i = 0; i < 4; i++)
		{
			uint32_t word = 0;
			for (comp_idx = 0; comp_idx < 3; comp_idx++)
			{
				int color = color_order[color_idx % 4];

				word |= ((im->comps_array[color][color_ptrs[color]++] & 0x3ff) << shift_array[comp_idx]);
				color_idx++;
			}
			if (must_swap)
				word = SWAP(word);

			fwrite(&word, sizeof(uint32_t), 1, f_out);
		}
		file_len -= 16;
	}
	fclose(f_out);
	return 0;
}
