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

#include "gcli_methods.h"
#include "packing.h"
#include "precinct_budget.h"
#include "precinct_budget_table.h"
#include <assert.h>
#include <memory.h>

#define ALIGN_TO_BITS(value, nbits) ((value + nbits - 1) & (~(nbits - 1)))

void precinct_get_best_gcli_method(precinct_t* precinct,
	precinct_budget_table_t* pbt,
	int* gtli_table_gcli,
	int* res_gcli_sb_methods)
{
	int lvl, gcli_method, y_pos, position, gtli;
	uint32_t bgt_min;
	uint32_t bgt_method, bgt_min_method;
	uint32_t line_bgt = 0;

	for (lvl = 0; lvl < bands_count_of(precinct); lvl++)
	{
		bgt_min = RA_BUDGET_INVALID;
		bgt_min_method = -1;

		for (gcli_method = 0; gcli_method < GCLI_METHODS_NB; gcli_method++)
		{
			gtli = gtli_table_gcli[lvl];

			if (gcli_method == method_get_idx(ALPHABET_RAW_4BITS, PRED_NONE, RUN_NONE))
				continue;

			if (pbt_get_gcli_bgt_of(pbt, gcli_method, 0)[gtli] == RA_BUDGET_INVALID)
				continue;

			bgt_method = 0;
			for (y_pos = 0; y_pos < precinct_in_band_height_of(precinct, lvl); y_pos++)
			{
				position = precinct_position_of(precinct, lvl, y_pos);

				line_bgt = pbt_get_sigf_bgt_of(pbt, gcli_method, position)[gtli];
				line_bgt += pbt_get_gcli_bgt_of(pbt, gcli_method, position)[gtli];
				bgt_method += line_bgt;
			}
			if ((bgt_min == RA_BUDGET_INVALID) || (bgt_method < bgt_min))
			{
				bgt_min = bgt_method;
				bgt_min_method = gcli_method;
			}
		}
		if (bgt_min == RA_BUDGET_INVALID)
		{
			bgt_min_method = method_get_idx(ALPHABET_RAW_4BITS, PRED_NONE, RUN_NONE);
		}
		res_gcli_sb_methods[lvl] = bgt_min_method;
	}
}

int detect_gcli_raw_fallback(precinct_t* precinct, const uint8_t Rl, precinct_budget_info_t* pbi)
{
	if (Rl == 0)
	{
		// Raw fallback restricted per band.
		const int nb_bands = bands_count_of(precinct);
		for (int band = 0; band < nb_bands; ++band)
		{
			int subpkt = 0;
			int size_noraw_sum = 0;
			int size_raw_sum = 0;
			for (int ypos = 0; ypos < precinct_in_band_height_of(precinct, band); ypos++)
			{
				subpkt = precinct_subpkt_of(precinct, precinct_position_of(precinct, band, ypos));
				if (pbi->subpkt_uses_raw_fallback[subpkt] == 1)
					break; // if already enabled
				size_noraw_sum += pbi->subpkt_size_gcli[subpkt];
				size_noraw_sum += pbi->subpkt_size_sigf[subpkt];
				size_raw_sum += pbi->subpkt_size_gcli_raw[subpkt];
			}

			if (pbi->subpkt_uses_raw_fallback[subpkt] == 0)
			{
				if (size_raw_sum <= size_noraw_sum)
				{
					for (int ypos = 0; ypos < precinct_in_band_height_of(precinct, band); ypos++)
					{
						subpkt = precinct_subpkt_of(precinct, precinct_position_of(precinct, band, ypos));
						pbi->subpkt_size_sigf[subpkt] = 0;
						pbi->subpkt_size_gcli[subpkt] = pbi->subpkt_size_gcli_raw[subpkt];
						pbi->subpkt_uses_raw_fallback[subpkt] = 1;
					}
				}
			}
		}
		return 0;
	}

	// Raw fallback per packet.
	const int nb_subpkts = precinct_nb_subpkts_of(precinct);
	for (int subpkt = 0; subpkt < nb_subpkts; ++subpkt)
	{
		if (pbi->subpkt_size_gcli_raw[subpkt] <= pbi->subpkt_size_gcli[subpkt] + pbi->subpkt_size_sigf[subpkt])
		{
			pbi->subpkt_size_sigf[subpkt] = 0;
			pbi->subpkt_size_gcli[subpkt] = pbi->subpkt_size_gcli_raw[subpkt];
			pbi->subpkt_uses_raw_fallback[subpkt] = 1;
		}
	}
	return 0;
}

void precinct_get_budget(precinct_t* precinct,
	precinct_budget_table_t* pbt,
	int* gtli_table_gcli,
	int* gtli_table_data,
	const uint8_t Rl,
	int* res_gcli_sb_methods,
	precinct_budget_info_t* out)
{
	int gtli_gcli, gtli_data;
	int gcli_method;
	int subpkt = 0;
	int raw_budget;
	const int raw_method = method_get_idx(ALPHABET_RAW_4BITS, PRED_NONE, RUN_NONE);
	const bool use_long_precinct_headers = precinct_use_long_headers(precinct);

	memset(out, 0, sizeof(precinct_budget_info_t));

	out->prec_header_size = ALIGN_TO_BITS(PREC_HDR_PREC_SIZE + PREC_HDR_QUANTIZATION_SIZE + PREC_HDR_REFINEMENT_SIZE + bands_count_of(precinct) * GCLI_METHOD_NBITS, PREC_HDR_ALIGNMENT);
	out->precinct_bits = out->prec_header_size;

	const int position_count = line_count_of(precinct);
	for (int position = 0; position < position_count; position++)
	{
		const int lvl = precinct_band_index_of(precinct, position);
		if (!precinct_is_line_present(precinct, lvl, precinct_ypos_of(precinct, position)))
		{
			continue;
		}

		subpkt = precinct_subpkt_of(precinct, position);
		out->pkt_header_size[subpkt] = ALIGN_TO_BITS(
			use_long_precinct_headers ?
			(PKT_HDR_DATA_SIZE_LONG + PKT_HDR_GCLI_SIZE_LONG + PKT_HDR_SIGN_SIZE_LONG + 1) :
			(PKT_HDR_DATA_SIZE_SHORT + PKT_HDR_GCLI_SIZE_SHORT + PKT_HDR_SIGN_SIZE_SHORT + 1),
			PKT_HDR_ALIGNMENT);

		gtli_gcli = gtli_table_gcli[lvl];
		gtli_data = gtli_table_data[lvl];
		gcli_method = res_gcli_sb_methods[lvl];

		out->subpkt_size_sigf[subpkt] += pbt_get_sigf_bgt_of(pbt, gcli_method, position)[gtli_gcli];
		out->subpkt_size_gcli[subpkt] += pbt_get_gcli_bgt_of(pbt, gcli_method, position)[gtli_gcli];
		out->subpkt_size_data[subpkt] += pbt_get_data_bgt_of(pbt, position)[gtli_data]; // Fs setting is handled by data_budget
		out->subpkt_size_sign[subpkt] += pbt_get_sign_bgt_of(pbt, position)[gtli_data]; // will be always 0 if Fs==0

		raw_budget = pbt_get_gcli_bgt_of(pbt, raw_method, position)[gtli_gcli];
		if (raw_budget == RA_BUDGET_INVALID)
		{
			out->subpkt_size_gcli_raw[subpkt] = RA_BUDGET_INVALID;
		}
		else if (out->subpkt_size_gcli_raw[subpkt] != RA_BUDGET_INVALID)
		{
			out->subpkt_size_gcli_raw[subpkt] += raw_budget;
		}
	}

	for (subpkt = 0; subpkt < precinct_nb_subpkts_of(precinct); subpkt++)
	{
		out->subpkt_size_sigf[subpkt] = ALIGN_TO_BITS(out->subpkt_size_sigf[subpkt], 8);
		out->subpkt_size_gcli[subpkt] = ALIGN_TO_BITS(out->subpkt_size_gcli[subpkt], 8);
		out->subpkt_size_data[subpkt] = ALIGN_TO_BITS(out->subpkt_size_data[subpkt], 8);
		out->subpkt_size_sign[subpkt] = ALIGN_TO_BITS(out->subpkt_size_sign[subpkt], 8);
		out->subpkt_size_gcli_raw[subpkt] = ALIGN_TO_BITS(out->subpkt_size_gcli_raw[subpkt], 8);
	}

	detect_gcli_raw_fallback(precinct, Rl, out);

	for (subpkt = 0; subpkt < precinct_nb_subpkts_of(precinct); subpkt++)
	{
		out->precinct_bits += out->pkt_header_size[subpkt];
		out->precinct_bits += out->subpkt_size_sigf[subpkt];
		out->precinct_bits += out->subpkt_size_gcli[subpkt];
		out->precinct_bits += out->subpkt_size_data[subpkt];
		out->precinct_bits += out->subpkt_size_sign[subpkt];
	}

#ifdef DEBUG_PREC_BUDGET
	fprintf(stderr, "Precinct Budget: precinct_bits=%8d (prec_hdr=%d):", out->precinct_bits, out->prec_header_size);
	for (subpkt = 0; subpkt < precinct_nb_subpkts_of(precinct); subpkt++)
		fprintf(stderr, " pkt%d (hdr=%d gcli=%d sigf=%d data=%d sign=%d)", subpkt, out->pkt_header_size[subpkt], out->subpkt_size_gcli[subpkt], out->subpkt_size_sigf[subpkt], out->subpkt_size_data[subpkt], out->subpkt_size_sign[subpkt]);
	fprintf(stderr, "\n");
#endif
}
