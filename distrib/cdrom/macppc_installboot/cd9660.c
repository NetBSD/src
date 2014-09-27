/*	$NetBSD: cd9660.c,v 1.4 2014/09/27 15:21:40 tsutsui Exp $	*/

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

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: cd9660.c,v 1.4 2014/09/27 15:21:40 tsutsui Exp $");
#endif	/* !__lint */

#include <sys/param.h>

#if !HAVE_NBTOOL_CONFIG_H
#include <sys/mount.h>
#endif

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <fs/cd9660/iso.h>

#include "installboot.h"

#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define MAXLEN		16


int
cd9660_match(ib_params *params)
{
	int rv, blocksize;
	struct iso_primary_descriptor ipd;

	assert(params != NULL);
	assert(params->fstype != NULL);
	assert(params->fsfd != -1);

	rv = pread(params->fsfd, &ipd, sizeof(ipd),
	    ISO_DEFAULT_BLOCK_SIZE * 16);
	if (rv == -1) {
		warn("Reading primary descriptor in `%s'", params->filesystem);
		return 0;
	} else if (rv != sizeof(ipd)) {
		warnx("Reading primary descriptor in `%s': short read",
		   params->filesystem);
		return 0;
	}

	if (ipd.type[0] != ISO_VD_PRIMARY ||
	    strncmp(ipd.id, ISO_STANDARD_ID, sizeof(ipd.id)) != 0 ||
	    ipd.version[0] != 1) {
		warnx("Filesystem `%s' is not ISO9660 format",
		   params->filesystem);
		return 0;
	}

	blocksize = isonum_723((char *)ipd.logical_block_size);
	if (blocksize != ISO_DEFAULT_BLOCK_SIZE) {
		warnx("Invalid blocksize %d in `%s'",
		    blocksize, params->filesystem);
		return 0;
	}

	params->fstype->blocksize = blocksize;
	params->fstype->needswap = 0;

	return 1;
}

int
cd9660_findstage2(ib_params *params, uint32_t *maxblk, ib_block *blocks)
{
	uint8_t buf[ISO_DEFAULT_BLOCK_SIZE];
	char name[MAXNAMLEN];
	char *ofwboot;
	off_t loc;
	int rv, blocksize, found, i;
	struct iso_primary_descriptor ipd;
	struct iso_directory_record *idr;

	assert(params != NULL);
	assert(params->stage2 != NULL);
	assert(maxblk != NULL);
	assert(blocks != NULL);

#if 0
	if (params->flags & IB_STAGE2START)
		return hardcode_stage2(params, maxblk, blocks);
#endif

	/* The secondary bootstrap must be clearly in /. */
	strlcpy(name, params->stage2, MAXNAMLEN);
	ofwboot = name;
	if (ofwboot[0] == '/')
		ofwboot++;
	if (strchr(ofwboot, '/') != NULL) {
		warnx("The secondary bootstrap `%s' must be in / "
		    "on filesystem `%s'", params->stage2, params->filesystem);
		return 0;
	}
	if (strchr(ofwboot, '.') == NULL) {
		/*
		 * XXX should fix isofncmp()?
		 */
		strlcat(ofwboot, ".", MAXNAMLEN);
	}

	rv = pread(params->fsfd, &ipd, sizeof(ipd),
	    ISO_DEFAULT_BLOCK_SIZE * 16);
	if (rv == -1) {
		warn("Reading primary descriptor in `%s'", params->filesystem);
		return 0;
	} else if (rv != sizeof(ipd)) {
		warnx("Reading primary descriptor in `%s': short read",
		   params->filesystem);
		return 0;
	}
	blocksize = isonum_723((char *)ipd.logical_block_size);

	idr = (void *)ipd.root_directory_record;
	loc = (off_t)isonum_733(idr->extent) * blocksize;
	rv = pread(params->fsfd, buf, blocksize, loc);
	if (rv == -1) {
		warn("Reading root directory record in `%s'",
		    params->filesystem);
		return 0;
	} else if (rv != sizeof(ipd)) {
		warnx("Reading root directory record in `%s': short read",
		   params->filesystem);
		return 0;
	}

	found = 0;
	for (i = 0; i < blocksize - sizeof(struct iso_directory_record);
	    i += (u_char)idr->length[0]) {
		idr = (void *)&buf[i];

#ifdef DEBUG
		printf("i = %d, idr->length[0] = %3d\n",
		    i, (u_char)idr->length[0]);
#endif
		/* check end of entries */
		if (idr->length[0] == 0) {
#ifdef DEBUG
		printf("end of entries\n");
#endif
			break;
		}

		if (idr->flags[0] & 2) {
			/* skip directory entries */
#ifdef DEBUG
			printf("skip directory entry\n");
#endif
			continue;
		}
		if (idr->name_len[0] == 1 &&
		    (idr->name[0] == 0 || idr->name[0] == 1)) {
			/* skip "." and ".." */
#ifdef DEBUG
			printf("skip dot dot\n");
#endif
			continue;
		}
#ifdef DEBUG
		{
			int j;

			printf("filename:");
			for (j = 0; j < isonum_711(idr->name_len); j++)
				printf("%c", idr->name[j]);
			printf("\n");
		}
#endif
		if (isofncmp(ofwboot, strlen(ofwboot),
		    idr->name, isonum_711(idr->name_len), 0) == 0) {
			found = 1;
			/* ISO filesystem always has contiguous file blocks */
			blocks[0].block = (int64_t)isonum_733(idr->extent);
			/* XXX bootxx assumes blocksize is 512 */
			blocks[0].block *= blocksize / 512;
			blocks[0].blocksize =
			    roundup(isonum_733(idr->size), blocksize);
			*maxblk = 1;
#ifdef DEBUG
			printf("block = %ld, blocksize = %ld\n",
			    (long)blocks[0].block, blocks[0].blocksize);
#endif
			break;
		}
	}

	if (found == 0) {
		warnx("Can't find secondary bootstrap `%s' in filesystem `%s'",
		    params->stage2, params->filesystem);
		return 0;
	}

	return 1;
}
