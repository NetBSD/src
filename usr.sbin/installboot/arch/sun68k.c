/*	$NetBSD: sun68k.c,v 1.8 2002/05/06 01:49:48 lukem Exp $ */

/*-
 * Copyright (c) 1998, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fredette, Paul Kranenburg, and Luke Mewburn.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: sun68k.c,v 1.8 2002/05/06 01:49:48 lukem Exp $");
#endif	/* !__lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if HAVE_CONFIG_H
#include "../../sys/dev/sun/sun_boot.h"
#else
#include <dev/sun/sun_boot.h>
#endif

#include "installboot.h"

int
sun68k_clearboot(ib_params *params)
{
	char	bb[SUN68K_BOOT_BLOCK_MAX_SIZE];
	ssize_t	rv;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);

	if (params->flags & IB_STARTBLOCK) {
		warnx("Can't use `-b bno' with `-c'");
		return (0);
	}
	/* first check that it _could_ exist here */
	rv = pread(params->fsfd, &bb, sizeof(bb), SUN68K_BOOT_BLOCK_OFFSET);
	if (rv == -1) {
		warn("Reading `%s'", params->filesystem);
		return (0);
	} else if (rv != sizeof(bb)) {
		warnx("Reading `%s': short read", params->filesystem);
		return (0);
	}

	/* now clear it out to nothing */
	memset(&bb, 0, sizeof(bb));

	if (params->flags & IB_VERBOSE)
		printf("%slearing boot block\n",
		    (params->flags & IB_NOWRITE) ? "Not c" : "C");
	if (params->flags & IB_NOWRITE)
		return (1);

	rv = pwrite(params->fsfd, &bb, sizeof(bb), SUN68K_BOOT_BLOCK_OFFSET);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		return (0);
	} else if (rv != sizeof(bb)) {
		warnx("Writing `%s': short write", params->filesystem);
		return (0);
	}

	return (1);
}

int
sun68k_setboot(ib_params *params)
{
	struct stat	bootstrapsb;
	char		bb[SUN68K_BOOT_BLOCK_MAX_SIZE];
	uint32_t	startblock;
	int		retval;
	ssize_t		rv;
	size_t		bbi;
	struct sun68k_bbinfo	*bbinfop;	/* bbinfo in prototype image */
	uint32_t	maxblk, nblk, blk_i;
	ib_block	*blocks = NULL;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);
	assert(params->fstype != NULL);
	assert(params->s1fd != -1);
	assert(params->stage1 != NULL);
	assert(SUN68K_BBINFO_MAGICSIZE == 32);

	if (params->stage2 == NULL) {
		warnx("You must provide the name of the secondary bootstrap");
		return (0);
	}

	retval = 0;

	if (fstat(params->s1fd, &bootstrapsb) == -1) {
		warn("Examining `%s'", params->stage1);
		goto done;
	}
	if (!S_ISREG(bootstrapsb.st_mode)) {
		warnx("`%s' must be a regular file", params->stage1);
		goto done;
	}
	if (bootstrapsb.st_size > sizeof(bb)) {
		warnx("`%s' cannot be larger than %lu bytes",
		    params->stage1, (unsigned long)sizeof(bb));
		goto done;
	}

	memset(&bb, 0, sizeof(bb));
	rv = read(params->s1fd, &bb, sizeof(bb));
	if (rv == -1) {
		warn("Reading `%s'", params->stage1);
		goto done;
	}

	/*
	 * Quick sanity check that the bootstrap given
	 * is *not* an ELF executable.
	 */
	if (memcmp(bb + 1, "ELF", strlen("ELF")) == 0) {
		warnx("`%s' is an ELF executable; need raw binary", 
		    params->stage1);
		goto done;
	}

	/* Look for the bbinfo structure. */
	for (bbi = 0; bbi < sizeof(bb); bbi += sizeof(uint32_t)) {
		bbinfop = (void *) (bb + bbi);
		if (memcmp(bbinfop->bbi_magic, SUN68K_BBINFO_MAGIC,
			    SUN68K_BBINFO_MAGICSIZE) == 0)
			break;
	}
	if (bbi >= sizeof(bb)) {
		warnx("`%s' does not have a bbinfo structure\n", 
		    params->stage1);
		goto done;
	}
	maxblk = be32toh(bbinfop->bbi_block_count);
	if (maxblk == 0 || maxblk > (sizeof(bb) / sizeof(uint32_t))) {
		warnx("bbinfo structure in `%s' has preposterous size `%u'",
		    params->stage1, maxblk);
		goto done;
	}

	/* Allocate space for our block list. */
	blocks = malloc(sizeof(*blocks) * maxblk);
	if (blocks == NULL) {
		warn("Allocating %lu bytes", 
		    (unsigned long) sizeof(*blocks) * maxblk);
		goto done;
	}

	/* Make sure the (probably new) secondary bootstrap is on disk. */
	sync(); sleep(1); sync();

	/* Collect the blocks for the secondary bootstrap. */
	nblk = maxblk;
	if (! params->fstype->findstage2(params, &nblk, blocks))
		goto done;
	if (nblk == 0) {
		warnx("Secondary bootstrap `%s' is empty",
		   params->stage2);
		goto done;
	}

	/* Save those blocks in the primary bootstrap. */
	bbinfop->bbi_block_count = htobe32(nblk);
	bbinfop->bbi_block_size = htobe32(blocks[0].blocksize);
	for (blk_i = 0; blk_i < nblk; blk_i++) {
		bbinfop->bbi_block_table[blk_i] = 
		    htobe32(blocks[blk_i].block);
		if (blocks[blk_i].blocksize < blocks[0].blocksize &&
		    blk_i + 1 != nblk) {
			warnx("Secondary bootstrap `%s' blocks do not have " \
			    "a uniform size\n", params->stage2);
			goto done;
		}
	}

	if (params->flags & IB_STARTBLOCK)
		startblock = params->startblock;
	else
		startblock = SUN68K_BOOT_BLOCK_OFFSET /
		    SUN68K_BOOT_BLOCK_BLOCKSIZE;

	if (params->flags & IB_VERBOSE) {
		printf("Bootstrap start sector: %u\n", startblock);
		printf("Bootstrap byte count:   %u\n", (unsigned)rv);
		printf("Bootstrap block table:  %u entries avail, %u used:",
		    maxblk, nblk);
		for (blk_i = 0; blk_i < nblk; blk_i++)
			printf(" %u", blocks[blk_i].block);
		printf("\n%sriting bootstrap\n",
		    (params->flags & IB_NOWRITE) ? "Not w" : "W");
	}
	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	rv = pwrite(params->fsfd, &bb, sizeof(bb),
	    startblock * SUN68K_BOOT_BLOCK_BLOCKSIZE);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != sizeof(bb)) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	} else {

		/* Sync filesystems (to clean in-memory superblock?) */
		sync();

		retval = 1;
	}

 done:
	if (blocks != NULL)
		free (blocks);
	return (retval);
}
