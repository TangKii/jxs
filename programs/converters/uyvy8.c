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

#include "uyvy8.h"
#include "helpers.h"

int uyvy8_decode(char* filename, xs_image_t* im)
{
	int pix;
	FILE* f_in;
	uint8_t buf[4];
	xs_data_in_t* ptr_Y;
	xs_data_in_t* ptr_Cb;
	xs_data_in_t* ptr_Cr;

	if ((im->width <= 0) || (im->height <= 0))
	{
		fprintf(stderr, "uyvy_decode error: unknown image width/height\n");
		return -1;
	}
	
	f_in = fopen(filename, "rb");
	if (f_in == NULL)
		return -1;

	im->ncomps = 3;
	im->sx[0] = 1;
	im->sx[1] = im->sx[2] = 2;
	im->sy[0] = im->sy[1] = im->sy[2] = 1;
	if (im->depth == -1)
		im->depth = 8;
	if (!xs_allocate_image(im, false))
		return -1;
	ptr_Y = im->comps_array[0];
	ptr_Cb = im->comps_array[1];
	ptr_Cr = im->comps_array[2];

	for (pix = 0; pix < im->width * im->height / 2; pix++)
	{
		if (fread(buf, sizeof(uint8_t), 4, f_in) == 0)
			break;
		*ptr_Cb++ = buf[0];
		*ptr_Y++ = buf[1];
		*ptr_Cr++ = buf[2];
		*ptr_Y++ = buf[3];
	}
	fclose(f_in);
	return 0;
}

int uyvy8_encode(char* filename, xs_image_t* im)
{
	int file_len;
	FILE* f_out;
	uint8_t buf[4];
	xs_data_in_t* ptr_Y = im->comps_array[0];
	xs_data_in_t* ptr_Cb = im->comps_array[1];
	xs_data_in_t* ptr_Cr = im->comps_array[2];
	
	if (im->ncomps == 3)
	{
		if (im->sx[0] != 1 || im->sx[1] != 2 || im->sx[2] != 2 ||
			im->sy[0] != 1 || im->sy[1] != 1 || im->sy[2] != 1)
		{
			fprintf(stderr, "uyvy8 needs 422 image data\n");
			return -1;
		}
	}
	else
	{
		fprintf(stderr, "unsupported image layout, cannot save as UYVY8\n");
		return -1;
	}

	f_out = fopen(filename, "wb");
	if (f_out == NULL)
		return -1;

	file_len = im->width * im->height * sizeof(uint8_t) * 2;
	while (file_len > 0)
	{
		buf[0] = *ptr_Cb++;
		buf[1] = *ptr_Y++;
		buf[2] = *ptr_Cr++;
		buf[3] = *ptr_Y++;
		fwrite(buf, sizeof(uint8_t), 4, f_out);
		file_len -= 4*sizeof(uint8_t);
	}
	fclose(f_out);
	return 0;
}
