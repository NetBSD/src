/*	$NetBSD: ppmtochrpicon.c,v 1.2 2001/08/20 12:20:06 wiz Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lonhyn T. Jasinskyj.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Usage:
 *
 * ppmtochrpicon [ppmfile]
 *
 * This programs reads from either a single file given as an argument
 * or from stdin if no args are given. It tries to find a <ICON...>
 * tag in the file and read a CHRP style ASCII boot icon. It is not
 * overly intelligent at dealing with other confusing stuff in the 
 * file or weird formatting due to the specialized nature of the input
 * files.
 *
 * It then produces a PPM file on stdout containing the boot icon's image.
 */

#include <stdlib.h>

#include <pbm.h>
#include <ppm.h>

#include "chrpicon.h"

static char *hex_digits = "0123456789ABCDEF";

int
main(int argc, char *argv[])
{
    FILE *ifp;
    CHRPI_spec_rec img_rec;
    CHRPI_spec img = &img_rec;
    pixval maxval;
    pixel **pixels;

    ppm_init(&argc, argv);

    if (argc > 2)
	pm_usage("[ppmfile]");

    /* either use stdin or open a file */
    if (argc > 1)
	ifp = pm_openr(argv[1]);
    else
	ifp = stdin;

    /* use the img struct to conveniently store some parameters */
    pixels = ppm_readppm(ifp, &img->width, &img->height, &maxval);

    if (maxval != 255)
        pm_error("can only use 8-bit per channel true-color "
                 "PPM files (maxval = 255)");

    if (img->width != 64 || img->height != 64)
        pm_message("CHRP only supports boot icons of "
                   "size 64x64, will truncate to fit");

    img->width = img->height = 64;

    /* the only supported color space is RGB = 3:3:2 */
    img->rbits = img->gbits = 3;
    img->bbits = 2;

    pm_close(ifp);

    CHRPI_writeicon(stdout, pixels, img);

    return 0;
}

void
CHRPI_writeicon(FILE *fp, pixel **pixels, CHRPI_spec img)
{
    CHRPI_putheader(stdout, img);
    CHRPI_putbitmap(stdout, pixels, img);
    CHRPI_putfooter(stdout, img);
}

void
CHRPI_putheader(FILE *fp, CHRPI_spec img)
{
    fprintf(fp, "<ICON SIZE=%d,%d COLOR-SPACE=%d,%d,%d>\n<BITMAP>\n",
            img->height, img->width,
            img->rbits, img->gbits, img->bbits);
}

void
CHRPI_putfooter(FILE *fp, CHRPI_spec img)
{
    fprintf(fp, "</BITMAP>\n</ICON>\n");
}
    

void
CHRPI_putbitmap(FILE *fp, pixel** pixels, CHRPI_spec img)
{
    int row, col;
    pixel *pP;

    for (row = 0; row < img->height; row++) {

        pixval r, g, b;
        char pixbyte;

        pP = pixels[row];

        for (col = 0; col < img->width; col++) {
            
            r = PPM_GETR(*pP);
            g = PPM_GETG(*pP);
            b = PPM_GETB(*pP);

            pixbyte = (r & 0xe0)  | ((g >> 3) & 0x1c) | ((b >> 6) & 0x03);

            /* write the byte in hex */
            fputc(hex_digits[(pixbyte>>4) & 0x0f], fp);
            fputc(hex_digits[(pixbyte & 0x0f)], fp);
            pP++;
        }

        fputc('\n', fp);
    }
}
