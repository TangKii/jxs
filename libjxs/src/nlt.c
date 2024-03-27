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

#include "nlt.h"
#include "common.h"
#include <assert.h>

static INLINE xs_data_in_t clamp(xs_data_in_t v, xs_data_in_t max_v)
{
	if (v > max_v)
	{
		return max_v;
	}	
	if (v < 0)
	{
		return 0;
	}
	return v;
}

static INLINE int64_t clamp64(int64_t v, int64_t max_v)
{
	if (v > max_v)
	{
		return max_v;
	}
	if (v < 0)
	{
		return 0;
	}
	return v;
}

static INLINE int64_t sqrt_approx_fixpoint(int64_t v, const uint8_t Bw)
{
	assert(v >= 0);
	int64_t r = 0;
	for (int i = 0; i < Bw ; ++i)
	{
		r <<= 1;
		v <<= 2;
		if ((v >> Bw) > r)
		{
			v -= (r + 1) << Bw;
			r += 2;
		}
	}
	return r >> 1;
}

void nlt_inverse_linear(xs_image_t* im, const uint8_t Bw)
{
	const uint8_t s = Bw - (uint8_t)im->depth;
	const xs_data_in_t dclev_and_rounding = ((1 << Bw) >> 1) + ((1 << s) >> 1);
	const xs_data_in_t max_val = (1 << im->depth) - 1;
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			*the_ptr = clamp((*the_ptr + dclev_and_rounding) >> s, max_val);
			++the_ptr;
		}
	}
}

void nlt_forward_linear(xs_image_t* im, const uint8_t Bw)
{
	const uint8_t s = Bw - (uint8_t)im->depth;
	const xs_data_in_t dclev = ((1 << Bw) >> 1);
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			*the_ptr = (*the_ptr << s) - dclev;
			++the_ptr;
		}
	}
}

void nlt_inverse_quadratic(xs_image_t* im, const uint8_t Bw, const xs_nlt_parameters_t nlt_parameters)
{
	const xs_data_in_t vdco = (xs_data_in_t)nlt_parameters.quadratic.alpha - (xs_data_in_t)nlt_parameters.quadratic.sigma * 32768;
	const uint8_t s = (Bw << 1) - (uint8_t)im->depth;
	const xs_data_in_t dclev = ((1 << Bw) >> 1);
	const xs_data_in_t s_r = (1 << s) >> 1;
	const xs_data_in_t max_val = (1 << im->depth) - 1;
	const xs_data_in_t max_coef = (1 << Bw) - 1;
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			int64_t v = (int64_t)clamp(*the_ptr + dclev, max_coef);
			*the_ptr = (xs_data_in_t)clamp64(((v * v + s_r) >> s) + vdco, max_val);
			++the_ptr;
		}
	}
}

void nlt_forward_quadratic(xs_image_t* im, const uint8_t Bw, const xs_nlt_parameters_t nlt_parameters)
{
	const xs_data_in_t vdco = (xs_data_in_t)nlt_parameters.quadratic.alpha - (xs_data_in_t)nlt_parameters.quadratic.sigma * 32768;
	const uint8_t s = Bw - (uint8_t)im->depth;
	const xs_data_in_t max_val = (1 << im->depth) - 1;
	const xs_data_in_t dclev = ((1 << Bw) >> 1);
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			int64_t v = (int64_t)clamp(*the_ptr - vdco, max_val) << s;
			v = sqrt_approx_fixpoint(v, Bw);
			*the_ptr = (xs_data_in_t)(v - dclev);
			++the_ptr;
		}
	}
}

void nlt_inverse_extended(xs_image_t* im, const uint8_t Bw, const xs_nlt_parameters_t nlt_parameters)
{
	const uint8_t e = Bw - nlt_parameters.extended.E;
	const int64_t B2 = (int64_t)nlt_parameters.extended.T1 * nlt_parameters.extended.T1;
	const int64_t A1 = B2 + ((int64_t)nlt_parameters.extended.T1 << e) + (1ll << (2ll * e - 2));
	const int64_t B1 = (int64_t)nlt_parameters.extended.T1 + (1ll << (e - 1));
	const int64_t A3 = B2 + ((int64_t)nlt_parameters.extended.T2 << e) - (1ll << (2ll * e - 2));
	const int64_t B3 = (int64_t)nlt_parameters.extended.T2 - (1ll << (e - 1));
	const uint8_t s = (Bw << 1) - (uint8_t)im->depth;
	const xs_data_in_t s_r = (1 << s) >> 1;
	const xs_data_in_t max_val = (1 << im->depth) - 1;
	const xs_data_in_t max_coef = (1 << Bw) - 1;
	const xs_data_in_t dclev = ((1 << Bw) >> 1);
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			int64_t v = (int64_t)*the_ptr + dclev;
			if (v < nlt_parameters.extended.T1)
			{
				v = clamp64(B1 - v, max_coef);
				v = A1 - v * v;
			}
			else if (v < nlt_parameters.extended.T2)
			{
				v = (v << e) + B2;
			}
			else
			{
				v = clamp64(v - B3, max_coef);
				v = A3 + v * v;
			}
			*the_ptr = (xs_data_in_t)clamp64((v + s_r) >> s, max_val);
			++the_ptr;
		}
	}
}

void nlt_forward_extended(xs_image_t* im, const uint8_t Bw, const xs_nlt_parameters_t nlt_parameters)
{
	const uint8_t e = Bw - nlt_parameters.extended.E;
	const int64_t B2 = (int64_t)nlt_parameters.extended.T1 * nlt_parameters.extended.T1;
	const int64_t A1 = B2 + ((int64_t)nlt_parameters.extended.T1 << e) + (1ll << (2ll * e - 2));
	const int64_t B1 = (int64_t)nlt_parameters.extended.T1 + (1ll << (e - 1));
	const int64_t A3 = B2 + ((int64_t)nlt_parameters.extended.T2 << e) - (1ll << (2ll * e - 2));
	const int64_t B3 = (int64_t)nlt_parameters.extended.T2 - (1ll << (e - 1));
	const int64_t Q1 = B2 + ((int64_t)nlt_parameters.extended.T1 << e);
	const int64_t Q2 = B2 + ((int64_t)nlt_parameters.extended.T2 << e);
	const uint8_t s = (Bw << 1) - (uint8_t)im->depth;
	const xs_data_in_t s_r = (1 << s) >> 1;
	const xs_data_in_t dclev = ((1 << Bw) >> 1);
	for (int c = 0; c < im->ncomps; ++c)
	{
		assert(im->sx[c] == 1 || im->sx[c] == 2);
		assert(im->sy[c] == 1 || im->sy[c] == 2);
		const size_t sample_count = (size_t)(im->width / im->sx[c]) * (size_t)(im->height / im->sy[c]);
		xs_data_in_t* the_ptr = im->comps_array[c];
		for (size_t i = sample_count; i != 0; --i)
		{
			int64_t v = (int64_t)*the_ptr << s;
			if (v < Q1)
			{				
				v = B1 - (int64_t)sqrt((double)(A1 - v) + 0.5);
			}
			else if (v < Q2)
			{
				v = (v - B2) >> e;
			}
			else
			{				
				v = B3 + (int64_t)sqrt((double)(v - A3) + 0.5);
			}
			*the_ptr = (xs_data_in_t)(v - dclev);
			++the_ptr;
		}
	}
}

void nlt_inverse_transform(xs_image_t* im, const xs_config_parameters_t* p)
{
	switch (p->Tnlt)
	{
	case XS_NLT_NONE:
	{
		nlt_inverse_linear(im, p->Bw);
		break;
	}
	case XS_NLT_QUADRATIC:
	{
		nlt_inverse_quadratic(im, p->Bw, p->Tnlt_params);
		break;
	}
	case XS_NLT_EXTENDED:
	{
		nlt_inverse_extended(im, p->Bw, p->Tnlt_params);
		break;
	}
	default:
		assert(false);
		break;
	}
}

void nlt_forward_transform(xs_image_t* im, const xs_config_parameters_t* p)
{
	switch (p->Tnlt)
	{
	case XS_NLT_NONE:
	{
		nlt_forward_linear(im, p->Bw);
		break;
	}
	case XS_NLT_QUADRATIC:
	{
		nlt_forward_quadratic(im, p->Bw, p->Tnlt_params);
		break;
	}
	case XS_NLT_EXTENDED:
	{
		nlt_forward_extended(im, p->Bw, p->Tnlt_params);
		break;
	}
	default:
		assert(false);
		break;
	}
}

void xs_config_nlt_extended_auto_thresholds(xs_config_t* cfg, const uint8_t bpp, const xs_data_in_t th1, const xs_data_in_t th2)
{
	assert(cfg->p.Tnlt == XS_NLT_EXTENDED);
	if (cfg->p.Bw == 0xff)  // if not yet set, assume 18
	{
		cfg->p.Bw = 18;
	}

	const uint8_t E = 3;
	assert(E < cfg->p.Bw);
	
	const uint8_t e = cfg->p.Bw - E;
	const uint8_t s = (cfg->p.Bw << 1) - bpp;
	cfg->p.Tnlt_params.extended.E = E;
	cfg->p.Tnlt_params.extended.T1 = (uint32_t)sqrt((double)((1ll << (2ll * e - 2)) + ((int64_t)th1 << s)) + 0.5) - (1ll << (e - 1));
	cfg->p.Tnlt_params.extended.T2 = (uint32_t)((th2 * (1ll << (s)) - (int64_t)cfg->p.Tnlt_params.extended.T1 * cfg->p.Tnlt_params.extended.T1) >> e);
}