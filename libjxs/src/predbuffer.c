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

#include "predbuffer.h"
#include "buf_mgmt.h"
#include "pred.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct directional_prediction_struct
{
	multi_buf_t* gclis_mb[MAX_GCLI + 1];
	multi_buf_t* predictors_mb[MAX_GCLI + 1];

	int idx_from_level[MAX_PRECINCT_HEIGHT][MAX_NBANDS];
};

struct predbuffer_struct
{
	directional_prediction_t direction[PRED_COUNT];
};

predbuffer_t* predbuffer_open(const precinct_t* prec)
{
	int i;
	predbuffer_t* pred = malloc(sizeof(predbuffer_t));
	if (pred)
	{
		int lines = line_count_of(prec);
		int levels = bands_count_of(prec);
		int ok = 1;
		int gtli;
		for (i = 0; i < PRED_COUNT; i++)
		{
			for (gtli = 0; gtli <= MAX_GCLI; gtli++)
			{
				pred->direction[i].gclis_mb[gtli] = NULL;
				pred->direction[i].predictors_mb[gtli] = NULL;
			}
		}
		for (i = 0; i < PRED_COUNT; i++)
		{
			for (gtli = 0; gtli <= MAX_GCLI; gtli++) {
				pred->direction[i].gclis_mb[gtli] = multi_buf_create(lines, sizeof(gcli_pred_t));
				if (pred->direction[i].gclis_mb[gtli] == NULL)
					ok = 0;
				pred->direction[i].predictors_mb[gtli] = multi_buf_create(lines, sizeof(gcli_data_t));
				if (pred->direction[i].predictors_mb[gtli] == NULL)
					ok = 0;
			}
		}
		if (ok)
		{
			int idx = 0;
			for (int lvl = 0; lvl < levels; lvl++)
			{
				for (int ypos = 0; ypos < precinct_in_band_height_of(prec, lvl); ypos++) {
					int width = (int)precinct_gcli_width_of(prec, lvl);
					for (i = 0; i < PRED_COUNT; i++) {
						pred->direction[i].idx_from_level[ypos][lvl] = idx;
						for (gtli = 0; gtli <= MAX_GCLI; gtli++)
						{
							if (multi_buf_set_size(pred->direction[i].gclis_mb[gtli],
								idx, width) < 0)
								goto error;
							if (multi_buf_set_size(pred->direction[i].predictors_mb[gtli],
								idx, width) < 0)
								goto error;
						}
					}
					idx++;
				}
			}

			for (i = 0; i < PRED_COUNT; i++)
			{
				for (gtli = 0; gtli <= MAX_GCLI; gtli++)
				{
					if (multi_buf_allocate(pred->direction[i].gclis_mb[gtli], 1) < 0)
						goto error;
					if (multi_buf_allocate(pred->direction[i].predictors_mb[gtli], 1) < 0)
						goto error;
				}
			}
			return pred;
		}
	error:
		predbuffer_close(pred);
	}
	return NULL;
}

void predbuffer_close(predbuffer_t* pred)
{
	if (pred)
	{
		int i, gtli;

		for (i = 0; i < PRED_COUNT; i++)
		{
			for (gtli = 0; gtli <= MAX_GCLI; gtli++)
			{
				multi_buf_destroy(pred->direction[i].gclis_mb[gtli]);
				multi_buf_destroy(pred->direction[i].predictors_mb[gtli]);
			}
		}
		free(pred);
	}
}

directional_prediction_t* directional_data_of(predbuffer_t* pred, int dir)
{
	assert(dir < PRED_COUNT);

	return &pred->direction[dir];
}

gcli_pred_t* prediction_values_of(const directional_prediction_t* dir, int lvl, int ypos, int gtli)
{
	int idx = dir->idx_from_level[ypos][lvl];

	return dir->gclis_mb[gtli]->bufs.s8[idx];
}

gcli_data_t* prediction_predictors_of(const directional_prediction_t* dir, int lvl, int ypos, int gtli)
{
	int idx = dir->idx_from_level[ypos][lvl];

	return dir->predictors_mb[gtli]->bufs.s8[idx];
}

int prediction_width_of(const directional_prediction_t* dir, int lvl)
{
	int idx = dir->idx_from_level[0][lvl];

	return dir->gclis_mb[0]->sizes[idx];
}
