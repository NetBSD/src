/* $NetBSD: mkbootimage.c,v 1.5 2002/04/03 06:16:03 lukem Exp $ */

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

#include <sys/param.h>				/* XXX for roundup, howmany */
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <dev/dec/dec_boot.h>

static void usage(void);

static void
usage()
{
	fprintf(stderr,
	    "usage: %s [-n] [-v] inputfile [outputfile]\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct stat insb;
	struct alpha_boot_block *bb;
	const char *infile, *outfile;
	char *outbuf;
	size_t outbufsize;
	ssize_t rv;
	int c, verbose, nowrite, infd, outfd;

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
	if (sizeof (struct alpha_boot_block) != ALPHA_BOOT_BLOCK_BLOCKSIZE)
		errx(EXIT_FAILURE,
		    "alpha_boot_block structure badly sized (build error)");

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
	outbufsize = roundup(insb.st_size, ALPHA_BOOT_BLOCK_BLOCKSIZE);
	outbufsize += sizeof (struct alpha_boot_block);

	outbuf = malloc(outbufsize);
	if (outbuf == NULL)
		err(EXIT_FAILURE, "allocating output buffer");
	memset(outbuf, 0, outbufsize);

	/* read the file into the buffer, leaving room for the boot block */
	rv = read(infd, outbuf + sizeof (struct alpha_boot_block),
	    insb.st_size);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", infile);
	else if (rv != insb.st_size)
		errx(EXIT_FAILURE, "read %s: short read", infile);
	(void)close(infd);

	/* fill in the boot block fields, and checksum the boot block */
	bb = (struct alpha_boot_block *)outbuf;
	bb->bb_secsize = howmany(insb.st_size, ALPHA_BOOT_BLOCK_BLOCKSIZE);
	bb->bb_secstart = 1;
	bb->bb_flags = 0;
	ALPHA_BOOT_BLOCK_CKSUM(bb, &bb->bb_cksum);

	if (verbose) {
		fprintf(stderr, "starting sector: %qu\n",
		    (unsigned long long)bb->bb_secstart);
		fprintf(stderr, "sector count: %qu\n",
		    (unsigned long long)bb->bb_secsize);
		fprintf(stderr, "checksum: %#qx\n",
		    (unsigned long long)bb->bb_cksum);
	}

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
