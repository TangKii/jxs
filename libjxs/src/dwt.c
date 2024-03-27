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

#include "dwt.h"
#include <assert.h>

typedef void (*filter_func_t)(xs_data_in_t* const base, xs_data_in_t* const end, const ptrdiff_t inc);

// Bounds-safe extended 5x3 DWT.
void dwt_inverse_filter_1d_(xs_data_in_t* const base, xs_data_in_t* const end, const ptrdiff_t inc)
{
	// Even samples (low pass).
	xs_data_in_t* p = base;
	*p -= (*(p + inc) + 1) >> 1;
	p += 2 * inc;
	for (; p < end - inc; p += 2 * inc)
	{
		*p -= (*(p - inc) + *(p + inc) + 2) >> 2;
	}
	if (p < end)
	{
		*p -= (*(p - inc) + 1) >> 1;
	}
	// Odd samples (high pass).
	p = base + inc;
	for (; p < end - inc; p += 2 * inc)
	{
		*p += (*(p - inc) + *(p + inc)) >> 1;
	}
	if (p < end)
	{
		*p += *(p - inc);
	}
}

// Bounds-safe extended 5x3 DWT.
void dwt_forward_filter_1d_(xs_data_in_t* const base, xs_data_in_t* const end, const ptrdiff_t inc)
{
	// Odd samples (high pass).
	xs_data_in_t* p = base + inc;
	for (; p < end - inc; p += 2 * inc)
	{
		*p -= (*(p - inc) + *(p + inc)) >> 1;
	}
	if (p < end)
	{
		*p -= *(p - inc);
	}
	// Even samples (low pass).
	p = base;
	*p += (*(p + inc) + 1) >> 1;
	p += 2 * inc;
	for (; p < end - inc; p += 2 * inc)
	{
		*p += (*(p - inc) + *(p + inc) + 2) >> 2;
	}
	if (p < end)
	{
		*p += (*(p - inc) + 1) >> 1;
	}
}

void dwt_tranform_vertical_(const ids_t* ids, xs_image_t* im, const int k, const int h_level, const int v_level, filter_func_t filter)
{
	assert(h_level >= 0 && v_level >= 0);
	const ptrdiff_t x_inc = (ptrdiff_t)1 << h_level;
	const ptrdiff_t y_inc = (ptrdiff_t)ids->comp_w[k] << v_level;
	xs_data_in_t* base = im->comps_array[k];
	xs_data_in_t* const end = base + ids->comp_w[k];
	for (; base < end; base += x_inc)
	{
		xs_data_in_t* const col_end = base + (size_t)ids->comp_w[k] * (size_t)ids->comp_h[k];
		filter(base, col_end, y_inc);
	}
}

void dwt_tranform_horizontal_(const ids_t* ids, xs_image_t* im, const int k, const int h_level, const int v_level, filter_func_t filter)
{
	assert(h_level >= 0 && v_level >= 0);
	const ptrdiff_t x_inc = (ptrdiff_t)1 << h_level;
	const ptrdiff_t y_inc = (ptrdiff_t)ids->comp_w[k] << v_level;
	xs_data_in_t* base = im->comps_array[k];
	xs_data_in_t* const end = base + (size_t)ids->comp_w[k] * (size_t)ids->comp_h[k];
	for (; base < end; base += y_inc)
	{
		xs_data_in_t* const row_end = base + ids->comp_w[k];
		filter(base, row_end, x_inc);
	}
}

// Take interleaved grid of wavelet coefficients, and inverse transform in-place. Annex E.
void dwt_inverse_transform(const ids_t* ids, xs_image_t* im)
{
	for (int k = 0; k < ids->ncomps - ids->sd; ++k)
	{
		assert(ids->nlxyp[k].y <= ids->nlxyp[k].x);
		for (int d = ids->nlxyp[k].x - 1; d >= ids->nlxyp[k].y; --d)
		{
			dwt_tranform_horizontal_(ids, im, k, d, ids->nlxyp[k].y, dwt_inverse_filter_1d_);
		}
		for (int d = ids->nlxyp[k].y - 1; d >= 0; --d)
		{
			dwt_tranform_horizontal_(ids, im, k, d, d, dwt_inverse_filter_1d_);
			dwt_tranform_vertical_(ids, im, k, d, d, dwt_inverse_filter_1d_);
		}
	}
}

// Take image samples, and forward transform in-place into interleaved grid of coefficients. Annex E.
void dwt_forward_transform(const ids_t* ids, xs_image_t* im)
{
	for (int k = 0; k < ids->ncomps - ids->sd; ++k)
	{
		assert(ids->nlxyp[k].y <= ids->nlxyp[k].x);
		for (int d = 0; d < ids->nlxyp[k].y; ++d)
		{
			dwt_tranform_vertical_(ids, im, k, d, d, dwt_forward_filter_1d_);
			dwt_tranform_horizontal_(ids, im, k, d, d, dwt_forward_filter_1d_);
		}
		for (int d = ids->nlxyp[k].y; d < ids->nlxyp[k].x; ++d)
		{
			dwt_tranform_horizontal_(ids, im, k, d, ids->nlxyp[k].y, dwt_forward_filter_1d_);
		}
	}
}
