/* $NetBSD: installboot.c,v 1.1 1999/11/28 00:32:29 simonb Exp $ */

/*
 * Copyright (c) 1999 Ross Harvey.  All rights reserved.
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
 *      This product includes software developed by Ross Harvey
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

#include <sys/param.h>		/* XXX for roundup, howmany */
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <isofs/cd9660/iso.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <include/dec_boot.h>

#include "installboot.h"

static void usage(void);
static void clr_bootstrap(const char *disk);
static void set_bootstrap(const char *disk, const char *bootstrap);

extern char *__progname;

int verbose, nowrite, append, isoblock;
struct stat disksb;

static void
usage()
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s [-nv] [-i block | -a] disk bootstrap\n", __progname);
	fprintf(stderr, "\t%s [-nv] -c disk\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const char *disk, *bootstrap;
	int c, clearflag;

	clearflag = verbose = nowrite = append = isoblock = 0;

	while ((c = getopt(argc, argv, "aci:nv")) != -1) {
		switch (c) {
		case 'a':
			/* Append to disk (image) */
			append = 1;
			break;
		case 'c':
			/* Clear any existing boot block */
			clearflag = 1;
			break;
		case 'i':
			/* Load bootstrap at iso block number */
			if ((isoblock = atoi(optarg)) <= 0)
				errx(1, "%s: bad iso block number", optarg);
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
	if (append && isoblock)
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

	if (append) {
		/* Only append to a regular file */
		if (stat(disk, &disksb) == -1)
			err(EXIT_FAILURE, "stat %s", disk);
		if (!S_ISREG(disksb.st_mode))
			errx(EXIT_FAILURE, "%s must be a regular file to append to", bootstrap);
	}

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
	ssize_t rv;
	int diskfd;

	if ((diskfd = open(disk, nowrite ? O_RDONLY : O_RDWR)) == -1)
		err(EXIT_FAILURE, "open %s", disk);

	rv = pread(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	if (bb.magic != DEC_BOOT_MAGIC) {
		fprintf(stderr, "old boot block magic number invalid (%#x)\n",
		    bb.magic);
		fprintf(stderr, "boot block invalid\n");
		goto done;
	}

	bb.map[0].num_blocks = bb.map[0].start_block = bb.mode = 0;
	bb.magic = DEC_BOOT_MAGIC;

	fprintf(stderr, "new boot block start sector: %#x\n",
	    bb.map[0].start_block);
	fprintf(stderr, "new boot block size: %#x\n",
	    bb.map[0].num_blocks);

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
	int diskfd, startblock;
	char *bootstrapbuf;
	size_t bootstrapsize;
	u_int32_t bootstrapload, bootstrapexec;
	ssize_t rv;

	/* Open the input file and check it out */
	if (stat(bootstrap, &bootstrapsb) == -1)
		err(EXIT_FAILURE, "stat %s", bootstrap);
	if (!S_ISREG(bootstrapsb.st_mode))
		errx(EXIT_FAILURE, "%s must be a regular file", bootstrap);
	load_bootstrap(bootstrap, &bootstrapbuf, &bootstrapload, &bootstrapexec,
	    &bootstrapsize);

	if ((diskfd = open(disk, nowrite ? O_RDONLY : O_RDWR)) == -1)
		err(EXIT_FAILURE, "open %s", disk);

	rv = pread(diskfd, &bb, sizeof bb, BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	/* fill in the updated boot block fields */
	if (append) {
		startblock = howmany(disksb.st_size, BOOT_BLOCK_BLOCKSIZE);
	} else if (isoblock) {
		startblock = isoblock * (ISO_DEFAULT_BLOCK_SIZE / BOOT_BLOCK_BLOCKSIZE);
	} else {
		startblock = BOOT_BLOCK_OFFSET / BOOT_BLOCK_BLOCKSIZE + 1;
	}

	bb.map[0].start_block = startblock;
	bb.map[0].num_blocks = howmany(bootstrapsize, BOOT_BLOCK_BLOCKSIZE);
	bb.magic = DEC_BOOT_MAGIC;
	bb.load_addr = bootstrapload;
	bb.exec_addr = bootstrapexec;
	bb.mode = DEC_BOOTMODE_CONTIGUOUS;

	if (verbose) {
		fprintf(stderr, "bootstrap starting sector: %i\n",
		    bb.map[0].start_block);
		fprintf(stderr, "bootstrap sector count: %i\n",
		    bb.map[0].num_blocks);
		fprintf(stderr, "bootstrap load address: %#x\n",
		    bb.load_addr);
		fprintf(stderr, "bootstrap execute address: %#x\n",
		    bb.exec_addr);
	}

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    goto done;
	}

	if (verbose)
		fprintf(stderr, "writing bootstrap\n");

	rv = pwrite(diskfd, bootstrapbuf, bootstrapsize,
	     startblock * BOOT_BLOCK_BLOCKSIZE);
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
