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

#ifndef BITPACKING_H
#define BITPACKING_H

#include <stdbool.h>
#include <stdint.h>

typedef enum unary_alphabet_e
{
	UNARY_ALPHABET_0 = 0,
	UNARY_ALPHABET_4_CLIPPED = 1,
	UNARY_ALPHABET_FULL = 2,
	UNARY_ALPHABET_NB
} unary_alphabet_t;

#define FIRST_ALPHABET UNARY_ALPHABET_0

#define MAX_UNARY 15
#define MAX_UNARY_CLIPPED 13

typedef struct bit_packer_t bit_packer_t;
typedef struct bit_unpacker_t bit_unpacker_t;

bit_packer_t* bitpacker_init();
int bitpacker_set_buffer(bit_packer_t* packer, void* ptr, size_t max_size);
uint64_t* bitpacker_get_buffer(bit_packer_t* packer);
int bitpacker_write(bit_packer_t* packer, uint64_t val, unsigned int nbits);
int bitpacker_write_unary_signed(bit_packer_t* packer, int8_t val, unary_alphabet_t alphabet);
int bitpacker_write_unary_unsigned(bit_packer_t* packer, int8_t val);

int bounded_code_get_min_max(int predictor, int gtli, int* min_allowed, int* max_allowed);
int bounded_code_get_unary_code(int8_t delta_val, int8_t min_allowed, int8_t max_allowed);

int bitunpacker_read_bounded_code(bit_unpacker_t* unpacker,
	int8_t min_allowed,
	int8_t max_allowed,
	int8_t* val);

int bitpacker_align(bit_packer_t* packer, int nbits);
int bitpacker_get_len(bit_packer_t* packer);
int bitpacker_add_padding(bit_packer_t* packer, int nbits);
int bitpacker_flush(bit_packer_t* packer);
int bitpacker_reset(bit_packer_t* packer);
int bitpacker_append(bit_packer_t* packer, void* ptr_from, int nbits);
int bitpacker_from_unpacker(bit_packer_t* packer, bit_unpacker_t* unpacker, int nbits);
void bitpacker_close(bit_packer_t* packer);


bit_unpacker_t* bitunpacker_init();
int bitunpacker_set_buffer(bit_unpacker_t* unpacker, void* ptr, size_t max_size);
int bitunpacker_read(bit_unpacker_t* unpacker, uint64_t* val, int nbits);
int bitunpacker_peek(bit_unpacker_t* unpacker, uint64_t* val, int nbits);
int bitunpacker_read_unary_signed(bit_unpacker_t* unpacker, int8_t* val, unary_alphabet_t alphabet);
int bitunpacker_read_unary_unsigned(bit_unpacker_t* unpacker, int8_t* val);
int bitunpacker_align(bit_unpacker_t* unpacker, int nbits);
int bitunpacker_reset(bit_unpacker_t* unpacker);
int bitunpacker_close(bit_unpacker_t* unpacker);
size_t bitunpacker_consumed(bit_unpacker_t* unpacker);
bool bitunpacker_consumed_all(bit_unpacker_t* unpacker);
size_t bitunpacker_consumed_bits(bit_unpacker_t* unpacker);
int bitunpacker_skip(bit_unpacker_t* packer, int nbits);
int bitunpacker_rewind(bit_unpacker_t* unpacker, int nbits);

#endif
