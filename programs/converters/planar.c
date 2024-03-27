/*************************************************************************
** Copyright (c) 2015-2016 intoPIX SA & Fraunhofer IIS                  **
** All rights reserved.                                                 **
**                                                                      **
** intoPIX SA & Fraunhofer IIS hereby grants to ISO/IEC JTC1 SC29 WG1   **
** (JPEG Committee) and each Member of ISO/IEC JTC1 SC29 WG1 (JPEG      **
** Committee) who participate in the Working Group dedicated to the     **
** standardization of JPEG-XS, a non-exclusive, nontransferable,        **
** worldwide, license under "intoPIX SA & Fraunhofer IIS" copyrights    **
** in this software to reproduce, distribute, display, perform and      **
** create derivative works for the sole and exclusive purposes of       **
** creating a test model in the frame of the JPEG-XS compression        **
** standard.                                                            **
**                                                                      **
** Modifications to this code shall be clearly indicated and            **
** identified by the relevant copyright notice(s) of the party          **
** generating these changes and/or derivative works.                    **
**                                                                      **
** Nothing contained in this software shall, except as herein           **
** expressly provided, be construed as conferring by implication,       **
** estoppel or otherwise, any license or right under (i) any existing   **
** or later issuing patent, whether or not the use of information in    **
** this software necessarily employs an invention of any existing or    **
** later issued patent, (ii) any copyright, (iii) any trademark, or     **
** (iv) any other intellectual property right.                          **
**                                                                      **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND       **
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED        **
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A      **
** PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE             **
** COPYRIGHT OWNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,      **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT     **
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF     **
** USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED      **
** AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT          **
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING       **
** IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF       **
** THE POSSIBILITY OF SUCH DAMAGE.                                      **
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "planar.h"
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

/*
	YUV Planar encoder with automatic chrominance detection (422 or 420)
*/
int yuv_planar_decode(const char* filename, xs_image_t* im_out)
{
	int nsamples420, nsamples422, nsamples444, comp, pix;
	size_t bytesRead, leftover;
	FILE* f_in;
	uint8_t *data,*ptr8;
	uint16_t *ptr16;
	int word_size = 8;
	int or_all = 0;

	if ((im_out->width <= 0) || (im_out->height <= 0))
	{
		fprintf(stderr, "yuv8p_decode error: unknown image width/height\n");
		return -1;
	}

	f_in = fopen(filename, "rb");
	if (f_in == NULL)
		return -1;

	/* detecting chrominance automatically */
	nsamples444 = im_out->width * im_out->height * 3;
	nsamples422 = im_out->width * im_out->height * 2;
	nsamples420 = im_out->width * im_out->height * 3 / 2;

	data = (uint8_t *) malloc(nsamples444 * sizeof(uint16_t));
	bytesRead = fread(data, sizeof(uint8_t), nsamples444*sizeof(uint16_t), f_in);
	leftover = fread(data, sizeof(uint8_t), 1, f_in);

	if (leftover || ((bytesRead != nsamples420) &&
					 (bytesRead != nsamples422) &&
					 (bytesRead != nsamples444) &&
					 (bytesRead != nsamples420 * 2) &&
					 (bytesRead != nsamples422 * 2) &&
					 (bytesRead != nsamples444 * 2)))
	{
		fprintf(stderr, "yuv_planar_decode: filesize does not match format.\n");
		fprintf(stderr, "expected %d for a %dx%d 4:4:4 yuv planar 8bit file\n", nsamples444, im_out->width, im_out->height);
		fprintf(stderr, "expected %d for a %dx%d 4:2:2 yuv planar 8bit file\n", nsamples422, im_out->width, im_out->height);
		fprintf(stderr, "expected %d for a %dx%d 4:2:0 yuv planar 8bit file\n", nsamples420, im_out->width, im_out->height);
		fprintf(stderr, "expected %d for a %dx%d 4:4:4 yuv planar 16bit file\n", 2*nsamples444, im_out->width, im_out->height);
		fprintf(stderr, "expected %d for a %dx%d 4:2:2 yuv planar 16bit file\n", 2*nsamples422, im_out->width, im_out->height);
		fprintf(stderr, "expected %d for a %dx%d 4:2:0 yuv planar 16bit file\n", 2*nsamples420, im_out->width, im_out->height);
		fclose(f_in);
		free(data);
		return -1;
	}
	fclose(f_in);
	im_out->ncomps = 3;
	if (bytesRead==nsamples420 || bytesRead==nsamples420*2)
	{
		im_out->sx[0] = 1;
		im_out->sx[2] = im_out->sx[1] = 2;
		im_out->sy[0] = 1;
		im_out->sy[2] = im_out->sy[1] = 2;
		word_size = (bytesRead==nsamples420*2) ? 16 : 8;
	}
	else if (bytesRead==nsamples422 || bytesRead==nsamples422*2)
	{
		im_out->sx[0] = 1;
		im_out->sx[2] = im_out->sx[1] = 2;
		im_out->sy[2] = im_out->sy[1] = im_out->sy[0] = 1;
		word_size = (bytesRead==nsamples422*2) ? 16 : 8;
	}
	else if (bytesRead==nsamples444 || bytesRead==nsamples444*2) {
		im_out->sx[2] = im_out->sx[1] = im_out->sx[0] = 1;
		im_out->sy[2] = im_out->sy[1] = im_out->sy[0] = 1;
		word_size = (bytesRead==nsamples444*2) ? 16 : 8;
	}
	if (!xs_allocate_image(im_out, false))
		return -1;

	ptr8 = data;
	ptr16 = (uint16_t*) data;
	or_all = 0;
	for (comp = 0; comp < 3; comp++)
	{
		const int sample_count = (im_out->width / im_out->sx[comp]) * (im_out->height / im_out->sy[comp]);
		for (pix = 0; pix < sample_count; pix++)
		{
			if (word_size == 8)
				im_out->comps_array[comp][pix] = *ptr8++;
			else
				im_out->comps_array[comp][pix] = *ptr16++;
			or_all |= im_out->comps_array[comp][pix];
		}
	}
	free(data);
	if (im_out->depth == -1)
	{
		if (word_size == 8)
			im_out->depth = 8;
		else
		{
			im_out->depth = BSR(or_all) + 1;
			im_out->depth = (im_out->depth < 8) ? 8 : im_out->depth;
			fprintf(stderr, "yuv_planar_decode: unspecified depth, using %d\n", im_out->depth);
		}
	}
	fprintf(stderr, "yuv_planar_decode: assuming w=%d h=%d depth=%dbits\n", im_out->width, im_out->height, im_out->depth);
	return 0;
}

/* generic planar encoder 
	uses 8 bits per sample for depth [1-8] (if not force_16bits) and 16 bits otherwise
*/
int planar_encode(char* filename, xs_image_t* im, int force_16bits)
{
	FILE* f_out;
	uint16_t val;
	size_t comp, pix;
	f_out = fopen(filename, "wb");
	if (f_out == NULL)
		return -1;

	for (comp = 0; comp < im->ncomps; comp++)
	{
		const int sample_count = (im->width / im->sx[comp]) * (im->height / im->sy[comp]);
		for (pix = 0; pix < sample_count; pix++)
		{
			if ((im->depth <= 8) && (!force_16bits))
				putc((uint8_t)im->comps_array[comp][pix], f_out);
			else
			{
				val = im->comps_array[comp][pix];
				fwrite(&val, sizeof(uint16_t), 1, f_out);
			}
		}
	}
	fclose(f_out);
	return 0;
}
