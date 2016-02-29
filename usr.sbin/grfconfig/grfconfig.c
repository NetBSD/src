/*	$NetBSD: grfconfig.c,v 1.16 2016/02/29 18:59:52 christos Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ezra Story and Bernd Ernesti.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1997\
 The NetBSD Foundation, Inc.  All rights reserved.");
#endif /* not lint */

#ifndef lint
__RCSID("$NetBSD: grfconfig.c,v 1.16 2016/02/29 18:59:52 christos Exp $");
#endif /* not lint */

#include <sys/file.h>
#include <sys/ioctl.h>
#include <err.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <amiga/dev/grfioctl.h>

static void print_modeline(FILE *fp, struct grfvideo_mode *, int); 
static void suggest(struct grfvideo_mode *, const char *, const char *);

static struct grf_flag {
	u_short	grf_flag_number;
	const char	*grf_flag_name;
} grf_flags[] = {
	{GRF_FLAGS_DBLSCAN,		"doublescan"},
	{GRF_FLAGS_LACE,		"interlace"},
	{GRF_FLAGS_PHSYNC,		"+hsync"},
	{GRF_FLAGS_NHSYNC,		"-hsync"},
	{GRF_FLAGS_PVSYNC,		"+vsync"},
	{GRF_FLAGS_NVSYNC,		"-vsync"},
	{GRF_FLAGS_SYNC_ON_GREEN,	"sync-on-green"},
	{0,				0}
};

/*
 * Dynamic mode loader for NetBSD/Amiga grf devices.
 */
int
main(int ac, char  **av)
{
	struct	grfvideo_mode gv[1];
	struct	grf_flag *grf_flagp;
	FILE	*fp;
	int	c, y, grffd;
	size_t  i;
	int	lineno = 0;
	int	uplim, lowlim;
	char	rawdata = 0, testmode = 0;
	char	*grfdevice = 0, *ptr;
	char	*modefile = 0;
	char	buf[_POSIX2_LINE_MAX];
	char	*cps[31];
	char	*p;
	const char	*errortext;


	while ((c = getopt(ac, av, "rt")) != -1) {
		switch (c) {
		case 'r':	/* raw output */
			rawdata = 1;
			break;
		case 't':	/* test the modefile without setting it */
			testmode = 1;
			break;
		default:
			printf("grfconfig [-r] device [file]\n");
			return (1);
		}
	}
	ac -= optind;
	av += optind;


	if (ac < 1)
		errx(EXIT_FAILURE, "No grf device specified");
	grfdevice = av[0];

	if (ac >= 2)
		modefile = av[1];

	if ((grffd = open(grfdevice, O_RDWR)) == -1)
		err(EXIT_FAILURE, "Can't open grf device `%s'", grfdevice);

	/* If a mode file is specificied, load it in, don't display any info. */

	if (modefile) {
		if (!(fp = fopen(modefile, "r")))
			err(EXIT_FAILURE, 
			    "Cannot open mode definition file `%s'", modefile);

		while (fgets(buf, sizeof(buf), fp)) {
			char *obuf, tbuf[_POSIX2_LINE_MAX], *tbuf2;
			/*
			 * check for end-of-section, comments, strip off trailing
			 * spaces and newline character.
			 */
			for (p = buf; isspace((unsigned char)*p); ++p)
				continue;
			if (*p == '\0' || *p == '#')
				continue;
			for (p = strchr(buf, '\0'); isspace((unsigned char)*--p);)
				continue;
			*++p = '\0';

			obuf = buf;
			tbuf2 = tbuf;
			while ((*tbuf2 = *obuf) != '\0') {
				if (*tbuf2 == '#') {
					*tbuf2 = '\0';
					break;
				}
				if (isupper((unsigned char)*tbuf2)) {
					*tbuf2 = tolower((unsigned char)*tbuf2);
				}
				obuf++;
				tbuf2++;
			}
			obuf = tbuf;

			lineno = lineno + 1;

#define SP " \b\t\r\n"
			memset(cps, 0, sizeof(cps));
			for (i = 0, ptr = strtok(buf, SP);
			    ptr != NULL && i < __arraycount(cps);
			    i++, ptr = strtok(NULL, SP))
				cps[i] = ptr;


			if (i < 14)
				errx(EXIT_FAILURE, "Too few values in mode "
				    "definition file: `%s'\n", obuf);

			gv->pixel_clock	= atoi(cps[1]);
			gv->disp_width	= atoi(cps[2]);
			gv->disp_height	= atoi(cps[3]);
			gv->depth	= atoi(cps[4]);
			gv->hblank_start	= atoi(cps[5]);
			gv->hsync_start	= atoi(cps[6]);
			gv->hsync_stop	= atoi(cps[7]);
			gv->htotal	= atoi(cps[8]);
			gv->vblank_start	= atoi(cps[9]);
			gv->vsync_start	= atoi(cps[10]);
			gv->vsync_stop	= atoi(cps[11]);
			gv->vtotal	= atoi(cps[12]);

			if ((y = atoi(cps[0])))
				gv->mode_num = y;
			else
				if (strncasecmp("c", cps[0], 1) == 0) {
					gv->mode_num = 255;
					gv->depth = 4;
				} else {
					errx(EXIT_FAILURE,
					    "Illegal mode number: %s", cps[0]);
				}

			if ((gv->pixel_clock == 0) ||
			    (gv->disp_width == 0) ||
			    (gv->disp_height == 0) ||
			    (gv->depth == 0) ||
			    (gv->hblank_start == 0) ||
			    (gv->hsync_start == 0) ||
			    (gv->hsync_stop == 0) ||
			    (gv->htotal == 0) ||
			    (gv->vblank_start == 0) ||
			    (gv->vsync_start == 0) ||
			    (gv->vsync_stop == 0) ||
			    (gv->vtotal == 0)) {
				errx(EXIT_FAILURE, "Illegal value in "
				    "mode #%d: `%s'", gv->mode_num, obuf);
			}

			if (strstr(obuf, "default") != NULL) {
				gv->disp_flags = GRF_FLAGS_DEFAULT;
			} else {
				gv->disp_flags = GRF_FLAGS_DEFAULT;
				for (grf_flagp = grf_flags;
				  grf_flagp->grf_flag_number; grf_flagp++) {
				    if (strstr(obuf, grf_flagp->grf_flag_name) != NULL) {
					gv->disp_flags |= grf_flagp->grf_flag_number;
				    }
				}
				if (gv->disp_flags == GRF_FLAGS_DEFAULT)
					errx(EXIT_FAILURE, "Your are using a "
					    "mode file with an obsolete "
					    "format");
			}

			/*
			 * Check for impossible gv->disp_flags:
			 * doublescan and interlace,
			 * +hsync and -hsync
			 * +vsync and -vsync.
			 */
			errortext = NULL;
			if ((gv->disp_flags & GRF_FLAGS_DBLSCAN) &&
			    (gv->disp_flags & GRF_FLAGS_LACE))
				errortext = "Interlace and Doublescan";
			if ((gv->disp_flags & GRF_FLAGS_PHSYNC) &&
			    (gv->disp_flags & GRF_FLAGS_NHSYNC))
				errortext = "+hsync and -hsync";
			if ((gv->disp_flags & GRF_FLAGS_PVSYNC) &&
			    (gv->disp_flags & GRF_FLAGS_NVSYNC))
				errortext = "+vsync and -vsync";

			if (errortext != NULL)
				errx(EXIT_FAILURE, "Illegal flags in "
				    "mode #%d: `%s' are both defined",
				    gv->mode_num, errortext);

			/* Check for old horizontal cycle values */
			if ((gv->htotal < (gv->disp_width / 4))) {
				gv->hblank_start *= 8;
				gv->hsync_start *= 8;
				gv->hsync_stop *= 8;
				gv->htotal *= 8;
				suggest(gv, "horizontal videoclock cycle "
				    "values", obuf);
				return EXIT_FAILURE;
			}

			/* Check for old interlace or doublescan modes */
			uplim = gv->disp_height + (gv->disp_height / 4);
			lowlim = gv->disp_height - (gv->disp_height / 4);
			if (((gv->vtotal * 2) > lowlim) &&
			    ((gv->vtotal * 2) < uplim)) {
				gv->vblank_start *= 2;
				gv->vsync_start *= 2;
				gv->vsync_stop *= 2;
				gv->vtotal *= 2;
				gv->disp_flags &= ~GRF_FLAGS_DBLSCAN;
				gv->disp_flags |= GRF_FLAGS_LACE;
				suggest(gv, "vertical values for interlace "
				    "modes", obuf);
				return EXIT_FAILURE;
			} else if (((gv->vtotal / 2) > lowlim) &&
			    ((gv->vtotal / 2) < uplim)) {
				gv->vblank_start /= 2;
				gv->vsync_start /= 2;
				gv->vsync_stop /= 2;
				gv->vtotal /= 2;
				gv->disp_flags &= ~GRF_FLAGS_LACE;
				gv->disp_flags |= GRF_FLAGS_DBLSCAN;
				suggest(gv, "vertical values for doublescan "
				    "modes", obuf);
				return EXIT_FAILURE;
			}

			if (testmode == 1) {
				if (lineno == 1)
					printf("num clk wid hi dep hbs "
					    "hss hse ht vbs vss vse vt "
					    "flags\n");
				print_modeline(stdout, gv, 1);
			} else {
				gv->mode_descr[0] = 0;
				if (ioctl(grffd, GRFIOCSETMON, (char *) gv) < 0)
					err(EXIT_FAILURE, "bad monitor "
					    "definition for mode #%d",
					    gv->mode_num);
			}
		}
		fclose(fp);
	} else {
		ioctl(grffd, GRFGETNUMVM, &y);
		y += 2;
		for (c = 1; c < y; c++) {
			c = gv->mode_num = (c != (y - 1)) ? c : 255;
			if (ioctl(grffd, GRFGETVMODE, gv) < 0)
				continue;
			if (rawdata) {
				print_modeline(stdout, gv, 0);
				continue;
			}
			if (c == 255)
				printf("Console: ");
			else
				printf("%2d: ", gv->mode_num);

			printf("%dx%d",
			    gv->disp_width,
			    gv->disp_height);

			if (c != 255)
				printf("x%d", gv->depth);
			else
				printf(" (%dx%d)",
				    gv->disp_width / 8,
				    gv->disp_height / gv->depth);

			printf("\t%ld.%ldkHz @ %ldHz",
			    gv->pixel_clock / (gv->htotal * 1000),
			    (gv->pixel_clock / (gv->htotal * 100)) 
    	    	    	    	% 10,
			    gv->pixel_clock / (gv->htotal * gv->vtotal));
			printf(" flags:");
				
			if (gv->disp_flags == GRF_FLAGS_DEFAULT) {
				printf(" default\n");
				continue;
			}

			for (grf_flagp = grf_flags;
			    grf_flagp->grf_flag_number; grf_flagp++)
				if (gv->disp_flags & grf_flagp->grf_flag_number)
					printf(" %s", grf_flagp->grf_flag_name);
			printf("\n");
		}
	}

	close(grffd);
	return EXIT_SUCCESS;
}

static void
suggest(struct	grfvideo_mode *gv, const char *d, const char *s)
{
	warnx("Old and no longer supported %s: %s", d, s);
	warnx("Wrong mode line, this could be a possible good model line:");
	fprintf(stderr, "%s: ", getprogname());
	print_modeline(stderr, gv, 0);
}

static void
print_modeline(FILE *fp, struct grfvideo_mode *gv, int rawflags)
{
	struct	grf_flag *grf_flagp;

	if (gv->mode_num == 255)
		fprintf(fp, "c ");
	else
		fprintf(fp, "%d ", gv->mode_num);

	fprintf(fp, "%ld %d %d %d %d %d %d %d %d %d %d %d",
		gv->pixel_clock,
		gv->disp_width,
		gv->disp_height,
		gv->depth,
		gv->hblank_start,
		gv->hsync_start,
		gv->hsync_stop,
		gv->htotal,
		gv->vblank_start,
		gv->vsync_start,
		gv->vsync_stop,
		gv->vtotal);

	if (rawflags) {
		fprintf(fp, " 0x%.2x\n", gv->disp_flags);
		return;
	}
	if (gv->disp_flags == GRF_FLAGS_DEFAULT) {
		fprintf(fp, " default\n");
		return;
	}

	for (grf_flagp = grf_flags; grf_flagp->grf_flag_number; grf_flagp++)
		if (gv->disp_flags & grf_flagp->grf_flag_number)
			fprintf(fp, " %s", grf_flagp->grf_flag_name);
	fprintf(fp, "\n");
}
