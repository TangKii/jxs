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
#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_PROFILE "p"
#define KEY_LEVEL "l"
#define KEY_SUBLEVEL "s"
#define KEY_BITRATE "rate"
#define KEY_SIZE "size"
#define KEY_BUDGET_LINES "budget_lines"
#define KEY_MCT "cpih"
#define KEY_CFA "cfa"
#define KEY_NLT "nlt"
#define KEY_CW "cw"
#define KEY_SLH "slh"
#define KEY_BW "bw"
#define KEY_FQ "fq"
#define KEY_NLX "nlx"
#define KEY_NLY "nly"
#define KEY_LH "lh"
#define KEY_RL "rl"
#define KEY_QUANT "qpih"
#define KEY_FS "fs"
#define KEY_RM "rm"
#define KEY_SD "sd"
#define KEY_GAINS "gains"
#define KEY_PRIORITIES "priorities"

#define KEY_USED_CHAR '$'

#ifndef _MSC_VER
static inline size_t strnlen_s(const char* str, size_t strsz)
{
	if (str == 0)
	{
		return 0;
	}
	return strnlen(str, strsz);
}
#endif

typedef struct xs_map_entry_t xs_map_entry_t;
struct xs_map_entry_t
{
	union {
		uint32_t raw;
		xs_profile_t profile;
		xs_level_t level;
		xs_sublevel_t sublevel;
		xs_cfa_pattern_t cfa;
	};
	const char* const name;
};

const xs_map_entry_t XS_PROFILES_NAME_MAP[] = {
	{ XS_PROFILE_UNRESTRICTED, "unrestricted" },
	{ XS_PROFILE_LIGHT_422_10, "light422.10" },
	{ XS_PROFILE_LIGHT_444_12, "light444.12" },
	{ XS_PROFILE_LIGHT_SUBLINE_422_10, "light-subline422.10" },
	{ XS_PROFILE_LIGHT_SUBLINE_422_10, "lightsubline422.10" },
	{ XS_PROFILE_MAIN_420_12, "main420.12" },
	{ XS_PROFILE_MAIN_422_10, "main422.10" },
	{ XS_PROFILE_MAIN_444_12, "main444.12" },
	{ XS_PROFILE_MAIN_4444_12, "main4444.12" },
	{ XS_PROFILE_HIGH_420_12, "high420.12" },
	{ XS_PROFILE_HIGH_444_12, "high444.12" },
	{ XS_PROFILE_HIGH_4444_12, "high4444.12" },
	{ XS_PROFILE_MLS_12, "mls.12" },
	{ XS_PROFILE_LIGHT_BAYER, "lightbayer" },
	{ XS_PROFILE_MAIN_BAYER, "mainbayer" },
	{ XS_PROFILE_HIGH_BAYER, "highbayer" },
	{ 0, 0 },
};
#define XS_PROFILES_NAME_MAP_COUNT ((sizeof(XS_PROFILES_NAME_MAP) / sizeof(xs_map_entry_t)) - 1)

const xs_map_entry_t XS_LEVELS_NAME_MAP[] = {
	{ XS_LEVEL_UNRESTRICTED, "unrestricted" },
	{ XS_LEVEL_1K_1, "1k-1" },
	{ XS_LEVEL_1K_1, "1k1" },
	{ XS_LEVEL_1K_1, "bayer2k1" },
	{ XS_LEVEL_1K_1, "bayer2k-1" },
	{ XS_LEVEL_2K_1, "2k-1" },
	{ XS_LEVEL_2K_1, "2k1" },
	{ XS_LEVEL_2K_1, "bayer4k1" },
	{ XS_LEVEL_2K_1, "bayer4k-1" },
	{ XS_LEVEL_4K_1, "4k-1" },
	{ XS_LEVEL_4K_1, "4k1" },
	{ XS_LEVEL_4K_1, "bayer8k1" },
	{ XS_LEVEL_4K_1, "bayer8k-1" },
	{ XS_LEVEL_4K_2, "4k-2" },
	{ XS_LEVEL_4K_2, "4k2" },
	{ XS_LEVEL_4K_2, "bayer8k2" },
	{ XS_LEVEL_4K_2, "bayer8k-2" },
	{ XS_LEVEL_4K_3, "4k-3" },
	{ XS_LEVEL_4K_3, "4k3" },
	{ XS_LEVEL_4K_3, "bayer8k3" },
	{ XS_LEVEL_4K_3, "bayer8k-3" },
	{ XS_LEVEL_8K_1, "8k-1" },
	{ XS_LEVEL_8K_1, "8k1" },
	{ XS_LEVEL_8K_1, "bayer16k1" },
	{ XS_LEVEL_8K_1, "bayer16k-1" },
	{ XS_LEVEL_8K_2, "8k-2" },
	{ XS_LEVEL_8K_2, "8k2" },
	{ XS_LEVEL_8K_2, "bayer16k2" },
	{ XS_LEVEL_8K_2, "bayer16k-2" },
	{ XS_LEVEL_8K_3, "8k-3" },
	{ XS_LEVEL_8K_3, "8k3" },
	{ XS_LEVEL_8K_3, "bayer16k3" },
	{ XS_LEVEL_8K_3, "bayer16k-3" },
	{ XS_LEVEL_10K_1, "10k-1" },
	{ XS_LEVEL_10K_1, "10k1" },
	{ XS_LEVEL_10K_1, "bayer20k1" },
	{ XS_LEVEL_10K_1, "bayer20k-1" },
	{ 0, 0 },
};
#define XS_LEVELS_NAME_MAP_COUNT ((sizeof(XS_LEVELS_NAME_MAP) / sizeof(xs_map_entry_t)) - 1)

const xs_map_entry_t XS_SUBLEVELS_NAME_MAP[] = {
	{ XS_SUBLEVEL_UNRESTRICTED, "unrestricted" },
	{ XS_SUBLEVEL_FULL, "full" },
	{ XS_SUBLEVEL_12_BPP, "sublev12bpp" },
	{ XS_SUBLEVEL_12_BPP, "12" },
	{ XS_SUBLEVEL_12_BPP, "12bpp" },
	{ XS_SUBLEVEL_9_BPP, "sublev9bpp" },
	{ XS_SUBLEVEL_9_BPP, "9" },
	{ XS_SUBLEVEL_9_BPP, "9bpp" },
	{ XS_SUBLEVEL_6_BPP, "sublev6bpp" },
	{ XS_SUBLEVEL_6_BPP, "6" },
	{ XS_SUBLEVEL_6_BPP, "6bpp" },
	{ XS_SUBLEVEL_4_BPP, "sublev4bpp" },
	{ XS_SUBLEVEL_4_BPP, "4" },
	{ XS_SUBLEVEL_4_BPP, "4bpp" },
	{ XS_SUBLEVEL_3_BPP, "sublev3bpp" },
	{ XS_SUBLEVEL_3_BPP, "3" },
	{ XS_SUBLEVEL_3_BPP, "3bpp" },
	{ XS_SUBLEVEL_2_BPP, "sublev2bpp" },
	{ XS_SUBLEVEL_2_BPP, "2" },
	{ XS_SUBLEVEL_2_BPP, "2bpp" },
	{ 0, 0 },
};
#define XS_SUBLEVELS_NAME_MAP_COUNT ((sizeof(XS_SUBLEVELS_NAME_MAP) / sizeof(xs_map_entry_t)) - 1)

const xs_map_entry_t XS_CFA_NAME_MAP[] = {
	{ XS_CFA_RGGB, "rggb" },
	{ XS_CFA_BGGR, "bggr" },
	{ XS_CFA_GRBG, "grbg" },
	{ XS_CFA_GBRG, "gbgr" },
	{ 0, 0 },
};
#define XS_CFA_NAME_MAP_COUNT ((sizeof(XS_CFA_NAME_MAP) / sizeof(xs_map_entry_t)) - 1)

static const char* _XS_ERROR_NAME = "ERROR-ASSERT\0";

const char* xs_find_name_in_map(const xs_map_entry_t* map, const uint32_t raw)
{
	assert(map != 0);
	while (map->name != 0)
	{
		if (raw == map->raw)
		{
			return map->name;
		}
		++map;
	}
	assert(false);
	return _XS_ERROR_NAME;
}

uint32_t xs_find_raw_in_map(const xs_map_entry_t* map, const char* name)
{
	assert(map != 0);
	assert(name != 0);
	while (map->name != 0)
	{
		if (strcmp(name, map->name) == 0)
		{
			return map->raw;
		}
		++map;
	}
	return (uint32_t)-1;
}

char* xs_create_clean_cfg_str_(const char* config_str, const size_t l)
{
	// Make lowercase copy and remove all whitespace.
	char* cfg_str = (char*)malloc((l + 2) * sizeof(char));
	if (cfg_str == 0)
	{
		return 0;
	}
	memset(cfg_str, 0, (l + 2) * sizeof(char));
	char* cfg_str_p = cfg_str;
	bool last_was_semicolon = false;
	for (size_t i = 0; i < l; ++i)
	{
		const char ch = config_str[i];
		if (isblank(ch)) continue;
		if (isspace(ch) || ch == ';')
		{
			if (!last_was_semicolon)
			{
				// replace WS with ';'
				*cfg_str_p++ = ';';
			}
			last_was_semicolon = true;
			continue;
		}
		if (!isprint(ch)) continue;
		// output lowercase
		*cfg_str_p++ = tolower(ch);
		last_was_semicolon = false;
	}
	if (!last_was_semicolon)
	{
		*cfg_str_p++ = ';';
	}
	return cfg_str;
}

typedef bool (*handle_entry_func_t)(xs_config_t* cfg, const xs_image_t* im, char* entry_str);

bool xs_handle_entries_(xs_config_t* cfg, const xs_image_t* im, char* cfg_str, handle_entry_func_t proc)
{
	char* b = cfg_str;
	char* e = cfg_str;

	while (*b != 0)
	{
		while (*e != ';') ++e;
		*e = 0;
		if (!proc(cfg, im, b))
		{
			return false;
		}
		*e = ';';
		b = ++e;
	}

	return true;
}

bool xs_parse_u8array_(uint8_t* values, int max_items, char* cfg_str, int* num)
{
	for (int i = 0; max_items > 0; ++i, --max_items)
	{
		if (*cfg_str == 0)
		{
			return true;
		}
		if (num != 0)
		{
			*num = i;
		}
		values[i] = (uint8_t)atoi(cfg_str);
		while (*cfg_str != ',' && *cfg_str != 0) ++cfg_str;
		if (*cfg_str == ',') ++cfg_str;
	}
	return *cfg_str == 0;
}

bool xs_parse_u32array_(uint32_t* values, int max_items, char* cfg_str, int* num)
{
	for (int i = 0; max_items > 0; ++i, --max_items)
	{
		if (*cfg_str == 0)
		{
			return true;
		}
		if (num != NULL)
		{
			*num = i + 1;
		}
		values[i] = (uint32_t)atol(cfg_str);
		while (*cfg_str != ',' && *cfg_str != 0) ++cfg_str;
		if (*cfg_str == ',') ++cfg_str;
	}
	return *cfg_str == 0;
}

bool xs_starts_with_(char* pre, char* str)
{
	return strncmp(pre, str, strlen(pre)) == 0;
}

bool xs_parse_ivalue_(char* pre, char* str, int* v)
{
	if (strncmp(pre, str, strlen(pre)) == 0)
	{
		while (*str++ != '=');
		*v = atoi(str);
		return true;
	}
	return false;
}

bool xs_cfg_handle_profile_level_sublevel_(xs_config_t* cfg, const xs_image_t* im, char* entry_str)
{
	if (xs_starts_with_(KEY_PROFILE"=", entry_str) || xs_starts_with_("profile=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		if (cfg->profile != XS_PROFILE_AUTO)
		{
			fprintf(stderr, "Profile already specified\n");
			return false;
		}
		while (*entry_str++ != '=');
		const uint32_t raw = xs_find_raw_in_map(XS_PROFILES_NAME_MAP, entry_str);
		if (raw == (uint32_t)-1)
		{
			fprintf(stderr, "Unrecognized profile name '%s'\n", entry_str);
			return false;
		}
		cfg->profile = (xs_profile_t)raw;
	}
	else if (xs_starts_with_(KEY_LEVEL"=", entry_str) || xs_starts_with_("lev=", entry_str) || xs_starts_with_("level=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		if (cfg->level != XS_LEVEL_AUTO)
		{
			fprintf(stderr, "Level already specified\n");
			return false;
		}
		while (*entry_str++ != '=');
		const uint32_t raw = xs_find_raw_in_map(XS_LEVELS_NAME_MAP, entry_str);
		if (raw == (uint32_t)-1)
		{
			fprintf(stderr, "Unrecognized level '%s'\n", entry_str);
			return false;
		}
		cfg->level = (xs_level_t)raw;
	}
	else if (xs_starts_with_(KEY_SUBLEVEL"=", entry_str) || xs_starts_with_("sublev=", entry_str) || xs_starts_with_("sublevel=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		if (cfg->sublevel != XS_SUBLEVEL_AUTO)
		{
			fprintf(stderr, "Sublevel already specified\n");
			return false;
		}
		while (*entry_str++ != '=');
		const uint32_t raw = xs_find_raw_in_map(XS_SUBLEVELS_NAME_MAP, entry_str);
		if (raw == (uint32_t)-1)
		{
			fprintf(stderr, "Unrecognized sublevel '%s'\n", entry_str);
			return false;
		}
		cfg->sublevel = (xs_sublevel_t)raw;
	}
	return true;
}

bool xs_cfg_handle_ra_(xs_config_t* cfg, const xs_image_t* im, char* entry_str)
{
	if (xs_starts_with_(KEY_BITRATE"=", entry_str) || xs_starts_with_("bpp=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		if (cfg->bitstream_size_in_bytes != 0)
		{
			fprintf(stderr, "One of the target rate options already specified\n");
			return false;
		}
		while (*entry_str++ != '=');
		if (entry_str[0] == '-')
		{
			cfg->bitstream_size_in_bytes = (size_t)-1;
			return true;
		}
		const double r = atof(entry_str);
		if (r < 0.0625)
		{
			fprintf(stderr, "Invalid target bit rate\n");
			return false;
		}
		cfg->bitstream_size_in_bytes = (size_t)(r * im->width * im->height / 8);
	}
	else if (xs_starts_with_(KEY_SIZE"=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		if (cfg->bitstream_size_in_bytes != 0)
		{
			fprintf(stderr, "One of the target rate options already specified\n");
			return false;
		}
		while (*entry_str++ != '=');
		if (entry_str[0] == '-')
		{
			cfg->bitstream_size_in_bytes = (size_t)-1;
			return true;
		}
		const long s = atol(entry_str);
		if (s < 1024 && s != -1)
		{
			fprintf(stderr, "Target size is too small\n");
			return false;
		}
		cfg->bitstream_size_in_bytes = (size_t)s;
	}
	else if (xs_starts_with_("budget_report_lines=", entry_str) || xs_starts_with_(KEY_BUDGET_LINES"=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		while (*entry_str++ != '=');
		const float s = (float)atof(entry_str);
		if (s < 0 || s > 1024)
		{
			fprintf(stderr, "Budget report lines invalid\n");
			return false;
		}
		cfg->budget_report_lines = s;
	}
	return true;
}

bool xs_cfg_handle_mct_(xs_config_t* cfg, const xs_image_t* im, char* entry_str)
{
	bool set = false;
	if (xs_starts_with_(KEY_MCT"=", entry_str) || xs_starts_with_("mct=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		while (*entry_str++ != '=');
		if (strcmp("none", entry_str) == 0 || strcmp("0", entry_str) == 0)
		{
			cfg->p.color_transform = XS_CPIH_NONE;
			set = true;
		}
		else if (strcmp("rct", entry_str) == 0 || strcmp("1", entry_str) == 0)
		{
			cfg->p.color_transform = XS_CPIH_RCT;
			set = true;
		}
		else if (xs_starts_with_("tetrix,", entry_str) || strcmp("tetrix", entry_str) == 0 || xs_starts_with_("3,", entry_str) || strcmp("3", entry_str) == 0)
		{
			cfg->p.color_transform = XS_CPIH_TETRIX;
			uint8_t values[] = { 0, 0, 0, 0 };
			if (!xs_parse_u8array_(values, 4, entry_str, 0))
			{
				fprintf(stderr, "Too many arguments for Tetrix specified\n");
				return false;
			}
			cfg->p.tetrix_params.Cf = (xs_tetrix_t)values[1];
			cfg->p.tetrix_params.e1 = values[2];
			cfg->p.tetrix_params.e2 = values[3];
			set = true;
		}
		if (!set)
		{
			fprintf(stderr, "Invalid color transform specified\n");
			return false;
		}
	}
	else if (xs_starts_with_(KEY_CFA"=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		while (*entry_str++ != '=');
		const uint32_t cfa = xs_find_raw_in_map(XS_CFA_NAME_MAP, entry_str);
		if (cfa == (uint32_t)-1)
		{
			fprintf(stderr, "Invalid CFA pattern specified\n");
			return false;
		}
		cfg->p.cfa_pattern = (xs_cfa_pattern_t)cfa;
	}
	else if (xs_starts_with_(KEY_NLT"=", entry_str))
	{
		*entry_str = KEY_USED_CHAR;
		while (*entry_str++ != '=');
		if (strcmp("none", entry_str) == 0)
		{
			cfg->p.Tnlt = XS_NLT_NONE;
		}
		else if (xs_starts_with_("quadratic,", entry_str) || xs_starts_with_("q,", entry_str))
		{
			cfg->p.Tnlt = XS_NLT_QUADRATIC;
			uint32_t values[3];
			int num_values;
			if (!xs_parse_u32array_(values, 3, entry_str, &num_values))
			{
				fprintf(stderr, "Invalid amount of arguments for quadratic NLT (needs either (DCO), or (sigma, alpha))\n");
				return false;
			}
			if (num_values == 2)
			{
				int v = (int)values[1];
				if (v < 0)
				{
					cfg->p.Tnlt_params.quadratic.sigma = 1;
					cfg->p.Tnlt_params.quadratic.alpha = (uint16_t)(v + 32768);
				}
				else
				{
					cfg->p.Tnlt_params.quadratic.sigma = 0;
					cfg->p.Tnlt_params.quadratic.alpha = (uint16_t)v;
				}
			}
			else if (num_values == 3)
			{
				cfg->p.Tnlt_params.quadratic.sigma = (uint16_t)values[1];
				cfg->p.Tnlt_params.quadratic.alpha = (uint16_t)values[2];
			}
		}
		else if (xs_starts_with_("extended,", entry_str) || xs_starts_with_("e,", entry_str))
		{
			*entry_str = KEY_USED_CHAR;
			cfg->p.Tnlt = XS_NLT_EXTENDED;
			uint32_t values[4];
			int num_values;
			if (!xs_parse_u32array_(values, 4, entry_str, &num_values))
			{
				fprintf(stderr, "Invalid amount of arguments for extended NLT (needs either (Th1, Th2), or (E, T1, T2))\n");
				return false;
			}
			if (num_values == 3)
			{
				xs_config_nlt_extended_auto_thresholds(cfg, im->depth, values[1], values[2]);
			}
			else if (num_values == 4)
			{
				cfg->p.Tnlt_params.extended.E = (uint8_t)values[1];
				cfg->p.Tnlt_params.extended.T1 = values[2];
				cfg->p.Tnlt_params.extended.T2 = values[3];
			}
			else
			{
				fprintf(stderr, "Invalid amount of arguments for extended NLT (needs either (Th1, Th2), or (E, T1, T2))\n");
				return false;
			}
		}
		else
		{
			fprintf(stderr, "Invalid NLT value\n");
			return false;
		}
	}
	return true;
}

bool xs_cfg_handle_generic_(xs_config_t* cfg, const xs_image_t* im, char* entry_str)
{
	int value;
	char* entry_str_org = entry_str;
	if (xs_parse_ivalue_(KEY_CW"=", entry_str, &value))
	{
		cfg->p.Cw = (uint16_t)value;
	}
	else if (xs_parse_ivalue_(KEY_SLH"=", entry_str, &value))
	{
		cfg->p.slice_height = (uint16_t)value;
	}
	else if (xs_parse_ivalue_(KEY_BW"=", entry_str, &value))
	{
		cfg->p.Bw = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_FQ"=", entry_str, &value))
	{
		cfg->p.Fq = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_NLX"=", entry_str, &value))
	{
		cfg->p.NLx = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_NLY"=", entry_str, &value))
	{
		cfg->p.NLy = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_LH"=", entry_str, &value))
	{
		cfg->p.Lh = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_RL"=", entry_str, &value))
	{
		cfg->p.Rl = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_QUANT"=", entry_str, &value) || xs_parse_ivalue_("quant=", entry_str, &value))
	{
		cfg->p.Qpih = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_FS"=", entry_str, &value))
	{
		cfg->p.Fs = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_RM"=", entry_str, &value))
	{
		cfg->p.Rm = (uint8_t)value;
	}
	else if (xs_parse_ivalue_(KEY_SD"=", entry_str, &value))
	{
		cfg->p.Sd = (uint8_t)value;
	}
	else if (xs_starts_with_(KEY_GAINS"=", entry_str))
	{
		while (*entry_str++ != '=');
		memset(cfg->p.lvl_gains, 255, (MAX_NBANDS + 1) * sizeof(uint8_t));
		cfg->gains_mode = XS_GAINS_OPT_EXPLICIT;
		if (strcmp("psnr", entry_str) == 0)
		{
			cfg->gains_mode = XS_GAINS_OPT_PSNR;
		}
		else if (strcmp("visual", entry_str) == 0)
		{
			cfg->gains_mode = XS_GAINS_OPT_VISUAL;
		}
		else if (!xs_parse_u8array_(cfg->p.lvl_gains, MAX_NBANDS, entry_str, 0))
		{
			fprintf(stderr, "Too many level gains specified\n");
			return false;
		}
	}
	else if (xs_starts_with_(KEY_PRIORITIES"=", entry_str))
	{
		while (*entry_str++ != '=');
		memset(cfg->p.lvl_priorities, 255, (MAX_NBANDS + 1) * sizeof(uint8_t));
		if (!xs_parse_u8array_(cfg->p.lvl_priorities, MAX_NBANDS, entry_str, 0))
		{
			fprintf(stderr, "Too many level priorities specified\n");
			return false;
		}
	}
	else
	{
		return true;
	}

	*entry_str_org = KEY_USED_CHAR;
	return true;
}

bool xs_cfg_handle_not_used_(xs_config_t* cfg, const xs_image_t* im, char* entry_str)
{
	if (*entry_str != KEY_USED_CHAR)
	{
		fprintf(stderr, "Invalid XS config line (unknown option '%s')\n", entry_str);
		return false;
	}
	return true;
}

bool xs_config_parse_and_init(xs_config_t* cfg, const xs_image_t* im, const char* config_str, const size_t config_str_max_len)
{
	const size_t l = strnlen_s(config_str, config_str_max_len);
	if (l == 0)
	{
		// No config given, all auto.
		xs_config_init(cfg, im, XS_PROFILE_AUTO, XS_LEVEL_AUTO, XS_SUBLEVEL_AUTO);
		return true;
	}
	if (l >= config_str_max_len || config_str[l] != 0)
	{
		fprintf(stderr, "Invalid XS config line (length mismatch or truncated)\n");
		return false;
	}
	char* cfg_str = xs_create_clean_cfg_str_(config_str, l);
	if (cfg_str == 0)
	{
		fprintf(stderr, "Error handling XS config line\n");
		return false;
	}

	cfg->profile = XS_PROFILE_AUTO;
	cfg->level = XS_LEVEL_AUTO;
	cfg->sublevel = XS_SUBLEVEL_AUTO;

	bool succes = false;
	do
	{
		if (!xs_handle_entries_(cfg, im, cfg_str, xs_cfg_handle_profile_level_sublevel_)) break;
		xs_config_init(cfg, im, cfg->profile, cfg->level, cfg->sublevel);
		if (!xs_handle_entries_(cfg, im, cfg_str, xs_cfg_handle_ra_)) break;
		if (!xs_handle_entries_(cfg, im, cfg_str, xs_cfg_handle_generic_)) break;
		if (!xs_handle_entries_(cfg, im, cfg_str, xs_cfg_handle_mct_)) break;
		if (!xs_handle_entries_(cfg, im, cfg_str, xs_cfg_handle_not_used_)) break;
		succes = true;
	}
	while (false);

	free(cfg_str);
	return succes;
}

int xs_config_dump_lvl_values_array(char* config_str, size_t config_str_max_len, const uint8_t* defaults, const uint8_t* values, const char* key)
{
	bool equal = true;
	for (int i = 0; i < MAX_NBANDS + 1; ++i)
	{
		if (defaults[i] != values[i])
		{
			equal = false;
			break;
		}
		if (values[i] == 0xff) break;
	}

	int total_l = 0;
	if (!equal)
	{
		int l = snprintf(config_str, config_str_max_len, "%s=", key);
		if (l < 0) return -1; config_str += l; config_str_max_len -= l; total_l += l;
		for (int i = 0; i < MAX_NBANDS; ++i)
		{
			const uint8_t v = values[i];
			if (v == 0xff) break;
			l = snprintf(config_str, config_str_max_len, "%u,", v);
			if (l < 0) return -1; config_str += l; config_str_max_len -= l; total_l += l;
		}
		config_str[-1] = ';';
	}

	return total_l;
}

bool xs_config_dump(xs_config_t* cfg, const int im_depth, char* config_str, const size_t config_str_max_len, const int details)
{
	// Informational function to dump an XS configuration string that can be passed to the parser function.
	assert(cfg != 0);
	assert(config_str != 0);
	assert(config_str_max_len >= 128);
	config_str[0] = 0;

	char* p = config_str;
	int l = 0;

	assert(cfg->profile != XS_PROFILE_AUTO);

	const bool show_all = details > 2;

	// Generate a default configuration basis (to have the reference of default config settings).
	xs_config_t auto_cfg;
	xs_config_init(&auto_cfg, NULL, cfg->profile, cfg->level, cfg->sublevel);
	xs_config_resolve_bw_fq(&auto_cfg, cfg->p.Tnlt, im_depth);
	const xs_config_parameters_t* defaults = &auto_cfg.p;

	size_t config_str_space_left = config_str_max_len;

	l = snprintf(p, config_str_space_left, KEY_PROFILE"=%s;", xs_find_name_in_map(XS_PROFILES_NAME_MAP, cfg->profile));
	if (l < 0) return false; p += l; config_str_space_left -= l;
	l = snprintf(p, config_str_space_left, KEY_LEVEL"=%s;", xs_find_name_in_map(XS_LEVELS_NAME_MAP, cfg->level));
	if (l < 0) return false; p += l; config_str_space_left -= l;

	if (cfg->bitstream_size_in_bytes != (size_t)-1)
	{
		l = snprintf(p, config_str_space_left, KEY_SUBLEVEL"=%s;", xs_find_name_in_map(XS_SUBLEVELS_NAME_MAP, cfg->sublevel));
		if (l < 0) return false; p += l; config_str_space_left -= l;

		l = snprintf(p, config_str_space_left, KEY_SIZE"=%zu;", cfg->bitstream_size_in_bytes);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}

	//err |= (p += snprintf(p, config_str_max_len, KEY_BUDGET_LINES"=%f;", cfg->budget_report_lines)) < 0;
	//if (l < 0) return false; p += l; config_str_max_len -= l;

	if (cfg->p.color_transform == XS_CPIH_TETRIX)
	{
		if (show_all ||
			defaults->tetrix_params.Cf != cfg->p.tetrix_params.Cf ||
			defaults->tetrix_params.e1 != cfg->p.tetrix_params.e1 ||
			defaults->tetrix_params.e2 != cfg->p.tetrix_params.e2)
		{
			l = snprintf(p, config_str_space_left, KEY_MCT"=tetrix,%d,%d,%d;", cfg->p.tetrix_params.Cf, cfg->p.tetrix_params.e1, cfg->p.tetrix_params.e2);
			if (l < 0) return false; p += l; config_str_space_left -= l;
		}
	}
	else if (show_all || defaults->color_transform != cfg->p.color_transform)
	{
		l = snprintf(p, config_str_space_left, KEY_MCT"=%s;", (cfg->p.color_transform == XS_CPIH_RCT) ? "rct" : "none");
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}

	if (defaults->color_transform == XS_CPIH_TETRIX)
	{
		if (show_all || defaults->cfa_pattern != cfg->p.cfa_pattern)
		{
			l = snprintf(p, config_str_space_left, KEY_CFA"=%s;", xs_find_name_in_map(XS_CFA_NAME_MAP, cfg->p.cfa_pattern));
			if (l < 0) return false; p += l; config_str_space_left -= l;
		}

		if (show_all || defaults->Tnlt != cfg->p.Tnlt)
		{
			if (cfg->p.Tnlt == XS_NLT_QUADRATIC)
			{
				l = snprintf(p, config_str_space_left, KEY_NLT"=quadratic,%d,%d;", cfg->p.Tnlt_params.quadratic.sigma, cfg->p.Tnlt_params.quadratic.sigma);
				if (l < 0) return false; p += l; config_str_space_left -= l;
			}
			else if (cfg->p.Tnlt == XS_NLT_EXTENDED)
			{
				l = snprintf(p, config_str_space_left, KEY_NLT"=extended,%d,%u,%u;", cfg->p.Tnlt_params.extended.E, cfg->p.Tnlt_params.extended.T1, cfg->p.Tnlt_params.extended.T2);
				if (l < 0) return false; p += l; config_str_space_left -= l;
			}
		}
	}

	if (show_all || defaults->Cw != cfg->p.Cw)
	{
		l = snprintf(p, config_str_space_left, KEY_CW"=%d;", cfg->p.Cw);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->slice_height != cfg->p.slice_height)
	{
		l = snprintf(p, config_str_space_left, KEY_SLH"=%d;", cfg->p.slice_height);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Bw != cfg->p.Bw)
	{
		l = snprintf(p, config_str_space_left, KEY_BW"=%d;", cfg->p.Bw);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Fq != cfg->p.Fq)
	{
		l = snprintf(p, config_str_space_left, KEY_FQ"=%d;", cfg->p.Fq);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->NLx != cfg->p.NLx)
	{
		l = snprintf(p, config_str_space_left, KEY_NLX"=%d;", cfg->p.NLx);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->NLy != cfg->p.NLy)
	{
		l = snprintf(p, config_str_space_left, KEY_NLY"=%d;", cfg->p.NLy);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}

	if (show_all || defaults->Lh != cfg->p.Lh)
	{
		l = snprintf(p, config_str_space_left, KEY_LH"=%d;", cfg->p.Lh);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Rl != cfg->p.Rl)
	{
		l = snprintf(p, config_str_space_left, KEY_RL"=%d;", cfg->p.Rl);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Qpih != cfg->p.Qpih)
	{
		l = snprintf(p, config_str_space_left, KEY_QUANT"=%d;", cfg->p.Qpih);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Fs != cfg->p.Fs)
	{
		l = snprintf(p, config_str_space_left, KEY_FS"=%d;", cfg->p.Fs);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Rm != cfg->p.Rm)
	{
		l = snprintf(p, config_str_space_left, KEY_RM"=%d;", cfg->p.Rm);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}
	if (show_all || defaults->Sd != cfg->p.Sd)
	{
		l = snprintf(p, config_str_space_left, KEY_SD"=%d;", cfg->p.Sd);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}

	if (details > 1)
	{
		l = xs_config_dump_lvl_values_array(p, config_str_space_left, defaults->lvl_gains, cfg->p.lvl_gains, KEY_GAINS);
		if (l < 0) return false; p += l; config_str_space_left -= l;
		l = xs_config_dump_lvl_values_array(p, config_str_space_left, defaults->lvl_priorities, cfg->p.lvl_priorities, KEY_PRIORITIES);
		if (l < 0) return false; p += l; config_str_space_left -= l;
	}

	return true;
}
