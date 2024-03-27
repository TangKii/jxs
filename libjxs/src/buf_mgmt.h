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

#ifndef BUF_MGMT_H
#define BUF_MGMT_H

#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"

enum {
	DUMP_MODE_NONE,
	DUMP_MODE_TXT,
	DUMP_MODE_BIN,
};

#define DEFAULT_DUMP_MODE DUMP_MODE_NONE

typedef struct multi_buf_t
{
	int magic;
	int n_buffers;
	int* sizes;
	int* sizes_byte;
	size_t element_size;
	union
	{
		int8_t** s8;
		uint8_t** u8;
		int16_t** s16;
		uint16_t** u16;
		int32_t** s32;
		uint32_t** u32;
	} bufs;
	FILE** fout;
	int dump_mode;

	void* storage;
} multi_buf_t;

multi_buf_t* multi_buf_create(int n_buffers, size_t element_size);
multi_buf_t* multi_buf_create_and_allocate(int n_buffers, int size, size_t element_size);
int multi_buf_enable_dump(multi_buf_t* mb, int binary);
int multi_buf_set_size(multi_buf_t* mb, int buf_idx, int size);
int multi_buf_allocate(multi_buf_t* mb, int alignment);
int multi_buf_reset(multi_buf_t* mb);
int multi_buf_destroy(multi_buf_t* mb);

enum
{
	MB_DUMP_U16,
	MB_DUMP_S16,
	MB_DUMP_HEX_16,
	MB_DUMP_HEX_32,
	MB_DUMP_S32,
	MB_DUMP_U32,
};

static INLINE int idx2d(int d1, int d2, int len2)
{
	return d1 * len2 + d2;
}

static INLINE int idx3d(int d1, int d2, int d3, int len2, int len3)
{
	return d1 * len2 * len3 + d2 * len3 + d3;
}

static INLINE int idx4d(int d1, int d2, int d3, int d4, int len2, int len3, int len4)
{
	return d1 * len2 * len3 * len4 + d2 * len3 * len4 + d3 * len4 + d4;
}

#endif
