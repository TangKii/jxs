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
#include "image_open.h"
#include "cmdline_options.h"
#include "file_io.h"
#include "file_sequence.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fragment_info_csv_writer(void* ctx, const int f_id, const int f_Sbits, const int f_Ncg, const int f_padding_bits)
{
	assert(f_id >= 0);
	fprintf((FILE*)ctx, "%d;%d;%d;%d\n", f_id, f_Sbits, f_Ncg, f_padding_bits);
}

int main(int argc, char **argv)
{
	xs_config_t xs_config;
	char* input_seq_n = NULL;
	char* output_seq_n = NULL;
	char* input_fn = NULL;
	char* output_fn = NULL;
	xs_image_t image = { 0 };
	uint8_t* bitstream_buf = NULL;
	size_t bitstream_buf_size, bitstream_buf_max_size;
	xs_dec_context_t* ctx = NULL;
	int ret = 0;
	int file_idx = 0;
	cmdline_options_t options;
	int optind;
	FILE* fragment_csv_fh = NULL;

	fprintf(stderr, "JPEG XS test model (XSM) version %s\n", xs_get_version_str());

	do
	{
		if ((optind = cmdline_options_parse(argc, argv, CMDLINE_OPT_DECODER, &options)) < 0)
		{
			ret = -1;
			break;
		}

		xs_config.verbose = options.verbose;

		if (argc - optind == 1)
		{
		}
		else if (argc - optind == 2)
		{
			output_seq_n = argv[optind + 1];
			output_fn = malloc(strlen(output_seq_n) + MAX_SEQ_NUMBER_DIGITS);
		}
		else
		{
			fprintf(stderr, "\nShow image XS configuration: %s -D [options] <input.jxs>\n", argv[0]);
			fprintf(stderr, "\nSingle image: %s [options] <input.jxs> <image_out.ext>\n", argv[0]);
			fprintf(stderr, "\nFor sequences: %s [options] <output_%%06d.jxs> <sequence_out_%%06d.dpx>\n\n", argv[0]);
			fprintf(stderr, "Options:\n");
			cmdline_options_print_usage(CMDLINE_OPT_DECODER);
			ret = -1;
			break;
		}

		input_seq_n = argv[optind];
		input_fn = malloc(strlen(input_seq_n) + MAX_SEQ_NUMBER_DIGITS);

		sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
		bitstream_buf_max_size = fileio_getsize(input_fn);
		if (bitstream_buf_max_size <= 0)
		{
			fprintf(stderr, "Unable to open file %s\n", input_seq_n);
			ret = -1;
			break;
		}

		bitstream_buf = (uint8_t*)malloc((bitstream_buf_max_size + 8) & (~0x7));
		if (!bitstream_buf)
		{
			fprintf(stderr, "Unable to allocate memory for codestream\n");
			ret = -1;
			break;
		}
		fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size);

		if (!xs_dec_probe(bitstream_buf, bitstream_buf_size, &xs_config, &image))
		{
			fprintf(stderr, "Unable to parse input codestream (%s)\n", input_fn);
			ret = -1;
			break;
		}

		if (options.verbose > 0 || options.dump_xs_cfg > 0)
		{
			fprintf(stdout, "Image: %dx%d %d-comp@%dbpp\n", image.width, image.height, image.ncomps, image.depth);
			fprintf(stdout, "Sampling: %dx%d", image.sx[0], image.sy[0]);
			for (int c = 1; c < image.ncomps; ++c)
			{
				fprintf(stdout, "/%dx%d", image.sx[c], image.sy[c]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "Format: %s\n", image_find_format_str(&image, &xs_config));
		}

		if (options.dump_xs_cfg > 0)
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

		if (argc - optind == 1)
		{
			// No output file given.
			fprintf(stderr, "Not decoding, no output file given\n");
			break;
		}

		if (!xs_allocate_image(&image, false))
		{
			fprintf(stderr, "Image memory allocation error\n");
			ret = -1;
			break;
		}

		ctx = xs_dec_init(&xs_config, &image);
		if (!ctx)
		{
			fprintf(stderr, "Error parsing the codestream\n");
			ret = -1;
			break;
		}

		if (options.fragments_csv_file[0] != 0)
		{
			if (is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n))
			{
				fprintf(stderr, "Fragment CSV output is not supported with sequences\n");
				ret = -1;
				break;
			}

			// Enable fragments information dumping into CSV for buffer model checking.
			fragment_csv_fh = fopen(options.fragments_csv_file, "wb");
			if (fragment_csv_fh == NULL)
			{
				fprintf(stderr, "Error creating fragments CSV output\n");
				ret = -1;
				break;
			}

			if (!xs_dec_set_fragment_info_cb(ctx, fragment_info_csv_writer, fragment_csv_fh))
			{
				fprintf(stderr, "Error handling fragments CSV output\n");
				ret = -1;
				break;
			}
		}

		do
		{
			if (!xs_dec_bitstream(ctx, bitstream_buf, bitstream_buf_max_size, &image))
			{
				fprintf(stderr, "Error while decoding the codestream\n");
				ret = -1;
				break;
			}
			if (!xs_dec_postprocess_image(&xs_config, &image))
			{
				fprintf(stderr, "Error postprocessing the image\n");
				ret = -1;
				break;
			}

			sequence_get_filepath(output_seq_n, output_fn, file_idx + options.sequence_first);
			if (is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n))
			{
				fprintf(stderr, "writing %s\n", output_fn);
			}

			if (image_save_auto(output_fn, &image) < 0)
			{
				ret = -1;
				break;
			}

			if (!(is_sequence_path(input_seq_n) || is_sequence_path(output_seq_n)))
			{
				break;
			}

			file_idx++;
			sequence_get_filepath(input_seq_n, input_fn, file_idx + options.sequence_first);
		} while (ret == 0 && fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size) >= 0);
	} while (false);

	// Cleanup.
	if (fragment_csv_fh)
	{
		fclose(fragment_csv_fh);
	}
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
		xs_dec_close(ctx);
	}
	if (bitstream_buf)
	{
		free(bitstream_buf);
	}
	return ret;
}
