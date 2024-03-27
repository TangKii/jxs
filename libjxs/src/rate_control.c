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

#include "rate_control.h"
#include "precinct.h"
#include "gcli_budget.h"
#include "data_budget.h"
#include "budget.h"
#include "sb_weighting.h"

#include <assert.h>

struct rate_control_t
{
	const xs_config_t* xs_config;
	int image_height;
	precinct_t* precinct;
	precinct_budget_table_t* pbt;
	predbuffer_t* pred_residuals;
	precinct_t* precinct_top;
	int gc_enabled_modes;

	int nibbles_image;
	int nibbles_report;
	int nibbles_consumed;
	int lines_consumed;

	ra_params_t ra_params;
	int* gtli_table_data;
	int* gtli_table_gcli;
	int* gtli_table_gcli_prec;
	int gcli_methods_table;
	int* gcli_sb_methods;

	precinct_budget_info_t* pbinfo;
};

int _do_rate_allocation(precinct_t* prec,
	ra_params_t* ra_params,
	precinct_budget_table_t* pbt,
	int* quantization,
	int* refinement,
	int* gtli_table_data,
	int* gtli_table_gcli,
	int* gcli_sb_methods,
	precinct_budget_info_t* bgt_info);

rate_control_t* rate_control_open(const xs_config_t* xs_config, const ids_t* ids, int column)
{
	rate_control_t* rc = (rate_control_t*)malloc(sizeof(rate_control_t));
	if (rc)
	{
		rc->xs_config = xs_config;

		rc->precinct = precinct_open_column(ids, xs_config->p.N_g, column);
		rc->pbt = pbt_open(ids->npi, GCLI_METHODS_NB);
		rc->pred_residuals = predbuffer_open(rc->precinct);

		assert(rc->precinct);
		assert(rc->pbt);

		rc->precinct_top = precinct_open_column(ids, xs_config->p.N_g, column);
		assert(rc->precinct_top);

		rc->gtli_table_data = (int*)malloc(ids->nbands * sizeof(int));
		rc->gtli_table_gcli_prec = (int*)malloc(ids->nbands * sizeof(int));
		rc->gtli_table_gcli = (int*)malloc(ids->nbands * sizeof(int));
		rc->gcli_sb_methods = (int*)malloc(MAX_NBANDS * sizeof(int));
		rc->pbinfo = (precinct_budget_info_t*)malloc(sizeof(precinct_budget_info_t));

		rc->ra_params.Rl = xs_config->p.Rl;
		rc->ra_params.all_enabled_methods = gcli_method_get_enabled(xs_config);
		rc->ra_params.lvl_gains = xs_config->p.lvl_gains;
		rc->ra_params.lvl_priorities = xs_config->p.lvl_priorities;

		rc->image_height = ids->h;
		rc->gc_enabled_modes = gcli_method_get_enabled(xs_config);
	}
	return rc;
}

void rate_control_init(rate_control_t* rc, int image_rate_bytes, int report_nbytes)
{
	if (rc)
	{
		rc->nibbles_consumed = 0;
		rc->lines_consumed = 0;
		rc->nibbles_image = image_rate_bytes * 2;
		rc->nibbles_report = report_nbytes * 2;
	}
}


void rate_control_close(rate_control_t* rc)
{
	free(rc->gcli_sb_methods);
	free(rc->gtli_table_gcli);
	free(rc->gtli_table_gcli_prec);
	free(rc->gtli_table_data);
	free(rc->pbinfo);
	precinct_close(rc->precinct_top);
	pbt_close(rc->pbt);
	predbuffer_close(rc->pred_residuals);
	precinct_close(rc->precinct);

	free(rc);
}

int rate_control_process_precinct(rate_control_t* rc, precinct_t* precinct, rc_results_t* rc_results)
{
	const bool infinite_budget = rc->xs_config->bitstream_size_in_bytes == (size_t)-1;

	precinct_t* precinct_top;

	copy_gclis(rc->precinct_top, rc->precinct);
	precinct_copy(rc->precinct, precinct);

	precinct_top = precinct_is_first_of_slice(precinct, rc->xs_config->p.slice_height) ? NULL : rc->precinct_top;

	fill_gcli_budget_table(rc->gc_enabled_modes, precinct, precinct_top, NULL, rc->pbt, rc->pred_residuals, 0, rc->xs_config->p.S_s);
	fill_data_budget_table(precinct, rc->pbt, rc->xs_config->p.N_g, rc->xs_config->p.Fs, rc->xs_config->p.Qpih);

	if (!precinct_is_first_of_slice(rc->precinct, rc->xs_config->p.slice_height))
	{
		fill_gcli_budget_table(gcli_method_get_enabled_ver(rc->gc_enabled_modes), rc->precinct, rc->precinct_top, rc->gtli_table_gcli_prec, rc->pbt, rc->pred_residuals, 1, rc->xs_config->p.S_s);
	}

	const int budget_cbr = budget_getcbr(rc->nibbles_image, rc->lines_consumed + precinct_spacial_lines_of(rc->precinct, rc->image_height), rc->image_height);
	const int budget_minimum = budget_cbr - rc->nibbles_report;
	rc->ra_params.budget = ((budget_cbr - rc->nibbles_consumed) << 2);
	if (infinite_budget)
	{
		rc->ra_params.budget = 0xfffffff;
	}

	const int precinct_bits = _do_rate_allocation(rc->precinct, &rc->ra_params, rc->pbt, &rc_results->quantization, &rc_results->refinement, rc->gtli_table_data, rc->gtli_table_gcli_prec, rc->gcli_sb_methods, rc->pbinfo);
	if (precinct_bits < 0)
	{
		fprintf(stderr, "Unable to make the rate allocation on the precinct\n");
		rc_results->rc_error = 1;
		return -1;
	}

	rc->lines_consumed += precinct_spacial_lines_of(rc->precinct, rc->image_height);
	rc->nibbles_consumed += (precinct_bits >> 2);

	int padding_nibbles = 0;
	if (!infinite_budget)
	{
		if (precinct_is_last_of_image(rc->precinct, rc->image_height))
			padding_nibbles = rc->nibbles_image - rc->nibbles_consumed;
		else if (rc->nibbles_consumed < budget_minimum)
			padding_nibbles = budget_minimum - rc->nibbles_consumed;
		assert(padding_nibbles >= 0);
		rc->nibbles_consumed += padding_nibbles;
	}

	rc_results->gcli_method = rc->gcli_methods_table;
	for (int lvl = 0; lvl < bands_count_of(rc->precinct); lvl++)
	{
		rc_results->gcli_sb_methods[lvl] = rc->gcli_sb_methods[lvl];
	}
	rc_results->pred_residuals = rc->pred_residuals;
	rc_results->bits_consumed = (rc->nibbles_consumed << 2);
	rc_results->gtli_table_data = rc->gtli_table_data;
	rc_results->gtli_table_gcli = rc->gtli_table_gcli_prec;

	rc_results->pbinfo = rc->pbinfo[0];
	rc_results->padding_bits = (padding_nibbles << 2);
	rc_results->precinct_total_bits = rc_results->pbinfo.precinct_bits + rc_results->padding_bits;

	rc_results->rc_error = 0;
	return 0;
}

int ra_helper_get_total_budget_(
	precinct_t* precinct,
	ra_params_t* ra_params,
	precinct_budget_table_t* pbt,
	int* gtli_table_data,
	int* gtli_table_gcli,
	int* gcli_sb_methods,
	precinct_budget_info_t* bgt_info)
{
	precinct_get_best_gcli_method(precinct, pbt, gtli_table_gcli, gcli_sb_methods);
	precinct_get_budget(precinct, pbt, gtli_table_gcli, gtli_table_data, ra_params->Rl, gcli_sb_methods, bgt_info);

	return bgt_info->precinct_bits;
}

int _do_rate_allocation(precinct_t* precinct, ra_params_t* ra_params, precinct_budget_table_t* pbt,
	int* quantization_out, int* refinement_out, int* gtli_table_data, int* gtli_table_gcli, int* gcli_sb_methods, precinct_budget_info_t* bgt_info)
{
	int total_budget = 0;
	const uint8_t* lvl_gains = ra_params->lvl_gains;
	const uint8_t* lvl_priorities = ra_params->lvl_priorities;
	const int max_refinement = bands_count_of(precinct) - 1;
	int empty;

	if (ra_params->budget < 0)
	{
		fprintf(stderr, "Error: rate allocation budget must be > 0! (%d)\n", ra_params->budget);
		return -1;
	}

	*quantization_out = 0;
	*refinement_out = 0;
	while (true)
	{
		compute_gtli_tables(*quantization_out, *refinement_out, bands_count_of(precinct), lvl_gains, lvl_priorities, gtli_table_data, gtli_table_gcli, &empty);
		total_budget = ra_helper_get_total_budget_(precinct, ra_params, pbt, gtli_table_data, gtli_table_gcli, gcli_sb_methods, bgt_info);
		if (total_budget == ra_params->budget)
		{
			return total_budget;
		}
		else if (total_budget < ra_params->budget)
		{
			break;
		}
		else if (!empty)
		{
			++*quantization_out;
		}
		else
		{
			fprintf(stderr, "RATE Error: unable to reach precinct budget=%d (min_possible=%d)\n", ra_params->budget, total_budget);
			return -1;
		}
	}

	while (true)
	{
		compute_gtli_tables(*quantization_out, *refinement_out, bands_count_of(precinct), lvl_gains, lvl_priorities, gtli_table_data, gtli_table_gcli, &empty);
		total_budget = ra_helper_get_total_budget_(precinct, ra_params, pbt, gtli_table_data, gtli_table_gcli, gcli_sb_methods, bgt_info);

		if (total_budget > ra_params->budget)
		{
			--*refinement_out;

			compute_gtli_tables(*quantization_out, *refinement_out, bands_count_of(precinct), lvl_gains, lvl_priorities, gtli_table_data, gtli_table_gcli, &empty);
			total_budget = ra_helper_get_total_budget_(precinct, ra_params, pbt, gtli_table_data, gtli_table_gcli, gcli_sb_methods, bgt_info);

			return total_budget;
		}
		else if (*refinement_out == max_refinement)
		{
			return total_budget;
		}
		else
		{
			++*refinement_out;
		}
	}
}
