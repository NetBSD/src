/*	$NetBSD: installboot.c,v 1.4.12.1 2014/08/19 23:45:38 tls Exp $	*/

/*-
 * Copyright (c) 2005 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "installboot.h"

#define BSIZE		512
#define MAX_SB_SIZE	(64 * 1024)	/* XXX */
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

static	void usage(void);

static	ib_params	installboot_params;

int
main(int argc, char **argv)
{
	ib_params *params;
	uint8_t *bb;
	struct apple_part_map_entry pme;
	size_t bbi;
	struct shared_bbinfo *bbinfop;
	off_t partoff;
	uint32_t nblk, maxblk, blk_i;
	int rv;
	ib_block *blocks;

	setprogname(argv[0]);
	params = &installboot_params;
	memset(params, 0, sizeof(*params));
	params->fsfd = -1;
	params->s1fd = -1;

	if (argc != 4)
		usage();

	params->filesystem = argv[1];

	if ((params->fsfd = open(params->filesystem, O_RDWR, 0600)) == -1)
		err(1, "Opening file system `%s' read", params->filesystem);
	if (fstat(params->fsfd, &params->fsstat) == -1)
		err(1, "Examining file system `%s'", params->filesystem);
#ifdef DEBUG
	printf("file system: %s, %ld bytes\n",
	    params->filesystem, (long)params->fsstat.st_size);
#endif

	/*
	 * Find space for primary boot from the second (NetBSD_BootBlock)
	 * partition.
	 */
	if (pread(params->fsfd, &pme, sizeof pme, BSIZE * 2) != sizeof(pme))
		err(1, "read pme from file system `%s'", params->filesystem);

	if (strcmp(pme.pmPartName, "NetBSD_BootBlock"))
		err(1, "invalid partition map in file system `%s'",
		    params->filesystem);

	/* pmPyPartStart is written by mkisofs */
	partoff = BSIZE * be32toh(pme.pmPyPartStart);

#ifdef DEBUG
	printf("NetBSD partition offset = %ld\n", (long)partoff);
#endif

	params->stage1 = argv[2];

	if ((params->s1fd = open(params->stage1, O_RDONLY, 0600)) == -1)
		err(1, "Opening primary bootstrap `%s'", params->stage1);
	if (fstat(params->s1fd, &params->s1stat) == -1)
		err(1, "Examining primary bootstrap `%s'", params->stage1);
	if (!S_ISREG(params->s1stat.st_mode))
		err(1, "`%s' must be a regular file", params->stage1);

	if (params->s1stat.st_size > MACPPC_BOOT_BLOCK_MAX_SIZE)
		err(1, "primary bootrap `%s' too large (%ld bytes)",
		    params->stage1, (long)params->s1stat.st_size);

#ifdef DEBUG
	printf("primary boot: %s, %ld bytes\n",
	    params->stage1, (long)params->s1stat.st_size);
#endif

	params->stage2 = argv[3];

	bb = calloc(1, MACPPC_BOOT_BLOCK_MAX_SIZE);
	if (bb == NULL)
		err(1, "Allocating %ul bytes for bbinfo",
		    MACPPC_BOOT_BLOCK_MAX_SIZE);

	rv = read(params->s1fd, bb, params->s1stat.st_size);

	if (rv == -1)
		err(1, "Reading `%s'", params->stage1);

	if (memcmp(bb + 1, "ELF", strlen("ELF")) == 0) {
		warnx("`%s' is an ELF executable; need raw binary",
		    params->stage1);
	}

	/* look for the bbinfo structure */
	for (bbi = 0; bbi < MACPPC_BOOT_BLOCK_MAX_SIZE;
	    bbi += sizeof(uint32_t)) {
		bbinfop = (void *)(bb + bbi);
		if (memcmp(bbinfop->bbi_magic, MACPPC_BBINFO_MAGIC,
		    sizeof(bbinfop->bbi_magic)) == 0) {
#ifdef DEBUG
			printf("magic found: %s\n", bbinfop->bbi_magic);
#endif
			break;
		}
	}
	if (bbi >= MACPPC_BOOT_BLOCK_MAX_SIZE)
		err(1, "bbinfo structure not found in `%s'", params->stage1);

	maxblk = be32toh(bbinfop->bbi_block_count);
	if (maxblk == 0 ||
	    maxblk > (MACPPC_BOOT_BLOCK_MAX_SIZE / sizeof(uint32_t)))
		err(1, "bbinfo structure in `%s' has preposterous size `%u'",
		    params->stage1, maxblk);
	
	blocks = malloc(sizeof(*blocks) * maxblk);
	if (blocks == NULL) {
		err(1, "Allocating %lu bytes for blocks",
		    (unsigned long)sizeof(*blocks) * maxblk);
	}

	if (S_ISREG(params->fsstat.st_mode)) {
		if (fsync(params->fsfd) == -1)
			err(1, "Synchronising file system `%s'",
			    params->filesystem);
	}

	nblk = maxblk;
	if (!cd9660_findstage2(params, &nblk, blocks)) {
		exit(1);
	}

	bbinfop->bbi_block_count = htobe32(nblk);
	bbinfop->bbi_block_size = htobe32(blocks[0].blocksize);
	for (blk_i = 0; blk_i < nblk; blk_i++) {
		bbinfop->bbi_block_table[blk_i] = htobe32(blocks[blk_i].block);
		if (blocks[blk_i].blocksize < blocks[0].blocksize &&
		    blk_i + 1 != nblk) {
			warnx("Secondary bootstrap `%s' blocks do not have "
			    "a uniform size", params->stage2);
			exit(1);
		}
	}

	/* XXX no write option */

	if (pwrite(params->fsfd, bb, MACPPC_BOOT_BLOCK_MAX_SIZE, partoff) !=
	    MACPPC_BOOT_BLOCK_MAX_SIZE)
		err(1, "write bootblock");

	if (S_ISREG(params->fsstat.st_mode)) {
		if (fsync(params->fsfd) == -1)
			err(1, "Synchronising file system `%s'",
			    params->filesystem);
	}

	free(bb);

	if (close(params->fsfd) == -1)
		err(1, "Closing file system `%s'", params->filesystem);
	if (close(params->s1fd) == -1)
		err(1, "Closing primary bootstrap `%s'", params->stage1);

	return 0;
}

static void
usage(void)
{
	const char *prog;

	prog = getprogname();
	fprintf(stderr, "usage: %s hybrid-cd-image primary secondary\n", prog);
	exit(1);
}
