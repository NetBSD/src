/*	$NetBSD: sparc.c,v 1.4 2002/05/15 02:18:23 lukem Exp $ */

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
__RCSID("$NetBSD: sparc.c,v 1.4 2002/05/15 02:18:23 lukem Exp $");
#endif	/* !__lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>

#include <assert.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

int
sparc_clearboot(ib_params *params)
{
	char	bb[SPARC_BOOT_BLOCK_MAX_SIZE];
	ssize_t	rv;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);

	if (params->flags & IB_STAGE2START) {
		warnx("Can't use `-B bno' with `-c'");
		return (0);
	}
	if (params->flags & IB_STAGE1START) {
		warnx("`-b bno' is not supported for %s",
		    params->machine->name);
		return (0);
	}

	/* first check that it _could_ exist here */
	rv = pread(params->fsfd, &bb, sizeof(bb), SPARC_BOOT_BLOCK_OFFSET);
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

	rv = pwrite(params->fsfd, &bb, sizeof(bb), SPARC_BOOT_BLOCK_OFFSET);
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
sparc_setboot(ib_params *params)
{
	char		bb[SPARC_BOOT_BLOCK_MAX_SIZE];
	int		retval;
	ssize_t		rv;
	size_t		bbi;
	struct sparc_bbinfo	*bbinfop;	/* bbinfo in prototype image */
	uint32_t	maxblk, nblk, blk_i;
	ib_block	*blocks;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);
	assert(params->fstype != NULL);
	assert(params->s1fd != -1);
	assert(params->stage1 != NULL);
	assert(SPARC_BBINFO_MAGICSIZE == 32);

#define	SPARC_AOUT_OFFSET	32

	retval = 0;
	blocks = NULL;

	if (params->stage2 == NULL) {
		warnx("You must provide the name of the secondary bootstrap");
		goto done;
	}
	if (params->flags & IB_STAGE1START) {
		warnx("`-b bno' is not supported for %s",
		    params->machine->name);
		goto done;
	}

	if (!S_ISREG(params->s1stat.st_mode)) {
		warnx("`%s' must be a regular file", params->stage1);
		goto done;
	}
	if (params->s1stat.st_size > sizeof(bb)) {
		warnx("`%s' cannot be larger than %lu bytes",
		    params->stage1, (unsigned long)sizeof(bb));
		goto done;
	}

	/*
	 * Read 1st stage boot program
	 * Leave room for a 32-byte a.out header.
	 */
	memset(&bb, 0, sizeof(bb));
	rv = read(params->s1fd, bb + SPARC_AOUT_OFFSET,
	    sizeof(bb) - SPARC_AOUT_OFFSET);
	if (rv == -1) {
		warn("Reading `%s'", params->stage1);
		goto done;
	}

	/*
	 * Quick sanity check that the bootstrap given
	 * is *not* an ELF executable.
	 */
	if (memcmp(bb + SPARC_AOUT_OFFSET + 1, "ELF", strlen("ELF")) == 0) {
		warnx("`%s' is an ELF executable; need raw binary", 
		    params->stage1);
		goto done;
	}

	/* Look for the bbinfo structure. */
	for (bbi = 0; bbi < sizeof(bb); bbi += sizeof(uint32_t)) {
		bbinfop = (void *) (bb + bbi);
		if (memcmp(bbinfop->bbi_magic, SPARC_BBINFO_MAGIC,
			    SPARC_BBINFO_MAGICSIZE) == 0)
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

	if (S_ISREG(params->fsstat.st_mode)) {
		if (fsync(params->fsfd) == -1)
			warn("Synchronising file system `%s'",
			    params->filesystem);
	} else {
		/* Ensure the secondary bootstrap is on disk. */
		sync();
	}

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

	/*
	 * sun4c/sun4m PROMs require an a.out(5) format header.
	 * Old-style sun4 PROMs do not expect a header at all.
	 * To deal with this, we construct a header that is also executable
	 * code containing a forward branch that gets us past the 32-byte
	 * header where the actual code begins. In assembly:
	 * 	.word	MAGIC		! a NOP
	 * 	ba,a	start		!
	 * 	.skip	24		! pad
	 * start:
	 */
#define SUN_MAGIC	0x01030107
#define SUN4_BASTART	0x30800007	/* i.e.: ba,a `start' */
	*((uint32_t *)bb) = htobe32(SUN_MAGIC);
	*((uint32_t *)bb + 1) = htobe32(SUN4_BASTART);

	if (params->flags & IB_VERBOSE) {
		printf("Bootstrap start sector: %u\n",
		    SPARC_BOOT_BLOCK_OFFSET / SPARC_BOOT_BLOCK_BLOCKSIZE);
		printf("Bootstrap byte count:   %u\n", (unsigned)rv);
		printf("Bootstrap block table:  %u entries of %u bytes available, %u used:",
		    maxblk, blocks[0].blocksize, nblk);
		for (blk_i = 0; blk_i < nblk; blk_i++)
			printf(" %u", blocks[blk_i].block);
		printf("\n%sriting bootstrap\n",
		    (params->flags & IB_NOWRITE) ? "Not w" : "W");
	}
	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	rv = pwrite(params->fsfd, &bb, sizeof(bb), SPARC_BOOT_BLOCK_OFFSET);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != sizeof(bb)) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	} else {
		retval = 1;
	}

 done:
	if (blocks != NULL)
		free (blocks);
	return (retval);
}
