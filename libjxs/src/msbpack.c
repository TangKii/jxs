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
#include <assert.h>

#include "tco.h"
#include "common.h"
#include "msbpack.h"

/*
	Configuration API
*/
int msbp_config_value = TCO_CONF_DEFAULT_MSBP_OPT;

int msbp_get_config_value(int sign_packing)
{
	return sign_packing ? msbp_config_value : 0;
}


int tco_msbp_set_config_value(int value)
{
	return msbp_config_value = MAX(0, value);
}


int msbp_enabled(int sign_packing)
{
	return msbp_get_config_value(sign_packing) != 0;
}


int msbp_test_range(int gcli, int gtli, int sign_packing)
{
	const int nbits = gcli - gtli;
	const int mbits = msbp_get_config_value(sign_packing) == 1 ? MAX_GCLI : msbp_get_config_value(sign_packing);
	return (mbits > 0) && (nbits > 1) && (nbits <= mbits);
}

/*
	Budgeting code
*/

#ifdef _DEBUG

#include <stdio.h>

int update_cost_of_msb_nibble_optimised(
	const sig_mag_data_t* datas_buf, const int data_len, const int group_size, const int group,
	const int gcli, const int gtli, const int dq_type
);

int update_cost_of_msb_nibble(
	const sig_mag_data_t* datas_buf, const int data_len, const int group_size, const int group,
	const int gcli, const int gtli, const int dq_type
)
{
	uint8_t nibble = 0;
	const int idx = group * group_size;
	const int bp = gcli - 1;
	int i;
	int comp = update_cost_of_msb_nibble_optimised(datas_buf, data_len, group_size, group, gcli, gtli, dq_type);

	// Form a nibble from the MSB-plane of the coefficient group, padding with zeros
	for (i = 0; (i < group_size) && (idx + i < data_len); i++)
	{
		int mag = datas_buf[idx + i] & ~SIGN_BIT_MASK;
		int qnt = (dq_type)?(uniform_dq(mag,gcli,gtli)):(deadzone_dq(mag,gtli));
		nibble |= (((qnt << gtli) >> bp) & 0x1) << (group_size - i - 1);
	}

	if (MSBP_IS_SHORT_CODE(nibble))
	{
		if (comp != -1)	// There is a single 1 in the MSB-plane so use a 3+1 bit code (that includes the sign bit)
		{
			fprintf(stderr, "MSBP optimisation failed for L3 code [%d, %d, %d, %d]\n",
				datas_buf[idx + 0] & ~SIGN_BIT_MASK,
				datas_buf[idx + 1] & ~SIGN_BIT_MASK,
				datas_buf[idx + 2] & ~SIGN_BIT_MASK,
				datas_buf[idx + 3] & ~SIGN_BIT_MASK);
			assert(FALSE);
		}
	}
	else if (MSBP_IS_ROT0(nibble) || MSBP_IS_ROT1(nibble))
	{
		if (comp != 1)
		{
			fprintf(stderr, "MSBP optimisation failed for L5 code [%d, %d, %d, %d]\n",
				datas_buf[idx + 0] & ~SIGN_BIT_MASK,
				datas_buf[idx + 1] & ~SIGN_BIT_MASK,
				datas_buf[idx + 2] & ~SIGN_BIT_MASK,
				datas_buf[idx + 3] & ~SIGN_BIT_MASK);
			assert(FALSE);
		}
	}
	else if (comp != 0)
	{
		fprintf(stderr, "MSBP optimisation failed for L4 code [%d, %d, %d, %d]\n",
			datas_buf[idx + 0] & ~SIGN_BIT_MASK,
			datas_buf[idx + 1] & ~SIGN_BIT_MASK,
			datas_buf[idx + 2] & ~SIGN_BIT_MASK,
			datas_buf[idx + 3] & ~SIGN_BIT_MASK);
		assert(FALSE);
	}

	if (MSBP_IS_SHORT_CODE(nibble))
		return -1;	// There is a single 1 in the MSB-plane so use a 3+1 bit code (that includes the sign bit)
	else if (MSBP_IS_ROT0(nibble) || MSBP_IS_ROT1(nibble))
		return +1;	// In a few cases a 5th code bit is required to specify a rotation of the MSB-plane
	return 0;
}

int update_cost_of_msb_nibble_optimised(
	const sig_mag_data_t* datas_buf, const int data_len, const int group_size, const int group,
	const int gcli, const int gtli, const int dq_type
)
#else
int update_cost_of_msb_nibble(
	const sig_mag_data_t* datas_buf, const int data_len, const int group_size, const int group,
	const int gcli, const int gtli, const int dq_type
)
#endif
{
	uint8_t nibble = 0;
	const int idx = group * group_size;
	const int bp = gcli - 1;
	int i;
	const uint16_t threshold_table[] = {
		0x6db6,		// 0b110110110110110
		0x7777,		// 0b111011101110111
		0x7bde,		// 0b111101111011110
		0x7df7,		// 0b111110111110111
		0x7efd,		// 0b111111011111101
		0x7f7f,		// 0b111111101111111
		0x7fbf,		// 0b111111110111111
		0x7fdf,		// 0b111111111011111
		0x7fef,		// 0b111111111101111
		0x7ff7,		// 0b111111111110111
		0x7ffb,		// 0b111111111111011
		0x7ffd		// 0b111111111111101
	};

	// Form a nibble from the quantised MSB-plane of the coefficient group, padding with zeros
	const uint16_t threshold = threshold_table[gcli - gtli - 2] >> (MAX_GCLI - gcli + 1);
	for (i = 0; (i < group_size) && (idx + i < data_len); i++)
	{
		int bit;
		if (dq_type != 0 && (gtli > 0))
		{
			int magnitude = datas_buf[idx + i] & ~SIGN_BIT_MASK;
			bit = (magnitude > threshold ? 1 : 0);
		}
		else
			bit = (datas_buf[idx + i] >> bp) & 0x1;
		nibble |= bit << (group_size - i - 1);
	}

	if (MSBP_IS_SHORT_CODE(nibble))
		return -1;	// There is a single 1 in the MSB-plane so use a 3+1 bit code (that includes the sign bit)
	else if (MSBP_IS_ROT0(nibble) || MSBP_IS_ROT1(nibble))
		return +1;	// In a few cases a 5th code bit is required to specify a rotation of the MSB-plane
	return 0;
}


int update_data_bgt_msbp_packing(
	const sig_mag_data_t* datas_buf,
	int data_len,
	const gcli_data_t* gclis_buf,
	int gclis_len,
	uint32_t* budget_tables,
	uint32_t* rot_budget_tables,
	int group_size,
	int dq_type)
{
	int group = 0, gcli = 0, gtli = 0;

	for (group = 0; group < gclis_len; group++)
	{
		gcli = *gclis_buf++;

		for (gtli = 0; gtli < MAX_GCLI; gtli++)
		{
			if (msbp_test_range(gcli, gtli, TRUE))
			{
				int update = update_cost_of_msb_nibble(datas_buf, data_len, group_size, group, gcli, gtli, dq_type);
				if (update > 0)
					rot_budget_tables[gtli] += update;
				else
					budget_tables[gtli] += update;
			}
		}
	}
	return 0;
}


/*
	Bitstream packing code
*/

uint32_t msbcodes[] = { 0, 6, 4, 10, 2, 14, 9, 12, 0, 11, 14, 13, 8, 13, 12, 15 };

uint32_t msbp_code(int nibble)
{
	assert(nibble > 0 && nibble <= 15);
	return msbcodes[nibble];
}

int msbp_decode_1(int nibble, sig_mag_data_t* buf, int buf_len, int idx, int bp)
{
	if ((nibble & 0x8) == 0)	// 0XXX
	{
		int sgn = (nibble & 0x1);
		int	pos = (nibble >> 1) & 0x3;
		MSBP_SAFE_OREQ(buf, idx + pos, buf_len, (sig_mag_data_t)0x1 << bp);
		MSBP_SAFE_OREQ(buf, idx + pos, buf_len, (sig_mag_data_t)sgn << SIGN_BIT_POSITION);
		return pos & 0x4;	// indicate a sign bit consumed and return its pos
	}
	else					// 1XXX
	{
		switch (nibble & 0x7)
		{
		case 0:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 1, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0;
		case 1:
			MSBP_SAFE_OREQ(buf, idx + 1, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0;
		case 2:
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 3, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0;
		case 3:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 3, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0;
		case 4:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 1, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0x8;		// indicate that a rotation bit is pending
		case 5:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 3, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0x8;		// indicate that a rotation bit is pending
		case 6:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0x8;		// indicate that a rotation bit is pending
		case 7:
			MSBP_SAFE_OREQ(buf, idx + 0, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 1, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 2, buf_len, (sig_mag_data_t)0x1 << bp);
			MSBP_SAFE_OREQ(buf, idx + 3, buf_len, (sig_mag_data_t)0x1 << bp);
			return 0;
		};
	}
	assert(FALSE);
	return -1;	// Should never happen but keeps compiler happy
}

//
// The following implementation of msbp_decode, though not as optimised, is more succinct
// and matches the description in the draft text submitted with this code
//
#define BIT(v, i)	(((v) & (1<<(3-i))) == 0 ? 0 : 1)
#define UMOD(x, n)	((x)<0 ? ((n)+((x)%(n))) : (x)%(n))

int msbp_decode_cd(int nibble, sig_mag_data_t* buf, int buf_len, int idx, int bp)
{
	uint32_t A[][4] = { { 1,0,0,0 },{ 1,1,0,0 },{ 1,1,1,0 },{ 1,0,1,0 },{ 1,1,1,1 } };
	int i, n, rot, fsgn=0, frot=0;

	// Decode pattern archetype
	for (i = 0, n = 4; i < 4; i++)
	{
		if (n == 4 && BIT(nibble, i) == 0)
			n = i;
	}
	// Decode available rotation bits
	switch (n)
	{
	case 0:
		rot = (BIT(nibble, 1) << 1) + BIT(nibble, 2);
		fsgn = rot;
		MSBP_SAFE_OREQ(buf, idx + rot, buf_len, (sig_mag_data_t)BIT(nibble, 3) << SIGN_BIT_POSITION);
		break;
	case 1:
		rot = (BIT(nibble, 2) << 1) + BIT(nibble, 3);
		break;
	case 2:
		rot = (BIT(nibble, 3) << 1);
		frot = 1;
		break;
	case 3:
		rot = 0;
		frot = 1;
	default:
		rot = 0;
	}
	// Write rotated archetype to MSB-plane
	for (i = 0; i < 4; i++)
	{
		int j = UMOD(i - rot, 4);
		MSBP_SAFE_OREQ(buf, idx + i, buf_len, (sig_mag_data_t)A[n][j] << bp);
	}

	return (fsgn << 1) + frot;
}

int msbp_decode(int nibble, sig_mag_data_t* buf, int buf_len, int idx, int bp)
{
	// use the version that matches the CD for proof of correctness.
	return msbp_decode_cd(nibble, buf, buf_len, idx, bp);
}

static void rotr_bitplane(sig_mag_data_t* buf, int buf_len, int group_size, int bp)
{
	int i;
	sig_mag_data_t mask = 0x1 << bp;
	sig_mag_data_t last = (group_size - 1 < buf_len ? buf[group_size - 1] : 0);		// zero padding

	for (i = MIN(buf_len - 1, group_size - 1); i>0; i--)
		if ((buf[i] & mask) != (buf[i - 1] & mask))
			buf[i] ^= mask;
	if ((group_size > 1) && (buf[0] & mask) != (last & mask))
		buf[0] ^= mask;
}

int msbp_unpack_rot(bit_unpacker_t* bitstream, sig_mag_data_t* buf, int buf_len, gcli_data_t *gclis, int group_size, int gtli)
{
	uint64_t val;
	int i = 0, idx = 0, group = 0;

	for (group = 0; group<(buf_len + group_size - 1) / group_size; group++, idx += group_size)
	{
		if (msbp_test_range(gclis[group], gtli, TRUE))
		{
			uint8_t nibble = 0;
			int     bp = gclis[group] - 1;

			// Form a nibble from the MSB-plane of the coefficient group, padding with zeros
			for (i = 0; (i < group_size) && (idx + i < buf_len); i++)
				nibble |= (((buf[idx + i] >> bp) & 0x1) << (group_size - i - 1));

			if (MSBP_IS_ROT0(nibble))	// for 1010, 1110, 1011 a rotation flag is stored to the sign bistream
			{
				bitunpacker_read(bitstream, &val, 1);
				if (val & 0x1)
					rotr_bitplane(buf + idx, buf_len - (group * group_size), group_size, bp);
			}
		}
	}
	return 0;
}
