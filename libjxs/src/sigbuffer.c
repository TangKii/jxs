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

#include "sigbuffer.h"
#include "buf_mgmt.h"
#include "pred.h"
#include "precinct.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct sigbuffer_struct
{
	multi_buf_t* sig_mb;
	multi_buf_t* sig_mb2;

	int idx_from_level[MAX_PRECINCT_HEIGHT][MAX_NBANDS];
};

sigbuffer_t* sigbuffer_open(const precinct_t* prec)
{
	sigbuffer_t* sig = malloc(sizeof(sigbuffer_t));
	if (!sig)
	{
		return NULL;
	}

	int lines = line_count_of(prec);
	int lvls = bands_count_of(prec);
	int idx = 0;

	sig->sig_mb = multi_buf_create(lines, sizeof(gcli_pred_t));
	sig->sig_mb2 = multi_buf_create(lines, sizeof(gcli_data_t));

	for (int lvl = 0; lvl < lvls; lvl++)
	{
		for (int ypos = 0; ypos < precinct_in_band_height_of(prec, lvl); ypos++)
		{
			int width = (int)precinct_gcli_width_of(prec, lvl);

			multi_buf_set_size(sig->sig_mb, idx, width);
			multi_buf_set_size(sig->sig_mb2, idx, width);
			sig->idx_from_level[ypos][lvl] = idx;
			idx++;
		}
	}

	assert(idx == lines);

	multi_buf_allocate(sig->sig_mb, 1);
	multi_buf_allocate(sig->sig_mb2, 1);

	return sig;
}


void sigbuffer_close(sigbuffer_t* sig)
{
	if (sig)
	{
		multi_buf_destroy(sig->sig_mb);
		multi_buf_destroy(sig->sig_mb2);
		free(sig);
	}
}

gcli_pred_t* sig_values_of(sigbuffer_t* sig, int lvl, int ypos)
{
	int idx = sig->idx_from_level[ypos][lvl];
	return sig->sig_mb->bufs.s8[idx];
}

gcli_data_t* sig_predictors_of(sigbuffer_t* sig, int lvl, int ypos)
{
	int idx = sig->idx_from_level[ypos][lvl];
	return sig->sig_mb2->bufs.s8[idx];
}

int* sig_width_of(sigbuffer_t* sig, int lvl, int ypos)
{
	int idx = sig->idx_from_level[ypos][lvl];
	return sig->sig_mb->sizes + idx;
}
