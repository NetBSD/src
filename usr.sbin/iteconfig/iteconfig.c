/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *      $Id: iteconfig.c,v 1.3 1994/04/10 00:56:57 chopps Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>
#include <amiga/dev/iteioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "pathnames.h"

void	printcmap __P((colormap_t *, int));
void	usage __P((void));
void	xioctl __P((int, int, void *));
colormap_t *xgetcmap __P((int, int));
long	xstrtol __P((char *));

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct itewinsize is, newis;
	struct itebell ib, newib;
	struct winsize ws;
	colormap_t *cm;
	int ch, fd, i, iflag, max_colors;
	long val;

	iflag = 0;

	fd = open(_PATH_AMIGACONSOLE, O_RDONLY | O_NONBLOCK);
	if (fd == -1)
		err(1, "open console");

	xioctl(fd, ITEIOCGWINSZ, &is);
	xioctl(fd, ITEIOCGBELL, &ib);

	memcpy(&newis, &is, sizeof(is));
	memcpy(&newib, &ib, sizeof(ib));

	while ((ch = getopt(argc, argv, "D:H:P:T:V:W:X:Y:d:h:ip:t:v:w:x:y:"))
	    != EOF) {
		switch (ch) {
		case 'D':		/* undocumented backward compat */
		case 'd':
			newis.depth = xstrtol(optarg);
			break;
		case 'H':		/* undocumented backward compat */
		case 'h':
			newis.height = xstrtol(optarg);
			break;
		case 'i':
			iflag = 1;
			break;
		case 'p':
			newib.pitch = xstrtol(optarg);
			break;
		case 't':
			newib.msec = xstrtol(optarg);
			break;
		case 'V':		/* undocumented backward compat */
		case 'v':
			newib.volume = xstrtol(optarg);
			break;
		case 'W':		/* undocumented backward compat */
		case 'w':
			newis.width = xstrtol(optarg);
			break;
		case 'X':		/* undocumented backward compat */
		case 'x':
			newis.x = xstrtol(optarg);
			break;
		case 'Y':		/* undocumented backward compat */
		case 'y':
			newis.y = xstrtol(optarg);
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (memcmp(&newis, &is, sizeof(is))) {
		xioctl(fd, ITEIOCSWINSZ, &newis);
		xioctl(fd, ITEIOCGWINSZ, &is);
	}
	if (memcmp(&newib, &ib, sizeof(ib))) {
		xioctl(fd, ITEIOCSBELL, &newib);
		xioctl(fd, ITEIOCGBELL, &ib);
	}
	
	/*
	 * get, set and get colors again
	 */
	i = 0;
	max_colors = 1 << is.depth;
	cm = xgetcmap(fd, max_colors);
	while (argc--) {
		val = xstrtol(*argv++);
		if (i >= max_colors) {
			warnx("warning: too many colors");
			break;
		}
		cm->entry[i] = val;
		i++;
	}
	xioctl(fd, VIOCSCMAP, cm);
	free(cm);
	cm = xgetcmap(fd, max_colors);

	/* do tty stuff to get it to register the changes. */
	xioctl(fd, TIOCGWINSZ, &ws);

	if (iflag) {
		printf("tty size: rows %d cols %d\n", ws.ws_row, ws.ws_col);
		printf("ite size: w: %d  h: %d  d: %d  [x: %d  y: %d]\n",
		    is.width, is.height, is.depth, is.x, is.y);
		printf("ite bell: vol: %d  millisec: %d  pitch: %d\n",
		    ib.volume, ib.msec, ib.pitch);
		printcmap(cm, ws.ws_col);
	}
	close(fd);
	exit(0);
}

void
xioctl(fd, cmd, addr)
	int fd, cmd;
	void *addr;
{
	if (ioctl(fd, cmd, addr) == -1) 
		err(1, "ioctl");
}

long
xstrtol(s)
	char *s;
{
	long rv;

	rv = strtol(s, NULL, 0);
	if (errno == ERANGE && (rv == LONG_MIN || rv == LONG_MAX))
		err(1, "bad format: \"%s\"", s);
	return(rv);
}

colormap_t *
xgetcmap(fd, ncolors)
	int fd;
	int ncolors;
{
	colormap_t *cm;

	cm = malloc(sizeof(colormap_t) + ncolors * sizeof(u_long));
	if (cm == NULL)
		err(1, "malloc");
	cm->first = 0;
	cm->size = ncolors;
	cm->entry = (u_long *) & cm[1];
	xioctl(fd, VIOCGCMAP, cm);
	return(cm);
}

void
printcmap(cm, ncols)
	colormap_t *cm;
	int ncols;
{
	int i, nel;

	switch (cm->type) {
	case CM_MONO:
		printf("monochrome");
		return;
	case CM_COLOR:
		printf("color levels: red: %d  green: %d  blue: %d",
		    cm->red_mask + 1, cm->green_mask + 1, cm->blue_mask + 1);
		break;
	case CM_GREYSCALE:
		printf("greyscale levels: %d", cm->grey_mask + 1);
		break;
	}
	printf("\n");
	
	nel = ncols / 11 - 1;
	for (i = 0; i < cm->size; i++) {
		printf("0x%08lx ", cm->entry[i]);
		if ((i + 1) % nel == 0)
			printf("\n");
	}
	if ((i + 1) % nel)
		printf("\n");
}

void
usage()
{
	fprintf(stderr, "%s\n\t\t%s\n",
	    "usage: iteconfig [-i] [-v volume] [-p period] [-t count]",
	    "[-w width] [-h height] [-d depth] [-x off] [-y off] [color ...]");
	exit(1);
}
