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

#ifndef GCLI_METHODS
#define GCLI_METHODS

#include <stdint.h>
#include "libjxs.h"
#include "bitpacking.h"
#include "predbuffer.h"

#define ALPHABET_NBITS 1
enum {
	ALPHABET_RAW_4BITS,
	ALPHABET_UNARY_UNSIGNED_BOUNDED,
	ALPHABET_COUNT,
};

#define PRED_NBITS 1
enum {
	PRED_NONE,
	PRED_VER,
	PRED_COUNT,
};

#define RUN_NBITS 2
enum {
	RUN_NONE,
	RUN_SIGFLAGS_ZRF,
	RUN_SIGFLAGS_ZRCSF,
	RUN_COUNT,
};

#define GCLI_METHODS_NB (1 << (ALPHABET_NBITS + PRED_NBITS + RUN_NBITS))

#define METHOD_ENABLE_MASK_PREDICTIONS_OFFSET 0
#define METHOD_ENABLE_MASK_ALPHABETS_OFFSET 2
#define METHOD_ENABLE_MASK_RUNS_OFFSET 4

#define PRED_OFFSET 0
#define ALPHABET_OFFSET (PRED_OFFSET + PRED_NBITS)
#define RUN_OFFSET (ALPHABET_OFFSET + ALPHABET_NBITS)

#define method_get_alphabet(gcli_method) (((gcli_method >> ALPHABET_OFFSET) & ((1<<ALPHABET_NBITS) - 1)))
#define method_get_pred(gcli_method) ((gcli_method >> PRED_OFFSET) & ((1<<PRED_NBITS) - 1))
#define method_get_run(gcli_method) ((gcli_method >> RUN_OFFSET) & ((1<<RUN_NBITS) - 1))
#define method_get_idx(alphabet, pred, run) ((alphabet << ALPHABET_OFFSET) | (pred << PRED_OFFSET) | (run << RUN_OFFSET))

#define method_uses_ver_pred(gcli_method) (method_get_pred(gcli_method) == PRED_VER)
#define method_uses_no_pred(gcli_method) (method_get_pred(gcli_method) == PRED_NONE)
#define method_prediction_type(gcli_method) (method_get_pred(gcli_method))
#define method_is_raw(gcli_method) (method_get_alphabet(gcli_method) == ALPHABET_RAW_4BITS)
#define method_uses_sig_flags(gcli_method) ((method_get_run(gcli_method) == RUN_SIGFLAGS_ZRF) || (method_get_run(gcli_method) == RUN_SIGFLAGS_ZRCSF))

int gcli_method_get_enabled(const xs_config_t* xs_config);
int gcli_method_get_enabled_ver(int enabled);
int gcli_method_get_enabled_nover(int enabled);

enum
{
	PRECINCT_ALL = 0,
	PRECINCT_FIRST_OF_SLICE = 1,
	PRECINCT_OTHERS = 2,
};

int gcli_method_is_enabled(uint32_t enabled, int gcli_method, int precinct_group);

int gcli_method_get_signaling(int gcli_method, uint32_t enabled_methods);
int gcli_method_from_signaling(int signaling, uint32_t enabled_methods);

#endif
