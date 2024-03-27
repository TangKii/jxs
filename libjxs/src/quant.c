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

#include <memory.h>
#include <stdint.h>
#include <assert.h>

#include "quant.h"
#include "common.h"

int uniform_dq(int sig_mag_value, int gcli, int gtli)
{
	uint32_t dq_out = 0;
	if (gcli > gtli)
	{
		int zeta = gcli - gtli + 1;
		int d = sig_mag_value & ~SIGN_BIT_MASK;
		dq_out = ((d << zeta) - d + (1 << gcli)) >> (gcli + 1);
		if (dq_out)
			dq_out |= (sig_mag_value & SIGN_BIT_MASK);
	}
	return dq_out;
}

int uniform_dq_inverse(int dq_in, int gcli, int gtli)
{
	int sign = (dq_in & SIGN_BIT_MASK);
	int phi = dq_in & ~SIGN_BIT_MASK;
	int zeta = gcli - gtli + 1;
	int rho = 0;
	for (rho = 0; phi > 0; phi >>= zeta)
		rho += phi;
	return sign | rho;
}

int deadzone_dq(int sig_mag_value, int gcli, int gtli)
{
	int dq_out = (sig_mag_value & ~SIGN_BIT_MASK) >> gtli;
	if (dq_out)
		dq_out |= (sig_mag_value & SIGN_BIT_MASK);
	return dq_out;
}

int deadzone_dq_inverse(int dq_in, int gcli, int gtli)
{
	sig_mag_data_t sigmag_data = dq_in;
	if (gtli > 0 && (dq_in & ~SIGN_BIT_MASK))
		sigmag_data |= (1 << (gtli - 1));
	return sigmag_data;
}

int apply_dq(int dq_type, int sig_mag_value, int gcli, int gtli)
{
	switch (dq_type) {
	case 1:
		return uniform_dq(sig_mag_value, gcli, gtli);
	case 0:
		return deadzone_dq(sig_mag_value, gcli, gtli);
	default:
		assert(!"invalid quantizer type");
		return -1;
	}
}

sig_mag_data_t apply_dq_inverse(int dq_type, int dq_in, int gcli, int gtli)
{
	switch (dq_type) {
	case 1:
		return uniform_dq_inverse(dq_in, gcli, gtli);
	case 0:
		return deadzone_dq_inverse(dq_in, gcli, gtli);
	default:
		assert(!"invalid quantizer type");
		return -1;
	}
}

int quant(sig_mag_data_t* buf, int buf_len, gcli_data_t* gclis, int group_size, int gtli, int dq_type)
{
	int idx = 0;
	for (int group = 0; group < (buf_len + group_size - 1) / group_size; group++)
	{
		const int gcli = gclis[group];
		if (gcli <= gtli)
		{
			memset(&buf[idx], 0, sizeof(sig_mag_data_t) * group_size);
		}
		else
		{
			if (gtli > 0)
			{
				for (int i = 0; i < group_size; i++)
				{
					const int sign = (SIGN_BIT_MASK & buf[idx + i]);
					buf[idx + i] = (~SIGN_BIT_MASK) & apply_dq(dq_type, buf[idx + i], gcli, gtli);
					buf[idx + i] = buf[idx + i] << gtli;
					buf[idx + i] |= sign;
				}
			}
		}
		idx += group_size;
	}
	return 0;
}

int dequant(sig_mag_data_t* buf, int buf_len, gcli_data_t* gclis, int group_size, int gtli, int dq_type)
{
	int idx = 0;
	for (int group = 0; group < (buf_len + group_size - 1) / group_size; group++)
	{
		const int gcli = gclis[group];
		if (gcli > gtli)
		{			
			if (gtli > 0)
			{
				for (int i = 0; i < group_size; i++)
					buf[idx + i] = apply_dq_inverse(dq_type, buf[idx + i], gcli, gtli);
			}
		}
		idx += group_size;
	}
	return 0;
}
