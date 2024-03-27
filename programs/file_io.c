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

#include <sys/stat.h>
#include "file_io.h"


JXS_SHARED_LIB_API size_t fileio_getsize(const char* filepath)
{

	FILE* file_in = fopen(filepath, "rb");
	size_t ret = 0;
	if (!file_in)
		return 0;

	fseek(file_in, 0, SEEK_END);
	ret = ftell(file_in);
	fseek(file_in, 0, SEEK_SET);
	fclose(file_in);
	return ret;
}

JXS_SHARED_LIB_API int fileio_read(const char* filepath, uint8_t* buf, size_t* buf_len, size_t max_buf_len)
{
	size_t size = fileio_getsize(filepath);
	size_t nread;
	if ((size > 0) && size <= max_buf_len)
	{
		FILE* file_in = fopen(filepath, "rb");
		nread = fread(buf, 1, size, file_in);
		if (nread != size)
			fprintf(stderr, "Error reading input file (expected %zu read %zu)\n", size, nread);
		*buf_len = size;
		fclose(file_in);
		return 0;
	}
	return -1;
}

JXS_SHARED_LIB_API int fileio_write(const char* filepath, uint8_t* buf, size_t buf_len)
{
	FILE* file_out = fopen(filepath, "wb");
	if (file_out)
	{
		fwrite(buf, buf_len, 1, file_out);
		fclose(file_out);
		return 0;
	}
	return -1;
}

JXS_SHARED_LIB_API int fileio_exists(const char* filepath)
{
	struct stat   buffer;
	return (stat(filepath, &buffer) == 0);
}

JXS_SHARED_LIB_API int fileio_writable(const char* filepath)
{
	int ret = 0;


	uint8_t test[] = "a";
	if (fileio_write(filepath, test, 1) == 0)
	{
		ret = 1;
		remove(filepath);
	}
	return ret;
}
