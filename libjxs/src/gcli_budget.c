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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gcli_budget.h"
#include "pred.h"
#include "budget.h"
#include "precinct.h"
#include "predbuffer.h"
#include "gcli_methods.h"
#include "sig_flags.h"

void gcli_invalid_coding(precinct_budget_table_t* pbt, int gcli_method)
{
	int gtli, position;
	for (position = 0; position < pbt_get_position_count(pbt); position++)
	{
		for (gtli = 0; gtli < MAX_GCLI + 1; gtli++)
		{
			pbt_get_sigf_bgt_of(pbt, gcli_method, position)[gtli] = 0;
			pbt_get_gcli_bgt_of(pbt, gcli_method, position)[gtli] = RA_BUDGET_INVALID;
		}
	}
}

int gcli_budget_raw_coding(const precinct_t* prec, precinct_budget_table_t* pbt, int gcli_method)
{
	int gtli, position;
	for (position = 0; position < pbt_get_position_count(pbt); position++)
	{
		int lvl = precinct_band_index_of(prec, position);
		int size_gcli_raw = 4 * precinct_gcli_width_of(prec, lvl);
		for (gtli = 0; gtli < MAX_GCLI + 1; gtli++)
		{
			pbt_get_sigf_bgt_of(pbt, gcli_method, position)[gtli] = 0;
			pbt_get_gcli_bgt_of(pbt, gcli_method, position)[gtli] = size_gcli_raw;
		}
	}
	return 0;
}

static int gcli_budget_compute_generic(const precinct_t* prec, int gcli_method, precinct_budget_table_t* pbt, predbuffer_t* residuals, int sig_flags_group_width, uint64_t active_methods)
{
	sig_flags_t* sig_flags = NULL;
	gcli_pred_t* sig_flags_coded_buf = NULL;
	gcli_data_t* sig_flags_predictors_buf = NULL;
	int position, gtli, lvl, ypos;
	unary_alphabet_t alph = FIRST_ALPHABET;
	directional_prediction_t* dp = directional_data_of(residuals, method_prediction_type(gcli_method));
	directional_prediction_t* dp_nopred = directional_data_of(residuals, PRED_NONE);

	if (method_uses_sig_flags(gcli_method))
	{
		sig_flags = sig_flags_malloc((int)precinct_max_gcli_width_of(prec), sig_flags_group_width);
		sig_flags_coded_buf = (gcli_pred_t*)malloc(precinct_max_gcli_width_of(prec) * sizeof(gcli_pred_t));
		sig_flags_predictors_buf = (gcli_data_t*)malloc(precinct_max_gcli_width_of(prec) * sizeof(gcli_pred_t));
	}

	for (position = 0; position < line_count_of(prec); position++)
	{
		lvl = precinct_band_index_of(prec, position);
		ypos = precinct_ypos_of(prec, position);
		for (gtli = 0; gtli < MAX_GCLI + 1; gtli++)
		{
			uint32_t bgt_sigf = 0;
			uint32_t bgt_gcli = 0;
			gcli_pred_t* res_buf_coded, * res_buf;
			int res_len_coded, res_len;
			gcli_data_t* predictors_buf;
			gcli_data_t* values_buf;

			res_buf = res_buf_coded = prediction_values_of(dp, lvl, ypos, gtli);
			res_len = res_len_coded = precinct_gcli_width_of(prec, lvl);
			predictors_buf = prediction_predictors_of(dp, lvl, ypos, gtli);
			values_buf = prediction_values_of(dp_nopred, lvl, ypos, gtli);

			if (method_uses_sig_flags(gcli_method))
			{
				gcli_pred_t* sig_flags_buf = res_buf;
				if (method_get_run(gcli_method) == RUN_SIGFLAGS_ZRCSF)
					sig_flags_buf = values_buf;

				sig_flags_init(sig_flags, sig_flags_buf, res_len, sig_flags_group_width);
				bgt_sigf = sig_flags_budget(sig_flags);

				sig_flags_filter_values(sig_flags, res_buf, sig_flags_coded_buf, &res_len_coded);
				sig_flags_filter_values(sig_flags, predictors_buf, sig_flags_predictors_buf, &res_len_coded);

				res_buf_coded = sig_flags_coded_buf;
				predictors_buf = sig_flags_predictors_buf;
			}

			if (method_uses_no_pred(gcli_method))
				bgt_gcli += budget_getunary_unsigned(res_buf_coded, res_len_coded);
			else if (method_get_alphabet(gcli_method) == ALPHABET_UNARY_UNSIGNED_BOUNDED)
				bgt_gcli += budget_bounded_code(res_buf_coded, predictors_buf, gtli, res_len_coded, lvl);
			else
				bgt_gcli += budget_getunary(res_buf_coded, res_len_coded, alph);

			pbt_get_sigf_bgt_of(pbt, gcli_method, position)[gtli] = bgt_sigf;
			pbt_get_gcli_bgt_of(pbt, gcli_method, position)[gtli] = bgt_gcli;
		}
	}
	if (sig_flags)
	{
		sig_flags_free(sig_flags);
		free(sig_flags_coded_buf);
		free(sig_flags_predictors_buf);
	}
	return 0;
}

static int compute_residuals(uint32_t active_methods,
	const precinct_t* prec,
	const precinct_t* prec_top,
	int* gtli_top_array,
	predbuffer_t* residuals)
{
	int pred, gcli_method, lvl, gtli, ypos;

	for (pred = 0; pred < PRED_COUNT; pred++)
	{
		directional_prediction_t* dp = directional_data_of(residuals, pred);

		for (gcli_method = 0; gcli_method < GCLI_METHODS_NB; gcli_method++)
		{
			if (gcli_method_is_enabled(active_methods, gcli_method, (prec_top == NULL) ? PRECINCT_FIRST_OF_SLICE : PRECINCT_OTHERS) == 0)
				continue;

			if ((method_prediction_type(gcli_method) == pred) ||
				((method_get_run(gcli_method) == RUN_SIGFLAGS_ZRCSF) && (pred == PRED_NONE)))
			{
				for (lvl = 0; lvl < bands_count_of(prec); lvl++)
				{
					for (ypos = 0; ypos < precinct_in_band_height_of(prec, lvl); ypos++)
					{
						gcli_data_t* gclis_top = precinct_gcli_top_of(prec, prec_top, lvl, ypos);
						for (gtli = 0; gtli < MAX_GCLI + 1; gtli++)
						{
							if ((pred == PRED_VER) && gclis_top)
							{
								int gtli_top = (gtli_top_array && (ypos == 0)) ? gtli_top_array[lvl] : gtli;
								tco_pred_ver(
									precinct_gcli_of(prec, lvl, ypos),
									gclis_top,
									precinct_gcli_width_of(prec, lvl),
									prediction_values_of(dp, lvl, ypos, gtli),
									prediction_predictors_of(dp, lvl, ypos, gtli),
									gtli, gtli_top);
							}
							else if ((pred == PRED_NONE) || ((pred == PRED_VER) && (gclis_top == NULL)))
							{
								tco_pred_none(
									precinct_gcli_of(prec, lvl, ypos),
									precinct_gcli_width_of(prec, lvl),
									prediction_values_of(dp, lvl, ypos, gtli), gtli);
								memset(prediction_predictors_of(dp, lvl, ypos, gtli), 0, precinct_gcli_width_of(prec, lvl) * sizeof(gcli_data_t));
							}
							else
								assert(0);
						}
					}
				}
				break;
			}
		}
	}
	return 0;
}

int fill_gcli_budget_table(
	uint32_t active_methods,
	const precinct_t* prec,
	const precinct_t* prec_top,
	int* gtli_top_array,
	precinct_budget_table_t* pbt,
	predbuffer_t* residuals,
	int update_only,
	int sigflags_group_width)
{
	compute_residuals(active_methods, prec, prec_top, gtli_top_array, residuals);

	for (int method = 0; method < GCLI_METHODS_NB; method++)
	{
		if (gcli_method_is_enabled(active_methods, method, (prec_top == NULL) ? PRECINCT_FIRST_OF_SLICE : PRECINCT_OTHERS) == 0)
		{
			if (!update_only)
				gcli_invalid_coding(pbt, method);
		}
		else
		{
			if (method_is_raw(method))
			{
				gcli_budget_raw_coding(prec, pbt, method);
			}
			else
			{
				gcli_budget_compute_generic(prec, method, pbt, residuals, sigflags_group_width, active_methods);
			}
		}
	}
	return 0;
}
