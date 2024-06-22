/* $NetBSD: hp300.c,v 1.17.2.1 2024/06/22 10:57:11 martin Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(__lint)
__RCSID("$NetBSD: hp300.c,v 1.17.2.1 2024/06/22 10:57:11 martin Exp $");
#endif /* !__lint */

/* We need the target disklabel.h, not the hosts one..... */
#ifdef HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#include <nbinclude/hp300/disklabel.h>
#include <nbinclude/sys/disklabel.h>
#else
#include <sys/disklabel.h>
#endif
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <assert.h>
#include <err.h>
#include <md5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

#define HP300_MAXBLOCKS	1	/* Only contiguous blocks are expected. */
#define LIF_VOLDIRSIZE	1024	/* size of LIF volume header and directory */

static int hp300_setboot(ib_params *);

struct ib_mach ib_mach_hp300 = {
	.name		=	"hp300",
	.setboot	=	hp300_setboot,
	.clearboot	=	no_clearboot,
	.editboot	=	no_editboot,
	.valid_flags	=	IB_APPEND,
};

static int
hp300_setboot(ib_params *params)
{
	int		retval;
	uint8_t		*bootstrap;
	size_t		bootstrap_size;
	ssize_t		rv;
	struct partition *boot;
	struct hp300_lifdir *lifdir;
	int		offset;
	int		i;
	unsigned int	secsize = HP300_SECTSIZE;
	uint64_t	boot_size, boot_offset;
#ifdef SUPPORT_CD9660
	uint32_t	nblk;
	ib_block	*blocks;
#endif
	struct disklabel *label;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);
	assert(params->s1fd != -1);
	assert(params->stage1 != NULL);

	retval = 0;
	bootstrap = MAP_FAILED;

	label = malloc(params->sectorsize);
	if (label == NULL) {
		warn("Failed to allocate memory for disklabel");
		goto done;
	}

#ifdef SUPPORT_CD9660
	if (params->stage2 != NULL) {
		/*
		 * Use contiguous blocks of SYS_BOOT in the target filesystem
		 * (assuming ISO9660) for a LIF directory entry used
		 * by BOOTROM on bootstrap.
		 */
		if (strcmp(params->fstype->name, "cd9660") != 0) {
			warn("Target filesystem `%s' is unexpected",
			    params->fstype->name);
		}

		if (S_ISREG(params->fsstat.st_mode)) {
			if (fsync(params->fsfd) == -1)
				warn("Synchronising file system `%s'",
				    params->filesystem);
		} else {
			/* Don't allow real file systems for sanity */
			warnx("`%s' must be a regular file to append "
			    "a bootstrap", params->filesystem);
			goto done;
		}

		/* Allocate space for our block list. */
		nblk = HP300_MAXBLOCKS;
		blocks = malloc(sizeof(*blocks) * nblk);
		if (blocks == NULL) {
			warn("Allocating %lu bytes for block list",
			    (unsigned long)sizeof(*blocks) * nblk);
			goto done;
		}

		/* Check the block of for the SYS_UBOOT in the target fs */
		if (!params->fstype->findstage2(params, &nblk, blocks))
			goto done;

		if (nblk == 0) {
			warnx("Secondary bootstrap `%s' is empty",
			    params->stage2);
			goto done;
		} else if (nblk > 1) {
			warnx("Secondary bootstrap `%s' doesn't have "
			    "contiguous blocks", params->stage2);
			goto done;
		}

		boot_offset = blocks[0].block * params->fstype->blocksize;
		/* need to read only LIF volume and directories */
		bootstrap_size = LIF_VOLDIRSIZE;

		if ((params->flags & IB_VERBOSE) != 0) {
			printf("Bootstrap `%s' found at offset %lu in `%s'\n",
			    params->stage2, (unsigned long)boot_offset,
			    params->filesystem);
		}

	} else
#endif
	if (params->flags & IB_APPEND) {
		if (!S_ISREG(params->fsstat.st_mode)) {
			warnx(
		    "`%s' must be a regular file to append a bootstrap",
			    params->filesystem);
			goto done;
		}
		boot_offset = roundup(params->fsstat.st_size, HP300_SECTSIZE);
	} else {
		/*
		 * The bootstrap can be well over 8k, and must go into a BOOT
		 * partition. Read NetBSD label to locate BOOT partition.
		 */
		if (pread(params->fsfd, label, params->sectorsize,
		    LABELSECTOR * params->sectorsize)
		    != (ssize_t)params->sectorsize) {
			warn("reading disklabel");
			goto done;
		}
		/* And a quick validation - must be a big-endian label */
		secsize = be32toh(label->d_secsize);
		if (label->d_magic != htobe32(DISKMAGIC) ||
		    label->d_magic2 != htobe32(DISKMAGIC) ||
		    secsize == 0 || secsize & (secsize - 1) ||
		    be16toh(label->d_npartitions) > MAXMAXPARTITIONS) {
			warnx("Invalid disklabel in %s", params->filesystem);
			goto done;
		}

		i = be16toh(label->d_npartitions);
		for (boot = label->d_partitions; ; boot++) {
			if (--i < 0) {
				warnx("No BOOT partition");
				goto done;
			}
			if (boot->p_fstype == FS_BOOT)
				break;
		}
		boot_size = be32toh(boot->p_size) * (uint64_t)secsize;
		boot_offset = be32toh(boot->p_offset) * (uint64_t)secsize;

		/*
		 * We put the entire LIF file into the BOOT partition even when
		 * it doesn't start at the beginning of the disk.
		 *
		 * Maybe we ought to be able to take a binary file and add
		 * it to the LIF filesystem.
		 */
		if (boot_size < (uint64_t)params->s1stat.st_size) {
			warn("BOOT partition too small (%llu < %llu)",
				(unsigned long long)boot_size,
				(unsigned long long)params->s1stat.st_size);
			goto done;
		}
	}

#ifdef SUPPORT_CD9660
	if (params->stage2 != NULL) {
		/* Use bootstrap file in the target filesystem. */
		bootstrap = mmap(NULL, bootstrap_size,
		    PROT_READ | PROT_WRITE, MAP_PRIVATE, params->fsfd,
		    boot_offset);
		if (bootstrap == MAP_FAILED) {
			warn("mmapping `%s'", params->filesystem);
			goto done;
		}
	} else
#endif
	{
		/* Use bootstrap specified as stage1. */
		bootstrap_size = params->s1stat.st_size;
		bootstrap = mmap(NULL, bootstrap_size,
		    PROT_READ | PROT_WRITE, MAP_PRIVATE, params->s1fd, 0);
		if (bootstrap == MAP_FAILED) {
			warn("mmapping `%s'", params->stage1);
			goto done;
		}
	}

	/* Relocate files, sanity check LIF directory on the way */
	lifdir = (void *)(bootstrap + HP300_SECTSIZE * 2);
	for (i = 0; i < 8; lifdir++, i++) {
		uint32_t addr = be32toh(lifdir->dir_addr);
		uint32_t limit = (params->s1stat.st_size - 1) / HP300_SECTSIZE
		    + 1;
		uint32_t end = addr + be32toh(lifdir->dir_length);
		if (end > limit) {
			warnx("LIF entry %d larger (%u %u) than LIF file",
				i, end, limit);
			goto done;
		}
		if (addr != 0 && boot_offset != 0)
			lifdir->dir_addr = htobe32(addr + boot_offset
							    / HP300_SECTSIZE);
	}

	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	/* Write LIF volume header and directory to sectors 0 and 1 */
	rv = pwrite(params->fsfd, bootstrap, LIF_VOLDIRSIZE, 0);
	if (rv != LIF_VOLDIRSIZE) {
		if (rv == -1)
			warn("Writing `%s'", params->filesystem);
		else
			warnx("Writing `%s': short write", params->filesystem);
		goto done;
	}

#ifdef SUPPORT_CD9660
	if (params->stage2 != NULL) {
		/*
		 * Bootstrap in the target filesystem is used.
		 * No need to write bootstrap to BOOT partition.
		 */
		retval = 1;
		goto done;
	}
#endif

	/* Write files to BOOT partition */
	offset = boot_offset <= HP300_SECTSIZE * 16 ? HP300_SECTSIZE * 16 : 0;
	i = roundup(params->s1stat.st_size, secsize) - offset;
	rv = pwrite(params->fsfd, bootstrap + offset, i, boot_offset + offset);
	if (rv != i) {
		if (rv == -1)
			warn("Writing boot filesystem of `%s'",
				params->filesystem);
		else
			warnx("Writing boot filesystem of `%s': short write",
				params->filesystem);
		goto done;
	}

	retval = 1;

 done:
	if (label != NULL)
		free(label);
	if (bootstrap != MAP_FAILED)
		munmap(bootstrap, bootstrap_size);
	return retval;
}
