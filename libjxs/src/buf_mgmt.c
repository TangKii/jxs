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

#include <malloc.h>
#include <string.h>

#include "buf_mgmt.h"

#define MB_MAGIC 0x12976569

multi_buf_t* multi_buf_create(int n_buffers, size_t element_size)
{
	multi_buf_t* ptr = (multi_buf_t*) malloc(sizeof(multi_buf_t));
	if (ptr)
	{
		ptr->magic = MB_MAGIC;
		ptr->n_buffers = n_buffers;
		ptr->element_size = element_size;
		ptr->bufs.s16 = (int16_t**) malloc(n_buffers*sizeof(int16_t*));
		ptr->sizes = (int*) malloc(n_buffers*sizeof(int));
		ptr->sizes_byte = (int*) malloc(n_buffers*sizeof(int));

		if ((!ptr->bufs.s16) || (!ptr->sizes) || (!ptr->sizes_byte))
		{
			multi_buf_destroy(ptr);
		}
		else
		{
			 
			memset(ptr->sizes, 0, n_buffers * sizeof(int));
			memset(ptr->sizes_byte, 0, n_buffers * sizeof(int));
			memset(ptr->bufs.s16, 0, n_buffers * sizeof(int16_t*));

			ptr->dump_mode = DEFAULT_DUMP_MODE;
			if (ptr->dump_mode == DUMP_MODE_NONE)
				ptr->fout = NULL;
			else
				multi_buf_enable_dump(ptr,ptr->dump_mode == DUMP_MODE_BIN);
		}
	}
	return ptr;
}

multi_buf_t* multi_buf_create_and_allocate(int n_buffers, int size, size_t element_size)
{
	int i;
	multi_buf_t* ptr = multi_buf_create(n_buffers, element_size);
	if (ptr)
	{
		for (i=0; i<n_buffers; i++)
			multi_buf_set_size(ptr,i, size);
		multi_buf_allocate(ptr,1);
	}
	return ptr;
}

int multi_buf_enable_dump(multi_buf_t* mb,int binary)
{
	int i;
	mb->fout = (FILE**) malloc(mb->n_buffers*sizeof(FILE*));
	if (mb->fout)
	{
		for (i=0; i<mb->n_buffers; i++)
			mb->fout[i] = NULL;
	
		if (binary)
			mb->dump_mode = DUMP_MODE_BIN;
		else
			mb->dump_mode = DUMP_MODE_TXT;
		return 0;
	}
	return -1;
}

int multi_buf_set_size(multi_buf_t* mb,int buf_idx, int size)
{
	mb->sizes[buf_idx] = size;
	mb->sizes_byte[buf_idx] = (int)(size*mb->element_size);
	return 0;
}

int multi_buf_reset(multi_buf_t* mb)
{
	int i;
	for (i=0; i<mb->n_buffers; i++)
		memset((void*) mb->bufs.u32[i], 0 , mb->sizes_byte[i]);
	return 0;
}

int multi_buf_allocate(multi_buf_t* mb,int alignment)
{
	int i;
	for (i=0; i<mb->n_buffers; i++)
	{
		mb->bufs.s16[i] = NULL;
		if (mb->sizes_byte[i] > 0)
		{
			mb->bufs.s16[i] = (int16_t*) malloc(mb->sizes_byte[i] + mb->element_size * (alignment - 1));
			if (!mb->bufs.s16[i])
				return -1;
		}
	}
	return 0;
}

int multi_buf_destroy(multi_buf_t* mb)
{
	if (mb)
	{
		int i;
		if (mb->magic != MB_MAGIC)
		{
			fprintf(stderr, "Error - memory corrupted!\n");
			return -1;
		}
		for (i=0; i<mb->n_buffers; i++)
			if (mb->sizes_byte && (mb->sizes_byte[i] > 0))
				free(mb->bufs.s16[i]);
		free(mb->bufs.s16);
		if (mb->sizes)
			free(mb->sizes);
		if (mb->sizes_byte)
			free(mb->sizes_byte);
		for (i=0; i<mb->n_buffers; i++)
			if ((mb->fout) && (mb->fout[i]))
				fclose(mb->fout[i]);
		if (mb->fout)
			free(mb->fout);
		free(mb);
	}
	return 0;
}
