/* $NetBSD: installboot.c,v 1.19 1999/10/04 19:19:11 ross Exp $ */

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

#include <sys/param.h>				/* XXX for roundup, howmany */
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <dev/sun/disklabel.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void usage(void);
static void clr_bootstrap(const char *disk);
void check_sparc(const struct boot_block * const, const char *);
static u_int16_t compute_sparc(const u_int16_t *);
static void sun_bootstrap(struct boot_block * const);
static void set_bootstrap(const char *disk, const char *bootstrap);

extern char *__progname;

int sunflag, verbose, nowrite;

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

	while ((c = getopt(argc, argv, "cnsv")) != -1) {
		switch (c) {
		case 'c':
			/* Clear any existing boot block */
			if (sunflag)
				goto argdeath;
			clearflag = 1;
			break;
		case 'n':
			/* Do not actually write the boot file */
			nowrite = 1;
			break;
		case 's':
			/*
			 * Sun checksum and magic number overlay
			 */
			if (clearflag)
				goto argdeath;
			sunflag = 1;
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
argdeath:
	errx(EXIT_FAILURE, "incompatible options");
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

	if (sunflag)
		check_sparc(&bb, "initial");

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

	if (sunflag)
		sun_bootstrap(&bb);

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

/*
 * The Sun and alpha checksums overlay, and the Sun magic number also
 * overlays the alpha checksum. If you think you are smart: stop here
 * and do exercise one: figure out how to salt unimportant u_int16_t
 * words in mid-sector so that the alpha and sparc checksums match,
 * and so the Sun magic number is embedded in the alpha checksum.
 *
 * The last u_int64_t in the sector is the alpha arithmetic checksum.
 * The last u_int16_t in the sector is the sun xor checksum.
 * The penultimate u_int16_t in the sector is the sun magic number.
 *
 *	A:   511     510     509     508     507     506     505     504
 *	S:   510     511     508     509     506     507     504     505
 *	63     :       :       :     32:31     :       :       :       0
 *	|      :       :       :      \:|      :       :       :       |
 *	7654321076543210765432107654321076543210765432107654321076543210
 *	|--  sparc   --||--  sparc   --|
 *	|-- checksum --||--  magic   --|
 *	|----------------------- alpha checksum -----------------------|
 *			1011111011011010
 *			   b   e   d   a
 */

static void
resum(struct boot_block * const bb, u_int16_t *bb16)
{
	memcpy(bb, bb16, sizeof *bb);
	CHECKSUM_BOOT_BLOCK(bb, &bb->bb_cksum);
	memcpy(bb16, bb, sizeof *bb);
}

static void
sun_bootstrap(struct boot_block * const bb)
{
#	define BB_ADJUST_OFFSET 64
	u_int16_t i, j, chkdelta, sunsum, bb16[256];

	/*
	 * Theory: the alpha checksum is adjusted so bits 47:32 add up
	 * to the Sun magic number. Then, another adjustment is computed
	 * so bits 63:48 add up to the Sun checksum, and applied in pieces
	 * so it changes the alpha checksum but not the Sun value.
	 *
  	 * Note: using memcpy(3) instead of a union as a strict c89/c9x
  	 * conformance experiment and to avoid a public interface delta.
	 */
	assert(sizeof bb16 == sizeof *bb);
	memcpy(bb16, bb, sizeof bb16);
	for (i = 0; i < 8; ++i) {
		j = BB_ADJUST_OFFSET + i;
		if (bb16[j]) {
			warnx("non-zero bits %04x in bytes %d..%d",
			    bb16[j], j * 2, j * 2 + 1);
			bb16[j] = 0;
			resum(bb, bb16);
		}
	}
	/*
	 * Make alpha checksum <47:32> come out to the sun magic.
	 */
	bb16[BB_ADJUST_OFFSET + 2] = htons(SUN_DKMAGIC) - bb16[254];
	resum(bb, bb16);
	sunsum = compute_sparc(bb16);		/* might be the final value */
	if (verbose)
		printf("target sun checksum is %04x\n", sunsum);
	/*
	 * Arrange to have alpha 63:48 add up to the sparc checksum.
	 */
	chkdelta = sunsum - bb16[255];
	bb16[BB_ADJUST_OFFSET + 3] = chkdelta >> 1;
	bb16[BB_ADJUST_OFFSET + 7] = chkdelta >> 1;
	/*
	 * By placing half the correction in two different u_int64_t words at
	 * positions 63:48, the sparc sum will not change but the alpha sum
	 * will have the full correction, but only if the target adjustment
	 * was even. If it was odd, reverse propagate the carry one place.
	 */
	if (chkdelta & 1) {
		if (verbose)
			printf("target adjustment %04x was odd, correcting\n",
			    chkdelta);
		bb16[BB_ADJUST_OFFSET + 2] += 0x8000;
		bb16[BB_ADJUST_OFFSET + 6] += 0x8000;
	}
	resum(bb, bb16);
	if (verbose)
		printf("final harmonized checksum: %016lx\n", bb->bb_cksum);
	check_sparc(bb, "final");
}

void
check_sparc(const struct boot_block * const bb, const char *when)
{
	u_int16_t bb16[256];
	const char * const wmsg = "warning, %s sparc %s 0x%04x invalid,"
	    " expected 0x%04x";

	memcpy(bb16, bb, sizeof bb16);
	if (compute_sparc(bb16) != bb16[255])
		warnx(wmsg, when, "checksum", bb16[255], compute_sparc(bb16));
	if (bb16[254] != htons(SUN_DKMAGIC))
		warnx(wmsg, when, "magic number", bb16[254],
		    htons(SUN_DKMAGIC));
}

static u_int16_t
compute_sparc(const u_int16_t *bb16)
{
	u_int16_t i, s = 0;

	for (i = 0; i < 255; ++i)
		s ^= bb16[i];
	return s;
}
