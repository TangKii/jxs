/***************************************************************************
** intoPIX SA, Fraunhofer IIS and Canon Inc. (hereinafter the "Software   **
** Copyright Holder") hold or have the right to license copyright with    **
** respect to the accompanying software (hereinafter the "Software").     **
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

#include "precinct.h"
#include "buf_mgmt.h"
#include "quant.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct precinct_t
{
	multi_buf_t* gclis_mb;
	multi_buf_t* sig_mag_data_mb;

	const ids_t* ids;

	int group_size;
	int idx_from_level[MAX_PRECINCT_HEIGHT][MAX_PACKETS];

	int y_idx;
	int column;
	int is_last_column;
};

#ifdef _MSC_VER
#include <intrin.h>
static unsigned int __inline BSR(unsigned long x)
{
	unsigned long r = 0;
	_BitScanReverse(&r, x);
	return r;
}
#else
#define BSR(x) (31 - __builtin_clz(x))
#endif
#define GCLI(x) (((x) == 0) ? 0 : (BSR((x)) + 1))  

precinct_t* precinct_open_column(const ids_t* ids, int group_size, int column)
{
	assert(column >= 0 && column < ids->npx);
	precinct_t* prec = malloc(sizeof(precinct_t));
	assert(prec != NULL);
	memset(prec, -1, sizeof(precinct_t));
	prec->ids = ids;
	prec->group_size = group_size;
	prec->sig_mag_data_mb = multi_buf_create(ids->npi, sizeof(sig_mag_data_t));
	prec->gclis_mb = multi_buf_create(ids->npi, sizeof(gcli_data_t));
	prec->y_idx = -1;
	prec->column = column;
	prec->is_last_column = (column == ids->npx - 1) ? 1 : 0;
	for (int idx = 0; idx < ids->npi; ++idx)
	{
		const int band = ids->pi[idx].b;
		const int y = ids->pi[idx].y - ids->l0[band];  // relative Y in band

		prec->idx_from_level[y][band] = idx;

		const int N_cg = (ids->pwb[prec->is_last_column][band] + group_size - 1) / group_size;
		const int nb_coefficients = N_cg * group_size;

		multi_buf_set_size(prec->sig_mag_data_mb, idx, nb_coefficients);
		multi_buf_set_size(prec->gclis_mb, idx, N_cg);
	}
	multi_buf_allocate(prec->gclis_mb, 1);
	multi_buf_allocate(prec->sig_mag_data_mb, group_size);
	return prec;
}

void precinct_close(precinct_t* prec)
{
	if (prec)
	{
		if (prec->gclis_mb)
			multi_buf_destroy(prec->gclis_mb);
		if (prec->sig_mag_data_mb)
			multi_buf_destroy(prec->sig_mag_data_mb);
		free(prec);
	}
}

void precinct_ptr_for_line_of_band(const precinct_t* prec, xs_image_t* image, const int band_idx, const int in_band_ypos, xs_data_in_t** ptr, size_t* x_inc, size_t* len)
{
	const ids_t* ids = prec->ids;

	assert(ids != NULL);
	assert(band_idx >= 0 && band_idx < ids->nbands);

	assert(prec->y_idx >= 0 && prec->y_idx < ids->npy);
	assert(prec->column >= 0 && prec->column < ids->npx);

	const int c = ids->band_idx_to_c_and_b[band_idx].c;
	const int b = ids->band_idx_to_c_and_b[band_idx].b;

	// Handle precinct component base.
	xs_data_in_t* the_ptr = image->comps_array[c];
	// Handle start y position of band.
	the_ptr += ids->band_is_high[b].y * ((size_t)1 << (ids->band_d[c][b].y - 1)) * ids->comp_w[c];
	// Handle start x position of band.
	the_ptr += ids->band_is_high[b].x * ((size_t)1 << (ids->band_d[c][b].x - 1));

	// Handle precinct y index.
	the_ptr += (size_t)ids->comp_w[c] * (ids->ph >> (image->sy[c] - 1)) * prec->y_idx;
	// Go to ypos line in precinct.
	the_ptr += (size_t)ids->comp_w[c] * in_band_ypos * ((size_t)1 << ids->band_d[c][b].y);

	// Handle precinct x index.
	the_ptr += (ids->pw[0] >> (image->sx[c] - 1)) * prec->column;

	assert(the_ptr >= image->comps_array[c]);
	assert(the_ptr < (image->comps_array[c] + ids->comp_w[c] * ids->comp_h[c]));

	*ptr = the_ptr;  // first sample
	*x_inc = 1ull << ids->band_d[c][b].x;  // increment to next sample
	*len = ids->pwb[prec->is_last_column][band_idx];  // number of samples in line of band in precinct

	assert(the_ptr + (*len - 1) * *x_inc < (image->comps_array[c] + ids->comp_w[c] * ids->comp_h[c]));
}

void precinct_to_image(const precinct_t* prec, xs_image_t* target, const uint8_t Fq)
{
	for (int band_idx = 0; band_idx < prec->ids->nbands; ++band_idx)
	{
		const int height = precinct_in_band_height_of(prec, band_idx);
		for (int ypos = 0; ypos < height; ++ypos)
		{
			xs_data_in_t* dst;
			size_t dst_inc, dst_len;
			precinct_ptr_for_line_of_band(prec, target, band_idx, ypos, &dst, &dst_inc, &dst_len);
			assert(dst != NULL);
			sig_mag_data_t* src = precinct_line_of(prec, band_idx, ypos);
			for (size_t i = 0; i < dst_len; ++i)
			{
				const sig_mag_data_t val = *src++;
				*dst = ((val & SIGN_BIT_MASK) ? -((sig_mag_data_in_t)val & ~SIGN_BIT_MASK) : (sig_mag_data_in_t)val) << Fq;
				dst += dst_inc;
			}
		}
	}
}

void precinct_from_image(precinct_t* prec, xs_image_t* image, const uint8_t Fq)
{
	const sig_mag_data_in_t Fq_r = (1 << Fq) >> 1;
	for (int band_idx = 0; band_idx < prec->ids->nbands; ++band_idx)
	{
		const int height = precinct_in_band_height_of(prec, band_idx);
		for (int ypos = 0; ypos < height; ++ypos)
		{
			xs_data_in_t* src;
			size_t src_inc, src_len;
			precinct_ptr_for_line_of_band(prec, image, band_idx, ypos, &src, &src_inc, &src_len);
			assert(src != NULL);
			sig_mag_data_t* dst = precinct_line_of(prec, band_idx, ypos);
			memset(dst, 0, sizeof(sig_mag_data_t) * precinct_width_of(prec, band_idx));
			for (size_t i = 0; i < src_len; ++i)
			{
				const sig_mag_data_in_t val = *src;
				if (val >= 0)
				{
					*dst++ = (val + Fq_r) >> Fq;
				}
				else
				{
					*dst++ = ((-val + Fq_r) >> Fq) | SIGN_BIT_MASK;
				}
				src += src_inc;
			}
		}
	}
}

int bands_count_of(const precinct_t* prec)
{
	return prec->ids->nbands;
}

int line_count_of(const precinct_t* prec)
{
	return prec->ids->npi;
}

sig_mag_data_t* precinct_line_of(const precinct_t* prec, int band_index, int ypos)
{
	int idx = prec->idx_from_level[ypos][band_index];
	assert(idx >= 0);
	return prec->sig_mag_data_mb->bufs.u32[idx];
}

size_t precinct_width_of(const precinct_t* prec, int band_index)
{
	int idx = prec->idx_from_level[0][band_index];
	assert(idx >= 0);
	return prec->sig_mag_data_mb->sizes[idx];
}

gcli_data_t* precinct_gcli_of(const precinct_t* prec, int band_index, int ypos)
{
	int idx = prec->idx_from_level[ypos][band_index];
	assert(idx >= 0);
	return prec->gclis_mb->bufs.s8[idx];
}

gcli_data_t* precinct_gcli_top_of(const precinct_t* prec, const precinct_t* prec_top, int band_index, int ypos)
{
	const precinct_t* prec_above = (ypos == 0) ? prec_top : prec;
	if (prec_above)
	{
		const int ylast = (ypos == 0) ? (precinct_in_band_height_of(prec_above, band_index) - 1) : (ypos - 1);
		return precinct_gcli_of(prec_above, band_index, ylast);
	}
	else
	{
		return NULL;
	}
}

int precinct_get_gcli_group_size(const precinct_t* prec)
{
	return prec->group_size;
}

int precinct_gcli_width_of(const precinct_t* prec, int band_index)
{
	int idx = prec->idx_from_level[0][band_index];
	assert(idx >= 0);
	assert(idx < prec->gclis_mb->n_buffers);
	return prec->gclis_mb->sizes[idx];
}

int precinct_max_gcli_width_of(const precinct_t* prec)
{
	return prec->ids->band_max_width;
}

int compute_gcli_buf(const sig_mag_data_t* in, int len, gcli_data_t* out, int max_out_len, int* out_len, int group_size)
{
	int i, j;
	sig_mag_data_t or_all;
	*out_len = (len + group_size - 1) / group_size;
	if (*out_len > max_out_len)
		return -1;

	for (i = 0; i < (len / group_size) * group_size; i += group_size)
	{
		for (or_all = 0, j = 0; j < group_size; j++)
			or_all |= *in++;
		*out++ = GCLI(or_all & (~SIGN_BIT_MASK));
	}

	if (len % group_size)
	{
		for (or_all = 0, j = 0; j < len % group_size; j++)
			or_all |= *in++;
		*out++ = GCLI(or_all & (~SIGN_BIT_MASK));
	}
	return 0;
}

void update_gclis(precinct_t* prec)
{
	int band, ypos;

	for (band = 0; band < bands_count_of(prec); band++)
	{
		for (ypos = 0; ypos < precinct_in_band_height_of(prec, band); ypos++)
		{
			const sig_mag_data_t* in = precinct_line_of(prec, band, ypos);
			gcli_data_t* dst = precinct_gcli_of(prec, band, ypos);
			int reclen, width = (int)precinct_width_of(prec, band);
			int gcli_count = (int)precinct_gcli_width_of(prec, band);
			compute_gcli_buf(in, width, dst, gcli_count, &reclen, prec->group_size);
		}
	}
}

void copy_gclis(precinct_t* dest, const precinct_t* src)
{
	int band, ypos;

	assert(bands_count_of(dest) == bands_count_of(src));

	for (band = 0; band < bands_count_of(dest); band++)
	{
		assert(precinct_in_band_height_of(dest, band) == precinct_in_band_height_of(src, band));

		for (ypos = 0; ypos < precinct_in_band_height_of(dest, band); ypos++)
		{
			int gcli_count = (int)precinct_gcli_width_of(dest, band);
			const gcli_data_t* in = precinct_gcli_of(src, band, ypos);
			gcli_data_t* out = precinct_gcli_of(dest, band, ypos);

			assert(gcli_count == precinct_gcli_width_of(src, band));
			memcpy(out, in, gcli_count * sizeof(gcli_data_t));
		}
	}
}

void quantize_precinct(precinct_t* prec, const int* gtli, int dq_type)
{
	int band, ypos;

	for (band = 0; band < bands_count_of(prec); band++)
	{
		for (ypos = 0; ypos < precinct_in_band_height_of(prec, band); ypos++)
		{
			sig_mag_data_t* data = precinct_line_of(prec, band, ypos);
			size_t width = precinct_width_of(prec, band);
			gcli_data_t* gclis = precinct_gcli_of(prec, band, ypos);

			quant(data, (int)width, gclis, prec->group_size, gtli[band], dq_type);
		}
	}
}

void dequantize_precinct(precinct_t* prec, const int* gtli, int dq_type)
{
	int band, ypos;

	for (band = 0; band < bands_count_of(prec); band++)
	{
		for (ypos = 0; ypos < precinct_in_band_height_of(prec, band); ypos++)
		{
			sig_mag_data_t* data = precinct_line_of(prec, band, ypos);
			size_t width = precinct_width_of(prec, band);
			gcli_data_t* gclis = precinct_gcli_of(prec, band, ypos);

			dequant(data, (int)width, gclis, prec->group_size, gtli[band], dq_type);
		}
	}
}

void copy_data(precinct_t* dest, const precinct_t* src)
{
	int band, ypos;

	assert(bands_count_of(dest) == bands_count_of(src));

	for (band = 0; band < bands_count_of(dest); band++)
	{
		assert(precinct_in_band_height_of(dest, band) == precinct_in_band_height_of(src, band));

		for (ypos = 0; ypos < precinct_in_band_height_of(dest, band); ypos++)
		{
			int count = (int)precinct_width_of(dest, band);
			const sig_mag_data_t* in = precinct_line_of(src, band, ypos);
			sig_mag_data_t* out = precinct_line_of(dest, band, ypos);

			assert(count == precinct_width_of(src, band));
			memcpy(out, in, count * sizeof(sig_mag_data_t));
		}
	}
}

int precinct_copy(precinct_t* dest, const precinct_t* src)
{
	dest->y_idx = src->y_idx;
	copy_gclis(dest, src);
	copy_data(dest, src);
	return 0;
}

void precinct_set_y_idx_of(precinct_t* prec, int y_idx)
{
	assert(y_idx >= 0 && y_idx < prec->ids->npy);
	prec->y_idx = y_idx;
}

int precinct_is_first_of_slice(const precinct_t* prec, int slice_height)
{
	assert(prec->y_idx >= 0 && slice_height > 0);
	return (((prec->y_idx * prec->ids->ph) % slice_height) == 0);
}

int precinct_is_last_of_image(const precinct_t* prec, int im_height)
{
	assert(prec->y_idx >= 0);
	return ((im_height + prec->ids->ph - 1) >> prec->ids->nlxy.y) == (prec->y_idx + 1);
}

int precinct_spacial_lines_of(const precinct_t* prec, int im_height)
{
	const int precheight = prec->ids->ph;
	const int leftover = (im_height % precheight);
	if ((!precinct_is_last_of_image(prec, im_height)) || (leftover == 0))
		return precheight;
	else
		return leftover;
}

int precinct_band_index_of(const precinct_t* prec, int position)
{
	return prec->ids->pi[position].b;
}

int precinct_is_line_present(precinct_t* prec, int lvl, int ypos)
{
	return ypos < precinct_in_band_height_of(prec, lvl);
}

int precinct_ypos_of(const precinct_t* prec, int position)
{
	return prec->ids->pi[position].y - prec->ids->l0[prec->ids->pi[position].b];
}

int precinct_subpkt_of(const precinct_t* prec, int position)
{
	return prec->ids->pi[position].s;
}

int precinct_nb_subpkts_of(const precinct_t* prec)
{
	return prec->ids->npc;
}

int precinct_position_of(const precinct_t* prec, int lvl, int ypos)
{
	return prec->idx_from_level[ypos][lvl];
}

int precinct_in_band_height_of(const precinct_t* prec, int band_index)
{
	const int is_last_precinct_y = (prec->y_idx < (prec->ids->npy - 1)) ? 0 : 1;
	return prec->ids->l1[is_last_precinct_y][band_index] - prec->ids->l0[band_index];
}

bool precinct_use_long_headers(const precinct_t* prec)
{
	return prec->ids->use_long_precinct_headers;
}