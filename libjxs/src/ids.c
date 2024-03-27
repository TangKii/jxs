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

#include "ids.h"
#include <assert.h>
#include <malloc.h>
#include <memory.h>

void ids_compute_packet_inclusion_(ids_t* ids)
{
	int idx = 0;  // progression order of inclusion
	int s = 0;  // packet index of precinct
	const int beta1 = ids->nlxy.x - ids->nlxy.y + 1;
	for (int beta = 0; beta < beta1; ++beta)
	{
		for (int i = 0; i < ids->ncomps - ids->sd; ++i)
		{
			const int b = ids->band_idx[i][beta];
			assert(b >= 0);
			ids->pi[idx].b = b;
			ids->pi[idx].y = 0;
			ids->pi[idx].s = s;
			++idx;
		}
	}
	for (int beta0 = beta1; beta0 < ids->nb; beta0 += 3)
	{
		const int nlines = 1 << (ids->nlxy.y - ids->band_d[0][beta0].y);  // this assumes sy[0] == 1
		for (int lambda = 0; lambda < nlines; ++lambda)
		{
			for (int beta = beta0; beta < beta0 + 3; ++beta)
			{
				int r = 1;
				for (int i = 0; i < ids->ncomps - ids->sd; ++i)
				{
					const int b = ids->band_idx[i][beta];
					if (b >= 0)
					{
						if (ids->l0[b] + lambda < ids->l1[0][b])
						{
							s += r;
							ids->pi[idx].b = b;
							ids->pi[idx].y = ids->l0[b] + lambda;
							ids->pi[idx].s = s;
							r = 0;
							++idx;
						}
					}
				}
			}
		}
	}
	for (int lambda = 0; lambda < ids->ph; ++lambda)
	{
		for (int i = ids->ncomps - ids->sd; i < ids->ncomps; ++i)
		{
			const int b = ids->band_idx[i][0];
			if (ids->l0[b] + lambda < ids->l1[0][b])
			{
				++s;
				ids->pi[idx].b = b;
				ids->pi[idx].y = ids->l0[b] + lambda;
				ids->pi[idx].s = s;
				++idx;
			}
		}
	}
	ids->npi = idx;
	ids->npc = s + 1;
	assert(idx <= MAX_PACKETS);
}

int ids_calculate_cs(const xs_image_t* im, const int ndecomp_h, const int cw)
{
	if (cw > 0)
	{
		int max_sx = 0;
		for (int c = 0; c < im->ncomps; ++c)
			if (max_sx < im->sx[c])
				max_sx = im->sx[c];
		return 8 * cw * max_sx * (1 << ndecomp_h);
	}
	return im->width;
}

int ids_calculate_nbands(const xs_image_t* im, const int ndecomp_h, const int ndecomp_v, const int Sd)
{
	assert(im != NULL);
	assert(im->ncomps > 0 && im->ncomps <= MAX_NCOMPS);
	assert(Sd >= 0 && Sd < im->ncomps);
	int nbands = 0;
	for (int c = 0; c < (im->ncomps - Sd); ++c)
	{
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const int ndecomp_v_p = ndecomp_v - (im->sy[c] >> 1);
		nbands += 2 * MIN(ndecomp_h, ndecomp_v_p) + MAX(ndecomp_h, ndecomp_v_p) + 1;
	}
	nbands += Sd;
	assert(nbands > 0 && nbands <= MAX_NBANDS);
	return nbands;
}

void ids_construct(ids_t* ids, const xs_image_t* im, const int ndecomp_h, const int ndecomp_v, const int sd, const int cw, const int lh)
{
	// This function implements Annex B.
	assert(im != NULL && ids != NULL);
	assert(im->ncomps > 0 && im->ncomps <= MAX_NCOMPS);
	assert(ndecomp_v <= ndecomp_h && ndecomp_h > 0 && ndecomp_v >= 0 && ndecomp_h <= MAX_NDECOMP_H && ndecomp_v <= MAX_NDECOMP_V);
	assert(im->sy[0] == 1);
	assert(lh == 0 || lh == 1);
	assert(sd >= 0 && sd <= im->ncomps);

	memset(ids, 0, sizeof(ids_t));
	
	ids->ncomps = im->ncomps;
	ids->w = im->width;
	ids->h = im->height;
	ids->sd = sd;
	ids->nlxy.x = ndecomp_h;
	ids->nlxy.y = ndecomp_v;
	ids->nb = 2 * ids->nlxy.y + ids->nlxy.x + 1;
	ids->use_long_precinct_headers = !((ids->w * ids->ncomps < 32752) && (lh == 0));

	// High-pass theta.
	for (int b = 1; b <= ids->nlxy.x - ids->nlxy.y; ++b)  // h-DWT only
	{
		ids->band_is_high[b].x = true;
	}
	assert((ids->nb - ids->nlxy.x + ids->nlxy.y - 1) % 3 == 0);
	for (int b = ids->nlxy.x - ids->nlxy.y + 1; b < ids->nb; b += 3)  // hv-DWT
	{
		ids->band_is_high[b].x = true;  // HL
		ids->band_is_high[b + 1].y = true;  // LH
		ids->band_is_high[b + 2].x = true;  // HH
		ids->band_is_high[b + 2].y = true;  // HH
	}

	for (int c = 0; c < ids->ncomps; ++c)
	{
		// Component levels and dimensions.
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		ids->comp_w[c] = ids->w / im->sx[c];
		ids->comp_h[c] = ids->h / im->sy[c];
		if (c < ids->ncomps - ids->sd)
		{
			ids->nlxyp[c].x = ids->nlxy.x;
			assert(im->sy[c] == 1 || im->sy[c] == 2);
			ids->nlxyp[c].y = ids->nlxy.y - (im->sy[c] >> 1);
		}
		// Band existence.
		ids->band_idx[c][0] = 1;
		ids->band_d[c][0].x = ids->nlxyp[c].x;
		ids->band_d[c][0].y = ids->nlxyp[c].y;
		for (int b = 1; b <= ids->nlxyp[c].x - ids->nlxyp[c].y; ++b)
		{
			ids->band_idx[c][b] = 1;
			ids->band_d[c][b].x = ids->nlxyp[c].x + 1 - b;
			ids->band_d[c][b].y = ids->nlxyp[c].y;
		}
		for (int b = ids->nlxyp[c].x - ids->nlxyp[c].y + 1; b < ids->nb - 3 * ids->nlxyp[c].y; ++b)
		{
			ids->band_idx[c][b] = -1;
			ids->band_d[c][b].x = -1;
			ids->band_d[c][b].y = -1;
		}
		for (int i = 3 * ids->nlxyp[c].y; i > 0; --i)
		{
			const int b = ids->nb - i;
			const int d = (i + 2) / 3;
			ids->band_idx[c][b] = 1;
			ids->band_d[c][b].x = d;
			ids->band_d[c][b].y = d;
		}
	}

	// Band indexing.
	for (int b = 0; b < ids->nb; ++b)
	{
		for (int c = 0; c < ids->ncomps - ids->sd; ++c)
		{
			if (ids->band_idx[c][b] == 1)
			{
				const int8_t idx = ids->nbands++;
				ids->band_idx[c][b] = idx;
				ids->band_idx_to_c_and_b[idx].c = c;
				ids->band_idx_to_c_and_b[idx].b = b;
			}
		}
	}
	for (int c = ids->ncomps - ids->sd; c < ids->ncomps; ++c)
	{
		if (ids->band_idx[c][0] == 1)
		{
			const int8_t idx = ids->nbands++;
			ids->band_idx[c][0] = idx;
			ids->band_idx_to_c_and_b[idx].c = c;
			ids->band_idx_to_c_and_b[idx].b = 0;
		}
	}
	assert(ids_calculate_nbands(im, ndecomp_h, ndecomp_v, ids->sd) == ids->nbands);

	// Band dimensions.
	for (int c = 0; c < ids->ncomps; ++c)
	{
		const int comp_w = ids->comp_w[c];
		const int comp_h = ids->comp_h[c];
		for (int b = 0; b < ids->nb; ++b)
		{
			if (ids->band_idx[c][b] < 0)
				continue;  // band does not exist

			if (ids->band_is_high[b].x)
			{
				const int d = 1 << (ids->band_d[c][b].x - 1);
				ids->band_dim[c][b].w = (comp_w + d - 1) / d / 2;
			}
			else
			{
				const int d = 1 << ids->band_d[c][b].x;
				ids->band_dim[c][b].w = (comp_w + d - 1) / d;
			}
			if (ids->band_is_high[b].y)
			{
				const int d = 1 << (ids->band_d[c][b].y - 1);
				ids->band_dim[c][b].h = (comp_h + d - 1) / d / 2;
			}
			else
			{
				const int d = 1 << ids->band_d[c][b].y;
				ids->band_dim[c][b].h = (comp_h + d - 1) / d;
			}
			if (ids->band_max_width < ids->band_dim[c][b].w)
			{
				ids->band_max_width = ids->band_dim[c][b].w;
			}
		}
	}

	// Precincts.
	ids->cs = ids_calculate_cs(im, ndecomp_h, cw);
	ids->ph = 1 << ids->nlxy.y;
	ids->npx = (ids->w + ids->cs - 1) / ids->cs;
	ids->npy = (ids->h + ids->ph - 1) / ids->ph;
	ids->np = ids->npx * ids->npy;
	ids->pw[0] = ids->cs;
	ids->pw[1] = (ids->w - 1) % ids->cs + 1;  // last
	for (int b = 0; b < ids->nbands; ++b)
	{
		ids_band_idx_to_comp_and_filter_type_t i2cft = ids->band_idx_to_c_and_b[b];
		const int sx = im->sx[i2cft.c] - 1;
		const int pw_in_b_0 = (ids->pw[0] + sx) >> sx;
		const int pw_in_b_1 = (ids->pw[1] + sx) >> sx;
		if (ids->band_is_high[i2cft.b].x)
		{
			const int d = 1 << (ids->band_d[i2cft.c][i2cft.b].x - 1);
			ids->pwb[0][b] = (pw_in_b_0 + d - 1) / d / 2;
			ids->pwb[1][b] = (pw_in_b_1 + d - 1) / d / 2;
		}
		else
		{
			const int d = 1 << ids->band_d[i2cft.c][i2cft.b].x;
			ids->pwb[0][b] = (pw_in_b_0 + d - 1) / d;
			ids->pwb[1][b] = (pw_in_b_1 + d - 1) / d;
		}
		if (b < ids->nbands - ids->sd)
		{
			const int tmp = 1 << MAX((ids->nlxyp[i2cft.c].y - ids->band_d[i2cft.c][i2cft.b].y), 0);
			ids->l0[b] = tmp * ids->band_is_high[i2cft.b].y;
			ids->l1[0][b] = ids->l0[b] + tmp;
			ids->l1[1][b] = ids->l0[b] + (ids->band_dim[i2cft.c][i2cft.b].h - (ids->npy - 1) * tmp);
		}
		else
		{
			ids->l0[b] = 0;
			ids->l1[0][b] = ids->ph;
			ids->l1[1][b] = (ids->band_dim[i2cft.c][i2cft.b].h + ids->ph - 1) % ids->ph + 1;
		}
	}

	// Compute packet inclusion (i.e. progression ordering).
	ids_compute_packet_inclusion_(ids);
}
