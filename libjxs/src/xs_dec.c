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

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "libjxs.h"
#include "xs_config.h"
#include "xs_markers.h"
#include "common.h"
#include "precinct.h"
#include "buf_mgmt.h"
#include "bitpacking.h"
#include "budget.h"
#include "packing.h"
#include "quant.h"
#include "ids.h"
#include "dwt.h"
#include "mct.h"
#include "nlt.h"

typedef struct xs_buffering_fragment_t xs_buffering_fragment_t;
struct xs_buffering_fragment_t
{
	int id;
	int Sbits;
	int Ncg;
	int padding_bits;
};

struct xs_dec_context_t
{
	const xs_config_t* xs_config;
	ids_t ids;

	precinct_t* precinct[MAX_PREC_COLS];
	precinct_t* precinct_top[MAX_PREC_COLS];
	int	gtlis_table_top[MAX_PREC_COLS][MAX_NBANDS];

	bit_unpacker_t* bitstream;
	unpacking_context_t* unpack_ctx;

#ifdef PACKING_GENERATE_FRAGMENT_CODE
	xs_fragment_info_cb_t user_fragment_info_cb;
	void* user_fragment_info_context;
	xs_buffering_fragment_t fragment_info_buf;
#endif
};

bool xs_dec_probe(uint8_t* bitstream_buf, size_t len, xs_config_t* xs_config, xs_image_t* image)
{
	bit_unpacker_t* bitstream = bitunpacker_init();
	bitunpacker_set_buffer(bitstream, bitstream_buf, len);
	const bool ok = xs_parse_head(bitstream, image, xs_config);
	bitunpacker_close(bitstream);
	if (xs_config->profile == XS_PROFILE_MLS_12)
	{
		xs_config->bitstream_size_in_bytes = (size_t)-1;
	}
	else if (xs_config->bitstream_size_in_bytes == 0)
	{
		xs_config->bitstream_size_in_bytes = len;
	}
	return ok;
}

xs_dec_context_t* xs_dec_init(xs_config_t* xs_config, xs_image_t* image)
{
	xs_dec_context_t* ctx;

	ctx = (xs_dec_context_t*)malloc(sizeof(xs_dec_context_t));
	if (!ctx)
	{
		return NULL;
	}

	memset(ctx, 0, sizeof(xs_dec_context_t));
	ctx->xs_config = xs_config;

	assert(xs_config != NULL);
	if (!xs_config_validate(xs_config, image))
	{
		free(ctx);
		return NULL;
	}

	ids_construct(&ctx->ids, image, xs_config->p.NLx, xs_config->p.NLy, xs_config->p.Sd, xs_config->p.Cw, xs_config->p.Lh);

	for (int column = 0; column < ctx->ids.npx; column++)
	{
		ctx->precinct[column] = precinct_open_column(&ctx->ids, ctx->xs_config->p.N_g, column);
		ctx->precinct_top[column] = precinct_open_column(&ctx->ids, ctx->xs_config->p.N_g, column);
	}
	ctx->unpack_ctx = unpacker_open(xs_config, ctx->precinct[0]);
	ctx->bitstream = bitunpacker_init();
	return ctx;
}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
void _xs_delayed_fragment_info_collecter_cb(void* vctx, const int f_id, const int f_Sbits, const int f_Ncg, const int f_padding_bits)
{
	xs_dec_context_t* ctx = (xs_dec_context_t*)vctx;
	// Delays calling thru the CB handler to allow additional late counts (signaled by f_id == -1).
	if (f_id == -1)
	{
		assert(ctx->fragment_info_buf.id != -1);
		ctx->fragment_info_buf.Sbits += f_Sbits;
		ctx->fragment_info_buf.Ncg += f_Ncg;
		ctx->fragment_info_buf.padding_bits += f_padding_bits;
		return;
	}
	if (ctx->fragment_info_buf.id >= 0)
	{
		// Send previous buffered info.
		ctx->user_fragment_info_cb(ctx->user_fragment_info_context, ctx->fragment_info_buf.id, ctx->fragment_info_buf.Sbits, ctx->fragment_info_buf.Ncg, ctx->fragment_info_buf.padding_bits);
	}
	// Buffer this info, but do not yet send.
	ctx->fragment_info_buf.id = f_id;
	ctx->fragment_info_buf.Sbits = f_Sbits;
	ctx->fragment_info_buf.Ncg = f_Ncg;
	ctx->fragment_info_buf.padding_bits = f_padding_bits;
}

void _xs_flush_fragment_info(xs_dec_context_t* ctx)
{
	if (ctx->user_fragment_info_cb != NULL && ctx->fragment_info_buf.id >= 0)
	{
		ctx->user_fragment_info_cb(ctx->user_fragment_info_context, ctx->fragment_info_buf.id, ctx->fragment_info_buf.Sbits, ctx->fragment_info_buf.Ncg, ctx->fragment_info_buf.padding_bits);
		ctx->fragment_info_buf.id = -1;
	}
}
#endif

bool xs_dec_set_fragment_info_cb(xs_dec_context_t* ctx, xs_fragment_info_cb_t ficb, void* fictx)
{
	assert(ctx != NULL);
#ifdef PACKING_GENERATE_FRAGMENT_CODE
	ctx->user_fragment_info_cb = ficb;
	ctx->user_fragment_info_context = fictx;
	// Set the delayed fragment info collector CB.
	unpacker_set_fragment_info_cb(ctx->unpack_ctx, _xs_delayed_fragment_info_collecter_cb, ctx);
	return true;
#else
	return ficb == NULL;
#endif
}

void xs_dec_close(xs_dec_context_t* ctx)
{
	if (!ctx)
	{
		return;
	}

	for (int column = 0; column < ctx->ids.npx; column++)
	{
		precinct_close(ctx->precinct[column]);
		precinct_close(ctx->precinct_top[column]);
	}
	unpacker_close(ctx->unpack_ctx);
	bitunpacker_close(ctx->bitstream);
	free(ctx);
}

void swap_ptrs(precinct_t** p1, precinct_t** p2)
{
	precinct_t* tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

bool xs_dec_bitstream(xs_dec_context_t* ctx, void* bitstream_buf, size_t bitstream_buf_size, xs_image_t* image_out)
{
	int slice_idx = 0;
	uint64_t bitstream_pos = 0;

	bitunpacker_set_buffer(ctx->bitstream, bitstream_buf, bitstream_buf_size);

	xs_parse_head(ctx->bitstream, NULL, NULL);

#ifdef PACKING_GENERATE_FRAGMENT_CODE
	memset(&ctx->fragment_info_buf, 0, sizeof(xs_buffering_fragment_t));
	ctx->fragment_info_buf.id = -1;
#endif

	for (int line_idx = 0; line_idx < ctx->ids.h; line_idx += ctx->ids.ph)
	{
		unpacked_info_t unpack_out;
		const int prec_y_idx = (line_idx / ctx->ids.ph);
		for (int column = 0; column < ctx->ids.npx; column++)
		{
			precinct_set_y_idx_of(ctx->precinct[column], prec_y_idx);
			const int first_of_slice = precinct_is_first_of_slice(ctx->precinct[column], ctx->xs_config->p.slice_height);

			if (first_of_slice && column == 0)
			{
				int slice_idx_check;
				xs_parse_slice_header(ctx->bitstream, &slice_idx_check);
				assert(slice_idx_check == (slice_idx++));
				if (ctx->xs_config->verbose > 1)
				{
					fprintf(stderr, "Read Slice Header (slice_idx=%d)\n", slice_idx_check);
				}
			}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
			const int extra_bits_before_precinct = (int)(bitunpacker_consumed_bits(ctx->bitstream) - bitstream_pos);
#else
			const int extra_bits_before_precinct = 0;
#endif
			if (unpack_precinct(ctx->unpack_ctx, ctx->bitstream, ctx->precinct[column],
				(!first_of_slice) ? ctx->precinct_top[column] : NULL, ctx->gtlis_table_top[column],
				&unpack_out, extra_bits_before_precinct) < 0)
			{
				if (ctx->xs_config->verbose) fprintf(stderr, "Corrupted codestream! line number %d\n", line_idx);
				return false;
			}
			bitstream_pos = bitunpacker_consumed_bits(ctx->bitstream);

			dequantize_precinct(ctx->precinct[column], unpack_out.gtli_table_data, ctx->xs_config->p.Qpih);

			precinct_to_image(ctx->precinct[column], image_out, ctx->xs_config->p.Fq);

			swap_ptrs(&ctx->precinct_top[column], &ctx->precinct[column]);
			memcpy(ctx->gtlis_table_top[column], unpack_out.gtli_table_gcli, MAX_NBANDS * sizeof(int));
		}
	}

	dwt_inverse_transform(&ctx->ids, image_out);
	mct_inverse_transform(image_out, &(ctx->xs_config->p));
	nlt_inverse_transform(image_out, &(ctx->xs_config->p));

	if (ctx->xs_config->verbose > 2)
	{
		const int bitpos = (int)bitunpacker_consumed_bits(ctx->bitstream);
		fprintf(stderr, "(bitpos=%d) Arrived at EOC (bytepos=%d)\n", bitpos, bitpos >> 3);
	}

#ifdef PACKING_GENERATE_FRAGMENT_CODE
	if (ctx->user_fragment_info_cb != NULL)
	{
		// Account for EOC marker in last fragment.
		_xs_delayed_fragment_info_collecter_cb(ctx, -1, 16, 0, 0);
		_xs_flush_fragment_info(ctx);
	}
#endif
	xs_parse_tail(ctx->bitstream);
	assert(bitunpacker_consumed_all(ctx->bitstream));
	return true;
}

bool xs_dec_postprocess_image(const xs_config_t* xs_config, xs_image_t* image_out)
{
	if (xs_config->p.color_transform == XS_CPIH_TETRIX)
	{
		if (!mct_4_to_bayer(image_out, xs_config->p.cfa_pattern))
		{
			return false;
		}
	}
	return true;
}