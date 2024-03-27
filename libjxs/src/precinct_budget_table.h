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

#ifndef PRECINCT_BUDGET_TABLE_H
#define PRECINCT_BUDGET_TABLE_H

#include <stdint.h>
#include "buf_mgmt.h"

typedef struct precinct_budget_table_t precinct_budget_table_t;
struct precinct_budget_table_t
{
	multi_buf_t* sigf_budget_table;
	multi_buf_t* gcli_budget_table;
	multi_buf_t* data_budget_table;
	multi_buf_t* sign_budget_table;
	int position_count;
	int method_count;
};

precinct_budget_table_t* pbt_open(int position_count, int method_count);

static INLINE uint32_t* pbt_get_sigf_bgt_of(precinct_budget_table_t* pbt, int gcli_method, int position)
{
	return pbt->sigf_budget_table->bufs.u32[idx2d(gcli_method, position, pbt->position_count)];
}

static INLINE uint32_t* pbt_get_gcli_bgt_of(precinct_budget_table_t* pbt, int gcli_method, int position)
{
	return pbt->gcli_budget_table->bufs.u32[idx2d(gcli_method, position, pbt->position_count)];
}

static INLINE uint32_t* pbt_get_data_bgt_of(precinct_budget_table_t* pbt, int position)
{
	return pbt->data_budget_table->bufs.u32[position];
}

static INLINE uint32_t* pbt_get_sign_bgt_of(precinct_budget_table_t* pbt, int position)
{
	return pbt->sign_budget_table->bufs.u32[position];
}

static INLINE int pbt_get_position_count(precinct_budget_table_t* pbt)
{
	return pbt->position_count;
}

static INLINE int pbt_get_method_count(precinct_budget_table_t* pbt)
{
	return pbt->method_count;
}

void pbt_close(precinct_budget_table_t* pbt);

#endif
