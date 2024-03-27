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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "libjxs.h"
#include "version.h"
#include "bitpacking.h"
#include "xs_config.h"
#include "xs_markers.h"

#define XS_MARKER_SOC 0xff10
#define XS_MARKER_EOC 0xff11
#define XS_MARKER_PIH 0xff12
#define XS_MARKER_CDT 0xff13
#define XS_MARKER_WGT 0xff14
#define XS_MARKER_COM 0xff15
#define XS_MARKER_NLT 0xff16
#define XS_MARKER_CWD 0xff17
#define XS_MARKER_CTS 0xff18
#define XS_MARKER_CRG 0xff19
#define XS_MARKER_SLH 0xff20
#define XS_MARKER_CAP 0xff50

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

typedef struct xs_crg_values_t xs_crg_values_t;
struct xs_crg_values_t
{
	xs_cfa_pattern_t cfa_pattern;
	uint16_t Xcrg[4];
	uint16_t Ycrg[4];
};

const xs_crg_values_t XS_CRG_VALUES[] = {
	{ XS_CFA_RGGB, { 0, 32768, 0, 32768 }, { 0, 0, 32768, 32768 } },
	{ XS_CFA_BGGR, { 32768, 32768, 0, 0 }, { 32768, 0, 32768, 0 } },
	{ XS_CFA_GRBG, { 32768, 0, 32768, 0 }, { 0, 0, 32768, 32768 } },
	{ XS_CFA_GBRG, { 0, 0, 32768, 32768 }, { 32768, 0, 32768, 0 } },
};
#define XS_CRG_VALUES_COUNT (sizeof(XS_CRG_VALUES) / sizeof(xs_crg_values_t))

int xs_write_picture_header(bit_packer_t* bitstream, xs_image_t* im, const xs_config_t* cfg)
{
	int nbits = 0;
	const int lpih = 26;

	assert(cfg->profile != XS_PROFILE_AUTO);
	assert(cfg->level != XS_LEVEL_AUTO);
	assert(cfg->sublevel != XS_SUBLEVEL_AUTO);

	nbits += bitpacker_write(bitstream, XS_MARKER_PIH, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, lpih, XS_MARKER_NBITS);
	if (cfg->bitstream_size_in_bytes == (size_t)-1)
	{
		// Write zeroes as we do not (yet) know the final codestream size.
		nbits += bitpacker_write(bitstream, 0, 32);
	}
	else
	{
		assert(cfg->bitstream_size_in_bytes != 0 && cfg->bitstream_size_in_bytes != (size_t)-1);
		nbits += bitpacker_write(bitstream, cfg->bitstream_size_in_bytes, 32);
	}
	nbits += bitpacker_write(bitstream, cfg->profile, 16);  // Ppih
	nbits += bitpacker_write(bitstream, (((uint16_t)cfg->level) << 8) | ((uint16_t)cfg->sublevel), 16);  // Plev
	nbits += bitpacker_write(bitstream, im->width, 16);
	nbits += bitpacker_write(bitstream, im->height, 16);
	nbits += bitpacker_write(bitstream, cfg->p.Cw, 16);
	nbits += bitpacker_write(bitstream, cfg->p.slice_height / (1 << (cfg->p.NLy)), 16);
	nbits += bitpacker_write(bitstream, im->ncomps, 8);
	nbits += bitpacker_write(bitstream, cfg->p.N_g, 8);
	nbits += bitpacker_write(bitstream, cfg->p.S_s, 8);
	nbits += bitpacker_write(bitstream, cfg->p.Bw, 8);
	nbits += bitpacker_write(bitstream, cfg->p.Fq, 4);
	nbits += bitpacker_write(bitstream, cfg->p.B_r, 4);
	nbits += bitpacker_write(bitstream, cfg->p.Fslc, 1);
	nbits += bitpacker_write(bitstream, cfg->p.Ppoc, 3);
	nbits += bitpacker_write(bitstream, ((uint8_t)cfg->p.color_transform & 0xf), 4);
	nbits += bitpacker_write(bitstream, cfg->p.NLx, 4);
	nbits += bitpacker_write(bitstream, cfg->p.NLy, 4);
	nbits += bitpacker_write(bitstream, cfg->p.Lh, 1);
	nbits += bitpacker_write(bitstream, cfg->p.Rl, 1);
	nbits += bitpacker_write(bitstream, cfg->p.Qpih, 2);
	nbits += bitpacker_write(bitstream, cfg->p.Fs, 2);
	nbits += bitpacker_write(bitstream, cfg->p.Rm, 2);
	return nbits;
}

bool xs_parse_picture_header(bit_unpacker_t* bitstream, xs_image_t* im, xs_config_t* cfg)
{
	const int Lpih = 26;
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_PIH)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != Lpih)
	{
		return false;
	}

	// Lcod
	bitunpacker_read(bitstream, &val, 32);
	cfg->bitstream_size_in_bytes = (uint32_t)val;

	// Ppih
	bitunpacker_read(bitstream, &val, 16);
	cfg->profile = (xs_profile_t)val;

	// Plev
	bitunpacker_read(bitstream, &val, 16);
	cfg->level = (xs_level_t)((val >> 8) & 0xff);
	cfg->sublevel = (xs_level_t)(val & 0xff);

	bitunpacker_read(bitstream, &val, 16);
	im->width = (uint16_t)val;
	bitunpacker_read(bitstream, &val, 16);
	im->height = (uint16_t)val;

	// Cw
	bitunpacker_read(bitstream, &val, 16);
	cfg->p.Cw = (uint16_t)val;

	// H_sl
	bitunpacker_read(bitstream, &val, 16);
	cfg->p.slice_height = (uint16_t)val;

	bitunpacker_read(bitstream, &val, 8);
	im->ncomps = (uint8_t)val;
	if (im->ncomps > MAX_NCOMPS)
	{
		return false;
	}

	// N_g and S_s
	bitunpacker_read(bitstream, &val, 8);
	cfg->p.N_g = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 8);
	cfg->p.S_s = (uint8_t)val;

	// Bw and Fq
	bitunpacker_read(bitstream, &val, 8);
	cfg->p.Bw = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.Fq = (uint8_t)val;

	// B_r, Fslc, and Ppoc
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.B_r = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 1);
	cfg->p.Fslc = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 3);
	cfg->p.Ppoc = (uint8_t)val;

	// Cpih
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.color_transform = (xs_cpih_t)val;

	// NLx and NLy
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.NLx = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.NLy = (uint8_t)val;
	cfg->p.slice_height *= (1 << cfg->p.NLy);

	// Lh and Rl
	bitunpacker_read(bitstream, &val, 1);
	cfg->p.Lh = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 1);
	cfg->p.Rl = (uint8_t)val;

	// Qpih
	bitunpacker_read(bitstream, &val, 2);
	cfg->p.Qpih = (uint8_t)val;

	// Fs and Rm
	bitunpacker_read(bitstream, &val, 2);
	cfg->p.Fs = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 2);
	cfg->p.Rm = (uint8_t)val;

	return true;
}

int xs_write_component_table(bit_packer_t* bitstream, xs_image_t* im)
{
	int nbits = 0;
	int lcdt = 2 * im->ncomps + 2;
	int comp;

	nbits += bitpacker_write(bitstream, XS_MARKER_CDT, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, lcdt, XS_MARKER_NBITS);

	for (comp = 0; comp < im->ncomps; comp++)
	{
		nbits += bitpacker_write(bitstream, im->depth, 8);
		nbits += bitpacker_write(bitstream, im->sx[comp], 4);
		nbits += bitpacker_write(bitstream, im->sy[comp], 4);
	}
	return nbits;
}

int xs_parse_component_table(bit_unpacker_t* bitstream, xs_image_t* im)
{
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_CDT)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 2ull * im->ncomps + 2)
	{
		return false;
	}

	for (int comp = 0; comp < im->ncomps; ++comp)
	{
		bitunpacker_read(bitstream, &val, 8);
		im->depth = (uint8_t)val;

		bitunpacker_read(bitstream, &val, 4);
		if (val != 1 && val != 2)
		{
			return false;
		}
		im->sx[comp] = (int)val;

		bitunpacker_read(bitstream, &val, 4);
		if (val != 1 && val != 2)
		{
			return false;
		}
		im->sy[comp] = (int)val;
	}

	return true;
}

int xs_write_weights_table(bit_packer_t* bitstream, xs_image_t* im, const xs_config_t* cfg)
{
	int nbits = 0;
	int Nl = 0;
	for (; Nl < MAX_NBANDS && cfg->p.lvl_gains[Nl] != 0xff; ++Nl);
	assert(cfg->p.lvl_gains[Nl] == 0xff && cfg->p.lvl_priorities[Nl] == 0xff);

	nbits += bitpacker_write(bitstream, XS_MARKER_WGT, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, 2ull * Nl + 2, XS_MARKER_NBITS);
	for (int lvl = 0; lvl < Nl; ++lvl)
	{
		assert(cfg->p.lvl_gains[lvl] != 0xff && cfg->p.lvl_priorities[lvl] != 0xff);
		nbits += bitpacker_write(bitstream, cfg->p.lvl_gains[lvl], 8);
		nbits += bitpacker_write(bitstream, cfg->p.lvl_priorities[lvl], 8);
	}

	return nbits;
}

bool xs_parse_weights_table(bit_unpacker_t* bitstream, xs_image_t* im, xs_config_t* cfg)
{
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_WGT)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);  // val contains number of bytes
	val -= 2;
	if ((val % 2) != 0)
	{
		return false;
	}

	const int Nl = (int)val / 2;
	for (int lvl = 0; lvl < Nl; ++lvl)
	{
		bitunpacker_read(bitstream, &val, 8);
		cfg->p.lvl_gains[lvl] = (uint8_t)val;
		bitunpacker_read(bitstream, &val, 8);
		cfg->p.lvl_priorities[lvl] = (uint8_t)val;
	}
	cfg->p.lvl_gains[Nl] = 0xff;
	cfg->p.lvl_priorities[Nl] = 0xff;
	cfg->p.lvl_gains[MAX_NBANDS] = 0xff;
	cfg->p.lvl_priorities[MAX_NBANDS] = 0xff;

	return true;
}

bool xs_parse_capabilities_marker(bit_unpacker_t* bitstream, xs_image_t* im, xs_config_t* cfg)
{
	uint64_t val;

	// CAP marker
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_CAP)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val > 4 || val < 2)
	{
		assert(!"invalid or unsupported CAP marker");
		return false;
	}
	val -= 2;

	if (val == 1)
	{
		bitunpacker_read(bitstream, &val, 8);
		val <<= 8;
	}
	else if (val == 2)
	{
		bitunpacker_read(bitstream, &val, 16);
	}
	cfg->cap_bits = (xs_cap_t)val;
	return true;
}

int xs_write_capabilities_marker(bit_packer_t* bitstream, xs_image_t* im, const xs_config_t* cfg)
{
	assert(cfg->cap_bits != XS_CAP_AUTO);

	// CAP marker
	int nbits = 0;
	nbits += bitpacker_write(bitstream, XS_MARKER_CAP, XS_MARKER_NBITS);

	if ((cfg->cap_bits & 0xffff) == 0)
	{
		nbits += bitpacker_write(bitstream, 2, XS_MARKER_NBITS);
	}
	else if ((cfg->cap_bits & 0xff) == 0)
	{
		nbits += bitpacker_write(bitstream, 3, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, cfg->cap_bits >> 8, 8);
	}
	else
	{
		nbits += bitpacker_write(bitstream, 4, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, cfg->cap_bits, 16);
	}
	return nbits;
}

bool xs_parse_com_marker(bit_unpacker_t* bitstream, xs_image_t* im)
{
	uint64_t size, val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_COM)
	{
		return false;
	}
	bitunpacker_read(bitstream, &size, XS_MARKER_NBITS);
	if (size < 4)
	{
		return false;
	}

	while (size > 2)
	{
		bitunpacker_read(bitstream, &val, 8);
		size--;
	}

	return true;
}

int xs_write_com_encoder_identification(bit_packer_t* bitstream)
{
	size_t id_length;
	const char* id = xs_get_verion_id_str(&id_length);

	int nbits = 0;
	nbits += bitpacker_write(bitstream, XS_MARKER_COM, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, id_length + 4, XS_MARKER_NBITS);  // Lcom
	nbits += bitpacker_write(bitstream, 0x0000, XS_MARKER_NBITS);  // Tcom	
	for (size_t i = 0; i < id_length; ++i)
	{
		nbits += bitpacker_write(bitstream, (uint64_t)id[i], 8);
	}
	return nbits;
}

bool xs_parse_nlt_marker(bit_unpacker_t* bitstream, xs_config_t* cfg)
{
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_NLT)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 5 && val != 12)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, 8);
	if (val == 1)
	{
		cfg->p.Tnlt = XS_NLT_QUADRATIC;
		bitunpacker_read(bitstream, &val, 1);
		cfg->p.Tnlt_params.quadratic.sigma = (uint16_t)val;
		bitunpacker_read(bitstream, &val, 15);
		cfg->p.Tnlt_params.quadratic.alpha = (uint16_t)val;
	}
	else if (val == 2)
	{
		cfg->p.Tnlt = XS_NLT_EXTENDED;
		bitunpacker_read(bitstream, &val, 32);
		cfg->p.Tnlt_params.extended.T1 = (uint32_t)val;
		bitunpacker_read(bitstream, &val, 32);
		cfg->p.Tnlt_params.extended.T2 = (uint32_t)val;
		bitunpacker_read(bitstream, &val, 8);
		cfg->p.Tnlt_params.extended.E = (uint8_t)val;
	}
	else
	{
		assert(!"unknown Tnlt");
		return false;
	}

	return true;
}

int xs_write_nlt_marker(bit_packer_t* bitstream, const xs_config_t* cfg)
{
	if (cfg->p.Tnlt == XS_NLT_QUADRATIC)
	{
		int nbits = 0;
		nbits += bitpacker_write(bitstream, XS_MARKER_NLT, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 5, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 1, 8);
		nbits += bitpacker_write(bitstream, cfg->p.Tnlt_params.quadratic.sigma, 1);
		nbits += bitpacker_write(bitstream, cfg->p.Tnlt_params.quadratic.alpha, 15);
		return nbits;
	}
	else if (cfg->p.Tnlt == XS_NLT_EXTENDED)
	{
		int nbits = 0;
		nbits += bitpacker_write(bitstream, XS_MARKER_NLT, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 12, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 2, 8);
		nbits += bitpacker_write(bitstream, cfg->p.Tnlt_params.extended.T1, 32);
		nbits += bitpacker_write(bitstream, cfg->p.Tnlt_params.extended.T2, 32);
		nbits += bitpacker_write(bitstream, cfg->p.Tnlt_params.extended.E, 8);
		return nbits;
	}
	return 0;
}

bool xs_parse_cwd_marker(bit_unpacker_t* bitstream, xs_config_t* cfg)
{
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_CWD)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 3)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, 8);
	cfg->p.Sd = (uint8_t)val;
	return true;
}

int xs_write_cwd_marker(bit_packer_t* bitstream, const xs_config_t* cfg)
{
	if (cfg->p.Sd > 0)
	{
		int nbits = 0;
		nbits += bitpacker_write(bitstream, XS_MARKER_CWD, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 3, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, cfg->p.Sd, 8);
		return nbits;
	}
	return 0;
}

bool xs_parse_cts_marker(bit_unpacker_t* bitstream, xs_config_t* cfg)
{
	uint64_t val;

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_CTS)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 4)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, 4);  // reserved
	bitunpacker_read(bitstream, &val, 4);
	if (val == 0)
	{
		cfg->p.tetrix_params.Cf = XS_TETRIX_FULL;
	}
	else if (val == 3)
	{
		cfg->p.tetrix_params.Cf = XS_TETRIX_INLINE;
	}
	else
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.tetrix_params.e1 = (uint8_t)val;
	bitunpacker_read(bitstream, &val, 4);
	cfg->p.tetrix_params.e2 = (uint8_t)val;

	return true;
}

int xs_write_cts_marker(bit_packer_t* bitstream, const xs_config_t* cfg)
{
	if (cfg->p.color_transform == XS_CPIH_TETRIX)
	{
		int nbits = 0;
		nbits += bitpacker_write(bitstream, XS_MARKER_CTS, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 4, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 0, 4);
		nbits += bitpacker_write(bitstream, cfg->p.tetrix_params.Cf, 4);
		nbits += bitpacker_write(bitstream, cfg->p.tetrix_params.e1, 4);
		nbits += bitpacker_write(bitstream, cfg->p.tetrix_params.e2, 4);
		return nbits;

	}
	return 0;
}

bool xs_parse_crg_marker(bit_unpacker_t* bitstream, const xs_image_t* im, xs_config_t* cfg)
{
	uint64_t val;
	uint16_t Xcrg[MAX_NCOMPS];
	uint16_t Ycrg[MAX_NCOMPS];

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_CRG)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 4ull * im->ncomps + 2)
	{
		return false;
	}
	if (im->ncomps != 4)
	{
		return false;
	}
	for (int c = 0; c < 4; ++c)
	{
		bitunpacker_read(bitstream, &val, 16);
		Xcrg[c] = (uint16_t)val;
		bitunpacker_read(bitstream, &val, 16);
		Ycrg[c] = (uint16_t)val;
	}
	const xs_crg_values_t* crg_values = NULL;
	for (int i = 0; i < XS_CRG_VALUES_COUNT; ++i)
	{
		const xs_crg_values_t* t = &XS_CRG_VALUES[i];
		if (t->Xcrg[0] == Xcrg[0] && t->Xcrg[1] == Xcrg[1] && t->Xcrg[2] == Xcrg[2] && t->Xcrg[3] == Xcrg[3] &&
			t->Ycrg[0] == Ycrg[0] && t->Ycrg[1] == Ycrg[1] && t->Ycrg[2] == Ycrg[2] && t->Ycrg[3] == Ycrg[3])
		{
			crg_values = t;
			break;
		}
	}
	if (crg_values == NULL)
	{
		assert(!"unknown CRG values");
		return false;
	}
	cfg->p.cfa_pattern = crg_values->cfa_pattern;
	return true;
}

int xs_write_crg_marker(bit_packer_t* bitstream, const xs_image_t* im, const xs_config_t* cfg)
{
	if (cfg->p.color_transform == XS_CPIH_TETRIX)
	{
		assert(im->ncomps == 4);
		int nbits = 0;
		nbits += bitpacker_write(bitstream, XS_MARKER_CRG, XS_MARKER_NBITS);
		nbits += bitpacker_write(bitstream, 4ull * im->ncomps + 2, XS_MARKER_NBITS);
		const xs_crg_values_t* crg_values = NULL;
		for (int i = 0; i < XS_CRG_VALUES_COUNT; ++i)
		{
			if (XS_CRG_VALUES[i].cfa_pattern == cfg->p.cfa_pattern)
			{
				crg_values = &XS_CRG_VALUES[i];
				break;
			}
		}
		assert(crg_values != NULL);
		for (int c = 0; c < 4; ++c)
		{
			nbits += bitpacker_write(bitstream, crg_values->Xcrg[c], 16);
			nbits += bitpacker_write(bitstream, crg_values->Ycrg[c], 16);
		}
		return nbits;
	}
	return 0;
}

bool xs_parse_head(bit_unpacker_t* bitstream, xs_image_t* im, xs_config_t* cfg)
{
	uint64_t val;

	assert(cfg == NULL && im == NULL || cfg != NULL && im != NULL);

	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_SOC)
	{
		assert(!"unexpected marker found");
		return false;
	}

	if (cfg != NULL)
	{
		xs_config_init(cfg, im, XS_PROFILE_UNRESTRICTED, XS_LEVEL_UNRESTRICTED, XS_SUBLEVEL_UNRESTRICTED);
	}

	do {
		if (bitunpacker_peek(bitstream, &val, 16) != 16)
		{
			assert(!"unexpected end of stream");
			return false;
		}

		if (cfg == NULL)
		{
			// Just skip headers until first SLH is found.
			if (val == XS_MARKER_SLH)
			{
				return true;
			}
			bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
			bitunpacker_peek(bitstream, &val, XS_MARKER_NBITS);
			bitunpacker_skip(bitstream, (val & 0xffff) * 8);
			continue;
		}
		
		bool ok = false;
		switch (val)
		{
		case XS_MARKER_CAP:
			ok |= xs_parse_capabilities_marker(bitstream, im, cfg);
			break;
		case XS_MARKER_PIH:
			ok |= xs_parse_picture_header(bitstream, im, cfg);
			break;
		case XS_MARKER_CDT:
			ok |= xs_parse_component_table(bitstream, im);
			break;
		case XS_MARKER_WGT:
			ok |= xs_parse_weights_table(bitstream, im, cfg);
			break;
		case XS_MARKER_COM:
			ok |= xs_parse_com_marker(bitstream, im);
			break;
		case XS_MARKER_SLH:
			// Here is the normal exit of the function.
			return true;
		case XS_MARKER_NLT:
			ok |= xs_parse_nlt_marker(bitstream, cfg);
			break;
		case XS_MARKER_CWD:
			ok |= xs_parse_cwd_marker(bitstream, cfg);
			break;
		case XS_MARKER_CTS:
			ok |= xs_parse_cts_marker(bitstream, cfg);
			break;
		case XS_MARKER_CRG:
			ok |= xs_parse_crg_marker(bitstream, im, cfg);
			break;
		default:
			assert(!"unexpected marker found");
			return false;
		}
		if (!ok)
		{
			assert(!"error while parsing XS header");
			return false;
		}
	} while (1);
}

int xs_write_head(bit_packer_t* bitstream, xs_image_t* im, const xs_config_t* cfg)
{
	assert(cfg != NULL && im != NULL);
	int nbits = 0;
	nbits += bitpacker_write(bitstream, XS_MARKER_SOC, XS_MARKER_NBITS);
	nbits += xs_write_capabilities_marker(bitstream, im, cfg);
	nbits += xs_write_picture_header(bitstream, im, cfg);
	nbits += xs_write_component_table(bitstream, im);
	nbits += xs_write_weights_table(bitstream, im, cfg);
	nbits += xs_write_nlt_marker(bitstream, cfg);
	nbits += xs_write_cwd_marker(bitstream, cfg);
	nbits += xs_write_cts_marker(bitstream, cfg);
	nbits += xs_write_crg_marker(bitstream, im, cfg);
	nbits += xs_write_com_encoder_identification(bitstream);
	return nbits;
}

int xs_write_tail(bit_packer_t* bitstream)
{
	return bitpacker_write(bitstream, XS_MARKER_EOC, XS_MARKER_NBITS);
}

bool xs_parse_tail(bit_unpacker_t* bitstream)
{
	uint64_t val;
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	return val == XS_MARKER_EOC;
}

int xs_write_slice_header(bit_packer_t* bitstream, int slice_idx)
{
	int nbits = 0;
	nbits += bitpacker_write(bitstream, XS_MARKER_SLH, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, 4, XS_MARKER_NBITS);
	nbits += bitpacker_write(bitstream, slice_idx, 16);
	assert(nbits == XS_SLICE_HEADER_NBYTES * 8);
	return nbits;
}

bool xs_parse_slice_header(bit_unpacker_t* bitstream, int* slice_idx)
{
	uint64_t val;
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != XS_MARKER_SLH)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, XS_MARKER_NBITS);
	if (val != 4)
	{
		return false;
	}
	bitunpacker_read(bitstream, &val, 16);
	*slice_idx = (int)val;
	return true;
}
