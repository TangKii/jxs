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

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "budget.h"
#include "common.h"
#include "quant.h"

#include "bitpacking.h"

uint32_t single_value_budget_getunary(gcli_pred_t value, int alphabet) {

	if (alphabet == UNARY_ALPHABET_4_CLIPPED)
	{
		if (value == 0)
			return 1;
		else if (value == 1)
			return 3;
		else if (value == -1)
			return 2;
		else if (value == 2)
			return 5;
		else if (value == -2)
			return 4;
		else if (ABS(value) < 13)
			return (ABS(value) + 4);
		else
			return 16;
	}
	else if (alphabet == UNARY_ALPHABET_0)
	{
		if (value > 0)
			return MIN((value + 2), 16);
		else if (value < 0)
			return MIN((-value + 2), 16);
		else
			return 1;
	}
	else
	{
		assert(!"specified unsupported alphabet size");
		return 0;
	}
}

uint32_t budget_getunary(const gcli_pred_t* pred_buf, int len, int alphabet)
{
	int i = 0, budget = 0;
	for (i = 0; i < len; i++)
		budget += single_value_budget_getunary(pred_buf[i], alphabet);

	return budget;
}

uint32_t budget_bounded_code(const gcli_pred_t* pred_buf, const gcli_data_t* decoded_predictors, int gtli, int len, int band_index)
{
	int i = 0, budget = 0;
	for (i = 0; i < len; i++)
	{
		int min_value;
		int max_value;
		if (decoded_predictors)
		{
			bounded_code_get_min_max(decoded_predictors[i], gtli, &min_value, &max_value);
		}
		else
		{
			min_value = -20;
			max_value = 20;
		}
		budget += bounded_code_get_unary_code(pred_buf[i], min_value, max_value) + 1;
	}
	return budget;
}

uint32_t budget_getunary_unsigned(const gcli_pred_t* pred_buf, int len)
{
	int i = 0, budget = 0;
	for (i = 0; i < len; i++)
		budget += (pred_buf[i] + 1);
	return budget;
}

uint32_t budget_getcbr(uint32_t total_budget, uint32_t n_lines, uint32_t total_lines)
{
	uint64_t bigint;
	bigint = total_budget;
	bigint *= n_lines;
	bigint /= total_lines;
	return (uint32_t)(bigint & ~0x1);
}
