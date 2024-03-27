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
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "bitpacking.h"

uint64_t ipx_htobe64(uint64_t in)
{
	int i = 0;
	union {
		uint64_t integer;
		uint8_t  bytes[8];
	} a, b;
	a.integer = in;
	for (i = 0; i < 8; i++)
		b.bytes[i] = a.bytes[7 - i];
	return b.integer;
}

#define MAXB (sizeof(uint64_t) * 8)

#define PACK_WORD(a) ipx_htobe64(a)

struct bit_packer_t
{
	uint64_t* ptr;
	uint64_t* ptr_cur;
	uint64_t* ptr_max;
	int bit_offset;
	size_t max_size;
	int flushed;
};

struct bit_unpacker_t
{
	uint64_t* ptr;
	uint64_t* ptr_cur;
	uint64_t cur;
	int bit_offset;
	size_t max_size;
	uint64_t consumed;
};

struct unary_lup_struct
{
	int val;
	int nbits;
};

struct unary_lup_struct unary_lup[] =
{
	{ 65535, 16 },
	{ 65533, 16 },
	{ 32765, 15 },
	{ 16381, 14 },
	{ 8189, 13 },
	{ 4093, 12 },
	{ 2045, 11 },
	{ 1021, 10 },
	{ 509, 9 },
	{ 253, 8 },
	{ 125, 7 },
	{ 61, 6 },
	{ 29, 5 },
	{ 13, 4 },
	{ 5, 3 },
	{ 0, 1 },
	{ 4, 3 },
	{ 12, 4 },
	{ 28, 5 },
	{ 60, 6 },
	{ 124, 7 },
	{ 252, 8 },
	{ 508, 9 },
	{ 1020, 10 },
	{ 2044, 11 },
	{ 4092, 12 },
	{ 8188, 13 },
	{ 16380, 14 },
	{ 32764, 15 },
	{ 65532, 16 },
	{ 65534, 16 },
};

struct unary_lup_struct	unary_lup_ver4clip[] =
{
	{ 65535, 16 },
	{ 65535, 16 },
	{ 65535, 16 },
	{ 65533, 16 },
	{ 32765, 15 },
	{ 16381, 14 },
	{ 8189, 13 },
	{ 4093, 12 },
	{ 2045, 11 },
	{ 1021, 10 },
	{ 509, 9 },
	{ 253, 8 },
	{ 125, 7 },
	{ 14, 4 },
	{ 2, 2 },
	{ 0, 1 },
	{ 6, 3 },
	{ 30, 5 },
	{ 124, 7 },
	{ 252, 8 },
	{ 508, 9 },
	{ 1020, 10 },
	{ 2044, 11 },
	{ 4092, 12 },
	{ 8188, 13 },
	{ 16380, 14 },
	{ 32764, 15 },
	{ 65532, 16 },
	{ 65534, 16 },
	{ 65534, 16 },
	{ 65534, 16 },
};

struct unary_lup_struct	unary_lup_full[] =
{
	{ 524285, 19 },
	{ 262141, 18 },
	{ 131069, 17 },
	{ 65533, 16 },
	{ 32765, 15 },
	{ 16381, 14 },
	{ 8189, 13 },
	{ 4093, 12 },
	{ 2045, 11 },
	{ 1021, 10 },
	{ 509, 9 },
	{ 253, 8 },
	{ 125, 7 },
	{ 14, 4 },
	{ 2, 2 },
	{ 0, 1 },
	{ 6, 3 },
	{ 30, 5 },
	{ 124, 7 },
	{ 252, 8 },
	{ 508, 9 },
	{ 1020, 10 },
	{ 2044, 11 },
	{ 4092, 12 },
	{ 8188, 13 },
	{ 16380, 14 },
	{ 32764, 15 },
	{ 65532, 16 },
	{ 131068, 17 },
	{ 262140, 18 },
	{ 524285, 19 },
};

bit_packer_t* bitpacker_init()
{
	bit_packer_t* packer = (bit_packer_t*)malloc(sizeof(bit_packer_t));
	if (!packer)
	{
		return NULL;
	}

	packer->ptr = NULL;
	packer->ptr_cur = NULL;

	return packer;
}

int bitpacker_set_buffer(bit_packer_t* packer, void* ptr, size_t max_size)
{
#ifndef _WIN32
	assert(((uint64_t)ptr & 0x3) == 0);
#endif
	packer->ptr = (uint64_t*)ptr;
	packer->max_size = (max_size / 8) * 8;
	packer->ptr_max = packer->ptr + (max_size / 8) - 1;
	bitpacker_reset(packer);
	return 0;
}

uint64_t* bitpacker_get_buffer(bit_packer_t* packer)
{
	return packer->ptr;
}

int bitpacker_reset(bit_packer_t* packer)
{
	memset(packer->ptr, 0, packer->max_size);
	packer->ptr_cur = packer->ptr;
	packer->bit_offset = 0;
	packer->flushed = 0;
	return 0;
}

int bitpacker_write(bit_packer_t* packer, uint64_t val, unsigned int nbits)
{
	unsigned int available0 = (MAXB - packer->bit_offset);
	int len0 = (available0 >= nbits) ? nbits : available0;
	int len1 = nbits - len0;

	assert(nbits <= MAXB);

	val &= ((uint64_t)-1) >> (MAXB - nbits);
	if (len0)
	{
		*(packer->ptr_cur) |= (val >> len1) << (available0 - len0);
		packer->bit_offset += len0;
	}
	if (len1)
	{
		if (packer->ptr_cur < packer->ptr_max)
		{
			*packer->ptr_cur = PACK_WORD(*packer->ptr_cur);
			packer->ptr_cur++;
			packer->bit_offset = 0;
			*(packer->ptr_cur) |= (val << (MAXB - len1));
			packer->bit_offset += len1;
		}
		else
		{
			fprintf(stderr, "Error: bitpacker reached end of buffer!\n");
			return -1;
		}
	}
	return nbits;
}

int bitpacker_write_unary_signed(bit_packer_t* packer, int8_t val, unary_alphabet_t alphabet)
{
	assert((val <= MAX_UNARY) && (val >= -MAX_UNARY));
	switch (alphabet)
	{
	case UNARY_ALPHABET_FULL:
		return bitpacker_write(packer, unary_lup_full[val + MAX_UNARY].val, unary_lup_full[val + MAX_UNARY].nbits);
	case UNARY_ALPHABET_4_CLIPPED:
		assert(abs(val) <= MAX_UNARY_CLIPPED);
		return bitpacker_write(packer, unary_lup_ver4clip[val + MAX_UNARY].val, unary_lup_ver4clip[val + MAX_UNARY].nbits);
	case UNARY_ALPHABET_0:
		return bitpacker_write(packer, unary_lup[val + MAX_UNARY].val, unary_lup[val + MAX_UNARY].nbits);
	default:
		assert(!"unsupported alphabet specified");
		return 0;
	}
}

int bounded_code_get_min_max(int predictor, int gtli, int* min_allowed, int* max_allowed)
{
	*min_allowed = -MAX(predictor - gtli, 0);
	*max_allowed = MAX(MAX_GCLI - MAX(predictor, gtli), 0);
	return 0;
}

int bounded_code_get_unary_code(int8_t val, int8_t min_allowed, int8_t max_allowed)
{
	assert(min_allowed <= 0);
	assert(val >= min_allowed);
	assert((val <= max_allowed) || (max_allowed == -1));

	const int trigger = abs(min_allowed);
	const int aval = abs(val);
	if (aval <= trigger)
	{
		if (val < 0)
			return 2 * aval - 1;
		else
			return 2 * aval;
	}
	else
	{
		return trigger + aval;
	}
}

int bitunpacker_read_bounded_code(bit_unpacker_t* unpacker, int8_t min_allowed, int8_t max_allowed, int8_t* val)
{
	int8_t tmp;
	int n;

	const int trigger = abs(min_allowed);

	n = bitunpacker_read_unary_unsigned(unpacker, &tmp);
	if (tmp > 2 * trigger)
	{
		*val = tmp - trigger;
	}
	else
	{
		*val = (tmp + 1) / 2;
		if (tmp % 2)
		{
			*val = -*val;
		}
	}
	return n;
}

int bitpacker_write_unary_unsigned(bit_packer_t* packer, int8_t val)
{
	return bitpacker_write(packer, (1ULL << (val + 1ULL)) - 2ULL, val + 1);
}

int bitpacker_flush(bit_packer_t* packer)
{
	if (packer->ptr_cur)
	{
		if ((!packer->flushed) && (packer->bit_offset > 0)) {
			assert(packer->ptr_cur <= packer->ptr_max);
			*packer->ptr_cur = PACK_WORD(*packer->ptr_cur);
		}
		packer->flushed = 1;
	}
	return 0;
}

void bitpacker_close(bit_packer_t* packer)
{
	if (packer)
	{
		bitpacker_flush(packer);
		free(packer);
	}
}

int bitpacker_align(bit_packer_t* packer, int nbits)
{
	if (packer->bit_offset % nbits)
	{
		return bitpacker_write(packer, 0, (nbits - (packer->bit_offset % nbits)));
	}
	return 0;
}

int bitpacker_get_len(bit_packer_t* packer)
{
	assert(packer->bit_offset >= 0);
	return (int)((packer->ptr_cur - packer->ptr) * 64) + packer->bit_offset;
}

int bitpacker_add_padding(bit_packer_t* packer, int nbits)
{
	assert(nbits >= 0);
	for (int i = 0; i < nbits; i += 64)
	{
		int burst = MIN(64, nbits - i);
		if (bitpacker_write(packer, 0, burst) < 0)
		{
			return -1;
		}
	}
	return nbits;
}

int bitpacker_append(bit_packer_t* packer, void* ptr_from, int nbits)
{
	uint64_t* ptr = (uint64_t*)ptr_from;
	const int mod = nbits % 64;
	for (int i = 0; i < nbits / 64; i++)
	{
		if (bitpacker_write(packer, PACK_WORD(*ptr++), 64) < 0)
		{
			return -1;
		}
	}
	if (mod)
	{
		if (bitpacker_write(packer, PACK_WORD(*ptr) >> (64 - mod), mod) < 0)
		{
			return -1;
		}
	}
	return nbits;
}

int bitpacker_from_unpacker(bit_packer_t* packer, bit_unpacker_t* unpacker, int nbits)
{
	uint64_t val;
	for (int i = 0; i < nbits; i++)
	{
		bitunpacker_read(unpacker, &val, 1);
		if (bitpacker_write(packer, val, 1) < 0)
		{
			return -1;
		}
	}
	return nbits;
}

bit_unpacker_t* bitunpacker_init()
{
	bit_unpacker_t* unpacker = (bit_unpacker_t*)malloc(sizeof(bit_unpacker_t));
	if (!unpacker)
	{
		return NULL;
	}

	unpacker->ptr = NULL;
	unpacker->ptr_cur = NULL;

	return unpacker;
}

int bitunpacker_set_buffer(bit_unpacker_t* unpacker, void* ptr, size_t max_size)
{
	if (!unpacker)
	{
		return -1;
	}
#ifndef _WIN32
	assert(((uint64_t)ptr & 0x3) == 0);
#endif
	unpacker->ptr = (uint64_t*)ptr;
	unpacker->max_size = max_size;
	bitunpacker_reset(unpacker);
	return 0;
}

int bitunpacker_reset(bit_unpacker_t* unpacker)
{
	unpacker->ptr_cur = unpacker->ptr;
	unpacker->cur = PACK_WORD(*unpacker->ptr_cur);
	unpacker->bit_offset = 0;
	unpacker->consumed = 0;
	return 0;
}

int bitunpacker_peek(bit_unpacker_t* unpacker, uint64_t* val, int nbits)
{
	bit_unpacker_t copy = *unpacker;

	return bitunpacker_read(&copy, val, nbits);
}

int bitunpacker_read(bit_unpacker_t* unpacker, uint64_t* val, int nbits)
{
	int available0 = MAXB - unpacker->bit_offset;
	int len0 = (available0 >= nbits) ? nbits : available0;
	int len1 = nbits - len0;
	*val = 0;

	if (len0)
	{
		*val |= ((unpacker->cur >> (available0 - len0)) << len1);
		unpacker->bit_offset += len0;
	}
	if (len1)
	{
		unpacker->consumed += (uint64_t)MAXB;
		unpacker->ptr_cur++;
		if ((size_t)(unpacker->ptr_cur + 1 - unpacker->ptr) * 8 > unpacker->max_size) {

			uint64_t buffer = 0UL;
			for (size_t i = 0; i < (unpacker->max_size % 8); ++i)
				((uint8_t*)&buffer)[i] = ((uint8_t*)(unpacker->ptr_cur))[i];
			unpacker->cur = PACK_WORD(buffer);
		}
		else {
			unpacker->cur = PACK_WORD(*unpacker->ptr_cur);
		}
		unpacker->bit_offset = 0;
		*val |= (unpacker->cur >> (MAXB - len1));
		unpacker->bit_offset += len1;
	}
	if (nbits < 64)
		*val = *val & ((1ULL << nbits) - 1ULL);

	return nbits;
}

size_t bitunpacker_consumed(bit_unpacker_t* unpacker)
{
	return (unpacker->consumed + (uint64_t)unpacker->bit_offset + 7ULL) / 8;
}

bool bitunpacker_consumed_all(bit_unpacker_t* unpacker)
{
	return bitunpacker_consumed(unpacker) == unpacker->max_size;
}

size_t bitunpacker_consumed_bits(bit_unpacker_t* unpacker)
{
	return unpacker->consumed + (uint64_t)unpacker->bit_offset;
}


int bitunpacker_close(bit_unpacker_t* unpacker)
{
	if (unpacker)
	{
		free(unpacker);
	}
	return 0;
}

int bitunpacker_read_unary_signed(bit_unpacker_t* unpacker, int8_t* val, unary_alphabet_t alphabet)
{
	const uint64_t nbits_start = bitunpacker_consumed_bits(unpacker);

	switch (alphabet)
	{
	case UNARY_ALPHABET_FULL:
	{
		uint64_t bit = 1;
		(*val) = -1;
		do
		{
			bitunpacker_read(unpacker, &bit, 1);
			(*val)++;
		} while (bit && (*val) < 17);

		if (*val == 1)
			(*val) = -1;
		else if (*val == 2)
			(*val) = 1;
		else if (*val == 3)
			(*val) = -2;
		else if (*val == 4)
			(*val) = 2;
		else if (*val > 4)
		{
			(*val) -= 2;
			bitunpacker_read(unpacker, &bit, 1);
			if (bit)
				(*val) = -(*val);
		}
		break;
	}
	case UNARY_ALPHABET_4_CLIPPED:
	{
		uint64_t bit = 1;
		(*val) = -1;
		do
		{
			bitunpacker_read(unpacker, &bit, 1);
			(*val)++;
		} while (bit && (*val) < 15);

		if (*val == 1)
			(*val) = -1;
		else if (*val == 2)
			(*val) = 1;
		else if (*val == 3)
			(*val) = -2;
		else if (*val == 4)
			(*val) = 2;

		if (*val > 4)
		{
			(*val) -= 2;
			if ((*val) && (*val != MAX_UNARY - 2))
				bitunpacker_read(unpacker, &bit, 1);
			if (bit)
				(*val) = -(*val);
		}
		break;
	}
	case UNARY_ALPHABET_0:
	{
		uint64_t bit = 1;
		(*val) = -1;
		while ((bit) && ((*val) < MAX_UNARY))
		{
			bitunpacker_read(unpacker, &bit, 1);
			(*val)++;
		}
		if ((*val) && (*val != MAX_UNARY))
			bitunpacker_read(unpacker, &bit, 1);
		if (bit)
			(*val) = -(*val);
		break;
	}
	default:
		assert(!"invalid alphabet specified");
		return -1;
	}

	return (int)(bitunpacker_consumed_bits(unpacker) - nbits_start);
}

int bitunpacker_read_unary_unsigned(bit_unpacker_t* unpacker, int8_t* val)
{
	uint64_t bit = 1;
	(*val) = -1;
	while (bit)
	{
		bitunpacker_read(unpacker, &bit, 1);
		++*val;
	}
	return *val + 1;
}

int bitunpacker_align(bit_unpacker_t* unpacker, int nbits)
{
	uint64_t a;
	if (unpacker->bit_offset % nbits)
	{
		return bitunpacker_read(unpacker, &a, (nbits - (unpacker->bit_offset % nbits)));
	}
	return 0;
}

int bitunpacker_skip(bit_unpacker_t* packer, int nbits)
{
	int i;
	uint64_t val;
	assert(nbits >= 0);
	for (i = 0; i < nbits; i += 64)
	{
		int burst = MIN(64, nbits - i);
		bitunpacker_read(packer, &val, burst);
	}
	return nbits;
}

int bitunpacker_rewind(bit_unpacker_t* unpacker, int nbits)
{
	int i;
	int len0 = (unpacker->bit_offset >= nbits) ? nbits : unpacker->bit_offset;
	int len1 = nbits - len0;

	if (len0)
	{
		unpacker->bit_offset -= len0;
	}
	for (i = 0; i < len1; i += MAXB)
	{
		int burst = MIN(MAXB, len1 - i);
		unpacker->consumed -= (uint64_t)MAXB;
		unpacker->ptr_cur--;
		unpacker->cur = PACK_WORD(*unpacker->ptr_cur);
		unpacker->bit_offset = MAXB;
		unpacker->bit_offset -= burst;
	}
	return 0;
}
