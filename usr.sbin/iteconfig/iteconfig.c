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
 *      $Id: iteconfig.c,v 1.1 1994/04/05 01:56:46 chopps Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>
#include <amiga/dev/iteioctl.h>

colormap_t *xgetcmap __P((int, int));
void printcmap __P((colormap_t *, int));
void xioctl __P((int, int, void *));
long xstrtol __P((char *));
void xusage __P((void));

char *pname;
char *optstr = "W:w:H:h:D:d:V:v:T:t:P:p:X:x:Y:y:i";

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
	struct ite_window_size is, newis;
	struct ite_bell_values ib, newib;
	struct winsize ws;
	colormap_t *cm;
	int opt, fd, f_info, i, max_colors, rv;
	long val;

	f_info = 0;
	pname = argv[0];

	fd = open("/dev/ite0", O_NONBLOCK | O_RDONLY);
	if (fd == -1) {
		perror("open console");
		exit(1);
	}

	xioctl(fd, ITE_GET_WINDOW_SIZE, &is);
	xioctl(fd, ITE_GET_BELL_VALUES, &ib);

	bcopy(&is, &newis, sizeof(is));
	bcopy(&ib, &newib, sizeof(ib));

	while ((opt = getopt(argc, argv, optstr)) != EOF) {
		switch (opt) {
		case 'i':
			f_info = 1;
			break;
		case 'X':
		case 'x':
			newis.x = xstrtol(optarg);
			break;
		case 'Y':
		case 'y':
			newis.y = xstrtol(optarg);
			break;
		case 'W':
		case 'w':
			newis.width = xstrtol(optarg);
			break;
		case 'H':
		case 'h':
			newis.height = xstrtol(optarg);
			break;
		case 'D':
		case 'd':
			newis.depth = xstrtol(optarg);
			break;
		case 'V':
		case 'v':
			newib.volume = xstrtol(optarg);
			break;
		case 'P':
		case 'p':
			newib.period = xstrtol(optarg);
			break;
		case 'T':
		case 't':
			newib.time = xstrtol(optarg);
			break;
		default:
		case '?':
			xusage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (bcmp(&newis, &is, sizeof(is))) {
		xioctl(fd, ITE_SET_WINDOW_SIZE, &newis);
		xioctl(fd, ITE_GET_WINDOW_SIZE, &is);
	}
	if (bcmp(&newib, &ib, sizeof(ib))) {
		xioctl(fd, ITE_SET_BELL_VALUES, &newib);
		xioctl(fd, ITE_GET_BELL_VALUES, &ib);
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
			fprintf(stderr, "%s: warn: to many colors\n", pname);
			break;
		}
		cm->entry[i] = val;
		i++;
	}
	xioctl(fd, VIEW_USECOLORMAP, cm);
	free(cm);
	cm = xgetcmap(fd, max_colors);

	/* do tty stuff to get it to register the changes. */
	xioctl(fd, TIOCGWINSZ, &ws);

	if (f_info) {
		printf("tty size: rows %d cols %d\n", ws.ws_row, ws.ws_col);
		printf("ite size: w: %d  h: %d  d: %d  [x: %d  y: %d]\n",
		    is.width, is.height, is.depth, is.x, is.y);
		printf("ite bell: vol: %d  count: %d  period: %d\n",
		    ib.volume, ib.time, ib.period);
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
	if (ioctl(fd, cmd, addr) != -1) 
		return;

	perror("ioctl");
	exit(1);
}

long
xstrtol(s)
	char *s;
{
	long rv;

	rv = strtol(s, NULL, 0);
	if (errno != ERANGE || (rv != LONG_MAX && rv != LONG_MIN))
		return(rv);

	fprintf(stderr, "%s: bad format \"%s\"\n", pname, s);
	exit(1);
}

colormap_t *
xgetcmap(fd, ncolors)
	int fd;
	int ncolors;
{
	colormap_t *cm;

	cm = malloc(sizeof(colormap_t) + ncolors * sizeof(u_long));
	if (cm == NULL) {
		perror("malloc");
		exit(1);
	}
	cm->first = 0;
	cm->size = ncolors;
	cm->entry = (u_long *) & cm[1];
	xioctl(fd, VIEW_GETCOLORMAP, cm);
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
		printf("monochrome\n");
		return;
	case CM_COLOR:
		printf("color levels: red: %d  green: %d  blue: %d\n",
		    cm->red_mask + 1, cm->green_mask + 1, cm->blue_mask + 1);
		break;
	case CM_GREYSCALE:
		printf("greyscale levels: %d\n", cm->grey_mask + 1);
		break;
	}
	
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
xusage()
{
	fprintf(stderr, "usage: %s [-i] [-w width] [-h height] [-d depth]"
	    " [-x off] [-y off] [-v volume] [-p period] [-t count]"
	    " [color ...]\n", pname);
	exit(1);
}
