/*************************************************************************
*  This source code has been written by Alexandre Willème (UCLouvain - 2018). It is based on source code of difftest_ng by Thomas Richter.
*  It has been tested on the test codestreams related to ISO/IEC 21122-4 a.k.a. JPEG XS Part 4
*  The license of difftest_ng is the following:
** Written by Thomas Richter (THOR Software) for Accusoft	        **
** All Rights Reserved							**
**************************************************************************
This source file is part of difftest_ng, a universal image measuring
and conversion framework.
    difftest_ng is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    difftest_ng is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with difftest_ng.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgx.h"
#include "helpers.h"

#ifdef WIN32 // other-way path separator on windows...
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

// A per-component buffer containing the component names.
struct ComponentName {
    struct ComponentName* m_pNext;
    char* m_pName;

    // Dimensions of the file.
    int m_ulWidth;
    int m_ulHeight;
    unsigned char m_ucDepth;
    unsigned char m_bSigned;
    unsigned char m_bFloat;  // is floating point (IEEE format)
    unsigned char m_bLE;  // endian-ness: true if little-endian
} *m_pNameList;


int pgx_encode(char* filename, xs_image_t* im)
{
    char buffer[256 + 20];
    char basename[256];

    if (im->depth > 16 || im->depth <= 0)
    {
        fprintf(stderr, "Bitdepth not supported for pgx output: %d\n", im->depth);
        return -1;
    }

    // Extract the name without extension.
    strcpy(basename, filename);
    char* dot = strrchr(basename, '.');
    if (!dot)
    {
        fprintf(stderr, "pgx_encoder: filename should have an extension (%s)\n", filename);
        return -1;
    }
    dot[0] = '\0';

    FILE* f_out = fopen(filename, "wb");
    if (!f_out)
    {
        fprintf(stderr, "Failed to create %s\n", filename);
        return -1;
    }

    // Write the file header containing the references to to all the components.
    for (int i = 0; i < im->ncomps; i++)
    {
        const int width = im->width / im->sx[i];
        const int height = im->height / im->sy[i];
        sprintf(buffer, "%s_%d.raw", basename, i);
        fprintf(f_out, "%s\n", buffer);

        // Write the raw output file.
        FILE* raw = fopen(buffer, "wb");
        if (!raw)
        {
            fprintf(stderr, "Failed to create %s\n", buffer);
            return -1;
        }
        clearerr(raw);

        xs_data_in_t* p = (xs_data_in_t*)im->comps_array[i];
        size_t sample_count = (size_t)(width * height);
        if (im->depth > 8)
        {
            while (sample_count-- > 0)
            {
                // Write in big endian.
                const xs_data_in_t data = *p++;
                putc(data >> 8, raw);
                putc(data, raw);
            }
        }
        else
        {
            while (sample_count-- > 0)
            {
                putc(*p++, raw);
            }
        }

        if (ferror(raw))
        {
            fclose(raw);
            fclose(f_out);
            fprintf(stderr, "Error writing the raw image data to %s\n", buffer);
            return -1;
        }
        fclose(raw);

        // Now write the image stats to the header file.
        sprintf(buffer, "%s_%d.h", basename, i);
        FILE* hdr = fopen(buffer, "w");
        if (!hdr)
        {
            fclose(f_out);
            fprintf(stderr, "Failed to create %s\n", buffer);
            return -1;
        }
        fprintf(hdr, "P%c %s %c%d %lu %lu\n", 'G', "ML", '+', im->depth, (unsigned long)(width), (unsigned long)(height));
        if (ferror(hdr))
        {
            fclose(f_out);
            fclose(hdr);
            fprintf(stderr, "failed to write the image header to %s\n", buffer);
            return -1;
        }
        fclose(hdr);
    }
    fclose(f_out);
    return 0;
}

int pgx_decode(char* filename, xs_image_t* im)
{
    char buffer[256 + 4];
    char path[256 + 4];
    int width, height, depth;
    int single;
    int embedded; // if the header is embedded in the file and the file names are generated.
    struct ComponentName* name;
    FILE* fp;
    size_t len;

    depth = 0;
    single = 0;
    embedded = 0;

    fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open the source image stream");
        return -1;
    }

    // Extract the name without extension.
    strcpy(path, filename);
    char* endpointer = strrchr(path, PATH_SEP);
    if (endpointer)
    {
        *(endpointer + 1) = '\0';
    }
    else
    {
        path[0] = '\0';
    }

    do
    {
        if (!embedded)
        {
            if (fgets(buffer, 255, fp) == NULL)
            {
                break;
            }
        }

        // Is this probably not the header file but the first data file?
        if (depth == 0 && buffer[0] == 'P' && (buffer[1] == 'G' || buffer[1] == 'F') && buffer[2] == ' ')
        {
            const char* dot = strrchr(filename, '.');
            if (dot && !strcmp(dot, ".pgx") && dot - 1 > filename && dot < filename + 256)
            {
                if (dot[-1] == '0')
                {
                    embedded = 1;
                }
                else
                {
                    embedded = 1;
                    single = 1;
                }
            }
        }

        // If embedded, try to algorithmically determine the next file name by replacing the digits before the basename.
        if (embedded)
        {
            if (single)
            {
                if (depth != 0) break;
                strcpy(buffer, filename);
            }
            else
            {
                FILE* tmp;
                const char* dot = strrchr(filename, '.');
                int dotpos = (int)(dot - filename) - 1;
                memcpy(buffer, filename, dotpos);
                sprintf(buffer + dotpos, "%d.pgx", depth);
                tmp = fopen(buffer, "rb");
                if (!tmp) break;
                fclose(tmp);
            }
        }
        // Check for a terminating /n and remove it.
        len = strlen(buffer);
        while (len > 0)
        {
            if (buffer[len - 1] == '\n' || buffer[len - 1] == ' ')
            {
                buffer[--len] = '\0';
            }
            else break;
        }

        // Remove path (because sometimes the .pgx contains the path).
        char* justname = (char*)strrchr(buffer, PATH_SEP);
        if (justname == NULL)
        {
            justname = buffer;
        }
        else
        {
            justname++;
        }

        if (len > 0)
        {
            struct ComponentName** last = &m_pNameList;
            char tmp[256 + 4];
            name = malloc(sizeof(struct ComponentName));
            if (!name)
            {
                fclose(fp);
                return -1;
            }
            name->m_pNext = NULL;
            name->m_ulWidth = 0;
            name->m_ulHeight = 0;
            name->m_ucDepth = 0;
            name->m_bSigned = 0;
            name->m_bFloat = 0;
            name->m_bLE = 0;
            strcpy(tmp, path);
            strcat(tmp, justname);
            name->m_pName = (char*)malloc(sizeof(char) * (strlen(tmp) + 1));
            if (!name->m_pName)
            {
                fclose(fp);
                return -1;
            }
            strcpy(name->m_pName, tmp);
            // Yup, there's really a file name left, attach it to the end.
            while (*last)
            {
                last = &((*last)->m_pNext);
            }
            *last = name;
            depth++;
        }
    } while (1);

    // Now parse the headers.
    name = m_pNameList;
    while (name != NULL)
    {
        char* data, * last, * dptr;
        FILE* header;
        int bitdepth;
        // Copy the name over, and replace the .raw with .h.
        strncpy(buffer, name->m_pName, 256);
        if (!embedded)
        {
            len = strlen(buffer);
            if (len > 4)
            {
                if (!strcmp(buffer + len - 4, ".raw"))
                {
                    buffer[len - 4] = 0;
                }
            }
            strcat(buffer, ".h");
        }

        header = fopen(buffer, "r");
        if (header == NULL)
        {
            fprintf(stderr, "Cannot open the pgx header file %s\n", buffer);
            return -1;
        }

        // Read the header into the file. This should be one single stream.
        data = fgets(buffer, 255, header);
        fclose(header);
        if (data == NULL)
        {
            fprintf(stderr, "Failed to read the pgx header file");
            return -1;
        }

        // Check whether the file header is fine. We only
        // support big endian PGX files, this is enough for
        // part4 compliance. We could also support
        // little endian files.
        if (!memcmp(buffer, "PF LM ", 6))
        {
            name->m_bLE = 1;
            name->m_bFloat = 1;
            fprintf(stderr, "invalid PGX file because float is not supported, PGX identifier %s broken", buffer);
            return -1;
        }
        else if (!memcmp(buffer, "PF ML ", 6))
        {
            name->m_bFloat = 1;
            fprintf(stderr, "invalid PGX file because float is not supported, PGX identifier %s broken", buffer);
            return -1;
        }
        else if (!memcmp(buffer, "PG LM ", 6))
        {
            name->m_bLE = 1;
        }
        else if (!memcmp(buffer, "PG ML ", 6))
        {
        }
        else
        {
            printf("Invalid PGX file, PGX identifier %s broken", buffer);
            return -1;
        }

        // The next must be + or -, defining the sign.
        dptr = buffer + 7;
        if (buffer[6] == '+' || buffer[6] == ' ')
        {
            name->m_bSigned = 0;
        }
        else if (buffer[6] == '-')
        {
            name->m_bSigned = 1;
            fprintf(stderr, "Invalid PGX file, signed mode is not supported");
            return -1;
        }
        else if (buffer[6] >= '0' && buffer[6] <= '9')
        {
            // Signedness not indicated, this is the bitdepth. Assume unsigned.
            name->m_bSigned = 0;
            dptr--;
        }
        else
        {
            fprintf(stderr, "Invalid PGX file, PGX signedness %c broken\n", buffer[6]);
            return -1;
        }

        // Get the bit bitdepth of the component.
        bitdepth = strtol(dptr, &last, 10);
        // Currently, not more than 16bpp.
        if (last <= dptr || last[0] != ' ' || bitdepth <= 0 || bitdepth > 16)
        {
            fprintf(stderr, "Invalid PGX file, bit depth invalid\n");
            return -1;
        }
        name->m_ucDepth = bitdepth;

        data = last + 1;
        name->m_ulWidth = strtol(data, &last, 10);
        if (last <= data || last[0] != ' ')
        {
            fprintf(stderr, "invalid PGX file, width invalid\n");
            return -1;
        }
        data = last + 1;
        name->m_ulHeight = strtol(data, &last, 10);
        if (last <= data || !isspace(last[0]))
        {
            fprintf(stderr, "invalid PGX file, height invalid\n");
            return -1;
        }
        // All done with this file. Get the next.
        name = name->m_pNext;
    }

    // Find the maximum width, height as base for the subsampling.
    name = m_pNameList;
    width = 0;
    height = 0;
    while (name != NULL)
    {
        if (name->m_ulWidth > width) width = name->m_ulWidth;
        if (name->m_ulHeight > height) height = name->m_ulHeight;
        name = name->m_pNext;
    }

    im->depth = m_pNameList->m_ucDepth;
    im->ncomps = depth;

    name = m_pNameList;
    im->width = 0;
    im->height = 0;
    for (int i = 0; i < im->ncomps; ++i)
    {
        // Map image dimensions.
        if (name->m_ulWidth > im->width) im->width = name->m_ulWidth;
        if (name->m_ulHeight > im->height) im->height = name->m_ulHeight;
        name = name->m_pNext;
    }

    name = m_pNameList;
    for (int i = 0; i < im->ncomps; ++i)
    {
        // Map component dimensions and sub-sampling.
        if (name->m_ulWidth == im->width) im->sx[i] = 1;
        else if (name->m_ulWidth == im->width / 2) im->sx[i] = 2;
        else
        {
            fprintf(stderr, "PGX file contains unsupported subsampling format\n");
            return -1;
        }
        if (name->m_ulHeight == im->height) im->sy[i] = 1;
        else if (name->m_ulHeight == im->height / 2) im->sy[i] = 2;
        else
        {
            fprintf(stderr, "PGX file contains unsupported subsampling format\n");
            return -1;
        }
        name = name->m_pNext;
    }

    if (!xs_allocate_image(im, false))
    {
        fprintf(stderr, "cannot allocate image \n");
        return -1;
    }

    name = m_pNameList;
    FILE* raw;
    for(int i = 0; i < im->ncomps; ++i)
    {
        size_t size = (size_t)(name->m_ulWidth * name->m_ulHeight);
        xs_data_in_t* data = im->comps_array[i];

        // Now read the data from the raw file.
        raw = fopen(name->m_pName, "rb");
        if (raw == NULL)
        {
            fprintf(stderr, "Unable to open the PGX raw data file %s\n", name->m_pName);
            return -1;
        }
        if (embedded)
        {
            int c;
            // Read off the first line.
            while ((c = fgetc(raw)) != -1 && c != '\n') {}
            if (c == -1)
            {
                fclose(raw);
                fprintf(stderr, "Invalid data header in embedded PGX file %s\n", name->m_pName);
                return -1;
            }
        }
        if (name->m_ucDepth <= 8)
        {
            while (size--)
            {
                const int in1 = fgetc(raw);
                if (in1 < 0)
                {
                    fclose(raw);
                    fprintf(stderr, "Incomplete PGX data file %s\n", name->m_pName);
                    return -1;
                }
                *data++ = in1;
            }
        }
        else if (name->m_ucDepth <= 16 && name->m_bLE)
        {
            while (size--)
            {
                const int in1 = fgetc(raw);
                const int in2 = fgetc(raw);
                if (in2 < 0)
                {
                    fclose(raw);
                    fprintf(stderr, "Incomplete PGX data file %s\n", name->m_pName);
                    return -1;
                }
                *data++ = (in2 << 8) | in1; // is little endian
            }
        }
        else if (name->m_ucDepth <= 16 && !name->m_bLE)
        {
            while (size--)
            {
                const int in1 = fgetc(raw);
                const int in2 = fgetc(raw);
                if (in2 < 0)
                {
                    fclose(raw);
                    fprintf(stderr, "Incomplete PGX data file %s\n", name->m_pName);
                    return -1;
                }
                *data++ = (in1 << 8) | in2; // is big endian
            }
        }        
        else
        {
            fclose(raw);
            fprintf(stderr, "Bit depth %d is not supported for PGX\n", name->m_ucDepth);
            return -1;
        }
        fclose(raw);
        name = name->m_pNext;
    }

    return 0;
}
