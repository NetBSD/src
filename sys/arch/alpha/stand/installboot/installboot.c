/* $NetBSD: installboot.c,v 1.16 1999/06/14 23:55:29 cgd Exp $ */

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

static void usage(void);
static void clr_bootstrap(const char *disk);
static void set_bootstrap(const char *disk, const char *bootstrap);

extern char *__progname;

int verbose, nowrite;

static void
usage()
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s [-n] [-v] disk bootstrap\n", __progname);
	fprintf(stderr, "\t%s [-n] [-v] -c disk\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const char *disk, *bootstrap;
	int c, clearflag;

	clearflag = verbose = nowrite = 0;

	while ((c = getopt(argc, argv, "cnv")) != -1) {
		switch (c) {
		case 'c':
			/* Clear any existing boot block */
			clearflag = 1;
			break;
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

	if ((clearflag && argc != 1) || (!clearflag && argc != 2))
		usage();

	disk = argv[0];
	bootstrap = argv[1];		/* NULL if argc == 1 */

	if (verbose) {
		fprintf(stderr, "disk: %s\n", disk);
		fprintf(stderr, "bootstrap: %s\n",
		    bootstrap != NULL ? bootstrap : "to be cleared");
	}
	if (sizeof (struct boot_block) != BOOT_BLOCK_BLOCKSIZE)
		errx(EXIT_FAILURE,
		    "boot_block structure badly sized (build error)");

	if (clearflag)
		clr_bootstrap(disk);
	else
		set_bootstrap(disk, bootstrap);

	exit(EXIT_SUCCESS);
}

static void
clr_bootstrap(const char *disk)
{
	struct boot_block bb;
	u_int64_t cksum;
	ssize_t rv;
	int diskfd;

	if ((diskfd = open(disk, nowrite ? O_RDONLY : O_RDWR)) == -1)
		err(EXIT_FAILURE, "open %s", disk);

	rv = pread(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	CHECKSUM_BOOT_BLOCK(&bb, &cksum);
	if (cksum != bb.bb_cksum) {
		fprintf(stderr,
		    "old boot block checksum invalid (was %#llx, calculated %#llx)\n",
		    (unsigned long long)bb.bb_cksum,
		    (unsigned long long)cksum);
		fprintf(stderr, "boot block invalid\n");
		goto done;
	} else if (verbose) {
		fprintf(stderr, "old boot block checksum: %#llx\n",
		    (unsigned long long)bb.bb_cksum);
		fprintf(stderr, "old boot block start sector: %#llx\n",
		    (unsigned long long)bb.bb_secstart);
		fprintf(stderr, "old boot block size: %#llx\n",
		    (unsigned long long)bb.bb_secsize);
	}

	bb.bb_secstart = bb.bb_secsize = bb.bb_flags = 0;
	CHECKSUM_BOOT_BLOCK(&bb, &bb.bb_cksum);

	fprintf(stderr, "new boot block checksum: %#llx\n",
	    (unsigned long long)bb.bb_cksum);
	fprintf(stderr, "new boot block start sector: %#llx\n",
	    (unsigned long long)bb.bb_secstart);
	fprintf(stderr, "new boot block size: %#llx\n",
	    (unsigned long long)bb.bb_secsize);

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    goto done;
	}

	if (verbose)
		fprintf(stderr, "writing\n");
	
	rv = pwrite(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "write %s: short write", disk);

done:
	(void)close(diskfd);
}

static void
set_bootstrap(const char *disk, const char *bootstrap)
{
	struct stat bootstrapsb;
	struct boot_block bb;
	int bootstrapfd, diskfd;
	char *bootstrapbuf;
	size_t bootstrapsize;
	ssize_t rv;

	/* Open the input file and check it out */
	if ((bootstrapfd = open(bootstrap, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "open %s", bootstrap);
	if (fstat(bootstrapfd, &bootstrapsb) == -1)
		err(EXIT_FAILURE, "fstat %s", bootstrap);
	if (!S_ISREG(bootstrapsb.st_mode))
		errx(EXIT_FAILURE, "%s must be a regular file", bootstrap);

	/*
	 * Allocate a buffer, with space to round up the input file
	 * to the next block size boundary, and with space for the boot
	 * block. 
	 */
	bootstrapsize = roundup(bootstrapsb.st_size, BOOT_BLOCK_BLOCKSIZE);

	bootstrapbuf = malloc(bootstrapsize);
	if (bootstrapbuf == NULL)
		err(EXIT_FAILURE, "allocating buffer to hold bootstrap");
	memset(bootstrapbuf, 0, bootstrapsize);

	/* read the file into the buffer */
	rv = read(bootstrapfd, bootstrapbuf, bootstrapsb.st_size);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", bootstrap);
	else if (rv != bootstrapsb.st_size)
		errx(EXIT_FAILURE, "read %s: short read", bootstrap);
	(void)close(bootstrapfd);

	if ((diskfd = open(disk, nowrite ? O_RDONLY : O_RDWR)) == -1)
		err(EXIT_FAILURE, "open %s", disk);

	rv = pread(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	/* fill in the updated boot block fields, and checksum boot block */
	bb.bb_secsize = howmany(bootstrapsb.st_size, BOOT_BLOCK_BLOCKSIZE);
	bb.bb_secstart = 1;
	bb.bb_flags = 0;
	CHECKSUM_BOOT_BLOCK(&bb, &bb.bb_cksum);

	if (verbose) {
		fprintf(stderr, "bootstrap starting sector: %llu\n",
		    (unsigned long long)bb.bb_secstart);
		fprintf(stderr, "bootstrap sector count: %llu\n",
		    (unsigned long long)bb.bb_secsize);
		fprintf(stderr, "new boot block checksum: %#llx\n",
		    (unsigned long long)bb.bb_cksum);
	}

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    goto done;
	}

	if (verbose)
		fprintf(stderr, "writing bootstrap\n");

	rv = pwrite(diskfd, bootstrapbuf, bootstrapsize,
	    BOOT_BLOCK_OFFSET + BOOT_BLOCK_BLOCKSIZE);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", disk);
	else if (rv != bootstrapsize)
		errx(EXIT_FAILURE, "write %s: short write", disk);

	if (verbose)
		fprintf(stderr, "writing boot block\n");

	rv = pwrite(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "write %s: short write", disk);

done:
	(void)close(diskfd);
}
