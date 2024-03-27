/*************************************************************************
*  This source code has been written by Alexandre Willème (UCLouvain -
*  2018). It is based on source code of difftest_ng by Thomas Richter.
*  It has been tested on the test codestreams related to ISO/IEC 21122-4
*  a.k.a. JPEG XS Part 4.

*  The license of difftest_ng is the following:
*    Written by Thomas Richter (THOR Software) for Accusoft.
*    All Rights Reserved
*
*    This source file is part of difftest_ng, a universal image measuring
*    and conversion framework.
*
*    difftest_ng is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*    difftest_ng is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*    You should have received a copy of the GNU General Public License
*    along with difftest_ng.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/

#ifndef PGX_H
#define PGX_H

#include "libjxs.h"
 
int pgx_encode(char* filename, xs_image_t* im);
int pgx_decode(char* filename, xs_image_t* im_out);

#endif
