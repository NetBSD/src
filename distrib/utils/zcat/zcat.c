/*	$NetBSD: zcat.c,v 1.1.1.1 1996/09/12 20:24:00 gwr Exp $	*/

/* mini zcat.c -- a minimal zcat using the zlib compression library
 * Copyright (C) 1995-1996 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/*
 * Credits, History:
 * This program is a reduced version of the minigzip.c
 * program originally written by Jean-loup Gailly.
 * This reduction is the work of Gordon Ross.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zlib.h"

#define BUFLEN 4096

char *prog;

void error           __P((const char *msg));
void gz_uncompress   __P((gzFile in, FILE   *out));
int  main            __P((int argc, char *argv[]));

/* ===========================================================================
 * Display error message and exit
 */
void error(msg)
    const char *msg;
{
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}

/* ===========================================================================
 * Uncompress input to output then close both files.
 */
void gz_uncompress(in, out)
    gzFile in;
    FILE   *out;
{
    char buf[BUFLEN];
    int len;
    int err;

    for (;;) {
        len = gzread(in, buf, sizeof(buf));
        if (len < 0) error (gzerror(in, &err));
        if (len == 0) break;

        if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
	    error("failed fwrite");
	}
    }
    if (fclose(out)) error("failed fclose");

    if (gzclose(in) != Z_OK) error("failed gzclose");
}


/* ===========================================================================
 * Usage:  miniunzip [files...]
 */

int main(argc, argv)
    int argc;
    char *argv[];
{
    gzFile zfp;

	/* save program name and skip */
    prog = argv[0];
    argc--, argv++;

	/* ignore any switches */
	while (*argv && (**argv == '-')) {
        argc--, argv++;
    }

    if (argc == 0) {
		zfp = gzdopen(fileno(stdin), "rb");
		if (zfp == NULL)
			error("can't gzdopen stdin");
		gz_uncompress(zfp, stdout);
		return 0;
    }

	do {
		/* file_uncompress(*argv); */
		zfp = gzopen(*argv, "rb");
		if (zfp == NULL) {
			fprintf(stderr, "%s: can't gzopen %s\n", prog, *argv);
			exit(1);
		}
		gz_uncompress(zfp, stdout);
	} while (argv++, --argc);
    return 0; /* to avoid warning */
}
