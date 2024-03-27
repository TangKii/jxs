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

#ifndef IDS_H
#define IDS_H

#include "common.h"

typedef struct ids_band_idx_to_comp_and_filter_type_t ids_band_idx_to_comp_and_filter_type_t;
typedef struct ids_xy_val_t ids_xy_val_t;
typedef struct ids_dim_val_t ids_dim_val_t;
typedef struct ids_packet_inclusion_t ids_packet_inclusion_t;
typedef struct ids_t ids_t;

struct ids_band_idx_to_comp_and_filter_type_t
{
	int8_t c : 3;
	int8_t b : 5;
};

struct ids_xy_val_t
{
	int8_t x : 4;
	int8_t y : 4;
};

struct ids_dim_val_t
{
	int w;
	int h;
};

struct ids_packet_inclusion_t
{
	int b;
	int y;
	int s;
};

struct ids_t
{
	int8_t ncomps;
	int w;
	int h;
	int comp_w[MAX_NCOMPS];
	int comp_h[MAX_NCOMPS];

	int8_t nbands;
	int8_t sd;
	int8_t nb;

	ids_xy_val_t nlxy;
	ids_xy_val_t nlxyp[MAX_NCOMPS];
	int8_t band_idx[MAX_NCOMPS][MAX_NFILTER_TYPES];
	ids_band_idx_to_comp_and_filter_type_t band_idx_to_c_and_b[MAX_NBANDS];
	ids_xy_val_t band_d[MAX_NCOMPS][MAX_NFILTER_TYPES];
	ids_xy_val_t band_is_high[MAX_NFILTER_TYPES];
	ids_dim_val_t band_dim[MAX_NCOMPS][MAX_NFILTER_TYPES];
	int band_max_width;

	int cs;
	int npx;
	int npy;
	int np;
	int pw[2];  // width of normal and last
	int ph;
	int pwb[2][MAX_NBANDS];  // widths of normal and last
	int l0[MAX_NBANDS];
	int l1[2][MAX_NBANDS];  // normal and last

	ids_packet_inclusion_t pi[MAX_PACKETS];
	int npc;
	int npi;

	bool use_long_precinct_headers;
};

int ids_calculate_cs(const xs_image_t* im, const int ndecomp_h, const int cw);
int ids_calculate_nbands(const xs_image_t* im, const int ndecomp_h, const int ndecomp_v, const int Sd);

void ids_construct(ids_t* ids, const xs_image_t* im, const int ndecomp_h, const int ndecomp_v, const int sd, const int cw, const int lh);

#endif