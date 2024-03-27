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

#include "data_budget.h"
#include "precinct.h"
#include "quant.h"
#include <assert.h>
#include <string.h>

int budget_get_data_budget(const gcli_data_t* gclis, int gclis_len, uint32_t* budget_table, int table_size, int group_size, int include_sign)
{
	memset(budget_table, 0, sizeof(int) * table_size);

	for (int i = 0; i < gclis_len; i++)
	{
		for (int gtli = 0; gtli < table_size; gtli++)
		{
			int n_bitplanes = (gclis[i] - gtli);
			if (group_size == 1) {
				n_bitplanes -= 1;
				if (include_sign)
					n_bitplanes += 1;
			}

			if (n_bitplanes <= 0)
				break;

			if (group_size > 1) {
				if (include_sign)
					n_bitplanes += 1;
			}

			budget_table[gtli] += (n_bitplanes * group_size);
		}
	}
	return 0;
}

int budget_get_sign_budget(const sig_mag_data_t* datas_buf, int data_len,
	const gcli_data_t* gclis, int gclis_len,
	uint32_t* budget_table, int table_size, int group_size,
	int dq_type)
{
	memset(budget_table, 0, sizeof(int) * table_size);

	for (int idx = 0, group = 0; group < (data_len + group_size - 1) / group_size; ++group)
	{
		const gcli_data_t gcli = gclis[group];
		for (int k = 0; k < group_size; ++k)
		{
			for (int gtli = 0; gtli < table_size; gtli++)
			{
				const int quant_val = apply_dq(dq_type, datas_buf[idx + k], gcli, gtli);
				if (quant_val != 0)
				{
					++budget_table[gtli];
				}
				else
					break;
			}
		}
		idx += group_size;
	}
	return 0;
}

int fill_data_budget_table(precinct_t* prec, precinct_budget_table_t* pbt, int group_size, const uint8_t sign_packing, int dq_type)
{
	int position;
	for (position = 0; position < line_count_of(prec); position++)
	{
		int lvl = precinct_band_index_of(prec, position);
		int ypos = precinct_ypos_of(prec, position);

		budget_get_data_budget(precinct_gcli_of(prec, lvl, ypos), (int)precinct_gcli_width_of(prec, lvl), pbt_get_data_bgt_of(pbt, position), MAX_GCLI + 1, group_size, sign_packing == 0);

		if (sign_packing == 1)
		{
			budget_get_sign_budget(precinct_line_of(prec, lvl, ypos), (int)precinct_width_of(prec, lvl), precinct_gcli_of(prec, lvl, ypos), (int)precinct_gcli_width_of(prec, lvl), pbt_get_sign_bgt_of(pbt, position), MAX_GCLI + 1, group_size, dq_type);
		}
		else
		{
			memset(pbt_get_sign_bgt_of(pbt, position), 0, sizeof(uint32_t) * MAX_GCLI + 1);
		}
	}
	return 0;
}
