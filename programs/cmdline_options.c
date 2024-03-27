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
#include <stdint.h>
#include <string.h>

#include "getopt.h"
#include "libjxs.h"

#include "cmdline_options.h"

#ifndef _MSC_VER
static inline int strcat_s(char* restrict dest, size_t destsz, const char* restrict src)
{
	if (dest == 0 || src == 0)
	{
		return -1;
	}
	strcat(dest, src);
	return 0;
}
#endif

const char* active_options[] = {
	  "c:Dw:h:d:f:n:v",
	  "Df:n:vF:",
};

struct
{
	char letter;
	char* usage_msg;
} options_help[] = {
	{'c', "-c <string>: XS configuration arguments (semicolon separated, can be specified multiple times)\n"},
	{'D', "-D: Print out actual XS configuration string (specify twice to also show gains/priorities)\n"},

	{'w', "-w <width>: image dimension width (required for raw input format like v210, yuv16, rgb16)\n"},
	{'h', "-h <height>: image dimension height (required for raw input format like v210, yuv16, rgb16)\n"},
	{'d', "-d depth image depth (required for rgb16/yuv16 input format)\n"},

	{'f', "-f <nb>: index of first image of sequence (N/A for single file operation)\n"},
	{'n', "-n <nb>: number of images from sequence to process (N/A for single file operation)\n"},
	{'v', "-v: verbose information (use multiple times to increase verbosity)\n"},

	{'F', "-F <fragments_csv>: write out codestream fragment information into a CSV\n"},

	{0, NULL},
};

#define XS_CONFIG_STR \
"\n" \
"The generic syntax of the XS configuration is always as follows:\n" \
"\n" \
"  - Each option is specified as a 'key=value' pair. The key is an alpha-numeric string.\n" \
"  - Options are separated by semi-colons (';'). Multiple '-c' arguments are internally concatenated\n" \
"    to produce a single XS configuration line comprised of key/value pairs.\n" \
"  - Values can be either text or number (integral/floating-point), but a value is never the empty string.\n" \
"  - The XS configuration line is case insensitive (everything is converted to lower case).\n" \
"  - Whitespaces are ignored.\n" \
"  - By not specifying an XS option, the encoder will resolve to either a default value (as defined\n" \
"    by the profile) or an automated best-effort guess.\n" \
"\n" \
"The following XS configuration keys are defined:\n" \
"\n" \
"  - 'p' or 'profile': The profile to use (by name). See ISO/IEC 21122-2.\n" \
"  - 'l' or 'lev' or 'level': The level to use (by name). See ISO/IEC 21122-2.\n" \
"  - 's' or 'sublev' or 'sublevel': The sublevel to use (by name). See ISO/IEC 21122-2.\n" \
"  - 'rate' or 'bpp': Specify a target bit-rate (in bpp). Cannot be combined with 'size'.\n" \
"  - 'size': Specify a target codestream size (in bytes). Cannot be combined with 'rate'.\n" \
"  - 'budget_lines' or 'budget_report_lines': The budget report buffer size (in lines)\n" \
"                                             for the encoder's rate allocation.\n" \
"  - 'cpih' or 'mct': Select the multiple component transform. Valid options are\n"\
"                     'none' or '0',\n" \
"                     'rct' or '1', and\n" \
"                     'tetrix,<Cf>,<e1>,<e2>' or '3,<Cf>,<e1>,<e2>'.\n" \
"  - 'cfa': In case of 1-component input with one of the Bayer profiles, a CFA pattern\n" \
"           needs to be specified. Supported patterns are 'RGGB', 'BGGR', 'GRBG', and 'GBGR'.\n" \
"  - 'nlt': Non-linear transform. Additional settings are required, depending on the chosen NLT.\n" \
"           Valid NLT's are: 'none', 'quadratic,<sigma>,<alpha>', 'extended,<theta1>,<theta2>', and\n" \
"           'extended, <E>,<T1>,<T2>'.\n" \
"  - 'cw': Precinct column width in multiples of 8. 'Cw=0' represents full-width.\n" \
"  - 'slh': Slice height (in lines).\n" \
"  - 'bw': Nominal bit precision of the wavelet coefficients (restricted to input-bit-depth, '18', or '20').\n" \
"  - 'fq': Number of fractional bits of the wavelet coefficients (restricted to '8', '6', or '0').\n" \
"  - 'nlx': Number of horizontal wavelet decompositions.\n" \
"  - 'nly': Number of vertical wavelet decompositions (must be smaller than or equal to NLX).\n" \
"  - 'lh': Long precinct header enforcement flag ('0' or '1').\n" \
"  - 'rl': Raw - mode selection per packet flag ('0' or '1').\n" \
"  - 'qpih' or 'quant': Quantization type (use '0' for dead - zone quantization, and '1' for uniform quantization).\n" \
"  - 'fs': Sign handling strategy (use '0' for jointly, and '1' for separate packets).\n" \
"  - 'rm': Run mode (use '0' so runs indicate zero prediction residuals, and\n" \
"          '1' so runs indicate zero coefficients).\n" \
"  - 'sd': Number of components for which the wavelet decomposition is suppressed.\n" \
"  - 'gains': Comma separated list of gain values(one value per band, which depends on number of\n" \
"             components, subsamnpling and NLX / NLY). Alternatively, the encoder has some built-in\n" \
"             tables for PSNR and Visual optimized compression. Specify either 'psnr' or 'visual'\n" \
"             to select usage of built-in gains/priorities.\n" \
"  - 'priorities': Comma separated list of priority values (same amount as gains).\n\n" \
"\n" \
"The following profiles are valid:\n" \
"\n" \
"  - Unrestricted\n" \
"  - Main420.12\n" \
"  - Main422.10\n" \
"  - Main444.12\n" \
"  - Main4444.12\n" \
"  - Light422.10\n" \
"  - Light444.12\n" \
"  - Light-Subline422.10\n" \
"  - High444.12\n" \
"  - High4444.12\n" \
"  - MLS.12\n" \
"  - LightBayer\n" \
"  - MainBayer\n" \
"  - HighBayer\n" \
"\n" \
"The following levels are valid:\n" \
"\n" \
"  - Unrestricted\n" \
"  - 1k-1, Bayer2k-1\n" \
"  - 2k-1, Bayer4k-1\n" \
"  - 4k-1, Bayer8k-1\n" \
"  - 4k-2, Bayer8k-2\n" \
"  - 4k-3, Bayer8k-3\n" \
"  - 8k-1, Bayer16k-1\n" \
"  - 8k-2, Bayer16k-2\n" \
"  - 8k-3, Bayer16k-3\n" \
"  - 10k-1, Bayer20k-1\n" \
"\n" \
"The following sublevels are valid:\n" \
"\n" \
"  - Unrestricted\n" \
"  - Full\n" \
"  - Sublev12bpp\n" \
"  - Sublev9bpp\n" \
"  - Sublev6bpp\n" \
"  - Sublev3bpp\n" \
"  - Sublev2bpp\n" \
"\n"

JXS_SHARED_LIB_API int cmdline_options_parse(int argc, char **argv, cmdline_opt_app_t program_idx, cmdline_options_t* options)
{
	int opt;

	memset(options->xs_config_string, 0, sizeof(options->xs_config_string));
	options->width = -1;
	options->height = -1;
	options->depth = -1;
	options->sequence_first = 0;
	options->sequence_n = -1;
	options->verbose = 0;
	options->dump_xs_cfg = 0;
	memset(options->fragments_csv_file, 0, sizeof(options->fragments_csv_file));

	while ((opt = getopt(argc, argv, active_options[program_idx])) != -1)
	{
		switch(opt)
		{
		case BADARG:
		case BADCH:
		{
			return -1;
		}
		case 'c':
		{
			int ok = strcat_s(options->xs_config_string, sizeof(options->xs_config_string) - 2, optarg);
			ok |= strcat_s(options->xs_config_string, sizeof(options->xs_config_string) - 1, ";");
			if (ok != 0)
			{
				fprintf(stderr, "Error while handling -c option\n");
				return -1;
			}
			break;
		}
		case 'D':
		{
			++options->dump_xs_cfg;
			break;
		}
		case 'w':
		{
			options->width = atoi(optarg);
			break;
		}
		case 'h':
		{
			options->height = atoi(optarg);
			break;
		}
		case 'd':
		{
			options->depth = atoi(optarg);
			break;
		}
		case 'f':
		{
			options->sequence_first = atoi(optarg);
			break;
		}
		case 'n':
		{
			options->sequence_n = atoi(optarg);
			break;
		}
		case 'v':
		{
			++options->verbose;
			break;
		}
		case 'F':
		{
			strncpy(options->fragments_csv_file, optarg, sizeof(options->fragments_csv_file) - 1);
			break;
		}
		}
	}
	return optind;
}

JXS_SHARED_LIB_API int cmdline_options_print_usage(cmdline_opt_app_t program_idx)
{
	for (int i = 0; i < strlen(active_options[program_idx]); i++)
	{
		int j = 0;
		while (options_help[j].letter != 0)
		{
			if (options_help[j].letter == active_options[program_idx][i])
			{
				fprintf(stderr, "%s", options_help[j].usage_msg);
			}
			j++;
		}
	}

	// print XS configuration help
	if (program_idx == CMDLINE_OPT_ENCODER)
	{
		fprintf(stderr, XS_CONFIG_STR);
	}

	return 0;
}
