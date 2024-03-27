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
#include "common.h"
#include "xs_config.h"
#include "xs_markers.h"
#include "precinct.h"
#include "predbuffer.h"
#include "buf_mgmt.h"
#include "bitpacking.h"
#include "gcli_budget.h"
#include "data_budget.h"
#include "budget.h"
#include "packing.h"
#include "quant.h"
#include "pred.h"
#include "sigbuffer.h"
#include "rate_control.h"
#include "ids.h"
#include "dwt.h"
#include "mct.h"
#include "nlt.h"

struct xs_enc_context_t
{
	const xs_config_t* xs_config;
	ids_t ids;

	precinct_t* precinct[MAX_PREC_COLS];
	packing_context_t* packer;

	bit_packer_t* bitstream;
	int bitstream_len;

	rate_control_t* rc[MAX_PREC_COLS];
};

xs_enc_context_t* xs_enc_init(xs_config_t* xs_config, xs_image_t* image)
{
	xs_enc_context_t* ctx;

	ctx = (xs_enc_context_t*)malloc(sizeof(xs_enc_context_t));
	if (!ctx)
	{
		return NULL;
	}

	memset(ctx, 0, sizeof(xs_enc_context_t));
	ctx->xs_config = xs_config;

	assert(xs_config != NULL && xs_config->profile != XS_PROFILE_AUTO);
	if (!xs_config_resolve_auto_values(xs_config, image))
	{
		free(ctx);
		return NULL;
	}
	if (!xs_config_validate(xs_config, image))
	{
		free(ctx);
		return NULL;
	}

	ids_construct(&ctx->ids, image, xs_config->p.NLx, xs_config->p.NLy, xs_config->p.Sd, xs_config->p.Cw, xs_config->p.Lh);

	for (int column = 0; column < ctx->ids.npx; column++)
	{
		ctx->rc[column] = rate_control_open(xs_config, &ctx->ids, column);
		ctx->precinct[column] = precinct_open_column(&ctx->ids, ctx->xs_config->p.N_g, column);
	}

	ctx->packer = packer_open(xs_config, ctx->precinct[0]);
	ctx->bitstream = bitpacker_init();
	ctx->bitstream_len = 0;
	return ctx;
}

void xs_enc_close(xs_enc_context_t* ctx)
{
	if (!ctx)
	{
		return;
	}

	if (ctx->packer)
	{
		packer_close(ctx->packer);
	}
	bitpacker_close(ctx->bitstream);

	for (int i = 0; i < ctx->ids.npx; i++)
	{
		precinct_close(ctx->precinct[i]);
		rate_control_close(ctx->rc[i]);
	}

	free(ctx);
}

bool xs_enc_preprocess_image(const xs_config_t* xs_config, xs_image_t* image)
{
	if (xs_config->p.color_transform == XS_CPIH_TETRIX)
	{
		if (!mct_bayer_to_4(image, xs_config->p.cfa_pattern))
		{
			return false;
		}
	}
	return true;
}

bool _xs_enc_init_column_rates(xs_enc_context_t* ctx, const int width, const int height, const int header_len)
{
	const int ncols = (width + ctx->ids.cs - 1) / ctx->ids.cs;

	if (ctx->xs_config->bitstream_size_in_bytes == (size_t)-1)
	{
		// Take some high budgets.
		for (int column = 0; column < ctx->ids.npx - 1; ++column)
		{
			rate_control_init(ctx->rc[column], (int)0x0fffffff, (int)0x0fffffff);
		}
		rate_control_init(ctx->rc[ctx->ids.npx - 1], (int)0x0fffffff, (int)0x0fffffff);

		return true;
	}

	const size_t min_col_nbytes = (size_t)ctx->ids.npy * 4;  // take some minimal overhead of signaling
	const size_t overhead = (size_t)(header_len / 8) + XS_MARKER_NBYTES + (XS_SLICE_HEADER_NBYTES * (((size_t)height + ctx->xs_config->p.slice_height - 1) / ctx->xs_config->p.slice_height));

	if (ctx->xs_config->bitstream_size_in_bytes < (overhead + min_col_nbytes))
	{
		return false;
	}

	const size_t rate_control_nbytes = ctx->xs_config->bitstream_size_in_bytes - overhead;
	const size_t budget_report_nbytes = (size_t)(ctx->xs_config->budget_report_lines / 2) * 2 * ctx->xs_config->bitstream_size_in_bytes / height;

	if (rate_control_nbytes <= min_col_nbytes)
	{
		return false;
	}

	const size_t nbytes_column = (rate_control_nbytes - min_col_nbytes) * ctx->ids.cs / width;
	const size_t report_column = budget_report_nbytes * ctx->ids.cs / width;
	const size_t nbytes_last_column = rate_control_nbytes - (ncols - 1) * nbytes_column;
	const size_t report_last_column = budget_report_nbytes - (ncols - 1) * report_column;
	
	for (int column = 0; column < ctx->ids.npx - 1; ++column)
	{
		rate_control_init(ctx->rc[column], (int)nbytes_column, (int)report_column);
	}
	rate_control_init(ctx->rc[ctx->ids.npx - 1], (int)nbytes_last_column, (int)report_last_column);

	return true;
}

bool xs_enc_image(xs_enc_context_t* ctx, xs_image_t* image, void* codestream_buf, size_t codestream_buf_byte_size, size_t* codestream_byte_size)
{
	rc_results_t rc_results;
	int slice_idx = 0;
	int markers_len = 0;

	if ((ctx->xs_config->bitstream_size_in_bytes != (size_t)-1) && (codestream_buf_byte_size / 8) * 8 < ctx->xs_config->bitstream_size_in_bytes)
	{
		if (ctx->xs_config->verbose) fprintf(stderr, "Error: codestream_buf is too small. Please allocate at least %u bytes\n", ((uint32_t)(ctx->xs_config->bitstream_size_in_bytes + 7) / 8 * 8));
		return false;
	}
	bitpacker_set_buffer(ctx->bitstream, codestream_buf, (int)codestream_buf_byte_size);

	const int header_len = xs_write_head(ctx->bitstream, image, ctx->xs_config);
	if (ctx->xs_config->verbose > 1)
	{
		fprintf(stderr, "HEADER_LEN %d\n", bitpacker_get_len(ctx->bitstream));
	}

	if (!_xs_enc_init_column_rates(ctx, image->width, image->height, header_len))
	{
		if (ctx->xs_config->verbose) fprintf(stderr, "Error: unable to allocate enough bytes per column (increase the rate, or reduce the number of columns)\n");
		return false;
	}

	nlt_forward_transform(image, &(ctx->xs_config->p));
	mct_forward_transform(image, &(ctx->xs_config->p));
	dwt_forward_transform(&ctx->ids, image);

	for (int line_idx = 0; line_idx < image->height; line_idx += ctx->ids.ph)
	{
		const int prec_y_idx = (line_idx / ctx->ids.ph);
		for (int column = 0; column < ctx->ids.npx; ++column)
		{
			precinct_set_y_idx_of(ctx->precinct[column], prec_y_idx);
			precinct_from_image(ctx->precinct[column], image, ctx->xs_config->p.Fq);

			update_gclis(ctx->precinct[column]);

			if (rate_control_process_precinct(ctx->rc[column], ctx->precinct[column], &rc_results) < 0) {
				return false;
			}

			quantize_precinct(ctx->precinct[column], rc_results.gtli_table_data, ctx->xs_config->p.Qpih);

			if (precinct_is_first_of_slice(ctx->precinct[column], ctx->xs_config->p.slice_height) && (column == 0))
			{
				if (ctx->xs_config->verbose > 1)
				{
					fprintf(stderr, "Write Slice Header (slice_idx=%d)\n", slice_idx);
				}
				markers_len += xs_write_slice_header(ctx->bitstream, slice_idx++);
			}

			if (pack_precinct(ctx->packer, ctx->bitstream, ctx->precinct[column], &rc_results) < 0)
			{
				return false;
			}

			if (rc_results.rc_error == 1)
				break;
		}
	}

	xs_write_tail(ctx->bitstream);	
	assert((ctx->xs_config->bitstream_size_in_bytes == (size_t)-1) || bitpacker_get_len(ctx->bitstream) / 8 == ctx->xs_config->bitstream_size_in_bytes);

	*codestream_byte_size = ((bitpacker_get_len(ctx->bitstream) + 7) / 8);
	bitpacker_flush(ctx->bitstream);
	return true;
}
