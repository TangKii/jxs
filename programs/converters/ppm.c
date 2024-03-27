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
#include <string.h>
#include "ppm.h"
#include "helpers.h"

#ifdef _MSC_VER
#include <intrin.h>
static unsigned int __inline BSR(unsigned long x)
{
	unsigned long r = 0;
	_BitScanReverse(&r, x);
	return r;
}
#else
#define BSR(x) (31 - __builtin_clz(x))
#endif

static int SkipBlanks(FILE *fp)
{
	int ch;

	do
	{
		ch = fgetc(fp);
	} while(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

	return ungetc(ch,fp);
}

static int ReadNumber(FILE *fp,int *result)
{
	int number   = 0;
	int negative = 0;
	int valid    = 0;
	int in;

	if (SkipBlanks(fp) == EOF)
		return 0;

	in = fgetc(fp);
	if (in == '+')
	{

	} else if (in == '-')
	{

		negative = 1;
	} else {

		if (ungetc(in,fp) == EOF)
			return 0;
	}

	if (SkipBlanks(fp) == EOF)
		return 0;

	do
	{
		in = fgetc(fp);
		if (in < '0' || in > '9')
			break;

		if (number >= 214748364)
		{
			fprintf(stderr,"Invalid number in PPM file, number %d too large.\n",number);
			return 0;
		}

		number = number * 10 + in - '0';
		valid  = 1;
	} while(1);

	if (ungetc(in,fp) == EOF)
		return 0;

	if (!valid)
	{
		fprintf(stderr,"Invalid number in PPM file, found no valid digit. First digit code is %d\n",in);
		return 0;
	}

	if (negative)
		number = -number;

	*result = number;
	return 1;
}


static int SkipComment(FILE *fp)
{
	int c;

	do
	{
		c = fgetc(fp);

	} while(c == ' ' || c == '\t' || c == '\r');


  if (c == '\n')
  {

	  do
	  {
		  c = fgetc(fp);
		  if (c == '#')
		  {

			  do
			  {
				  c = fgetc(fp);
			  } while(c != '\n' && c != '\r' && c != -1);
		  } else {

			  if (ungetc(c,fp) == EOF)
				  return EOF;
			  break;
		  }
	  } while(1);
  }
  else
  {
	  return ungetc(c,fp);
  }
  return 0;
}


int ppm_decode(char *filename, xs_image_t *im)
{
	int data;
	int result     = -1;
	int precision  = 0;
	int components = 0;
	int width      = 0;
	int height     = 0;
	int raw        = 1;
	xs_data_in_t* p[3]  = {NULL};
	int x,y,c;
	FILE *fp = fopen(filename,"rb");

	if (fp == NULL)
	{
		perror("unable to open the source image stream");
		return -1;
	}

	data = fgetc(fp);
	if (data != 'P')
	{
		fprintf(stderr,"input stream %s is not a valid PPM file\n",filename);
		goto exit;
	}

	data      = fgetc(fp);
	precision = 0;
	switch(data)
	{
	case '6':

		components = 3;
		break;
	case '3':

		components = 3;
		raw        = 0;
		break;
	case '5':

		components = 1;
		break;
	case '2':
		components = 1;
		raw        = 0;
		break;
	case '4':

		components = 1;
		precision  = 1;
		break;
	case '1':

		components = 1;
		precision  = 1;
		raw        = 0;
		break;
	case 'F':
	case 'f':

		fprintf(stderr,"floating point input image formats for %s are currently not supported.\n",filename);
		goto exit;
		break;
	default:

		fprintf(stderr,"input stream %s is not a valid PPM file\n",filename);
		goto exit;
	}


	if (SkipComment(fp) == EOF)
		goto exit;
	if (ReadNumber(fp,&width) == 0)
		goto exit;
	if (SkipComment(fp) == EOF)
		goto exit;
	if (ReadNumber(fp,&height) == 0)
		goto exit;

	if (precision == 0)
	{
		if (SkipComment(fp) == EOF)
			goto exit;
		if (ReadNumber(fp,&precision) == 0)
			goto exit;
	}


	im->ncomps = components;
	im->width = width;
	im->height = height;
	im->sx[0] = im->sx[1] = im->sx[2] = 1;
	im->sy[0] = im->sy[1] = im->sy[2] = 1;

	if (im->depth == -1)
		im->depth = BSR(precision)+1;

	if (!xs_allocate_image(im, false))
		goto exit;

	for(c = 0;c < components;c++)
		p[c] = im->comps_array[c];

	data = fgetc(fp);

	if (data == '\r')
	{
		data = fgetc(fp);
		if (data != '\n')
		{

			if (ungetc(data,fp) == EOF)
				goto exit;
			data = '\r';
		}
	}

	if (data != ' ' && data != '\n' && data != '\r' && data != '\t')
	{
		fprintf(stderr,"Malformed PPM stream in %s.\n",filename);
		goto exit;
	}

	if (raw)
	{

		if (precision == 1)
		{
			for (y = 0; y < height; y++)
			{
				int mask = 0x00;
				for(x = 0;x < width;x++)
				{

					if (mask == 0)
					{
						data = fgetc(fp);
						if (data == EOF)
						{
							fprintf(stderr,"Unexpected EOF in PPM stream %s.\n",filename);
							goto exit;
						}
						mask = 0x80;
					}
					*p[0]++ = (data & mask)?0:1;
					mask  >>= 1;
				}
			}
		}
		else if (precision < 256)
		{

			for (y = 0; y < height; y++)
			{
				for (x = 0;x < width; x++)
				{
					for (c = 0;c < components;c++)
					{
						data = fgetc(fp);
						if (data == EOF)
						{
							fprintf(stderr,"Unexpected EOF in PPM stream %s.\n",filename);
							goto exit;
						}
						*p[c]++ = data;
					}
				}
			}
		}
		else if (precision < 65536)
		{

			for (y = 0; y < height; y++)
			{
				for (x = 0;x < width; x++)
				{
					for (c = 0;c < components;c++)
					{
						int lo,hi;
						hi = fgetc(fp);
						lo = fgetc(fp);
						if (lo == EOF || hi == EOF)
						{
							fprintf(stderr,"Unexpected EOF in PPM stream %s.\n",filename);
							goto exit;
						}
						*p[c]++ = lo | (hi << 8);
					}
				}
			}
		}
		else
		{
			fprintf(stderr,"Malformed PPM stream %s, invalid precision.\n",filename);
			goto exit;
		}
	}
	else
	{

		for (y = 0; y < height; y++)
		{
			for (x = 0;x < width; x++)
			{
				for (c = 0;c < components;c++)
				{
					if (ReadNumber(fp,&data) == 0)
						goto exit;
					*p[c]++ = data;
				}
			}
		}
	}
	result = 0;

 exit:
	if (ferror(fp))
	{
		perror("error while reading the input stream");
		result = -1;
	}
	fclose(fp);

	return result;
}

int ppm_encode(char* filename, xs_image_t* im, int little_endian)
{
	FILE* f_out;
	xs_data_in_t* p[3] = {NULL};
	int y,x,c;
	int result = -1;
	int bitpacking = 0;
	int width  = im->width;
	int height = im->height;

	f_out = fopen(filename, "wb");
	if (f_out == NULL)
		return -1;

	if (im->depth == 1)
	{
		if (im->ncomps != 1)
		{
			fprintf(stderr,"binary multi-component files are not supported by PNM\n");
			goto exit;
		}
		fprintf(f_out,"P4\n%d %d\n",width,height);
		bitpacking = 1;
	}
	else if (im->ncomps == 1)
	{
		fprintf(f_out,"P5\n%d %d\n%d\n",width,height,(1 << im->depth) - 1);
	}
	else if (im->ncomps == 3)
	{
		if (im->sx[0] != 1 || im->sx[1] != 1 || im->sx[2] != 1 ||
			im->sy[0] != 1 || im->sy[1] != 1 || im->sy[2] != 1)
		{
			fprintf(stderr, "subsampling is not supported by PNM\n");
			goto exit;
		}
		fprintf(f_out,"P6\n%d %d\n%d\n",width,height,(1 << im->depth) - 1);
	}
	else
	{
		fprintf(stderr,"unsupported image layout, cannot save as PNM\n");
		goto exit;
	}

	for(c = 0;c < im->ncomps;c++)
	{
		p[c] = im->comps_array[c];
	}

	if (bitpacking)
	{

		for(y = 0;y < height;y++)
		{
			int mask = 0x80;
			int data = 0;
			for(x = 0;x < width;x++)
			{
				if (*p[c]++)
					data |= mask;
				mask >>= 1;
				if (mask == 0)
				{
					fputc(data,f_out);
					mask = 0x80;
					data = 0;
				}
			}
			if (mask != 0x80)
				fputc(data,f_out);
		}
		result = 0;
	}
	else if (im->depth <= 8)
	{

		for(y = 0;y < height;y++)
		{
			for(x = 0;x < width;x++)
			{
				for(c = 0;c < im->ncomps;c++)
				{
					fputc(*p[c]++,f_out);
				}
			}
		}
		result = 0;
	}
	else if (im->depth <= 16)
	{

		for(y = 0;y < height;y++)
		{
			for(x = 0;x < width;x++)
			{
				for(c = 0;c < im->ncomps;c++)
				{
					int data = *p[c]++;
					fputc(data >> 8,f_out);
					fputc(data     ,f_out);
				}
			}
		}
		result = 0;
	}
	else
	{
		fprintf(stderr,"image precisions >16 are not supported by PNM\n");
	}

 exit:
	if (ferror(f_out))
	{
		perror("error writing PNM stream");
	}
	fclose(f_out);

	return result;
}

#ifdef TEST_PPM

int main()
{

}

#endif
