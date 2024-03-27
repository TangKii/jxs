/***************************************************************************
** Canon Inc. (hereinafter the "Software Copyright Holder") hold or have  **
** the right to license copyright with respect to the accompanying        **
** software (hereinafter the "Software").                                 **
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
#ifndef MSBPACK_H
#define MSBPACK_H

#include "sig_mag.h"
#include "gcli.h"
#include "bitpacking.h"


#define MSBP_IS_SHORT_CODE(nibble)		( ((nibble)==0x1) || ((nibble)==0x2) || ((nibble)==0x4) || ((nibble)==0x8) )
#define MSBP_IS_ROT1(nibble)			( ((nibble) == 0x5) || ((nibble) == 0x7) || ((nibble) == 0xD) )
#define MSBP_IS_ROT0(nibble)			( ((nibble) == 0xA) || ((nibble) == 0xE) || ((nibble) == 0xB) )

#define MSBP_SAFE_OREQ(buf, idx, buf_len, val) { if(((idx)<(buf_len)) && ((idx)>=0)) (buf)[idx] |= (val); }

int msbp_get_config_value(int sign_packing);
int msbp_test_range(int gcli, int gtli, int sign_packing);
int msbp_enabled(int sign_packing);

int update_cost_of_msb_nibble(
	const sig_mag_data_t* datas_buf, const int data_len, const int group_size, const int group,
	const int gcli, const int gtli, const int dq_type
);
int update_data_bgt_msbp_packing(
	const sig_mag_data_t* datas_buf,
	int data_len,
	const gcli_data_t* gclis_buf,
	int gclis_len,
	uint32_t* budget_tables,
	uint32_t* rot_budget_tables,
	int group_size,
	int dq_type);

uint32_t msbp_code(int nibble);
int msbp_decode(int nibble, sig_mag_data_t* buf, int buf_len, int idx, int bp);
int msbp_unpack_rot(bit_unpacker_t* bitstream, sig_mag_data_t* buf, int buf_len, gcli_data_t *gclis, int group_size, int gtli);


#endif