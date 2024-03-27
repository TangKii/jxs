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
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include "sig_flags.h"
#include "bitpacking.h"
#include "common.h"

#define SIGFLAGS_NEXTLVL_SIZE(w , g) (((w) + (g) - 1) / (g))
#define SIGFLAGS_LAST_GROUP_SIZE(w, g) (((w) % (g)) ? ((w) % (g)) : (g))

struct sig_flags_t
{
	int max_w;
	int min_group_width;

	int w;
	int group_width;

	int8_t* lvls;
	int lvls_size;
	int lvls_max_size;
	int lvls_ptr;
	int lvls_zero_count;
	int lvls_one_count;
	int last_group_width;
};

sig_flags_t* sig_flags_malloc(int max_w, int min_group_width)
{
	sig_flags_t* sig_flags = (sig_flags_t*)malloc(sizeof(sig_flags_t));
	if (sig_flags)
	{
		sig_flags->min_group_width = min_group_width;
		sig_flags->group_width = min_group_width;
		sig_flags->max_w = max_w;
		sig_flags->lvls_size = 0;
		sig_flags->lvls_ptr = 0;
		sig_flags->lvls_max_size = SIGFLAGS_NEXTLVL_SIZE(max_w, sig_flags->group_width);;
		sig_flags->lvls = (int8_t*)malloc(sig_flags->lvls_max_size * sizeof(int8_t));
		assert(sig_flags->lvls != NULL);
		memset(sig_flags->lvls, 0, sig_flags->lvls_max_size * sizeof(int8_t));
		sig_flags->lvls_one_count = 0;
		sig_flags->lvls_zero_count = 0;
	}
	return sig_flags;
}

void sig_flags_free(sig_flags_t* sig_flags)
{
	if (sig_flags)
	{
		if (sig_flags->lvls)
		{
			free(sig_flags->lvls);
		}
		free(sig_flags);
	}
}

int sig_flags_init(sig_flags_t* sig_flags, int8_t* input_buf, int buf_len, int group_width)
{
	int i = 0;

	assert(buf_len <= sig_flags->max_w);
	assert(group_width >= sig_flags->min_group_width);

	sig_flags->w = buf_len;
	sig_flags->lvls_zero_count = 0;
	sig_flags->lvls_one_count = 0;
	sig_flags->lvls_size = SIGFLAGS_NEXTLVL_SIZE(buf_len, sig_flags->group_width);
	sig_flags->last_group_width = SIGFLAGS_LAST_GROUP_SIZE(buf_len, sig_flags->group_width);

	memset(sig_flags->lvls, 0, sig_flags->lvls_max_size * sizeof(int8_t));

	for (i = 0; i < buf_len; i++)
	{
		if (input_buf[i] != 0)
		{
			sig_flags->lvls[i / sig_flags->group_width] = 1;
		}
	}
	for (i = 0; i < sig_flags->lvls_size; i++) {
		sig_flags->lvls_zero_count += (!sig_flags->lvls[i]);
		sig_flags->lvls_one_count += sig_flags->lvls[i];
	}
	return 0;
}

int sig_flags_inclusion_mask(sig_flags_t* sig_flags, uint8_t* inclusion)
{
	int i;
	for (i = 0; i < sig_flags->w; i++)
	{
		inclusion[i] = sig_flags->lvls[i / sig_flags->group_width];
	}
	return 0;
}

int sig_flags_filter_values(sig_flags_t* sig_flags, int8_t* buf_in, int8_t* buf_out, int* out_len)
{
	int i;
	*out_len = 0;
	for (i = 0; i < sig_flags->w; i++)
	{
		buf_out[*out_len] = buf_in[i];
		if (sig_flags->lvls[i / sig_flags->group_width])
		{
			*out_len += 1;
		}
	}
	return 0;
}

int sig_flags_budget(sig_flags_t* sig_flags)
{
	return sig_flags->lvls_size;
}

int sig_flags_write(bit_packer_t* bitstream, sig_flags_t* sig_flags)
{
	int i;
	int nbits = 0;

	for (i = 0; i < sig_flags->lvls_size; i++)
	{
		int val = sig_flags->lvls[i];
		val = !val;
		nbits += bitpacker_write(bitstream, val, 1);
	}
	return nbits;
}

int sig_flags_read(bit_unpacker_t* bitstream, sig_flags_t* sig_flags, int buf_len, int group_width)
{
	int i;
	int nbits = 0;
	uint64_t val;

	assert(buf_len <= sig_flags->max_w);
	assert(group_width >= sig_flags->min_group_width);

	sig_flags->w = buf_len;
	sig_flags->lvls_zero_count = 0;
	sig_flags->lvls_one_count = 0;
	sig_flags->lvls_size = SIGFLAGS_NEXTLVL_SIZE(buf_len, sig_flags->group_width);
	sig_flags->last_group_width = SIGFLAGS_LAST_GROUP_SIZE(buf_len, sig_flags->group_width);

	for (i = 0; i < sig_flags->lvls_size; i++)
	{
		nbits += bitunpacker_read(bitstream, &val, 1);
		val = !val;
		sig_flags->lvls[i] = val & 0x1;
	}
	return nbits;
}
