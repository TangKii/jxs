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

#include "xs_config.h"
#include "xs_gains.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

typedef struct xs_profile_config_t xs_profile_config_t;
struct xs_profile_config_t
{
	xs_profile_t profile;
	xs_config_parameters_t parameters;

	float budget_report_lines;
	int Nbpp_at_sublev_full;
	int Nsbu;
	int Ssbo;
	uint16_t max_column_width;
};

typedef struct xs_level_values_t xs_level_values_t;
struct xs_level_values_t
{
	xs_level_t level;
	uint16_t max_w;            // W_{max}
	uint16_t max_h;            // H_{max}
	uint32_t max_samples;      // L_{max}
	uint32_t max_sample_rate;  // R_{s,max}
};

typedef struct xs_sublevel_values_t xs_sublevel_values_t;
struct xs_sublevel_values_t
{
	xs_sublevel_t sublevel;
	int max_bpp;
};

const xs_profile_config_t XS_PROFILES[] = {
	//                                                                                                                                                                                           budget_lines
	//                                                  Cw     N_g    Bw         B_r  Ppoc  NLy   Rl    Fs    Sd                                                            e1        CFA pattern    | Nbpp       Ssbo
	//                                                   | slc_h | S_s |      Fq  | Fslc| NLx | Lh  |Qpih | Rm  |                                                  Cf        | e2         |          |   |   Nsbu   |  max_cw
	{ XS_PROFILE_UNRESTRICTED,         { XS_CPIH_AUTO,   0, 16, 4, 8, 0xff, 0xff, 4, 0, 0, 5, 2, 1, 1, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE }, 20, 128,  0,    0,    0 },
	{ XS_PROFILE_LIGHT_422_10,         { XS_CPIH_NONE,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 0, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  0,  20,  4, 1024,    0 },
	{ XS_PROFILE_LIGHT_444_12,         { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 0, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  0,  36,  4, 1024,    0 },
	{ XS_PROFILE_LIGHT_SUBLINE_422_10, { XS_CPIH_NONE,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  0,  20,  2, 1024, 2048 },
	{ XS_PROFILE_MAIN_420_12,          { XS_CPIH_NONE,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  18, 16, 1024,    0 },
	{ XS_PROFILE_MAIN_422_10,          { XS_CPIH_NONE,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  20, 16, 1024,    0 },
	{ XS_PROFILE_MAIN_444_12,          { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  36, 16, 1024,    0 },
	{ XS_PROFILE_MAIN_4444_12,         { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  48, 16, 1024,    0 },
	{ XS_PROFILE_HIGH_420_12,          { XS_CPIH_NONE,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 2, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  18, 16, 1024,    0 },
	{ XS_PROFILE_HIGH_444_12,          { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 2, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  36, 16, 1024,    0 },
	{ XS_PROFILE_HIGH_4444_12,         { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 2, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  48, 16, 1024,    0 },
	{ XS_PROFILE_MLS_12,               { XS_CPIH_AUTO,   0, 16, 4, 8, 0xff,    0, 4, 0, 0, 5, 2, 0, 1, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE }, 20,  48,  0,    0,    0 },
	{ XS_PROFILE_LIGHT_BAYER,          { XS_CPIH_TETRIX, 0, 16, 4, 8, 0xff, 0xff, 4, 0, 0, 5, 0, 1, 0, 1, 0, 1, 1, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_RGGB },  6,  64,  4, 1024,    0 },
	{ XS_PROFILE_MAIN_BAYER,           { XS_CPIH_TETRIX, 0, 16, 4, 8, 0xff, 0xff, 4, 0, 0, 5, 1, 1, 1, 1, 0, 1, 1, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_RGGB },  6,  64,  8, 1024,    0 },
	{ XS_PROFILE_HIGH_BAYER,           { XS_CPIH_TETRIX, 0, 16, 4, 8, 0xff, 0xff, 4, 0, 0, 5, 2, 1, 1, 1, 0, 1, 1, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_RGGB },  6,  64, 16, 1024,    0 },
};
#define XS_PROFILES_COUNT (sizeof(XS_PROFILES) / sizeof(xs_profile_config_t))

const xs_level_values_t XS_LEVELS[] = {
	{ XS_LEVEL_1K_1, 1280, 5120, 26214400, 83558400 },
	{ XS_LEVEL_2K_1, 2048, 8192, 4194304, 133693440 },
	{ XS_LEVEL_4K_1, 4096, 16384, 8912896, 267386880 },
	{ XS_LEVEL_4K_2, 4096, 16384, 16777216, 534773760 },
	{ XS_LEVEL_4K_3, 4096, 16384, 16777216, 1069547520 },
	{ XS_LEVEL_8K_1, 8192, 32768, 35651584, 1069547520 },
	{ XS_LEVEL_8K_2, 8192, 32768, 67108864, 2139095040 },
	{ XS_LEVEL_8K_3, 8192, 32768, 67108864, 4278190080 },
	{ XS_LEVEL_10K_1, 10240, 40960, 104857600, 3342336000 },
	{ XS_LEVEL_UNRESTRICTED, 0xffff, 0xffff, 0xffffffff, 0xffffffff },
};
#define XS_LEVELS_COUNT (sizeof(XS_LEVELS) / sizeof(xs_level_values_t))

const xs_sublevel_values_t XS_SUBLEVELS[] = {
	{ XS_SUBLEVEL_2_BPP, 2 },
	{ XS_SUBLEVEL_3_BPP, 3 },
	{ XS_SUBLEVEL_6_BPP, 6 },
	{ XS_SUBLEVEL_9_BPP, 9 },
	{ XS_SUBLEVEL_12_BPP, 12 },
	{ XS_SUBLEVEL_FULL, 0 },  // special case
	{ XS_SUBLEVEL_UNRESTRICTED, 1000000 },
};
#define XS_SUBLEVELS_COUNT (sizeof(XS_SUBLEVELS) / sizeof(xs_sublevel_values_t))

typedef struct default_weights_t default_weights_t;
struct default_weights_t
{
	xs_gains_mode_t mode;
	int8_t nly;
	int8_t nlx;
	xs_cpih_t color_transform;

	int8_t ncomps;
	int8_t chroma_sx; // c1 and c2
	int8_t chroma_sy; // c1 and c2

	uint8_t	gains[MAX_NCOMPS][MAX_NFILTER_TYPES];
	uint8_t priorities[MAX_NCOMPS][MAX_NFILTER_TYPES];
};

// TODO: add more default weights for various other combinations
const default_weights_t default_weights[] =
{
#if !defined(XS_THEORETICAL_GAINS_CALCULATION)
	// Values as used in ED1 reference implementation.
	{XS_GAINS_OPT_PSNR, 0, 1, XS_CPIH_NONE, 3, 1, 1, {0, 0, 0, 0, 0, 0, }, {2, 1, 0, 4, 3, 5, }},
	{XS_GAINS_OPT_PSNR, 0, 1, XS_CPIH_NONE, 3, 2, 1, {0, 0, 0, 0, 0, 0, }, {2, 1, 0, 4, 3, 5, }},
	{XS_GAINS_OPT_PSNR, 0, 1, XS_CPIH_RCT, 3, 1, 1, {1, 0, 0, 1, 0, 0, }, {0, 2, 1, 3, 4, 5, }},
	{XS_GAINS_OPT_PSNR, 0, 2, XS_CPIH_NONE, 3, 1, 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 5, 4, 3, 7, 8, 6, }},
	{XS_GAINS_OPT_PSNR, 0, 2, XS_CPIH_NONE, 3, 2, 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 5, 4, 3, 7, 8, 6, }},
	{XS_GAINS_OPT_PSNR, 0, 2, XS_CPIH_RCT, 3, 1, 1, {2, 0, 0, 1, 0, 0, 1, 0, 0, }, {6, 0, 1, 2, 4, 3, 5, 8, 7, }},
	{XS_GAINS_OPT_PSNR, 0, 3, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {5, 4, 3, 1, 2, 0, 6, 8, 7, 10, 9, 11, }},
	{XS_GAINS_OPT_PSNR, 0, 3, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {5, 4, 3, 1, 2, 0, 6, 8, 7, 10, 9, 11, }},
	{XS_GAINS_OPT_PSNR, 0, 3, XS_CPIH_RCT, 3, 1, 1, {2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {3, 5, 4, 0, 2, 1, 6, 8, 7, 9, 10, 11, }},
	{XS_GAINS_OPT_PSNR, 0, 4, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 1, 0, 9, 11, 10, 5, 3, 4, 7, 6, 8, 13, 12, 14, }},
	{XS_GAINS_OPT_PSNR, 0, 4, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 1, 0, 9, 11, 10, 5, 3, 4, 7, 6, 8, 13, 12, 14, }},
	{XS_GAINS_OPT_PSNR, 0, 4, XS_CPIH_RCT, 3, 1, 1, {3, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {12, 1, 0, 8, 11, 10, 2, 3, 4, 5, 6, 7, 9, 13, 14, }},
	{XS_GAINS_OPT_PSNR, 0, 5, XS_CPIH_NONE, 3, 1, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {8, 7, 6, 5, 3, 4, 13, 12, 14, 1, 0, 2, 9, 11, 10, 16, 15, 17, }},
	{XS_GAINS_OPT_PSNR, 0, 5, XS_CPIH_NONE, 3, 2, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {8, 7, 6, 5, 3, 4, 13, 12, 14, 1, 0, 2, 9, 11, 10, 16, 15, 17, }},
	{XS_GAINS_OPT_PSNR, 0, 5, XS_CPIH_RCT, 3, 1, 1, {3, 2, 2, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {6, 8, 7, 1, 4, 5, 12, 14, 15, 0, 2, 3, 9, 11, 10, 13, 16, 17, }},
	{XS_GAINS_OPT_PSNR, 1, 1, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {8, 7, 6, 1, 5, 0, 2, 4, 3, 10, 9, 11, }},
	{XS_GAINS_OPT_PSNR, 1, 1, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {8, 7, 6, 1, 5, 0, 2, 4, 3, 10, 9, 11, }},
	{XS_GAINS_OPT_PSNR, 1, 1, XS_CPIH_NONE, 3, 2, 2, {1, 1, 1, 1, 0, 0, 1, 0, }, {2, 5, 4, 7, 1, 0, 6, 3, }},
	{XS_GAINS_OPT_PSNR, 1, 1, XS_CPIH_RCT, 3, 1, 1, {2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {6, 9, 8, 0, 5, 2, 1, 4, 3, 7, 10, 11, }},
	{XS_GAINS_OPT_PSNR, 1, 2, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {11, 10, 9, 0, 2, 1, 8, 4, 7, 5, 3, 6, 13, 12, 14, }},
	{XS_GAINS_OPT_PSNR, 1, 2, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {11, 10, 9, 0, 2, 1, 8, 4, 7, 5, 3, 6, 13, 12, 14, }},
	{XS_GAINS_OPT_PSNR, 1, 2, XS_CPIH_NONE, 3, 2, 2, {2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, }, {10, 5, 4, 7, 1, 0, 9, 3, 2, 8, 6, }},
	{XS_GAINS_OPT_PSNR, 1, 2, XS_CPIH_RCT, 3, 1, 1, {2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {5, 11, 10, 0, 2, 1, 4, 7, 9, 3, 6, 8, 12, 13, 14, }},
	{XS_GAINS_OPT_PSNR, 1, 3, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 1, 0, 14, 12, 13, 4, 3, 5, 7, 6, 9, 8, 11, 10, 16, 15, 17, }},
	{XS_GAINS_OPT_PSNR, 1, 3, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 1, 0, 14, 12, 13, 4, 3, 5, 7, 6, 9, 8, 11, 10, 16, 15, 17, }},
	{XS_GAINS_OPT_PSNR, 1, 3, XS_CPIH_NONE, 3, 2, 2, {2, 2, 2, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, }, {6, 11, 10, 4, 8, 7, 9, 1, 0, 13, 3, 2, 12, 5, }},
	{XS_GAINS_OPT_PSNR, 1, 3, XS_CPIH_RCT, 3, 1, 1, {3, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {15, 1, 0, 11, 12, 13, 2, 3, 4, 5, 7, 8, 6, 10, 9, 14, 16, 17, }},
	{XS_GAINS_OPT_PSNR, 1, 4, XS_CPIH_NONE, 3, 1, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {14, 12, 13, 4, 3, 5, 17, 15, 16, 0, 1, 2, 7, 10, 8, 11, 9, 6, 20, 18, 19, }},
	{XS_GAINS_OPT_PSNR, 1, 4, XS_CPIH_NONE, 3, 2, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {14, 12, 13, 4, 3, 5, 17, 15, 16, 0, 1, 2, 7, 10, 8, 11, 9, 6, 20, 18, 19, }},
	{XS_GAINS_OPT_PSNR, 1, 4, XS_CPIH_NONE, 3, 2, 2, {3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, }, {16, 7, 6, 13, 3, 2, 8, 11, 10, 12, 1, 0, 15, 5, 4, 14, 9, }},
	{XS_GAINS_OPT_PSNR, 1, 4, XS_CPIH_RCT, 3, 1, 1, {3, 2, 2, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {8, 13, 14, 3, 6, 7, 15, 16, 17, 0, 1, 2, 4, 12, 10, 5, 11, 9, 18, 19, 20, }},
	{XS_GAINS_OPT_PSNR, 1, 5, XS_CPIH_NONE, 3, 1, 1, {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {0, 1, 2, 18, 19, 20, 6, 7, 8, 15, 16, 17, 3, 4, 5, 10, 12, 14, 9, 11, 13, 21, 22, 23, }},
	{XS_GAINS_OPT_PSNR, 1, 5, XS_CPIH_NONE, 3, 2, 1, {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {0, 1, 2, 18, 19, 20, 6, 7, 8, 15, 16, 17, 3, 4, 5, 10, 12, 14, 9, 11, 13, 21, 22, 23, }},
	{XS_GAINS_OPT_PSNR, 1, 5, XS_CPIH_NONE, 3, 2, 2, {3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, }, {9, 16, 15, 7, 13, 12, 17, 3, 2, 6, 11, 10, 14, 1, 0, 19, 5, 4, 18, 8, }},
	{XS_GAINS_OPT_PSNR, 1, 5, XS_CPIH_RCT, 3, 1, 1, {4, 2, 2, 3, 2, 2, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {21, 1, 0, 15, 19, 18, 5, 9, 8, 14, 17, 16, 2, 4, 3, 7, 13, 11, 6, 12, 10, 20, 23, 22, }},
	{XS_GAINS_OPT_PSNR, 2, 2, XS_CPIH_NONE, 3, 1, 1, {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 13, 12, 14, 17, 15, 16, 9, 10, 11, 4, 7, 5, 8, 6, 3, 20, 18, 19, }},
	{XS_GAINS_OPT_PSNR, 2, 2, XS_CPIH_NONE, 3, 2, 1, {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 13, 12, 14, 17, 15, 16, 9, 10, 11, 4, 7, 5, 8, 6, 3, 20, 18, 19, }},
	{XS_GAINS_OPT_PSNR, 2, 2, XS_CPIH_NONE, 3, 2, 2, {2, 2, 2, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {6, 16, 15, 2, 8, 7, 1, 0, 14, 12, 10, 13, 11, 9, 5, 4, 3, }},
	{XS_GAINS_OPT_PSNR, 2, 2, XS_CPIH_RCT, 3, 1, 1, {2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {0, 1, 2, 12, 14, 15, 13, 16, 17, 9, 10, 11, 3, 8, 6, 4, 7, 5, 18, 19, 20, }},
	{XS_GAINS_OPT_PSNR, 2, 3, XS_CPIH_NONE, 3, 1, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {11, 9, 10, 1, 0, 2, 16, 17, 15, 19, 20, 18, 12, 13, 14, 7, 3, 4, 6, 5, 8, 21, 22, 23, }},
	{XS_GAINS_OPT_PSNR, 2, 3, XS_CPIH_NONE, 3, 2, 1, {2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {11, 9, 10, 1, 0, 2, 16, 17, 15, 19, 20, 18, 12, 13, 14, 7, 3, 4, 6, 5, 8, 21, 22, 23, }},
	{XS_GAINS_OPT_PSNR, 2, 3, XS_CPIH_NONE, 3, 2, 2, {2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {0, 10, 9, 13, 5, 4, 3, 12, 11, 2, 1, 19, 17, 15, 18, 16, 14, 8, 7, 6, }},
	{XS_GAINS_OPT_PSNR, 2, 3, XS_CPIH_RCT, 3, 1, 1, {3, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {9, 11, 12, 0, 3, 4, 15, 18, 17, 16, 20, 19, 10, 13, 14, 2, 5, 6, 1, 7, 8, 21, 22, 23, }},
	{XS_GAINS_OPT_PSNR, 2, 4, XS_CPIH_NONE, 3, 1, 1, {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 22, 23, 21, 4, 3, 5, 15, 16, 17, 18, 19, 20, 14, 12, 13, 11, 9, 6, 7, 10, 8, 24, 26, 25, }},
	{XS_GAINS_OPT_PSNR, 2, 4, XS_CPIH_NONE, 3, 2, 1, {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {2, 0, 1, 22, 23, 21, 4, 3, 5, 15, 16, 17, 18, 19, 20, 14, 12, 13, 11, 9, 6, 7, 10, 8, 24, 26, 25, }},
	{XS_GAINS_OPT_PSNR, 2, 4, XS_CPIH_NONE, 3, 2, 2, {3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {9, 22, 21, 5, 13, 12, 14, 4, 3, 2, 11, 10, 1, 0, 20, 18, 16, 19, 17, 15, 8, 7, 6, }},
	{XS_GAINS_OPT_PSNR, 2, 4, XS_CPIH_RCT, 3, 1, 1, {3, 2, 2, 3, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {0, 1, 2, 21, 24, 23, 3, 6, 7, 15, 17, 18, 16, 19, 20, 12, 13, 14, 5, 10, 8, 4, 11, 9, 22, 26, 25, }},
	{XS_GAINS_OPT_PSNR, 2, 5, XS_CPIH_NONE, 3, 1, 1, {3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {14, 13, 12, 9, 11, 10, 25, 24, 26, 0, 1, 2, 19, 20, 18, 23, 22, 21, 17, 15, 16, 4, 8, 5, 3, 6, 7, 28, 27, 29, }},
	{XS_GAINS_OPT_PSNR, 2, 5, XS_CPIH_NONE, 3, 2, 1, {3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, {14, 13, 12, 9, 11, 10, 25, 24, 26, 0, 1, 2, 19, 20, 18, 23, 22, 21, 17, 15, 16, 4, 8, 5, 3, 6, 7, 28, 27, 29, }},
	{XS_GAINS_OPT_PSNR, 2, 5, XS_CPIH_NONE, 3, 2, 2, {3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {0, 13, 12, 25, 7, 6, 8, 17, 16, 18, 5, 4, 3, 15, 14, 2, 1, 24, 22, 20, 23, 21, 19, 11, 10, 9, }},
	{XS_GAINS_OPT_PSNR, 2, 5, XS_CPIH_RCT, 3, 1, 1, {4, 3, 3, 3, 2, 2, 3, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, }, {12, 15, 14, 3, 11, 10, 24, 26, 27, 0, 4, 5, 18, 21, 20, 19, 23, 22, 13, 16, 17, 2, 9, 6, 1, 7, 8, 25, 28, 29, }},
#endif

	{XS_GAINS_OPT_VISUAL, 1, 5, XS_CPIH_NONE, 3, 2, 1, {6, 5, 5, 5, 4, 4, 4, 3, 3, 4, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {15, 16, 17, 11, 12, 13, 0, 1, 2, 14, 9, 10, 20, 21, 22, 6, 3, 4, 5, 7, 8, 23, 18, 19, }},
	{XS_GAINS_OPT_VISUAL, 1, 5, XS_CPIH_NONE, 3, 1, 1, {6, 5, 5, 5, 4, 4, 4, 3, 3, 4, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {15, 16, 17, 11, 12, 13, 0, 1, 2, 14, 9, 10, 20, 21, 22, 6, 3, 4, 5, 7, 8, 23, 18, 19, }},
	{XS_GAINS_OPT_VISUAL, 1, 5, XS_CPIH_RCT, 3, 1, 1, {8, 6, 6, 7, 5, 5, 7, 4, 4, 6, 4, 4, 5, 3, 3, 3, 1, 1, 3, 1, 1, 2, 0, 0, }, {12, 15, 16, 8, 10, 11, 21, 1, 0, 9, 14, 13, 17, 18, 19, 3, 4, 5, 2, 6, 7, 20, 22, 23, }},
	{XS_GAINS_OPT_VISUAL, 2, 5, XS_CPIH_NONE, 3, 2, 1, {6, 6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {11, 10, 9, 3, 5, 4, 25, 24, 26, 6, 7, 8, 19, 20, 18, 23, 22, 21, 2, 0, 1, 13, 17, 14, 12, 15, 16, 28, 27, 29, }},
	{XS_GAINS_OPT_VISUAL, 2, 5, XS_CPIH_NONE, 3, 1, 1, {6, 6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, }, {11, 10, 9, 3, 5, 4, 25, 24, 26, 6, 7, 8, 19, 20, 18, 23, 22, 21, 2, 0, 1, 13, 17, 14, 12, 15, 16, 28, 27, 29, }},
	{XS_GAINS_OPT_VISUAL, 2, 5, XS_CPIH_RCT, 3, 1, 1, {8, 6, 6, 7, 5, 5, 7, 5, 5, 6, 4, 4, 5, 3, 3, 5, 3, 3, 4, 1, 1, 3, 1, 1, 3, 1, 1, 2, 0, 0, }, {6, 10, 9, 0, 5, 4, 23, 24, 25, 3, 7, 8, 17, 20, 19, 18, 22, 21, 27, 1, 2, 12, 16, 13, 11, 14, 15, 26, 28, 29, }},
};

const default_weights_t* _get_default_weights(const xs_gains_mode_t mode, const int8_t nlx, const int8_t nly, const xs_cpih_t color_transform, const xs_image_t* im)
{
	// Search the big table of pre-defined gains/priorities.
	if (im->sx[0] != 1 || im->sy[0] != 1)
	{
		return NULL;
	}

	uint8_t chroma_sx = 0;
	uint8_t chroma_sy = 0;
	if (im->ncomps >= 3 && im->sx[1] == im->sx[2])
	{
		chroma_sx = im->sx[1];
	}
	if (im->ncomps >= 3 && im->sy[1] == im->sy[2])
	{
		chroma_sy = im->sy[1];
	}

	for (int item = 0; item < sizeof(default_weights) / sizeof(default_weights_t); ++item)
	{
		const default_weights_t* d = &default_weights[item];
		if (d->mode == mode && d->nlx == nlx && d->nly == nly && d->ncomps == im->ncomps && d->chroma_sx == chroma_sx && d->chroma_sy == chroma_sy && d->color_transform == color_transform)
		{
			return d;
		}
	}

	return NULL;
}

int _find_profile_index(const xs_profile_t profile)
{
	for (int i = 0; i < XS_PROFILES_COUNT; ++i)
	{
		if (XS_PROFILES[i].profile == profile)
		{
			return i;
		}
	}
	return -1;
}

int _find_level_index(const xs_level_t level)
{
	const xs_level_values_t* l = &XS_LEVELS[XS_LEVELS_COUNT - 1];
	for (int i = 0; i < XS_LEVELS_COUNT; ++i)
	{
		if (XS_LEVELS[i].level == level)
		{
			return i;
		}
	}
	return -1;
}

int _find_sublevel_index(const xs_sublevel_t sublevel)
{
	const xs_sublevel_values_t* l = &XS_SUBLEVELS[XS_SUBLEVELS_COUNT - 1];
	for (int i = 0; i < XS_SUBLEVELS_COUNT; ++i)
	{
		if (XS_SUBLEVELS[i].sublevel == sublevel)
		{
			return i;
		}
	}
	return -1;
}

bool xs_config_init(xs_config_t* cfg, const xs_image_t* im, xs_profile_t profile, xs_level_t level, xs_sublevel_t sublevel)
{
	// im can be NULL in certain cases, this is used for config dumping.
	// Take the profile, level, and sublevel to fill in the "best" defaults of that profile.
	if (profile == XS_PROFILE_AUTO)
	{
		assert(im != NULL);
		// Select the best, based on image.
		switch (im->ncomps)
		{
		case 1:
			profile = XS_PROFILE_HIGH_444_12;
			break;
		case 3:
			if (im->sx[0] == 1 && im->sx[1] == 2 && im->sx[2] == 2 && im->sy[0] == 1 && im->sy[1] == 2 && im->sy[2] == 2)
			{
				profile = XS_PROFILE_HIGH_420_12;
			}
			else
			{
				profile = XS_PROFILE_HIGH_444_12;
			}
			break;
		case 4:
			profile = XS_PROFILE_HIGH_4444_12;
			break;
		default:
			if (cfg->verbose) fprintf(stderr, "Warning: Unable to automatically select a matching profile, using UNRESTRICTED profile\n");
			profile = XS_PROFILE_UNRESTRICTED;
			break;
		}
	}

	const int verbose = cfg->verbose; // make this setting survive
	memset(cfg, 0, sizeof(xs_config_t));
	cfg->verbose = verbose;
	cfg->gains_mode = XS_GAINS_OPT_PSNR;
	cfg->profile = profile;
	cfg->level = level;
	cfg->sublevel = sublevel;
	cfg->cap_bits = XS_CAP_AUTO;
	const int profile_index = _find_profile_index(profile);
	assert(profile_index != -1);
	cfg->p = XS_PROFILES[profile_index].parameters;
	cfg->budget_report_lines = XS_PROFILES[profile_index].budget_report_lines;

	if (cfg->profile == XS_PROFILE_MLS_12)
	{
		cfg->bitstream_size_in_bytes = (size_t)-1;  // for MLS, disables RD allocation (user can override this value)
	}

	return true;
}

bool _select_level_priorities_and_gains(xs_config_t* cfg, const xs_image_t* im)
{
	assert(cfg != NULL);
	assert(im != NULL);

	uint8_t* gains = (uint8_t*)&cfg->p.lvl_gains;
	uint8_t* priorities = (uint8_t*)&cfg->p.lvl_priorities;

	if (cfg->gains_mode != XS_GAINS_OPT_EXPLICIT && gains[0] == 0xff && priorities[0] == 0xff)
	{
		// No gains/priorities specified, use LUT first.
		const default_weights_t* def = _get_default_weights(cfg->gains_mode, cfg->p.NLx, cfg->p.NLy, cfg->p.color_transform, im);
		if (def != NULL)
		{
			const int nbands = ids_calculate_nbands(im, cfg->p.NLx, cfg->p.NLy, cfg->p.Sd);
			memcpy(gains, def->gains, nbands * sizeof(uint8_t));
			memcpy(priorities, def->priorities, nbands * sizeof(uint8_t));
			gains[nbands] = 0xff;
			priorities[nbands] = 0xff;
		}
		else
		{
#ifdef XS_THEORETICAL_GAINS_CALCULATION
			// Call the theoretical gains/priority calculation for the current image/XS_cfg.
			if (!xs_calculate_gains_and_priorities(cfg, im->ncomps, im->sx, im->sy, gains, priorities))
			{
				if (cfg->verbose) fprintf(stderr, "Error: Unable to derive calculated gains and priorities (possibly an implemantation limitation), specify values manually instead\n");
				return false;
			}
#else
			const int nbands = ids_calculate_nbands(im, cfg->p.NLx, cfg->p.NLy, cfg->p.Sd);
			fprintf(stderr, "Error: No built-in gains/priorities defined for the given configuration, specify manually (need %d band values)\n", nbands);
			return false;
#endif
		}
	}
	else
	{
		cfg->gains_mode = XS_GAINS_OPT_EXPLICIT;
	}

	gains[MAX_NBANDS] = 0xff;
	priorities[MAX_NBANDS] = 0xff;

	return true;
}

xs_cap_t _calculate_cap_bits(const xs_config_t* cfg, const xs_image_t* im)
{
	assert(im->ncomps <= MAX_NCOMPS);
	bool using_sy = false;
	for (int c = 0; !using_sy && c < im->ncomps; ++c)
	{
		using_sy = im->sy[c] > 1;
	}

	xs_cap_t auto_cap_bits = 0;
	if (cfg->p.color_transform == XS_CPIH_TETRIX)
	{
		auto_cap_bits |= XS_CAP_STAR_TETRIX;
	}
	if (cfg->p.Tnlt == XS_NLT_QUADRATIC)
	{
		auto_cap_bits |= XS_CAP_NLT_Q;
	}
	if (cfg->p.Tnlt == XS_NLT_EXTENDED)
	{
		auto_cap_bits |= XS_CAP_NLT_E;
	}
	if (using_sy)
	{
		auto_cap_bits |= XS_CAP_SY;
	}
	if (cfg->p.Sd > 0)
	{
		auto_cap_bits |= XS_CAP_SD;
	}
	if (cfg->profile == XS_PROFILE_MLS_12 ||
		(cfg->profile == XS_PROFILE_UNRESTRICTED && cfg->p.Fq == 0) ||
		cfg->bitstream_size_in_bytes == (size_t)-1)
	{
		auto_cap_bits |= XS_CAP_MLS;
	}
	if (cfg->p.Rl != 0)
	{
		auto_cap_bits |= XS_CAP_RAW_PER_PKT;
	}
	return auto_cap_bits;
}

int _calculate_max_bpp(const int profile_index, const int sublevel_index)
{
	const xs_sublevel_values_t* l = &XS_SUBLEVELS[sublevel_index];
	if (l->sublevel == XS_SUBLEVEL_FULL)
	{
		const xs_profile_config_t* p = &XS_PROFILES[profile_index];
		return p->Nbpp_at_sublev_full;
	}
	return l->max_bpp;
}

void xs_config_resolve_bw_fq(xs_config_t* cfg, const xs_nlt_t target_Tnlt, const int im_depth)
{
	// Resolve Bw and Fq when not explicitly specified.
	switch (cfg->profile)
	{
	case XS_PROFILE_UNRESTRICTED:
	{
		if (cfg->p.Bw == 0xff)
		{
			cfg->p.Bw = 20;
		}
		break;
	}
	case XS_PROFILE_MLS_12:
	{
		if (cfg->p.Bw == 0xff)
		{
			assert(im_depth > 0);
			cfg->p.Bw = im_depth;
		}
		break;
	}
	case XS_PROFILE_LIGHT_BAYER:
	case XS_PROFILE_MAIN_BAYER:
	case XS_PROFILE_HIGH_BAYER:
	{
		if (cfg->p.Bw == 0xff)
		{
			cfg->p.Bw = target_Tnlt == XS_NLT_NONE ? 20 : 18;
		}
		if (cfg->p.Fq == 0xff)
		{
			cfg->p.Fq = cfg->p.Bw - 12;
		}
		break;
	}
	default:
	{
		break;
	}
	}
}

bool xs_config_resolve_auto_values(xs_config_t* cfg, const xs_image_t* im)
{
	// Look up parameters if set to AUTO. Validation is happening later.
	// Assume nothing about image (im) correctness at this point.
	if (cfg->p.color_transform == XS_CPIH_AUTO)
	{
		switch (cfg->profile)
		{
		case XS_PROFILE_LIGHT_444_12:
		case XS_PROFILE_MAIN_444_12:
		case XS_PROFILE_MAIN_4444_12:
		case XS_PROFILE_HIGH_444_12:
		case XS_PROFILE_HIGH_4444_12:
		{
			cfg->p.color_transform = XS_CPIH_NONE;
			if (im->ncomps >= 3 && im->sx[0] == 1 && im->sx[1] == 1 && im->sx[2] == 1 && im->sy[0] == 1 && im->sy[1] == 1 && im->sy[2] == 1)
			{
				// Enable RCT if 3 non-subsampled components are present.
				cfg->p.color_transform = XS_CPIH_RCT;
			}
			break;
		}
		default:
		{
			cfg->p.color_transform = XS_CPIH_NONE;
			break;
		}
		}
	}

	if (cfg->level == XS_LEVEL_AUTO)
	{
		cfg->level = XS_LEVEL_UNRESTRICTED;
		// Select the lowest possible level, matching image.
		for (int i = 0; i < XS_LEVELS_COUNT; ++i)
		{
			const xs_level_values_t* l = &XS_LEVELS[i];
			if (im->width <= (int)l->max_w && im->height <= (int)l->max_h && im->width * im->height <= (int)l->max_samples || l->level == XS_LEVEL_UNRESTRICTED)
			{
				cfg->level = l->level;
				break;
			}
		}
	}

	if (cfg->sublevel == XS_SUBLEVEL_AUTO)
	{		
		cfg->sublevel = XS_SUBLEVEL_UNRESTRICTED;
		if (cfg->bitstream_size_in_bytes > 0)
		{
			const float bitrate = (float)cfg->bitstream_size_in_bytes * ((cfg->p.color_transform == XS_CPIH_TETRIX) ? 2 : 8) / MAX(.0001f, im->width * im->height);
			// Select the lowest possible sublevel.		
			for (int i = 0; i < XS_SUBLEVELS_COUNT; ++i)
			{
				const xs_sublevel_values_t* l = &XS_SUBLEVELS[i];
				if (bitrate <= l->max_bpp || l->sublevel == XS_SUBLEVEL_UNRESTRICTED)
				{
					cfg->sublevel = l->sublevel;
					break;
				}
			}
		}
	}

	xs_config_resolve_bw_fq(cfg, cfg->p.Tnlt, im->depth);

	if (cfg->cap_bits == XS_CAP_AUTO)
	{
		cfg->cap_bits = _calculate_cap_bits(cfg, im);
	}

	// Fill in the level gains and priorities (if not manually configured or read from codestream).
	if (!_select_level_priorities_and_gains(cfg, im))
	{
		return false;
	}

	return true;
}

bool xs_config_validate(const xs_config_t* cfg, const xs_image_t* im)
{
	// Check the settings (against conflicts) and manage the AUTO values to become the lowest matching setting.
	assert(cfg != NULL);
	assert(im != NULL);

	const int profile_index = _find_profile_index(cfg->profile);
	const int level_index = _find_level_index(cfg->level);
	const int sublevel_index = _find_sublevel_index(cfg->sublevel);

	if (profile_index == -1)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unknown profile value\n");
		return false;
	}

	if (level_index == -1)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unknown level value\n");
		return false;
	}

	if (sublevel_index == -1)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unknown sublevel value\n");
		return false;
	}

	if (im->ncomps < 1 || im->ncomps > MAX_NCOMPS)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unsupported number of components\n");
		return false;
	}

	if (im->depth < 8 || im->depth > 16)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unsupported image bit depth\n");
		return false;
	}

	int max_sx = im->sx[0];
	int max_sy = im->sy[0];
	int min_sx = im->sx[0];
	int min_sy = im->sy[0];
	for (int c = 1; c < im->ncomps; ++c)
	{
		if (max_sx < im->sx[c]) max_sx = im->sx[c];
		if (max_sy < im->sy[c]) max_sy = im->sy[c];
		if (min_sx > im->sx[c]) min_sx = im->sx[c];
		if (min_sy > im->sy[c]) min_sy = im->sy[c];
	}

	if (min_sx != 1 || min_sy != 1 || max_sx <= 0 || max_sx > 2 || max_sy <= 0 || max_sy > 2)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unsupported sub-sampling format\n");
		return false;
	}

	if (cfg->p.NLx < 1 || cfg->p.NLx > MAX_NDECOMP_H)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unsupported number of horizontal decompositions\n");
		return false;
	}
	if (cfg->p.NLy < (max_sy - 1) || cfg->p.NLy > cfg->p.NLx || cfg->p.NLy > MAX_NDECOMP_V)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unsupported number of vertical decompositions\n");
		return false;
	}

	if (cfg->p.Sd >= im->ncomps)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Number of surpressed components for wavelet transform (Sd) is too large\n");
		return false;
	}
	for (int c = im->ncomps - cfg->p.Sd; c < im->ncomps; ++c)
	{
		if (im->sx[c] > 1 || im->sy[c] > 1)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Suppressed components for wavelet transform (Sd) cannot be subsampled\n");
			return false;
		}
	}

	if (im->width > 65535 || im->width < max_sx * (1 << cfg->p.NLx) || im->height > 65535 || im->height < max_sy * (1 << cfg->p.NLy))
	{
		if (cfg->verbose) fprintf(stderr, "Error: Image dimensions are not supported (might be due to sx/sy or NLx/NLy settings)\n");
		return false;
	}
	if (cfg->p.Cw != 0 && (im->width % (8 * cfg->p.Cw * max_sx * (1 << cfg->p.NLx))) < max_sx * (1 << cfg->p.NLx))
	{
		if (cfg->verbose) fprintf(stderr, "Error: Invalid Cw value\n");
		return false;
	}
	if (cfg->p.slice_height == 0 || (cfg->p.slice_height % (1 << cfg->p.NLy)) != 0)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Invalid slice height\n");
		return false;
	}

	if (cfg->p.N_g != 4 || cfg->p.S_s != 8 || cfg->p.B_r != 4 || cfg->p.Fslc != 0 || cfg->p.Ppoc != 0)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Invalid Ng, Ss, Br, Fslc, or Ppoc value(s)\n");
		return false;
	}

	if (cfg->bitstream_size_in_bytes == 0)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Missing or invalid target bitstream size or bit rate setting\n");
		return false;
	}

	{
		const xs_level_values_t* l = &XS_LEVELS[level_index];
		if (!((uint16_t)im->width <= l->max_w && (uint16_t)im->height <= l->max_h && (uint32_t)im->width * (uint32_t)im->height <= l->max_samples))
		{
			if (cfg->verbose) fprintf(stderr, "Error: Image and level mismatch\n");
			return false;
		}
	}

	if (cfg->bitstream_size_in_bytes != (size_t)-1)
	{
		const float bitrate = (float)cfg->bitstream_size_in_bytes * ((cfg->p.color_transform == XS_CPIH_TETRIX) ? 2 : 8) / MAX(.0001f, im->width * im->height);
		const float max_bpp = (float)_calculate_max_bpp(profile_index, sublevel_index);
		if (!(bitrate <= max_bpp))
		{
			if (cfg->verbose) fprintf(stderr, "Error: Overall bit rate (%.02f bpp) exceeds allowed bit rate of sublevel (max %d bpp)\n", bitrate, (int)max_bpp);
			return false;
		}
	}

	if (cfg->profile != XS_PROFILE_UNRESTRICTED && cfg->p.slice_height != 16)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Slice height must be 16 lines high\n");
		return false;
	}

	switch (cfg->p.color_transform)
	{
	case XS_CPIH_NONE:
	{
		break;
	}
	case XS_CPIH_RCT:
	{
		if (im->ncomps < 3 || im->sx[0] != 1 || im->sx[1] != 1 || im->sx[2] != 1 || im->sy[0] != 1 || im->sy[1] != 1 || im->sy[2] != 1)
		{
			if (cfg->verbose) fprintf(stderr, "Error: RCT color transform and sub-sampling cannot be combined\n");
			return false;
		}
		break;
	}
	case XS_CPIH_TETRIX:
	{
		if (im->ncomps != 4 || im->sx[0] != 1 || im->sx[1] != 1 || im->sx[2] != 1 || im->sx[3] != 1 || im->sy[0] != 1 || im->sy[1] != 1 || im->sy[2] != 1 || im->sy[3] != 1)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid image format to use Star-Tetrix color transform\n");
			return false;
		}
		if (cfg->p.tetrix_params.Cf != XS_TETRIX_FULL && cfg->p.tetrix_params.Cf != XS_TETRIX_INLINE)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid Cf value\n");
			return false;
		}
		if (cfg->p.tetrix_params.e1 > 3 || cfg->p.tetrix_params.e2 > 3)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid e1/e2 value(s)\n");
			return false;
		}
		break;
	}
	default:
	{
		if (cfg->verbose) fprintf(stderr, "Error: Unknown color transform value (Cpih)\n");
		return false;
	}
	}

	switch (cfg->profile)
	{
	case XS_PROFILE_UNRESTRICTED:
	{
		break;
	}
	case XS_PROFILE_MAIN_422_10:
	case XS_PROFILE_MAIN_444_12:
	case XS_PROFILE_MAIN_4444_12:
	case XS_PROFILE_HIGH_444_12:
	case XS_PROFILE_HIGH_4444_12:
	{
		if (cfg->p.NLy > 0 && cfg->p.Cw != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Cw must be 0 when using more than one vertical wavelet decomposition\n");
			return false;
		}
		if (cfg->p.Lh != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Lh (force long headers) must be 0\n");
			return false;
		}
		break;
	}
	case XS_PROFILE_LIGHT_422_10:
	case XS_PROFILE_LIGHT_444_12:
	case XS_PROFILE_MAIN_420_12:
	case XS_PROFILE_HIGH_420_12:
	{
		if (cfg->p.Cw != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Cw must be 0\n");
			return false;
		}
		if (cfg->p.Lh != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Lh (force long headers) must be 0\n");
			return false;
		}
		break;
	}
	case XS_PROFILE_LIGHT_SUBLINE_422_10:
	{
		if (cfg->p.Lh != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Lh (force long headers) must be 0\n");
			return false;
		}
		break;
	}
	case XS_PROFILE_MLS_12:
	{
		if (cfg->p.Lh != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Lh (force long headers) must be 0\n");
			return false;
		}
		if (cfg->p.Bw != im->depth)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid Bw value\n");
			return false;
		}
		if (cfg->p.Fq != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid Fq value, must be 0\n");
			return false;
		}
		break;
	}
	case XS_PROFILE_LIGHT_BAYER:
	case XS_PROFILE_MAIN_BAYER:
	case XS_PROFILE_HIGH_BAYER:
	{
		if (im->ncomps != 4)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Number of components must be 4\n");
			return false;
		}
		if (cfg->p.Sd != 1)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Number of surpressed components for wavelet transform (Sd) must be 1\n");
			return false;
		}
		if (cfg->p.Bw != 18 && cfg->p.Bw != 20 ||
			cfg->p.Bw == 18 && cfg->p.Tnlt == XS_NLT_NONE ||
			cfg->p.Bw == 20 && cfg->p.Tnlt != XS_NLT_NONE)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid Bw value\n");
			return false;
		}
		if (cfg->p.Fq != cfg->p.Bw - 12)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid Fq value\n");
			return false;
		}
		break;
	}
	default:
	{
		assert(false);
		return false;
	}
	}

	const int max_column_width = XS_PROFILES[profile_index].max_column_width;
	if (max_column_width != 0)
	{
		const int cs = ids_calculate_cs(im, cfg->p.NLx, cfg->p.Cw);
		if (cs > max_column_width)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Maximum column/precinct width exceeds %d (use Cw != 0 to limit the column width)\n", max_column_width);
			return false;
		}
	}

	if (cfg->p.Fq != 0 && cfg->p.Fq != 6 && cfg->p.Fq != 8)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Invalid Fq value\n");
		return false;
	}

	switch (cfg->profile)
	{
	case XS_PROFILE_UNRESTRICTED:
	{
		// Any NLT allowed.
		break;
	}
	case XS_PROFILE_LIGHT_BAYER:
	{
		// Any NLT allowed.
		if (cfg->p.color_transform != XS_CPIH_TETRIX || cfg->p.tetrix_params.Cf != XS_TETRIX_INLINE)
		{
			if (cfg->verbose) fprintf(stderr, "Error: LightBayer profile requires INLINE TETRIX color transform\n");
			return false;
		}
		break;
	}
	case XS_PROFILE_HIGH_BAYER:
	case XS_PROFILE_MAIN_BAYER:
	{
		// Any NLT allowed.
		if (cfg->p.color_transform != XS_CPIH_TETRIX)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Bayer profile requires TETRIX color transform\n");
			return false;
		}
		break;
	}
	default:
	{
		if (cfg->p.Tnlt != XS_NLT_NONE)
		{
			if (cfg->verbose) fprintf(stderr, "Error: NLT is not supported for the selected profile\n");
			return false;
		}
		break;
	}
	}

	// Handle cap bits.
	{
		if ((cfg->cap_bits & ~(XS_CAP_STAR_TETRIX | XS_CAP_NLT_Q | XS_CAP_NLT_E | XS_CAP_SY | XS_CAP_SD | XS_CAP_MLS | XS_CAP_RAW_PER_PKT)) != 0)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Unknown CAP bits set\n");
			return false;
		}
		const xs_cap_t auto_cap_bits = _calculate_cap_bits(cfg, im);  // these represent the minimal needed cap bits
		if ((cfg->cap_bits & auto_cap_bits) != auto_cap_bits)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Mandatory CAP bits are not set (mismatch with used features)\n");
			return false;
		}
	}

	{
		const int nbands = ids_calculate_nbands(im, cfg->p.NLx, cfg->p.NLy, cfg->p.Sd);
		int counted_levels = 0;
		for (counted_levels = 0; counted_levels <= MAX_NBANDS && cfg->p.lvl_gains[counted_levels] != 0xff; ++counted_levels) {}
		if (counted_levels != nbands)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid amount of gains defined (given %d, need %d)\n", counted_levels, nbands);
			return false;
		}

		for (counted_levels = 0; counted_levels <= MAX_NBANDS && cfg->p.lvl_priorities[counted_levels] != 0xff; ++counted_levels) {}
		if (counted_levels != nbands)
		{
			if (cfg->verbose) fprintf(stderr, "Error: Invalid amount of priorities defined (given %d, need %d)\n", counted_levels, nbands);
			return false;
		}
	}

	return true;
}

bool xs_config_retrieve_buffer_model_parameters(const xs_config_t* cfg, xs_buffer_model_parameters_t* bmp)
{
	assert(bmp != NULL);

	const int profile_index = _find_profile_index(cfg->profile);
	const int level_index = _find_level_index(cfg->level);
	const int sublevel_index = _find_sublevel_index(cfg->sublevel);

	if (profile_index == -1 || level_index == -1 || sublevel_index == -1)
	{
		if (cfg->verbose) fprintf(stderr, "Error: Invalid profile, level or sublevel\n");
		return false;
	}

	memset(bmp, 0, sizeof(xs_buffer_model_parameters_t));

	const xs_profile_config_t* p = &XS_PROFILES[profile_index];
	bmp->profile = cfg->profile;
	bmp->level = cfg->level;
	bmp->sublevel = cfg->sublevel;
	bmp->Nbpp = _calculate_max_bpp(profile_index, sublevel_index);
	bmp->Nsbu = p->Nsbu;
	bmp->Ssbo = p->Ssbo;
	if (p->max_column_width != 0)
	{
		bmp->Wcmax = p->max_column_width;
	}
	else
	{
		bmp->Wcmax = XS_LEVELS[level_index].max_w;
	}	
	bmp->Ssbu = ((cfg->level == XS_LEVEL_UNRESTRICTED) || (cfg->sublevel == XS_SUBLEVEL_UNRESTRICTED)) ? 0 : (bmp->Wcmax * bmp->Nbpp);
	bmp->N_g = cfg->p.N_g;

	return true;
}