/* iteconfig.c 
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
 *	$Id: iteconfig.c,v 1.2 1994/02/01 12:13:17 chopps Exp $
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "grf/grf_types.h"
#include "grf/grf_colormap.h"
#include "viewioctl.h"
#include "iteioctl.h"
#include "termios.h"

int version = 1;
int revision = 0;

char *program_name;
char *optstr = "W:H:D:V:T:P:X:Y:hiv";
char *help_string = "options: \n"
    "   [-vih] [-D DEPTH] [-H HEIGHT] [-P PERIOD] [-T COUNT]\n"
    "   [-V VOLUME] [-W WIDTH] [-X OFFSET] [-Y OFFSET]\n"
    "   ColorX is one of 0xRRGGBB, 0xGG or 0xM";

void xioctl (int fdesc, int cmd, void *addr);
long xstrtol (char *s);
void ident_program (void);
colormap_t *xget_colormap (int cfd, int ncolors);
void print_colormap (colormap_t *cm, int cols);

int 
main (int argc, char **argv)
{
    colormap_t *cm;
    struct ite_window_size is, newis;
    struct ite_bell_values ib, newib;
    struct winsize ws;
    int opt, cfd;
    int opt_info = 0, i, max_colors;
    long val;
    
    program_name = argv[0];
    
    cfd = open ("/dev/console", O_NONBLOCK|O_RDONLY);
    if (cfd == -1) {
    	fprintf (stderr, "%s: failed to open console device\n", argv[0]);
    	exit (0);
    }

    xioctl (cfd, ITE_GET_WINDOW_SIZE, &is);
    xioctl (cfd, ITE_GET_BELL_VALUES, &ib);

    bcopy (&is, &newis, sizeof (is));
    bcopy (&ib, &newib, sizeof (ib));

    while (EOF != (opt = getopt (argc, argv, optstr))) {
	switch (opt) {
	  case 'i':
	    opt_info = 1;
	    break;
	  case 'X':
	    newis.x = xstrtol (optarg);
	    break;	    
	  case 'Y':
	    newis.y = xstrtol (optarg);
	    break;	    
	  case 'W':
	    newis.width = xstrtol (optarg);
	    break;
	  case 'H':
	    newis.height = xstrtol (optarg);
	    break;
	  case 'D':
	    newis.depth = xstrtol (optarg);
	    break;
	  case 'V':
            newib.volume = xstrtol (optarg);
	    break;
	  case 'P':
	    newib.period = xstrtol (optarg);
	    break;
	  case 'T':
	    newib.time = xstrtol (optarg);
	    break;
	  case '?':
	  case 'h':
	    ident_program ();
	    fprintf (stdout, "usage: %s [options] [Color0 [Color1 ...[ColorX]]]\n", argv[0]);
	    fprintf (stdout, "%s\n", help_string);
	    exit (0);
	  case 'v':
	    ident_program ();
	    exit (0);
	}
    }

    if (bcmp (&newis, &is, sizeof (is))) {
	xioctl (cfd, ITE_SET_WINDOW_SIZE, &newis);
	xioctl (cfd, ITE_GET_WINDOW_SIZE, &is);
    }
    
    if (bcmp (&newib, &ib, sizeof (ib))) {
	xioctl (cfd, ITE_SET_BELL_VALUES, &newib);
	xioctl (cfd, ITE_GET_BELL_VALUES, &ib);
    }

    max_colors = 1 << is.depth;

    cm = xget_colormap (cfd, max_colors);

    if (optind < argc) {
	int idx;
	for (i = 0; optind < argc; i++, optind++) {
	    val = xstrtol (argv[optind]);
	    if (i >= max_colors) {
		fprintf (stderr, "%s: maximum colors reached, droping all after 0x%x\n",
			 program_name, val);
		break;
	    }
	    cm->entry[i] = val;
	}
	
	xioctl (cfd, VIEW_USECOLORMAP, cm);
	free (cm);
	
	/* fetch it again to get valid new colors. */
	cm = xget_colormap (cfd, max_colors);
    }

    /* do tty stuff to get it to register the changes. */
    xioctl (cfd, TIOCGWINSZ, &ws);

    if (opt_info) {
	/* print ite info */
	printf ("tty size:: Rows:%d Cols:%d\n",ws.ws_row, ws.ws_col);
	printf ("ite size:: X:%d Y:%d W:%d H:%d D:%d\n",
		is.x, is.y, is.width, is.height, is.depth);
	printf ("ite bell:: Vol:%d Time:%d Period:%d\n",
		ib.volume, ib.time, ib.period);

	print_colormap (cm, ws.ws_col);

    }
    
    close (cfd);
    return (0);
}

void
ident_program (void)
{
    static done = 0;

    if (!done) {
	fprintf (stdout, "%s %d.%d Copyright (C) 1993 Christian E. Hopps\n",
		 program_name, version, revision);
	
	done = 1;
    }
}

void
xioctl (int fdesc, int cmd, void *addr)
{
    if (ioctl (fdesc, cmd, addr)) {
    	perror ("ioctl:");
    	exit (1);
    }
}

long
xstrtol (char *s)
{
    long val = strtol (s, NULL, 0);
    if (errno != ERANGE || (val != LONG_MAX && val != LONG_MIN)) {
	return val;
    }
    fprintf (stderr, "%s: unrecognized number format \"%s\"\n",
	     program_name, s);
    fprintf (stderr, "\"%s --help\" for available options\n",
	     program_name);
    
    exit (1);
}

void *
xmalloc (size_t bytes)
{
    void *mem = malloc (bytes);
    if (!mem) {
	fprintf (stderr, "%s: no available memory\n",
		 program_name);
	exit (1);
    }
    return (mem);
}

colormap_t *
xget_colormap (int cfd, int ncolors)
{
    colormap_t *cm = xmalloc (sizeof (colormap_t) + ncolors*sizeof (u_long));
    cm->first = 0;
    cm->size = ncolors;
    cm->entry = (u_long *)&cm[1];
    xioctl (cfd, VIEW_GETCOLORMAP, cm);
    return (cm);
}

void
print_colormap (colormap_t *cm, int cols)
{
    int i, nl = cols/12 - 1;
    switch (cm->type) {
      case CM_MONO:
	printf ("Monochrome Display\n");
	return;
      case CM_COLOR:
	printf ("Color Display:: Valid Color Levels:: Red:%d Green:%d Blue:%d\n",
		cm->red_mask+1, cm->green_mask+1, cm->blue_mask+1);
	break;
      case CM_GREYSCALE:
	printf ("Greyscale Display:: Valid Grey Levels:%d\n", cm->grey_mask+1);
	break;
    }
    for (i = 0; i < cm->size; i++) {
	printf ("0x%08lx ", cm->entry[i]);
	if (!((i+1)%nl)) 
	    printf ("\n");
    }
    if ((i+1)%nl) 
	printf ("\n");
}
