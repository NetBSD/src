/* $NetBSD: installboot.c,v 1.2.2.3 2001/03/12 13:29:45 bouyer Exp $ */

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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <dev/dec/dec_boot.h>

#include "installboot.h"

static void usage(void);
static void clr_bootstrap(const char *disk);
static void set_bootstrap(const char *disk, const char *bootstrap);
static void set_sunsum(struct vax_boot_block *bb);

int verbose, nowrite, append, isoblock, sunsum;
struct stat disksb;

static void
usage()
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s [-nsv] [-i block | -a] disk bootstrap\n",
	    getprogname());
	fprintf(stderr, "\t%s [-nsv] -c disk\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const char *disk, *bootstrap;
	int c, clearflag;

	clearflag = verbose = nowrite = append = isoblock = sunsum = 0;

	while ((c = getopt(argc, argv, "aci:nsv")) != -1) {
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
		case 's':
			/* Recompute the sun checksum */
			sunsum = 1;
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
		if (sunsum){fprintf(stderr, "will restore sun checksum\n");}
	}
	if (sizeof (struct vax_boot_block) != VAX_BOOT_BLOCK_BLOCKSIZE)
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
	struct vax_boot_block bb;
	ssize_t rv;
	int diskfd;

	if ((diskfd = open(disk, nowrite ? O_RDONLY : O_RDWR)) == -1)
		err(EXIT_FAILURE, "open %s", disk);

	rv = pread(diskfd, &bb, sizeof bb, VAX_BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	if (bb.bb_id_offset * 2 != offsetof(struct vax_boot_block, bb_magic1)
	    || bb.bb_magic1 != VAX_BOOT_MAGIC1) {
		fprintf(stderr, "old boot block magic number invalid\n");
		fprintf(stderr, "boot block invalid\n");
		goto done;
	}

	bb.bb_id_offset = 1;
	bb.bb_mbone = 0;
	bb.bb_lbn_hi = 0;
	bb.bb_lbn_low = 0;

        /* restore sun checksum */
	if (sunsum)
		set_sunsum(&bb);

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    goto done;
	}

	if (verbose)
		fprintf(stderr, "writing\n");
	
	rv = pwrite(diskfd, &bb, sizeof bb, VAX_BOOT_BLOCK_OFFSET);
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
	struct vax_boot_block bb;
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

	rv = pread(diskfd, &bb, sizeof bb, VAX_BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "read %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "read %s: short read", disk);

	/* fill in the updated boot block fields */
	if (append) {
		startblock = howmany(disksb.st_size, VAX_BOOT_BLOCK_BLOCKSIZE);
	} else if (isoblock) {
		startblock = isoblock * (ISO_DEFAULT_BLOCK_SIZE / VAX_BOOT_BLOCK_BLOCKSIZE);
	} else {
		startblock = VAX_BOOT_BLOCK_OFFSET / VAX_BOOT_BLOCK_BLOCKSIZE + 1;
	}

	bb.bb_id_offset = offsetof(struct vax_boot_block, bb_magic1) / 2;
	bb.bb_mbone = 1;
	bb.bb_lbn_hi = htole16((u_int16_t) (startblock >> 16));
	bb.bb_lbn_low = htole16((u_int16_t) (startblock >>  0));
	/*
	 * Now the identification block
	 */
	bb.bb_magic1 = VAX_BOOT_MAGIC1;
	bb.bb_mbz1 = 0;
	bb.bb_sum1 = ~(bb.bb_magic1 + bb.bb_mbz1 + bb.bb_pad1);

	bb.bb_mbz2 = 0;
	bb.bb_volinfo = VAX_BOOT_VOLINFO_NONE;
	bb.bb_pad2a = 0;
	bb.bb_pad2b = 0;

	bb.bb_size = htole32(bootstrapsize / VAX_BOOT_BLOCK_BLOCKSIZE);
	bb.bb_load = htole32(VAX_BOOT_LOAD);
	bb.bb_entry = htole32(VAX_BOOT_ENTRY);
	bb.bb_sum3 = htole32(le32toh(bb.bb_size) + le32toh(bb.bb_load) \
	    + le32toh(bb.bb_entry));

        /* restore sun checksum */
	if (sunsum)
		set_sunsum(&bb);

	if (verbose) {
		fprintf(stderr, "bootstrap starting sector: %i\n",
		    startblock);
		fprintf(stderr, "bootstrap sector count: %i\n",
		    le32toh(bb.bb_size));
	}

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    goto done;
	}

	if (verbose)
		fprintf(stderr, "writing bootstrap\n");

	rv = pwrite(diskfd, bootstrapbuf, bootstrapsize,
	     startblock * VAX_BOOT_BLOCK_BLOCKSIZE);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", disk);
	else if (rv != bootstrapsize)
		errx(EXIT_FAILURE, "write %s: short write", disk);

	if (verbose)
		fprintf(stderr, "writing boot block\n");

	rv = pwrite(diskfd, &bb, sizeof bb, VAX_BOOT_BLOCK_OFFSET);
	if (rv == -1)
		err(EXIT_FAILURE, "write %s", disk);
	else if (rv != sizeof bb)
		errx(EXIT_FAILURE, "write %s: short write", disk);

done:
	(void)close(diskfd);
}

static void
set_sunsum(struct vax_boot_block *bb)
{
	u_int16_t *sp, sum;

	sp = (u_int16_t *)bb;
	sum = 0;
	while (sp < (u_int16_t *)((bb)+1) - 1) {
	sum ^= *sp++;   /* XXX no need to ntohs/htons for XOR */
	}
	sp = (u_int16_t *)bb;
#define bb_sunsum sp[VAX_BOOT_BLOCK_BLOCKSIZE/2 - 1]
	if (verbose) {
		fprintf(stderr,"old sun checksum:  0x%04x\n",be16toh(bb_sunsum));
		fprintf(stderr,"recalculated sun checksum:  0x%04x\n",be16toh(sum));
	}
	bb_sunsum = sum;
}
