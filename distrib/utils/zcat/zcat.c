/*	$NetBSD: zcat.c,v 1.5 2017/01/12 01:58:03 christos Exp $	*/

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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "zlib.h"

#define BUFLEN 4096

char *prog;

static void error(const char *, ...) __printflike(1, 2);
static void gz_uncompress(gzFile, int);

/* ===========================================================================
 * Display error message and exit
 */
static void
error(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	int l;

	l = snprintf_ss(buf, sizeof(buf), "%s: ", prog);
	write(STDERR_FILENO, buf, l);
	va_start(ap, fmt);
	l = vsnprintf_ss(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	write(STDERR_FILENO, buf, l);
	_exit(EXIT_SUCCESS);
}

/* ===========================================================================
 * Uncompress input to output then close both files.
 */
static void
gz_uncompress(gzFile in, int out)
{
	char buf[BUFLEN];
	int len;
	int err;

	for (;;) {
		len = gzread(in, buf, sizeof(buf));
		if (len < 0)
			error ("%s", gzerror(in, &err));
		if (len == 0)
			break;

		if ((int)write(out, buf, (size_t)len) != len) {
			error("failed fwrite");
		}
	}
	if (close(out))
		error("failed fclose");

	if (gzclose(in) != Z_OK)
		error("failed gzclose");
}


/* ===========================================================================
 * Usage:  zcat [files...]
 */

int
main(int argc, char *argv[])
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
		zfp = gzdopen(STDIN_FILENO, "rb");
		if (zfp == NULL)
			error("can't gzdopen stdin");
		gz_uncompress(zfp, STDOUT_FILENO);
		return 0;
	}

	do {
		/* file_uncompress(*argv); */
		zfp = gzopen(*argv, "rb");
		if (zfp == NULL) {
			error("can't gzopen `%s'", *argv);
			_exit(EXIT_FAILURE);
		}
		gz_uncompress(zfp, STDOUT_FILENO);
	} while (argv++, --argc);
	return 0; /* to avoid warning */
}


/*
 * XXX: hacks to keep gzio.c from pulling in deflate stuff
 */

int deflateInit2_(z_streamp strm, int  level, int  method,
    int windowBits, int memLevel, int strategy,
    const char *version, int stream_size)
{

	return -1;
}

int deflate(z_streamp strm, int flush)
{

	return -1;
}

int deflateEnd(z_streamp strm)
{

	return -1;
}

int deflateParams(z_streamp strm, int level, int strategy)
{

	return Z_STREAM_ERROR;
}
