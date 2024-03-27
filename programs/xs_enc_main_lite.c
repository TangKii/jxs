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

#include "libjxs.h"
#include "file_io.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LITE  1

#if LITE==0
#include "file_sequence.h"
#include "cmdline_options.h"
#include "image_open.h"
#else
#include "converters/planar.h"

typedef struct xs_profile_config1_t xs_profile_config1_t;
struct xs_profile_config1_t
{
	xs_profile_t profile;
	xs_config_parameters_t parameters;

	float budget_report_lines;
	int Nbpp_at_sublev_full;
	int Nsbu;
	int Ssbo;
	uint16_t max_column_width;
};

const xs_profile_config1_t XS_PROFILES1 = {
XS_PROFILE_MAIN_444_12,          { XS_CPIH_AUTO,   0, 16, 4, 8,   20,    8, 4, 0, 0, 5, 1, 0, 0, 1, 0, 1, 0, { 0xff }, { 0xff }, XS_NLT_NONE, { 0 }, { XS_TETRIX_FULL, 0, 0 }, XS_CFA_NONE },  6,  36, 16, 1024,    0 
};

#endif

unsigned char ctx_data[4088];

int main(int argc, char **argv)
{
	xs_config_t xs_config;
	xs_image_t image = { 0 };
	char* input_seq_n = NULL;
	char* output_seq_n = NULL;
	char* input_fn = NULL;
	char* output_fn = NULL;
	uint8_t* bitstream_buf = NULL;
	size_t bitstream_buf_size, bitstream_buf_max_size;
	xs_enc_context_t* ctx = NULL;
	int ret = 0;
#if LITE == 0
	int file_idx = 0;
	cmdline_options_t options;
	int optind;
#endif
	fprintf(stderr, "JPEG XS test model (XSM) version %s\n", xs_get_version_str());
	do
	{
#if LITE==0
		if ((optind = cmdline_options_parse(argc, argv, CMDLINE_OPT_ENCODER, &options)) < 0)
		{
			ret = -1;
			break;
		}

		if (argc - optind != 2)
		{
			fprintf(stderr, "\nSingle image: %s [options] <image_in.ext> <output.jxs>\n", argv[0]);
			fprintf(stderr, "\nFor sequences: %s [options] <sequence_in_%%06d.dpx> <output_%%06d.jxs>\n\n", argv[0]);
			fprintf(stderr, "Options:\n");
			cmdline_options_print_usage(CMDLINE_OPT_ENCODER);
			ret = -1;
			break;
		}

#else
		// options.width = 768;
		// options.height = 1600;
		// options.depth = -1;
		// options.sequence_first = 0;
		// options.sequence_n = -1;
		// options.verbose = 0;
		// options.dump_xs_cfg = 0;
		// options.verbose = 1;
#endif		

#if LITE==0
		input_seq_n = argv[optind];
		output_seq_n = argv[optind + 1];

		input_fn = malloc(strlen(input_seq_n) + MAX_SEQ_NUMBER_DIGITS);
		output_fn = malloc(strlen(output_seq_n) + MAX_SEQ_NUMBER_DIGITS);

		sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
		if (image_open_auto(input_fn, &image, options.width, options.height, options.depth) < 0)
		{
			fprintf(stderr, "Unable to open image %s!\n", input_fn);
			ret = -1;
			break;
		}
#else
		image.width = 768;
		image.height = 1600;
		image.depth = -1;
		if(yuv_planar_decode("input.yuv", &image) < 0)
		{
			fprintf(stderr, "Unable to open image %s!\n", input_fn);
			ret = -1;
			break;
		}
#endif

#if LITE == 0
		if (!xs_config_parse_and_init(&xs_config, &image, options.xs_config_string, sizeof(options.xs_config_string) - 1))
		{
			fprintf(stderr, "Error in XS config\n");
			ret = -1;
			break;
		}
#else
		memset(&xs_config, 0, sizeof(xs_config_t));
		xs_config.verbose = 1;
		xs_config.gains_mode = XS_GAINS_OPT_PSNR;
		xs_config.profile = XS_PROFILE_MAIN_444_12;
		xs_config.level = XS_LEVEL_1K_1;
		xs_config.sublevel = XS_SUBLEVEL_2_BPP;
		xs_config.cap_bits = XS_CAP_AUTO;
		xs_config.bitstream_size_in_bytes = 2* image.width * image.height / 8;
		
		xs_config.p = XS_PROFILES1.parameters;
		xs_config.budget_report_lines = XS_PROFILES1.budget_report_lines;
		
#endif

#if LITE == 0
		if (!xs_enc_preprocess_image(&xs_config, &image))
		{
			fprintf(stderr, "Error preprocessing the image\n");
			ret = -1;
			break;
		}
#endif
#if LITE == 1
		ctx = xs_enc_init(&xs_config, &image);
		//fileio_write("ctx.bin", (uint8_t *)ctx, 4088);
		
#else
		xs_config.verbose = 3;
		image.width = 768;
		image.height = 1600;
		//image.depth = -1;
		size_t readsize = 0;
		fileio_read("ctx.bin", ctx_data, &readsize, 4088);
		ctx = (xs_enc_context_t *)ctx_data;

		//fprintf(stderr, "ctx %d\n", ctx->xs_config->bitstream_size_in_bytes);
		fprintf(stderr, "ctx %d\n", *(uint8_t *)ctx);
		fprintf(stderr, "ctx %d\n", *((uint8_t *)(ctx) + 1));
#endif
		if (!ctx)
		{
			fprintf(stderr, "Unable to allocate encoding context\n");
			ret = -1;
			break;
		}

		if (xs_config.bitstream_size_in_bytes == (size_t)-1)
		{
			// Take the RAW image size and add some extra for margin.
			bitstream_buf_max_size = image.width * image.height * image.ncomps * ((image.depth + 7) >> 3) + 1024 * 1024;
		}
		else
		{
			bitstream_buf_max_size = (xs_config.bitstream_size_in_bytes + 7) & (~0x7);
		}
		bitstream_buf = malloc(bitstream_buf_max_size);
		if (!bitstream_buf)
		{
			fprintf(stderr, "Unable to allocate codestream mem\n");
			ret = -1;
			break;
		}
#if LITE == 0
		//if (options.dump_xs_cfg > 0)
		{
			char dump_str[1024];
			memset(dump_str, 0, 1024);
			if (!xs_config_dump(&xs_config, image.depth, dump_str, 1024, options.dump_xs_cfg))
			{
				fprintf(stderr, "Unable to dump configuration for codestream (%s)\n", input_fn);
				ret = -1;
				break;
			}
			fprintf(stdout, "Configuration: \"%s\"\n", dump_str);
		}
#else
			// char dump_str[1024];
			// memset(dump_str, 0, 1024);
			// if (!xs_config_dump(&xs_config, image.depth, dump_str, 1024, 2))
			// {
			// 	fprintf(stderr, "Unable to dump configuration for codestream (%s)\n", input_fn);
			// 	ret = -1;
			// 	break;
			// }
			// fprintf(stdout, "Configuration: \"%s\"\n", dump_str);
#endif
		do
		{
			#if LITE == 0
			sequence_get_filepath(output_seq_n, output_fn, file_idx + options.sequence_first);
			if (!fileio_writable(output_fn))
			{
				fprintf(stderr, "Output file is not writable %s\n", output_fn);
				ret = -1;
				break;
			}
			#endif
			if (!xs_enc_image(ctx, &image, (uint8_t*)bitstream_buf, bitstream_buf_max_size, &bitstream_buf_size))
			{
				fprintf(stderr, "Unable to encode image %s\n", input_fn);
				ret = -1;
				break;
			}
			xs_free_image(&image);
			#if LITE == 0
			if (is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n))
			{
				fprintf(stderr, "writing %s\n", output_fn);
			}
			if (fileio_write(output_fn, bitstream_buf, bitstream_buf_size) < 0)
			{
				fprintf(stderr, "Unable to write output encoded codestream to %s\n", output_fn);
				ret = -1;
				break;
			}

			if (!(is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n)))
			{
				break;
			}
			file_idx++;
			if ((options.sequence_n > 0) && (file_idx >= options.sequence_n))
			{
				break;
			}
			sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
			if (!fileio_exists(input_fn))
			{
				break;
			}
			if ((ret = image_open_auto(input_fn, &image, options.width, options.height, options.depth)) < 0)
			{
				fprintf(stderr, "Unable to open image %s!\n", input_fn);
				ret = -1;
				break;
			}
			if (!xs_enc_preprocess_image(&xs_config, &image))
			{
				fprintf(stderr, "Error preprocessing the image\n");
				ret = -1;
				break;
			}
			#else
			if (fileio_write("output.jxs", bitstream_buf, bitstream_buf_size) < 0)
			{
				fprintf(stderr, "Unable to write output encoded codestream to %s\n", output_fn);
				ret = -1;
				break;
			}
			#endif
		} while (ret == 0);
	} while (false);

	// Cleanup.
	if (input_fn)
	{
		free(input_fn);
	}
	if (output_fn)
	{
		free(output_fn);
	}
	xs_free_image(&image);
	if (ctx)
	{
		xs_enc_close(ctx);
	}
	if (bitstream_buf)
	{
		free(bitstream_buf);
	}
	return ret;
}
