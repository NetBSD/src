/*	$NetBSD: chrpicontoppm.c,v 1.1.1.1.2.1 1999/12/27 18:33:42 wrstuden Exp $	*/

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
 * chrpicontoppm.c - read a CHRP style boot icon file and convert
 * it to a PPM.
 */

/*
 * Usage:
 *
 * chrpicontoppm [chrpiconfile]
 *
 * This programs reads from either a single file given as an argument
 * or from stdin if no args are given. It expects a true color 
 * PPM file as the input. The image should be 64x64, otherwise it
 * is cropped to that size.
 *
 * It then produces a CHRP style boot icon file on stdout.
 */

#include <stdlib.h>

#include <pbm.h>
#include <ppm.h>

#include "chrpicon.h"


int
main(int argc, char *argv[])
{
    FILE *ifp;
    CHRPI_spec_rec img_rec;
    CHRPI_spec img = &img_rec;
    pixel *pixelrow;
    pixel *pP;
    chrpi_pixel *imgP;
    int row, col;
    pixval maxval = 255;


    ppm_init(&argc, argv);

    if (argc > 2)
	pm_usage("[chrpiconfile]");

    /* either use stdin or open a file */
    if (argc > 1) {
	if ((ifp = fopen(argv[1], "r")) == NULL) {
            perror("ppmfile open");
            exit(1);
        }
    }
    else
	ifp = stdin;

    if (CHRPI_getheader(ifp, img))
	pm_error("can't find <ICON...> header in boot icon file");
    
    if (CHRPI_getbitmap(ifp, img))
	pm_error("can't read <BITMAP...> section in boot icon file");

    if (img->rbits != 3 || img->gbits != 3 || img->bbits != 2)
	pm_error("can only handle RGB 3:3:2 colorspace icon files");

    ppm_writeppminit(stdout, img->width, img->height, maxval, PLAIN_PPM);
    pixelrow = ppm_allocrow(img->width);

    for (row = 0; row < img->height; row++) {

        pixval r, g, b;

        pP = pixelrow;
        imgP = img->pixels[row];

        for (col = 0; col < img->width; col++) {
            
            r = ((*imgP >> 5) & 7);
            g = ((*imgP >> 2) & 7);
            b = (*imgP & 3);

            r = (r << 5) | (r << 2) | (r >> 1);
            g = (g << 5) | (g << 2) | (g >> 1);
            b = (b << 6) | (b << 4) | (b >> 4) | b;
            
            PPM_ASSIGN(*pP, r, g, b);

            pP++;
            imgP++;
        }

        ppm_writeppmrow(stdout, pixelrow, img->width, maxval, PLAIN_PPM);
    }

    ppm_freerow(pixelrow);

    pm_close(ifp);
    pm_close(stdout);
    exit(0);
}


chrpi_pixel *
CHRPI_allocrow(int cols)
{
    return calloc(cols, sizeof(chrpi_pixel));
}

int
CHRPI_getheader(FILE *fp, CHRPI_spec img)
{
    char line[MAX_LINE_LENGTH + 1];

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (strstr(line, ICON_TAG)) {
            /* found the ICON identifier, primitively parse it */
            if (sscanf(line, " %*s SIZE=%d,%d COLOR-SPACE=%d,%d,%d",
                       &img->height, &img->width,
                       &img->rbits, &img->gbits, &img->bbits
                       ) != 5)
                return -1;

            return 0;
        }
    }
    
    return -1;
}
    

int
CHRPI_getbitmap(FILE *fp, CHRPI_spec img)
{
    char line[MAX_LINE_LENGTH + 1];
    int foundtag = 0;
    char hexstr[3] = { 0, 0, 0 };
    char *p;
    int r, c;


    /* first find the BITMAP tag */
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (strncmp(line, BITMAP_TAG, strlen(BITMAP_TAG)) == 0) {
            foundtag++;
            break;
        }
    }
        
    if (!foundtag)
        return -1;

    if ((img->pixels = calloc(img->height, sizeof(chrpi_pixel *))) == NULL)
        return -1;

    for (r = 0; r < img->height; r++)
        if ((img->pixels[r] = CHRPI_allocrow(img->width)) == NULL)
            return -1;

    for (r = 0; r < img->height; r++) {

        /* get a row */
        if ((p = fgets(line, MAX_LINE_LENGTH, fp)) == NULL) {
            return -1;
        }

        /* go down the pixels and convert them */
        for (c = 0; c < img->width; c++) {
            hexstr[0] = *p++;
            hexstr[1] = *p++;
            
            img->pixels[r][c] = (chrpi_pixel)(strtoul(hexstr, NULL, 16));
        }
    }

    return 0;
}
