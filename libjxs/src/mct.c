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

#include "mct.h"

#include "common.h"
#include <assert.h>
#include <malloc.h>
#include <stdio.h>

void swap_ptr(xs_data_in_t** p1, xs_data_in_t** p2)
{
	xs_data_in_t* tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

void mct_forward_rct(xs_image_t* im)
{
	assert(im->ncomps >= 3);
	const int len = im->width * im->height;
	xs_data_in_t* c0 = im->comps_array[0]; // R
	xs_data_in_t* c1 = im->comps_array[1]; // G
	xs_data_in_t* c2 = im->comps_array[2]; // B
	for (int i = 0; i < len; ++i)
	{
		const xs_data_in_t g = *c1;
		const xs_data_in_t tmp = (*c0 + 2 * g + *c2) >> 2;
		*c1 = *c2 - g;
		*c2 = *c0 - g;
		*c0 = tmp;
		++c0; ++c1; ++c2;
	}
}

void mct_inverse_rct(xs_image_t* im)
{
	assert(im->ncomps >= 3);
	const int len = im->width * im->height;
	xs_data_in_t* c0 = im->comps_array[0];
	xs_data_in_t* c1 = im->comps_array[1];
	xs_data_in_t* c2 = im->comps_array[2];
	for (int i = 0; i < len; ++i)
	{
		const xs_data_in_t tmp = *c0 - ((*c1 + *c2) >> 2);
		*c0 = tmp + *c2;
		*c2 = tmp + *c1;
		*c1 = tmp;
		++c0; ++c1; ++c2;
	}
}

//xs_data_in_t* mct_tetrix_access_slow(xs_image_t* im, const int c, const int Cf, const int Ct, int rx, int ry, const int x, const int y)
//{
//	const uint8_t DX[] = { 0, 1, 0, 1, 1, 0, 1, 0 }; // Ct * 4 + c
//	const uint8_t DY[] = { 1, 1, 0, 0, 1, 1, 0, 0 }; // Ct * 4 + c
//	const uint8_t K[] = { 2, 0, 3, 1, 3, 1, 2, 0 }; // Ct * 4 + dx * 2 + dy
//
//	const int dx = DX[(Ct << 2) + c];
//	const int dy = DY[(Ct << 2) + c];
//
//	if (((x << 1) + rx + dx) < 0 || ((x << 1) + rx + dx) >= (im->width << 1))
//	{
//		rx = -rx;
//	}
//	if ((Cf == 3 && (ry + dy) < 0) || (Cf == 3 && (ry + dy) > 1) ||
//		((y << 1) + ry + dy) < 0 || ((y << 1) + ry + dy) >= (im->height << 1))
//	{
//		ry = -ry;
//	}
//	const int nx = ((x << 1) + rx + dx) >> 1;
//	const int ny = ((y << 1) + ry + dy) >> 1;
//	const int nk = K[(Ct << 2) + (((2 + rx + dx) % 2) << 1) + ((2 + ry + dy) % 2)];
//	return im->comps_array[nk] + ny * im->width + nx;
//}

xs_data_in_t* mct_tetrix_access(xs_image_t* im, const int c, const int Cf, const int Ct, const int rx, const int ry, const int x, const int y)
{
	// Stupid magic.
	assert(c >= 0 && c <= 3);
	assert(Cf == 0 || Cf == 3);
	assert(Ct == 0 || Ct == 1);
	assert(rx >= -1 && rx <= 1);
	assert(ry >= -1 && ry <= 1);
	int t_x = rx + ((Ct + c) & 1);
	int t_y = ry + (((~(c)) >> 1) & 1);
	assert(t_x >= -1 && t_x <= 2);
	assert(t_y >= -1 && t_y <= 2);
	const int k = ((((~(t_y)) << 1) & 2) | (((Ct) ^ (t_x)) & 1));
	assert(k >= 0 && k <= 3);
	if (Cf == 3) { t_y &= 1; }
	t_x += x << 1;
	t_y += y << 1;
	if (t_x < 0) { t_x += 2; }
	else if (t_x >= (im->width << 1)) { t_x -= 2; }
	if (t_y < 0) { t_y += 2; }
	else if (t_y >= (im->height << 1)) { t_y -= 2; }
	t_x >>= 1;
	t_y >>= 1;
	assert(t_x >= 0 && t_x < im->width);
	assert(t_y >= 0 && t_y < im->height);
	//assert(mct_tetrix_access_slow(im, c, Cf, Ct, rx, ry, x, y) == (im->comps_array[k] + t_y * im->width + t_x));
	return im->comps_array[k] + (size_t)t_y * (size_t)im->width + t_x;
}

void mct_forward_tetrix(xs_image_t* im, const xs_cfa_pattern_t cfa_pattern, const xs_cts_parameters_t cts_parameters)
{
	assert(im->ncomps == 4);
	assert(im->sx[0] == im->sx[1] && im->sx[0] == im->sx[2] && im->sx[0] == im->sx[3]);
	assert(im->sy[0] == im->sy[1] && im->sy[0] == im->sy[2] && im->sy[0] == im->sy[3]);

	const int Cf = cts_parameters.Cf;
	const int Ct = (cfa_pattern == XS_CFA_RGGB || cfa_pattern == XS_CFA_BGGR) ? 0 : 1;
	const uint8_t e1 = cts_parameters.e1;
	const uint8_t e2 = cts_parameters.e2;

	// Reassign component order.
	swap_ptr(&im->comps_array[0], &im->comps_array[2]);
	swap_ptr(&im->comps_array[1], &im->comps_array[3]);

	// Forward CbCr.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			{
				xs_data_in_t* gl = mct_tetrix_access(im, 1, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* gr = mct_tetrix_access(im, 1, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* gt = mct_tetrix_access(im, 1, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* gb = mct_tetrix_access(im, 1, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* s = im->comps_array[1] + (size_t)y * im->width + x;
				*s -= (*gl + *gr + *gt + *gb) >> 2;
			}
			{
				xs_data_in_t* gl = mct_tetrix_access(im, 2, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* gr = mct_tetrix_access(im, 2, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* gt = mct_tetrix_access(im, 2, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* gb = mct_tetrix_access(im, 2, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* s = im->comps_array[2] + (size_t)y * im->width + x;
				*s -= (*gl + *gr + *gt + *gb) >> 2;
			}
		}
	}
	// Forward Y.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			{
				xs_data_in_t* bl = mct_tetrix_access(im, 0, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* br = mct_tetrix_access(im, 0, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* rt = mct_tetrix_access(im, 0, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* rb = mct_tetrix_access(im, 0, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* s = im->comps_array[0] + (size_t)y * im->width + x;
				*s += (((*bl + *br) << e2) + ((*rt + *rb) << e1)) >> 3;
			}
			{
				xs_data_in_t* bt = mct_tetrix_access(im, 3, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* bb = mct_tetrix_access(im, 3, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* rl = mct_tetrix_access(im, 3, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* rr = mct_tetrix_access(im, 3, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* s = im->comps_array[3] + (size_t)y * im->width + x;
				*s += (((*bt + *bb) << e2) + ((*rl + *rr) << e1)) >> 3;
			}
		}
	}
	// Forward delta.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			xs_data_in_t* ytl = mct_tetrix_access(im, 3, Cf, Ct, -1, -1, x, y);
			xs_data_in_t* ytr = mct_tetrix_access(im, 3, Cf, Ct, 1, -1, x, y);
			xs_data_in_t* ybl = mct_tetrix_access(im, 3, Cf, Ct, -1, 1, x, y);
			xs_data_in_t* ybr = mct_tetrix_access(im, 3, Cf, Ct, 1, 1, x, y);
			xs_data_in_t* s = im->comps_array[3] + (size_t)y * im->width + x;
			*s -= (*ytl + *ytr + *ybl + *ybr) >> 2;
		}
	}
	// Forward average.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			xs_data_in_t* dtl = mct_tetrix_access(im, 0, Cf, Ct, -1, -1, x, y);
			xs_data_in_t* dtr = mct_tetrix_access(im, 0, Cf, Ct, 1, -1, x, y);
			xs_data_in_t* dbl = mct_tetrix_access(im, 0, Cf, Ct, -1, 1, x, y);
			xs_data_in_t* dbr = mct_tetrix_access(im, 0, Cf, Ct, 1, 1, x, y);
			xs_data_in_t* s = im->comps_array[0] + (size_t)y * im->width + x;
			*s += (*dtl + *dtr + *dbl + *dbr) >> 3;
		}
	}
}

void mct_inverse_tetrix(xs_image_t* im, const xs_cfa_pattern_t cfa_pattern, const xs_cts_parameters_t cts_parameters)
{
	assert(im->ncomps == 4);
	assert(im->sx[0] == im->sx[1] && im->sx[0] == im->sx[2] && im->sx[0] == im->sx[3]);
	assert(im->sy[0] == im->sy[1] && im->sy[0] == im->sy[2] && im->sy[0] == im->sy[3]);

	const int Cf = cts_parameters.Cf;
	const int Ct = (cfa_pattern == XS_CFA_RGGB || cfa_pattern == XS_CFA_BGGR) ? 0 : 1;
	const uint8_t e1 = cts_parameters.e1;
	const uint8_t e2 = cts_parameters.e2;

	// Inverse average.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			xs_data_in_t* dtl = mct_tetrix_access(im, 0, Cf, Ct, -1, -1, x, y);
			xs_data_in_t* dtr = mct_tetrix_access(im, 0, Cf, Ct, 1, -1, x, y);
			xs_data_in_t* dbl = mct_tetrix_access(im, 0, Cf, Ct, -1, 1, x, y);
			xs_data_in_t* dbr = mct_tetrix_access(im, 0, Cf, Ct, 1, 1, x, y);
			xs_data_in_t* s = im->comps_array[0] + (size_t)y * im->width + x;
			*s -= (*dtl + *dtr + *dbl + *dbr) >> 3;
		}
	}
	// Inverse delta.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			xs_data_in_t* ytl = mct_tetrix_access(im, 3, Cf, Ct, -1, -1, x, y);
			xs_data_in_t* ytr = mct_tetrix_access(im, 3, Cf, Ct, 1, -1, x, y);
			xs_data_in_t* ybl = mct_tetrix_access(im, 3, Cf, Ct, -1, 1, x, y);
			xs_data_in_t* ybr = mct_tetrix_access(im, 3, Cf, Ct, 1, 1, x, y);
			xs_data_in_t* s = im->comps_array[3] + (size_t)y * im->width + x;
			*s += (*ytl + *ytr + *ybl + *ybr) >> 2;
		}
	}
	// Inverse Y.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			{
				xs_data_in_t* bl = mct_tetrix_access(im, 0, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* br = mct_tetrix_access(im, 0, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* rt = mct_tetrix_access(im, 0, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* rb = mct_tetrix_access(im, 0, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* s = im->comps_array[0] + (size_t)y * im->width + x;
				*s -= (((*bl + *br) << e2) + ((*rt + *rb) << e1)) >> 3;
			}
			{
				xs_data_in_t* bt = mct_tetrix_access(im, 3, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* bb = mct_tetrix_access(im, 3, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* rl = mct_tetrix_access(im, 3, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* rr = mct_tetrix_access(im, 3, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* s = im->comps_array[3] + (size_t)y * im->width + x;
				*s -= (((*bt + *bb) << e2) + ((*rl + *rr) << e1)) >> 3;
			}
		}
	}
	// Inverse CbCr.
	for (int y = 0; y < im->height; ++y)
	{
		for (int x = 0; x < im->width; ++x)
		{
			{
				xs_data_in_t* gl = mct_tetrix_access(im, 1, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* gr = mct_tetrix_access(im, 1, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* gt = mct_tetrix_access(im, 1, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* gb = mct_tetrix_access(im, 1, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* s = im->comps_array[1] + (size_t)y * im->width + x;
				*s += (*gl + *gr + *gt + *gb) >> 2;
			}
			{
				xs_data_in_t* gl = mct_tetrix_access(im, 2, Cf, Ct, 0, -1, x, y);
				xs_data_in_t* gr = mct_tetrix_access(im, 2, Cf, Ct, 0, 1, x, y);
				xs_data_in_t* gt = mct_tetrix_access(im, 2, Cf, Ct, -1, 0, x, y);
				xs_data_in_t* gb = mct_tetrix_access(im, 2, Cf, Ct, 1, 0, x, y);
				xs_data_in_t* s = im->comps_array[2] + (size_t)y * im->width + x;
				*s += (*gl + *gr + *gt + *gb) >> 2;
			}
		}
	}
	// Reassign component order.
	swap_ptr(&im->comps_array[0], &im->comps_array[2]);
	swap_ptr(&im->comps_array[1], &im->comps_array[3]);
}

void mct_forward_transform(xs_image_t* im, const xs_config_parameters_t* p)
{
	switch (p->color_transform)
	{
	case XS_CPIH_NONE:
	{
		break;
	}
	case XS_CPIH_RCT:
	{
		mct_forward_rct(im);
		break;
	}
	case XS_CPIH_TETRIX:
	{
		mct_forward_tetrix(im, p->cfa_pattern, p->tetrix_params);
		break;
	}
	default:
		assert(!"Unknown color transform");
		break;
	}
}

void mct_inverse_transform(xs_image_t* im, const xs_config_parameters_t* p)
{
	switch (p->color_transform)
	{
	case XS_CPIH_NONE:
	{
		break;
	}
	case XS_CPIH_RCT:
	{
		mct_inverse_rct(im);
		break;
	}
	case XS_CPIH_TETRIX:
	{
		mct_inverse_tetrix(im, p->cfa_pattern, p->tetrix_params);
		break;
	}
	default:
		assert(!"Unknown color transform");
		break;
	}
}

bool assign_bayer_components(xs_data_in_t* comp_ptr[4], xs_image_t* im, const xs_cfa_pattern_t cfa_pattern)
{
	switch (cfa_pattern)
	{
	case XS_CFA_RGGB:
	case XS_CFA_BGGR:  // TODO: verify
	{
		comp_ptr[0] = im->comps_array[0];
		comp_ptr[1] = im->comps_array[1];
		comp_ptr[2] = im->comps_array[2];
		comp_ptr[3] = im->comps_array[3];
		break;
	}
	case XS_CFA_GRBG:
	case XS_CFA_GBRG:  // TODO: verify
	{
		comp_ptr[0] = im->comps_array[1];
		comp_ptr[1] = im->comps_array[0];
		comp_ptr[2] = im->comps_array[3];
		comp_ptr[3] = im->comps_array[2];
		break;
	}
	default:
	{
		fprintf(stderr, "Error: Unsupported component registration (CRG) pattern\n");
		return false;
	}
	}
	return true;
}

bool mct_bayer_to_4(xs_image_t* im, const xs_cfa_pattern_t cfa_pattern)
{
	assert(im->ncomps == 1);
	if (im->sx[0] != 1 || im->sy[0] != 1)
	{
		fprintf(stderr, "Error: Bayer input cannot be subsampled\n");
		return false;
	}
	const int w = im->width;
	const int h = im->height;
	if ((w % 2) != 0 || (h % 2) != 0)
	{
		fprintf(stderr, "Error: Bayer input dimensions must be multiples of 2\n");
		return false;
	}

	xs_data_in_t* mono = im->comps_array[0];
	im->comps_array[0] = NULL;
	im->ncomps = 4;
	im->width = w >> 1;
	im->height = h >> 1;
	im->sx[1] = im->sx[2] = im->sx[3] = 1;
	im->sy[1] = im->sy[2] = im->sy[3] = 1;
	if (!xs_allocate_image(im, false))
	{
		free(mono);
		fprintf(stderr, "Error: Cannot allocate image memory\n");
		return false;
	}

	xs_data_in_t* src = mono;
	xs_data_in_t* dst[4];
	if (!assign_bayer_components(dst, im, cfa_pattern))
	{
		return false;
	}
	for (int line = 0; line < im->height; ++line)
	{
		for (int x = 0; x < im->width; ++x)
		{
			*(dst[0]++) = *src;
			*(dst[2]++) = *(src + w);
			++src;
			*(dst[1]++) = *src;
			*(dst[3]++) = *(src + w);
			++src;
		}
		src += w;
	}

	free(mono);

	return true;
}

bool mct_4_to_bayer(xs_image_t* im, const xs_cfa_pattern_t cfa_pattern)
{
	assert(im->ncomps == 4);
	for (int c = 0; c < 4; ++c)
	{
		if (im->sx[c] != 1 || im->sy[c] != 1)
		{
			fprintf(stderr, "Error: Bayer input cannot be subsampled\n");
			return false;
		}
	}

	const int w = im->width << 1;
	const int h = im->height << 1;
	xs_data_in_t* mono = (xs_data_in_t*)malloc((size_t)w * h * sizeof(xs_data_in_t));
	if (mono == NULL)
	{
		fprintf(stderr, "Error: Cannot allocate image memory\n");
		return false;
	}
	xs_data_in_t* dst = mono;
	xs_data_in_t* src[4];
	if (!assign_bayer_components(src, im, cfa_pattern))
	{
		return false;
	}
	for (int line = 0; line < im->height; ++line)
	{
		for (int x = 0; x < im->width; ++x)
		{
			*dst = *(src[0]++);
			*(dst + w) = *(src[2]++);
			++dst;
			*dst = *(src[1]++);
			*(dst + w) = *(src[3]++);
			++dst;
		}
		dst += w;
	}
	xs_free_image(im);
	im->ncomps = 1;
	im->width = w;
	im->height = h;
	im->sx[1] = im->sx[2] = im->sx[3] = 0;
	im->sy[1] = im->sy[2] = im->sy[3] = 0;
	im->comps_array[0] = mono;

	return true;
}
