/*	$NetBSD: mkbootimage.c,v 1.1 1999/06/28 00:35:23 sakamoto Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
 */

#include <sys/param.h>				/* XXX for roundup */
#include <sys/stat.h>
#include <sys/time.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <machine/endian.h>
#include <machine/bswap.h>
#include "bootimage.h"

#if BYTE_ORDER == LITTLE_ENDIAN
#define	BO(x)	bswap32(x)
#else
#define	BO(x)
#endif

static void usage(void);

extern char *__progname;

static void
usage()
{
	fprintf(stderr,
	    "usage: %s [-n] [-v] inputfile [outputfile]\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const char *infile, *outfile;
	char *outbuf;
	struct image_block *p;
	struct stat insb;
	struct timeval tp;
	struct timezone tzp;
	size_t outbufsize;
	ssize_t rv;
	int c, verbose, nowrite, infd, outfd;
	long *offset;

	verbose = nowrite = 0;

	while ((c = getopt(argc, argv, "nv")) != -1) {
		switch (c) {
		case 'n':
			/* Do not actually write the boot file */
			nowrite = 1;
			break;
		case 'v':
			/* Chat */
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 && argc != 2)
		usage();

	infile = argv[0];
	outfile = argv[1];		/* NULL if argc == 1 */

	if (verbose) {
		fprintf(stderr, "input file: %s\n", infile);
		fprintf(stderr, "output file: %s\n",
		    outfile != NULL ? outfile : "<stdout>");
	}

	/* Open the input file and check it out */
	if ((infd = open(infile, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "open %s", infile);
	if (fstat(infd, &insb) == -1)
		err(EXIT_FAILURE, "fstat %s", infile);
	if (!S_ISREG(insb.st_mode))
		errx(EXIT_FAILURE, "%s must be a regular file", infile);

	/*
	 * Allocate a buffer, with space to round up the input file
	 * to the next block size boundary, and with space for the boot
	 * block.
	 */
	outbufsize = roundup(insb.st_size, BLOCK_SIZE);
	outbufsize += HEADER_SIZE;
	outbufsize = roundup(outbufsize, 512);

	outbuf = malloc(outbufsize);
	if (outbuf == NULL)
		err(EXIT_FAILURE, "allocating output buffer");
	memset(outbuf, 0, outbufsize);

	/* copy the boot image info the buffer */
	for (p = image_block; p->offset != -1; p++)
		memcpy(outbuf + p->offset, p->data, p->size);

	/* fill used block bitmap */
	memset(outbuf + FILE_BLOCK_MAP_START, 0xff,
		FILE_BLOCK_MAP_END - FILE_BLOCK_MAP_START);

	/* fix the file size in the header */
	*(long *)(outbuf + FILE_SIZE_OFFSET) =
		(long)BO(insb.st_size);
	*(long *)(outbuf + FILE_SIZE_ALIGN_OFFSET) =
		(long)BO(roundup(insb.st_size, BLOCK_SIZE));

	/* set mtime */
	gettimeofday(&tp, &tzp);
	for (offset = mtime_offset; *offset != -1; offset++)
		*(long *)(outbuf + *offset) = (long)BO(tp.tv_sec);

	/* read the file into the buffer, leaving room for the boot block */
	rv = read(infd, outbuf + HEADER_SIZE, insb.st_size);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", infile);
	else if (rv != insb.st_size)
		errx(EXIT_FAILURE, "read %s: short read", infile);
	(void)close(infd);

	if (nowrite)
		exit(EXIT_SUCCESS);

	/* set up the output file descriptor */
	if (outfile == NULL) {
		outfd = STDOUT_FILENO;
		outfile = "<stdout>";
	} else if ((outfd = open(outfile, O_WRONLY|O_CREAT, 0666)) == -1)
		err(EXIT_FAILURE, "open %s", outfile);

	/* write the data */
	rv = write(outfd, outbuf, outbufsize);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", outfile);
	else if (rv != outbufsize)
		errx(EXIT_FAILURE, "write %s: short write", outfile);
	(void)close(outfd);

	exit(EXIT_SUCCESS);
}
