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
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "budget.h"
#include "common.h"
#include "packing.h"
#include "pred.h"
#include "rate_control.h"
#include "buf_mgmt.h"
#include "sb_weighting.h"
#include "gcli_methods.h"
#include "sig_flags.h"
#include "xs_markers.h"

#define ALIGN(value, n) ((value + n - 1) & (~(n - 1)))

struct packing_context_t
{
	const xs_config_t* xs_config;
	int enabled_methods;
	sigbuffer_t* gcli_significance;
	sigbuffer_t* gcli_nonsig_flags;
};

struct unpacking_context_t
{
	const xs_config_t* xs_config;
	int level_count;
	int* gtli_table_data;
	int* gtli_table_gcli;
	gcli_pred_t* gclis_pred;
	sigbuffer_t* gclis_significance;
	int use_sign_subpkt;
	int* inclusion_mask;
	uint32_t enabled_methods;
#ifdef PACKING_GENERATE_FRAGMENT_CODE
	xs_fragment_info_cb_t fragment_info_cb;
	void* fragment_info_context;
	int fragment_cnt;
#endif
};

packing_context_t* packer_open(const xs_config_t* xs_config, const precinct_t* prec)
{
	packing_context_t* pack_ctx = (packing_context_t*)malloc(sizeof(packing_context_t));
	if (!pack_ctx)
	{
		return NULL;
	}
	pack_ctx->xs_config = xs_config;
	pack_ctx->enabled_methods = gcli_method_get_enabled(xs_config);
	pack_ctx->gcli_significance = sigbuffer_open(prec);
	pack_ctx->gcli_nonsig_flags = sigbuffer_open(prec);

	return pack_ctx;
}

void packer_close(packing_context_t* ctx)
{
	if (ctx->gcli_significance)
		sigbuffer_close(ctx->gcli_significance);
	if (ctx->gcli_nonsig_flags)
		sigbuffer_close(ctx->gcli_nonsig_flags);
}

unpacking_context_t* unpacker_open(const xs_config_t* xs_config, const precinct_t* prec)
{
	unpacking_context_t* unpack_ctx = (unpacking_context_t*)malloc(sizeof(unpacking_context_t));
	if (!unpack_ctx)
	{
		return NULL;
	}
	unpack_ctx->xs_config = xs_config;
	unpack_ctx->level_count = bands_count_of(prec);
	unpack_ctx->gtli_table_data = (int*)malloc(line_count_of(prec) * sizeof(int));
	unpack_ctx->gtli_table_gcli = (int*)malloc(line_count_of(prec) * sizeof(int));	
	unpack_ctx->gclis_pred = (gcli_pred_t*)malloc((precinct_max_gcli_width_of(prec) + xs_config->p.N_g - 1) / xs_config->p.N_g * sizeof(gcli_pred_t));
	unpack_ctx->use_sign_subpkt = xs_config->p.Fs == 1;
	unpack_ctx->inclusion_mask = (int*)malloc((precinct_max_gcli_width_of(prec) + xs_config->p.N_g - 1) / xs_config->p.N_g * sizeof(int));
	unpack_ctx->enabled_methods = gcli_method_get_enabled(xs_config);
	unpack_ctx->gclis_significance = sigbuffer_open(prec);
#ifdef PACKING_GENERATE_FRAGMENT_CODE
	unpack_ctx->fragment_info_cb = NULL;
	unpack_ctx->fragment_info_context = NULL;
	unpack_ctx->fragment_cnt = 0;
#endif
	return unpack_ctx;
}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
void unpacker_set_fragment_info_cb(unpacking_context_t* unpack_ctx, xs_fragment_info_cb_t ficb, void* fictx)
{
	assert(unpack_ctx != NULL);
	unpack_ctx->fragment_info_cb = ficb;
	unpack_ctx->fragment_info_context = fictx;
}
#endif

void unpacker_close(unpacking_context_t* unpack_ctx)
{
	if (unpack_ctx)
	{
		sigbuffer_close(unpack_ctx->gclis_significance);
		if (unpack_ctx->gtli_table_data)
			free(unpack_ctx->gtli_table_data);
		if (unpack_ctx->gtli_table_gcli)
			free(unpack_ctx->gtli_table_gcli);
		if (unpack_ctx->gclis_pred)
			free(unpack_ctx->gclis_pred);
		if (unpack_ctx->inclusion_mask)
			free(unpack_ctx->inclusion_mask);
		free(unpack_ctx);
	}
}

int pack_data(bit_packer_t* bitstream, sig_mag_data_t* buf, int buf_len, gcli_data_t* gclis, int group_size, int gtli, const uint8_t sign_packing)
{
	int idx = 0, bit_len = 0;

	for (int group = 0; group < (buf_len + group_size - 1) / group_size; group++)
	{
		if (gclis[group] > gtli)
		{
			if (sign_packing == 0)
			{
				for (int i = 0; i < group_size; i++)
				{
					bit_len += bitpacker_write(bitstream, buf[idx + i] >> SIGN_BIT_POSITION, 1);
				}
			}
			for (int bp = gclis[group] - 1; bp >= gtli; --bp)
			{
				for (int i = 0; i < group_size; i++)
				{
					assert((idx + i < buf_len) || (buf[idx + i] == 0));  // optimal to have zeros outside right edge
					bit_len += bitpacker_write(bitstream, buf[idx + i] >> bp, 1);
				}
			}
		}
		idx += group_size;
	}
	return bit_len;
}

int unpack_data(bit_unpacker_t* bitstream, sig_mag_data_t* buf, int buf_len, gcli_data_t* gclis, int group_size, int gtli, const uint8_t sign_packing)
{
	uint64_t val;
	int idx = 0;

	for (int group = 0; group < (buf_len + group_size - 1) / group_size; group++)
	{
		memset(&buf[idx], 0, sizeof(sig_mag_data_t) * group_size);
		if (gclis[group] > gtli)
		{
			if (sign_packing == 0)
			{
				for (int i = 0; i < group_size; i++)
				{
					bitunpacker_read(bitstream, &val, 1);
					buf[idx + i] |= (sig_mag_data_t)val << SIGN_BIT_POSITION;
				}
			}
			for (int bp = gclis[group] - 1; bp >= gtli; --bp)
			{
				for (int i = 0; i < group_size; i++)
				{
					bitunpacker_read(bitstream, &val, 1);
					buf[idx + i] |= (sig_mag_data_t)((val & 0x01) << bp);
				}
			}
		}
		idx += group_size;
	}
	return 0;
}

int pack_sign(bit_packer_t* bitstream, sig_mag_data_t* buf, int buf_len, gcli_data_t* gclis, int group_size, int gtli)
{
	int bit_len = 0;
	for (int idx = 0, group = 0; group < (buf_len + group_size - 1) / group_size; ++group)
	{
		for (int i = 0; i < group_size; i++)
		{
			if ((gclis[group] > gtli) && ((buf[idx + i] & ~SIGN_BIT_MASK) >> gtli))
			{
				bit_len += bitpacker_write(bitstream, buf[idx + i] >> SIGN_BIT_POSITION, 1);
			}
		}
		idx += group_size;
	}
	return bit_len;
}

int unpack_sign(bit_unpacker_t* bitstream, sig_mag_data_t* buf, int buf_len, int group_size)
{
	for (int idx = 0, group = 0; group < (buf_len + group_size - 1) / group_size; ++group)
	{
		for (int i = 0; i < group_size; i++)
		{
			if (buf[idx + i] != 0)
			{
				uint64_t val;
				bitunpacker_read(bitstream, &val, 1);
				if (idx + i < buf_len)
				{
					buf[idx + i] |= (sig_mag_data_t)((val & 0x01) << SIGN_BIT_POSITION);
				}
			}
		}
		idx += group_size;
	}
	return 0;
}

int unary_encode(bit_packer_t* bitstream, gcli_pred_t* gcli_pred_buf, int len, int no_sign, unary_alphabet_t alph)
{
	int n = 1, bit_len = 0;
	int i;
	for (i = 0; (i < len) && (n > 0); i++)
	{
		if (!no_sign)
			n = bitpacker_write_unary_signed(bitstream, gcli_pred_buf[i], alph);
		else
			n = bitpacker_write_unary_unsigned(bitstream, gcli_pred_buf[i]);
		bit_len += n;
	}
	if (n <= 0)
		return -1;
	return bit_len;
}

int bounded_encode(bit_packer_t* bitstream, gcli_pred_t* gcli_pred_buf, gcli_data_t* predictors, int len, int gtli, int band_index)
{
	int n = 1, bit_len = 0;
	int i;
	for (i = 0; (i < len) && (n > 0); i++)
	{
		int min_value;
		int max_value;
		if (predictors)
		{
			bounded_code_get_min_max(predictors[i], gtli, &min_value, &max_value);
		}
		else
		{

			min_value = -20;
			max_value = 20;
		}
		n = bitpacker_write_unary_unsigned(bitstream, bounded_code_get_unary_code(gcli_pred_buf[i], min_value, max_value));
		bit_len += n;
	}
	if (n <= 0)
		return -1;
	return bit_len;
}

int unary_decode2(bit_unpacker_t* bitstream, gcli_pred_t* gcli_pred_buf, int* inclusion_mask, int len, int no_sign, unary_alphabet_t alph)
{
	int i;
	int nbits = 0;
	for (i = 0; i < len; i++)
	{
		if (inclusion_mask[i])
		{
			if (!no_sign)
				nbits += bitunpacker_read_unary_signed(bitstream, &(gcli_pred_buf[i]), alph);
			else
				nbits += bitunpacker_read_unary_unsigned(bitstream, &(gcli_pred_buf[i]));
		}
		else
		{
			gcli_pred_buf[i] = 0;
		}
	}
	return nbits;
}

int bounded_decode2(bit_unpacker_t* bitstream, gcli_pred_t* gcli_pred_buf, int* inclusion_mask, gcli_data_t* gcli_top_buf, int gtli, int gtli_top, int len)
{
	int i;
	int nbits = 0;
	for (i = 0; (i < len); i++)
	{
		if (inclusion_mask[i])
		{
			int min_value;
			int max_value;
			if (gcli_top_buf)
			{
				int predictor;
				predictor = tco_pred_ver_compute_predictor(gcli_top_buf[i], gtli_top, gtli);
				bounded_code_get_min_max(predictor, gtli, &min_value, &max_value);
			}
			else
			{

				min_value = -20;
				max_value = 20;
			}
			nbits += bitunpacker_read_bounded_code(bitstream, min_value, max_value, &gcli_pred_buf[i]);
		}
		else
		{
			gcli_pred_buf[i] = 0;
		}
	}
	return nbits;
}

int pack_raw_gclis(bit_packer_t* bitstream, gcli_data_t* gclis, int len)
{
	int bit_len = 0, n = 1;
	int i;
	for (i = 0; (i < len) && (n > 0); i++)
	{
		n = bitpacker_write(bitstream, gclis[i], 4);
		bit_len += n;
	}
	if (n <= 0)
		return -1;
	return bit_len;
}

int unpack_raw_gclis(bit_unpacker_t* bitstream, gcli_data_t* gclis, int len)
{
	int i;
	uint64_t val;
	for (i = 0; i < len; i++)
	{
		bitunpacker_read(bitstream, &val, 4);
		gclis[i] = (gcli_data_t)val;
	}
	return 0;
}


int pack_gclis_significance(packing_context_t* ctx, bit_packer_t* bitstream, precinct_t* precinct, rc_results_t* ra_result, int idx_start, int idx_stop)
{
	int nbits = 0;
	int idx;
	sig_flags_t* sig_flags = NULL;

	for (idx = idx_start; idx <= idx_stop; idx++)
	{
		const int lvl = precinct_band_index_of(precinct, idx);
		const int ypos = precinct_ypos_of(precinct, idx);
		if (!precinct_is_line_present(precinct, lvl, ypos))
		{
			continue;
		}
		const int gtli = ra_result->gtli_table_gcli[lvl];
		const int sb_gcli_method = ra_result->gcli_sb_methods[lvl];
		if (method_uses_sig_flags(sb_gcli_method))
		{
			int gcli_len;
			directional_prediction_t* pred_res;
			directional_prediction_t* pred_sig_flags;
			if (!sig_flags)
			{
				sig_flags = sig_flags_malloc(precinct_max_gcli_width_of(precinct), ctx->xs_config->p.S_s);
			}

			pred_res = directional_data_of(ra_result->pred_residuals, method_get_pred(sb_gcli_method));
			pred_sig_flags = pred_res;
			if (method_get_run(sb_gcli_method) == RUN_SIGFLAGS_ZRCSF)
				pred_sig_flags = directional_data_of(ra_result->pred_residuals, PRED_NONE);

			gcli_len = prediction_width_of(pred_sig_flags, lvl);

			sig_flags_init(sig_flags, prediction_values_of(pred_sig_flags, lvl, ypos, gtli), gcli_len, ctx->xs_config->p.S_s);

			sig_flags_filter_values(sig_flags, prediction_values_of(pred_res, lvl, ypos, gtli), sig_values_of(ctx->gcli_significance, lvl, ypos), sig_width_of(ctx->gcli_significance, lvl, ypos));
			sig_flags_filter_values(sig_flags, prediction_predictors_of(pred_res, lvl, ypos, gtli), sig_predictors_of(ctx->gcli_significance, lvl, ypos), sig_width_of(ctx->gcli_significance, lvl, ypos));

			nbits += sig_flags_write(bitstream, sig_flags);
		}

		if (ctx->xs_config->verbose > 3)
		{
			fprintf(stderr, "SIGF (bitpos=%d) (lvl=%d ypos=%d) (w=%d,wg=%d))\n", (int)bitpacker_get_len(bitstream), lvl, ypos, (int)precinct_width_of(precinct, lvl), (int)precinct_gcli_width_of(precinct, lvl));
		}
	}
	if (sig_flags)
		sig_flags_free(sig_flags);
	return nbits;
}

int unpack_gclis_significance(bit_unpacker_t* bitstream, unpacking_context_t* ctx, precinct_t* prec, int* sb_gcli_methods, int idx_start, int idx_stop)
{
	int bits = 0;
	sig_flags_t* sig_flags = NULL;
	for (int idx = idx_start; idx <= idx_stop; idx++)
	{
		int lvl = precinct_band_index_of(prec, idx);
		int ypos = precinct_ypos_of(prec, idx);
		if (!precinct_is_line_present(prec, lvl, ypos))
		{
			continue;
		}
		const int sb_gcli_method = sb_gcli_methods[lvl];
		if (method_uses_sig_flags(sb_gcli_method))
		{
			if (!sig_flags)
				sig_flags = sig_flags_malloc((int)precinct_max_gcli_width_of(prec), ctx->xs_config->p.S_s);

			bits = sig_flags_read(bitstream, sig_flags, precinct_gcli_width_of(prec, lvl), ctx->xs_config->p.S_s);
			sig_flags_inclusion_mask(sig_flags, (uint8_t*)sig_values_of(ctx->gclis_significance, lvl, ypos));
		}
		if (ctx->xs_config->verbose > 3)
		{
			fprintf(stderr, "SIGF (bitpos=%d) (lvl=%d ypos=%d) (w=%d,wg=%d))\n", (int)bitunpacker_consumed_bits(bitstream), lvl, ypos, (int)precinct_width_of(prec, lvl), (int)precinct_gcli_width_of(prec, lvl));
		}
	}
	if (sig_flags)
		sig_flags_free(sig_flags);
	return bits;
}

int pack_gclis(
	packing_context_t* ctx,
	bit_packer_t* bitstream,
	precinct_t* precinct,
	rc_results_t* ra_result,
	const int position)
{
	int lvl = precinct_band_index_of(precinct, position);
	int ypos = precinct_ypos_of(precinct, position);
	int gtli = ra_result->gtli_table_gcli[lvl];
	int subpkt = precinct_subpkt_of(precinct, position);
	int sb_method = ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt] == 1 ? method_get_idx(ALPHABET_RAW_4BITS, 0, 0) : ra_result->gcli_sb_methods[lvl];
	bool method_use_gcli_opt = method_uses_sig_flags(sb_method);
	unary_alphabet_t alph = FIRST_ALPHABET;
	int values_len;
	gcli_pred_t* values_to_code;
	gcli_data_t* predictors;
	int unsigned_unary_alph;

	if (ctx->xs_config->verbose > 3)
	{
		fprintf(stderr, "GCLI (bitpos=%d) (Dr=%d, method=%d) lvl=%d ypos=%d gtli=%d)\n", bitpacker_get_len(bitstream), ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt], sb_method, lvl, ypos, gtli);
	}

	if (ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt] == 1)
	{
		return pack_raw_gclis(bitstream, precinct_gcli_of(precinct, lvl, ypos), (int)precinct_gcli_width_of(precinct, lvl)) < 0;
	}

	int err = 0;
	int nbits = 0;
	if (method_use_gcli_opt)
	{
		values_to_code = sig_values_of(ctx->gcli_significance, lvl, ypos);
		values_len = *sig_width_of(ctx->gcli_significance, lvl, ypos);
		predictors = sig_predictors_of(ctx->gcli_significance, lvl, ypos);
	}
	else
	{
		int pred_type;
		directional_prediction_t* pred;
		pred_type = method_prediction_type(sb_method);
		assert(pred_type >= 0);
		pred = directional_data_of(ra_result->pred_residuals, pred_type);
		values_to_code = prediction_values_of(pred, lvl, ypos, gtli);
		values_len = prediction_width_of(pred, lvl);
		predictors = prediction_predictors_of(pred, lvl, ypos, gtli);
	}
	unsigned_unary_alph = method_uses_no_pred(sb_method);
	if ((method_get_alphabet(sb_method) != ALPHABET_UNARY_UNSIGNED_BOUNDED) || (method_get_pred(sb_method) == PRED_NONE))
	{
		err = (err) || ((nbits = unary_encode(bitstream, values_to_code, values_len, unsigned_unary_alph, alph)) < 0);
	}
	else
	{
		err = (err) || ((nbits = bounded_encode(bitstream, values_to_code, predictors, values_len, gtli, lvl)) < 0);
	}
	assert(nbits >= 0);
	return err;
}

int unpack_gclis(
	bit_unpacker_t* bitstream,
	precinct_t* precinct,
	precinct_t* precinct_top,
	int* gtli_table_top,
	unpacking_context_t* ctx,
	const int lvl,
	const int ypos,
	const int sb_gcli_method,
	const int scenario)
{
	const int gtli = ctx->gtli_table_gcli[lvl];
	const int gtli_top = (ypos == 0) ? (gtli_table_top[lvl]) : (gtli);
	const int gcli_width = (int)precinct_gcli_width_of(precinct, lvl);
	gcli_data_t* gclis = precinct_gcli_of(precinct, lvl, ypos);
	unary_alphabet_t alph = FIRST_ALPHABET;

	if (ctx->xs_config->verbose > 3)
	{
		fprintf(stderr, "GCLI (bitpos=%d) (Dr=%d, method=%d) lvl=%d ypos=%d gtli=%d)\n", (int)bitunpacker_consumed_bits(bitstream), method_is_raw(sb_gcli_method), sb_gcli_method, lvl, ypos, gtli);
	}

	if (method_is_raw(sb_gcli_method))
	{
		unpack_raw_gclis(bitstream, gclis, gcli_width);
	}
	else
	{
		int i, nbits;
		int no_prediction;
		int sig_flags_are_zrcsf = (method_get_run(sb_gcli_method) == RUN_SIGFLAGS_ZRCSF);
		gcli_data_t* gclis_top = precinct_gcli_top_of(precinct, precinct_top, lvl, ypos);

		if (method_uses_sig_flags(sb_gcli_method))
			for (i = 0; i < gcli_width; i++)
				ctx->inclusion_mask[i] = sig_values_of(ctx->gclis_significance, lvl, ypos)[i];
		else
			for (i = 0; i < gcli_width; i++)
				ctx->inclusion_mask[i] = 1;

		no_prediction = method_uses_no_pred(sb_gcli_method) || ((method_uses_ver_pred(sb_gcli_method) && method_get_alphabet(sb_gcli_method) == ALPHABET_UNARY_UNSIGNED_BOUNDED) && (gclis_top == NULL));

		if ((method_get_alphabet(sb_gcli_method) != ALPHABET_UNARY_UNSIGNED_BOUNDED) || no_prediction)
			nbits = unary_decode2(bitstream, ctx->gclis_pred, ctx->inclusion_mask, gcli_width, no_prediction, alph);
		else
			nbits = bounded_decode2(bitstream, ctx->gclis_pred, ctx->inclusion_mask, gclis_top, gtli, gtli_top, gcli_width);
		assert(nbits >= 0);

		if ((method_uses_ver_pred(sb_gcli_method) && gclis_top))
		{
			tco_pred_ver_inverse(ctx->gclis_pred, gclis_top, gcli_width, gclis, gtli, gtli_top);
			if (sig_flags_are_zrcsf)
				for (i = 0; i < gcli_width; i++)
					if (!ctx->inclusion_mask[i])
						gclis[i] = 0;
		}
		else if (method_uses_no_pred(sb_gcli_method) || (method_uses_ver_pred(sb_gcli_method) && gclis_top == NULL))
		{
			tco_pred_none_inverse(ctx->gclis_pred, gcli_width, gclis, gtli);
		}
		else
		{
			assert(0);
		}
	}
	return 0;
}

int pack_precinct(packing_context_t* ctx, bit_packer_t* bitstream, precinct_t* precinct, rc_results_t* ra_result)
{
	int err = 0;
	int len_after = 0;
	int lvl, ypos, idx_start, idx_stop, idx, gtli;
	int len_before_subpkt = 0;
	int subpkt;

	if (ctx->xs_config->verbose > 2)
	{
		fprintf(stderr, "(bitpos=%d) Precinct\n", (int)bitpacker_get_len(bitstream));
	}

	const int position_count = line_count_of(precinct);
	const bool use_long_precinct_headers = precinct_use_long_headers(precinct);

	err = err || (bitpacker_write(bitstream, ((ra_result->precinct_total_bits - ra_result->pbinfo.prec_header_size) >> 3), PREC_HDR_PREC_SIZE) < 0);
	assert(ra_result->quantization < 16);
	err = err || (bitpacker_write(bitstream, ra_result->quantization, PREC_HDR_QUANTIZATION_SIZE) < 0);
	err = err || (bitpacker_write(bitstream, ra_result->refinement, PREC_HDR_REFINEMENT_SIZE) < 0);

	for (int band = 0; band < bands_count_of(precinct); ++band)
	{
		const int method_signaling = gcli_method_get_signaling(ra_result->gcli_sb_methods[band], ctx->enabled_methods);
		err = err || (bitpacker_write(bitstream, method_signaling, GCLI_METHOD_NBITS) < 0);
	}
	bitpacker_align(bitstream, PREC_HDR_ALIGNMENT);

	const int len_before_prc_data = (int)bitpacker_get_len(bitstream);

	if (ctx->xs_config->verbose > 3)
	{
		fprintf(stderr, "(bitpos=%d) precinct header packed (%d %d) (ra=%d)\n", bitpacker_get_len(bitstream), ra_result->quantization, ra_result->refinement, ((ra_result->precinct_total_bits - ra_result->pbinfo.prec_header_size) >> 3));
	}

	subpkt = 0;
	for (idx_start = idx_stop = 0; idx_stop < position_count; idx_stop++)
	{
		if ((idx_stop != (position_count - 1)) && (precinct_subpkt_of(precinct, idx_stop) == precinct_subpkt_of(precinct, idx_stop + 1)))
		{
			continue;
		}

		lvl = precinct_band_index_of(precinct, idx_start);
		ypos = precinct_ypos_of(precinct, idx_start);
		if (!precinct_is_line_present(precinct, lvl, ypos))
		{
			++subpkt;
			idx_start = idx_stop + 1;
			continue;
		}

		bitpacker_write(bitstream, (uint64_t)ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt], 1);
		bitpacker_write(bitstream, (uint64_t)ra_result->pbinfo.subpkt_size_data[subpkt] >> 3, use_long_precinct_headers ? PKT_HDR_DATA_SIZE_LONG : PKT_HDR_DATA_SIZE_SHORT);
		bitpacker_write(bitstream, (uint64_t)ra_result->pbinfo.subpkt_size_gcli[subpkt] >> 3, use_long_precinct_headers ? PKT_HDR_GCLI_SIZE_LONG : PKT_HDR_GCLI_SIZE_SHORT);
		bitpacker_write(bitstream, (uint64_t)ra_result->pbinfo.subpkt_size_sign[subpkt] >> 3, use_long_precinct_headers ? PKT_HDR_SIGN_SIZE_LONG : PKT_HDR_SIGN_SIZE_SHORT);
		bitpacker_align(bitstream, PKT_HDR_ALIGNMENT);

		if (ctx->xs_config->verbose > 3)
		{
			fprintf(stderr, "(bitpos=%d) Subpacket Ldat%d Lcnt=%d Lsgn=%d (Dr=%d)\n", bitpacker_get_len(bitstream),
				ra_result->pbinfo.subpkt_size_data[subpkt] >> 3,
				ra_result->pbinfo.subpkt_size_gcli[subpkt] >> 3,
				ra_result->pbinfo.subpkt_size_sign[subpkt] >> 3,
				ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt]);
		}

		len_before_subpkt = bitpacker_get_len(bitstream);
		if (ra_result->pbinfo.subpkt_uses_raw_fallback[subpkt] == 0)
		{
			pack_gclis_significance(ctx, bitstream, precinct, ra_result, idx_start, idx_stop);
		}
		bitpacker_align(bitstream, SUBPKT_ALIGNMENT);

		if ((bitpacker_get_len(bitstream) - len_before_subpkt) != ra_result->pbinfo.subpkt_size_sigf[subpkt])
		{
			fprintf(stderr, "Error: (SIG) length mismatch - rate_control=%d packing=%d\n", ra_result->pbinfo.subpkt_size_sigf[subpkt], bitpacker_get_len(bitstream) - len_before_subpkt);
			return -1;
		}

		len_before_subpkt = bitpacker_get_len(bitstream);
		for (idx = idx_start; idx <= idx_stop; idx++)
		{
			lvl = precinct_band_index_of(precinct, idx);
			ypos = precinct_ypos_of(precinct, idx);
			if (precinct_is_line_present(precinct, lvl, ypos))
			{
				err = (err) || pack_gclis(ctx, bitstream, precinct, ra_result, idx);
				if (err)
					return -1;
			}
		}
		bitpacker_align(bitstream, SUBPKT_ALIGNMENT);

		if ((bitpacker_get_len(bitstream) - len_before_subpkt) != ra_result->pbinfo.subpkt_size_gcli[subpkt])
		{
			fprintf(stderr, "Error: (GCLI) length mismatch - rate_control=%d packing=%d\n", ra_result->pbinfo.subpkt_size_gcli[subpkt], bitpacker_get_len(bitstream) - len_before_subpkt);
			return -1;
		}

		len_before_subpkt = bitpacker_get_len(bitstream);
		for (idx = idx_start; idx <= idx_stop; idx++)
		{
			lvl = precinct_band_index_of(precinct, idx);
			ypos = precinct_ypos_of(precinct, idx);
			if (precinct_is_line_present(precinct, lvl, ypos))
			{
				gtli = ra_result->gtli_table_data[lvl];
				if (ctx->xs_config->verbose > 3)
				{
					fprintf(stderr, "DATA (bitpos=%d) (lvl=%d ypos=%d gtli=%d)\n", bitpacker_get_len(bitstream), lvl, ypos, gtli);
				}
				err = err || (pack_data(bitstream, precinct_line_of(precinct, lvl, ypos), (int)precinct_width_of(precinct, lvl), precinct_gcli_of(precinct, lvl, ypos), ctx->xs_config->p.N_g, gtli, ctx->xs_config->p.Fs) < 0);
			}
		}
		bitpacker_align(bitstream, SUBPKT_ALIGNMENT);

		if ((bitpacker_get_len(bitstream) - len_before_subpkt) != ra_result->pbinfo.subpkt_size_data[subpkt])
		{
			fprintf(stderr, "Error: (DATA) length mismatch - rate_control=%d packing=%d\n", ra_result->pbinfo.subpkt_size_data[subpkt], bitpacker_get_len(bitstream) - len_before_subpkt);
			return -1;
		}

		len_before_subpkt = bitpacker_get_len(bitstream);
		if (ctx->xs_config->p.Fs == 1)
		{
			for (idx = idx_start; idx <= idx_stop; idx++)
			{
				lvl = precinct_band_index_of(precinct, idx);
				ypos = precinct_ypos_of(precinct, idx);
				if (precinct_is_line_present(precinct, lvl, ypos))
				{
					gtli = ra_result->gtli_table_data[lvl];
					if (ctx->xs_config->verbose > 3)
					{
						fprintf(stderr, "SIGN (bitpos=%d) (lvl=%d ypos=%d gtli=%d)\n", bitpacker_get_len(bitstream), lvl, ypos, gtli);
					}
					err = err || (pack_sign(bitstream, precinct_line_of(precinct, lvl, ypos), (int)precinct_width_of(precinct, lvl), precinct_gcli_of(precinct, lvl, ypos), ctx->xs_config->p.N_g, gtli) < 0);
				}
			}
			bitpacker_align(bitstream, SUBPKT_ALIGNMENT);
		}
		if ((bitpacker_get_len(bitstream) - len_before_subpkt) != ra_result->pbinfo.subpkt_size_sign[subpkt])
		{
			fprintf(stderr, "Error: (SIGN) length mismatch - rate_control=%d packing=%d\n", ra_result->pbinfo.subpkt_size_sign[subpkt], bitpacker_get_len(bitstream) - len_before_subpkt);
			return -1;
		}

		++subpkt;
		idx_start = idx_stop + 1;
	}

	if (err)
	{
		fprintf(stderr, "Error: packing could not write all data to output buffer\n");
		return -1;
	}
	len_after = bitpacker_get_len(bitstream);

	if ((ra_result->pbinfo.precinct_bits > 0) && ((len_after - len_before_prc_data) != (ra_result->pbinfo.precinct_bits - ra_result->pbinfo.prec_header_size)))
	{
		fprintf(stderr, "Error: (packed_len != computed_len) packed=%d expected=%d\n", len_after - len_before_prc_data, ra_result->pbinfo.precinct_bits - ra_result->pbinfo.prec_header_size);
		return -1;
	}
	if (len_after % 4)
	{
		fprintf(stderr, "Error: end of precincts are supposed to be aligned on 4 bits\n");
		return -1;
	}

	bitpacker_add_padding(bitstream, ra_result->padding_bits);

	return 0;
}

int unpack_precinct(unpacking_context_t* ctx, bit_unpacker_t* bitstream, precinct_t* precinct, precinct_t* precinct_top, int* gtli_table_top, unpacked_info_t* info, int extra_bits_before_precinct)
{
	uint64_t val;
	int empty;
	int len_before_subpkt = 0;
	int gcli_sb_methods[MAX_NBANDS];
	int subpkt = 0;
	int subpkt_len;

	const int position_count = line_count_of(precinct);
	const bool use_long_precinct_headers = precinct_use_long_headers(precinct);
	const int bitpos_prc_start = (int)bitunpacker_consumed_bits(bitstream);

	if (ctx->xs_config->verbose > 2)
	{
		fprintf(stderr, "(bitpos=%d) Precinct (bytepos=%d)\n", bitpos_prc_start, bitpos_prc_start >> 3);
	}

	// Start of precinct.
	bitunpacker_read(bitstream, &val, PREC_HDR_PREC_SIZE);
	const int Lprc = ((int)val << 3);
	bitunpacker_read(bitstream, &val, PREC_HDR_QUANTIZATION_SIZE);
	const int quantization = (int)val;
	bitunpacker_read(bitstream, &val, PREC_HDR_REFINEMENT_SIZE);
	const int refinement = (int)val;

	for (int band = 0; band < bands_count_of(precinct); ++band)
	{
		bitunpacker_read(bitstream, &val, GCLI_METHOD_NBITS);
		gcli_sb_methods[band] = gcli_method_from_signaling((int)val, ctx->enabled_methods);
	}
	bitunpacker_align(bitstream, PREC_HDR_ALIGNMENT);

	const int bitpos_at_prc_data = (int)bitunpacker_consumed_bits(bitstream);
#ifdef PACKING_GENERATE_FRAGMENT_CODE
	// Add precinct header bits.
	extra_bits_before_precinct += bitpos_at_prc_data - bitpos_prc_start;
#endif

	if (ctx->xs_config->verbose > 3)
	{
		fprintf(stderr, "(bitpos=%d) precinct header read (prec_len=%d quant=(%d,%d)\n", bitpos_at_prc_data, Lprc, quantization, refinement);
	}

	compute_gtli_tables(quantization, refinement, ctx->level_count, ctx->xs_config->p.lvl_gains, ctx->xs_config->p.lvl_priorities, ctx->gtli_table_data, ctx->gtli_table_gcli, &empty);

	for (int idx_start = 0, idx_stop = 0; idx_stop < position_count; idx_stop++)
	{
		if ((idx_stop != (position_count - 1)) && (precinct_subpkt_of(precinct, idx_stop) == precinct_subpkt_of(precinct, idx_stop + 1)))
		{
			continue;
		}

		if (!precinct_is_line_present(precinct, precinct_band_index_of(precinct, idx_start), precinct_ypos_of(precinct, idx_start)))
		{
			++subpkt;
			idx_start = idx_stop + 1;
			continue;
		}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
		const int bitpos_packet_start = (int)bitunpacker_consumed_bits(bitstream);
#endif

		// Start of packet.
		bitunpacker_read(bitstream, &val, 1);
		const int uses_raw_fallback = (int)val & 0x1;

		bitunpacker_read(bitstream, &val, use_long_precinct_headers ? PKT_HDR_DATA_SIZE_LONG : PKT_HDR_DATA_SIZE_SHORT);
		info->data_len[subpkt] = (int)val;

		bitunpacker_read(bitstream, &val, use_long_precinct_headers ? PKT_HDR_GCLI_SIZE_LONG : PKT_HDR_GCLI_SIZE_SHORT);
		info->gcli_len[subpkt] = (int)val;

		bitunpacker_read(bitstream, &val, use_long_precinct_headers ? PKT_HDR_SIGN_SIZE_LONG : PKT_HDR_SIGN_SIZE_SHORT);
		info->sign_len[subpkt] = (int)val;

		bitunpacker_align(bitstream, PKT_HDR_ALIGNMENT);

		if (ctx->xs_config->verbose > 2)
		{
			const int bitpos = (int)bitunpacker_consumed_bits(bitstream);
			fprintf(stderr, "(bitpos=%d) Subpacket DATALEN=%d GCLILEN=%d SIGNLEN=%d (force_raw%d)\n", bitpos,
				info->data_len[subpkt],
				info->gcli_len[subpkt],
				info->sign_len[subpkt],
				uses_raw_fallback);
		}

		if (!uses_raw_fallback)
		{
			unpack_gclis_significance(bitstream, ctx, precinct, gcli_sb_methods, idx_start, idx_stop);
		}
		bitunpacker_align(bitstream, SUBPKT_ALIGNMENT);

		len_before_subpkt = (int)bitunpacker_consumed_bits(bitstream);

		for (int idx = idx_start; idx <= idx_stop; idx++)
		{
			const int lvl = precinct_band_index_of(precinct, idx);
			const int ypos = precinct_ypos_of(precinct, idx);
			if (precinct_is_line_present(precinct, lvl, ypos))
			{
				int sb_gcli_method = uses_raw_fallback ? method_get_idx(ALPHABET_RAW_4BITS, 0, 0) : gcli_sb_methods[precinct_band_index_of(precinct, idx)];
				unpack_gclis(bitstream, precinct, precinct_top, gtli_table_top, ctx, lvl, ypos, sb_gcli_method, quantization);
			}
		}

		bitunpacker_align(bitstream, SUBPKT_ALIGNMENT);

		subpkt_len = (int)bitunpacker_consumed_bits(bitstream) - len_before_subpkt;
		if (subpkt_len != (info->gcli_len[subpkt] << 3))
		{
			fprintf(stderr, "Error: (GCLI) corruption detected - unpacked=%d , expected=%d\n", subpkt_len, info->gcli_len[subpkt] << 3);
			return -1;
		}

		len_before_subpkt = (int)bitunpacker_consumed_bits(bitstream);

		for (int idx = idx_start; idx <= idx_stop; idx++)
		{
			const int lvl = precinct_band_index_of(precinct, idx);
			const int ypos = precinct_ypos_of(precinct, idx);
			if (precinct_is_line_present(precinct, lvl, ypos))
			{
				const int gtli = ctx->gtli_table_data[lvl];
				if (ctx->xs_config->verbose > 4)
				{
					fprintf(stderr, "DATA (bitpos=%d) ", (int)bitunpacker_consumed_bits(bitstream));
					fprintf(stderr, " (lvl=%d ypos=%d gtli=%d)\n", lvl, ypos, gtli);
				}
				unpack_data(bitstream, precinct_line_of(precinct, lvl, ypos), (int)precinct_width_of(precinct, lvl), precinct_gcli_of(precinct, lvl, ypos), ctx->xs_config->p.N_g, gtli, ctx->use_sign_subpkt);
			}
		}
		bitunpacker_align(bitstream, SUBPKT_ALIGNMENT);

		subpkt_len = (int)bitunpacker_consumed_bits(bitstream) - len_before_subpkt;
		if (subpkt_len != (info->data_len[subpkt] << 3))
		{
			fprintf(stderr, "Error: (DATA) corruption detected - unpacked=%d , expected=%d\n", subpkt_len, info->data_len[subpkt] << 3);
			return -1;
		}

		if (ctx->use_sign_subpkt)
		{
			len_before_subpkt = (int)bitunpacker_consumed_bits(bitstream);
			for (int idx = idx_start; idx <= idx_stop; idx++)
			{
				const int lvl = precinct_band_index_of(precinct, idx);
				const int ypos = precinct_ypos_of(precinct, idx);
				if (precinct_is_line_present(precinct, lvl, ypos))
				{
					if (ctx->xs_config->verbose > 3)
					{
						fprintf(stderr, "SIGN (bitpos=%d) (lvl=%d ypos=%d gtli=%d)\n", (int)bitunpacker_consumed_bits(bitstream), lvl, ypos, ctx->gtli_table_data[lvl]);
					}
					unpack_sign(bitstream, precinct_line_of(precinct, lvl, ypos), (int)precinct_width_of(precinct, lvl), ctx->xs_config->p.N_g);
				}
			}
			bitunpacker_align(bitstream, SUBPKT_ALIGNMENT);

			subpkt_len = (int)bitunpacker_consumed_bits(bitstream) - len_before_subpkt;
			if (subpkt_len != (info->sign_len[subpkt] << 3))
			{
				fprintf(stderr, "Error: (SIGN) corruption detected - unpacked=%d , expected=%d\n", subpkt_len, info->sign_len[subpkt] << 3);
				return -1;
			}
		}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
		if (ctx->fragment_info_cb != NULL)
		{
			int n_gclis = 0;
			for (int idx = idx_start; idx <= idx_stop; idx++)
			{
				n_gclis += (int)precinct_gcli_width_of(precinct, precinct_band_index_of(precinct, idx));
			}

			const int bitpos_packet_end = (int)bitunpacker_consumed_bits(bitstream);
			// account for EOC if really last fragment (the decoder will verify the actual EOC being present)
			const int _fragment_size = bitpos_packet_end - bitpos_packet_start + extra_bits_before_precinct;
			extra_bits_before_precinct = 0;
			ctx->fragment_info_cb(ctx->fragment_info_context, ctx->fragment_cnt, _fragment_size, n_gclis, 0);
			++ctx->fragment_cnt;
		}
#endif

		++subpkt;
		idx_start = idx_stop + 1;
	}

	const int padding_len = Lprc - ((int)bitunpacker_consumed_bits(bitstream) - bitpos_at_prc_data);
	assert(padding_len >= 0);
#ifdef PACKING_GENERATE_FRAGMENT_CODE
	if (ctx->fragment_info_cb != NULL && padding_len > 0)
	{
		// Late contribution (the padding), so refer to last fragment ID (value -1).
		ctx->fragment_info_cb(ctx->fragment_info_context, -1, 0, 0, padding_len);
	}
#endif
	bitunpacker_skip(bitstream, padding_len);

	memcpy(info->gtli_table_data, ctx->gtli_table_data, position_count * sizeof(int));
	memcpy(info->gtli_table_gcli, ctx->gtli_table_gcli, position_count * sizeof(int));
	return 0;
}
