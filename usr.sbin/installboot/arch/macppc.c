/*	$NetBSD: macppc.c,v 1.5 2002/05/20 16:05:27 lukem Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
__RCSID("$NetBSD: macppc.c,v 1.5 2002/05/20 16:05:27 lukem Exp $");
#endif	/* !__lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

static struct bbinfo_params bbparams = {
	MACPPC_BBINFO_MAGIC,
	MACPPC_BOOT_BLOCK_OFFSET,
	MACPPC_BOOT_BLOCK_BLOCKSIZE,
	MACPPC_BOOT_BLOCK_MAX_SIZE,
	0,
	BBINFO_BIG_ENDIAN,
};

static int writeapplepartmap(ib_params *, struct bbinfo_params *, uint8_t *);


int
macppc_clearboot(ib_params *params)
{

	assert(params != NULL);

	if (params->flags & IB_STAGE1START) {
		warnx("`-b bno' is not supported for %s",
		    params->machine->name);
		return (0);
	}
		/* XXX: maybe clear the apple partition map too? */
	return (shared_bbinfo_clearboot(params, &bbparams, NULL));
}

int
macppc_setboot(ib_params *params)
{

	assert(params != NULL);

	if (params->flags & IB_STAGE1START) {
		warnx("`-b bno' is not supported for %s",
		    params->machine->name);
		return (0);
	}
	return (shared_bbinfo_setboot(params, &bbparams, writeapplepartmap));
}


static int
writeapplepartmap(ib_params *params, struct bbinfo_params *bb_params,
    uint8_t *bb)
{
	struct apple_drvr_map dm;
	struct apple_part_map_entry pme;

	assert (params != NULL);
	assert (bb_params != NULL);
	assert (bb != NULL);

	if (params->flags & IB_NOWRITE)
		return (1);

		/* block 0: driver map  */
	if (pread(params->fsfd, &dm, MACPPC_BOOT_BLOCK_BLOCKSIZE, 0) !=
	    MACPPC_BOOT_BLOCK_BLOCKSIZE) {
		warn("Can't read sector 0 of `%s'", params->filesystem);
		return (0);
	}
	dm.sbSig =		htobe16(APPLE_DRVR_MAP_MAGIC);
	dm.sbBlockSize =	htobe16(512);
	dm.sbBlkCount =		htobe32(0);
	if (pwrite(params->fsfd, &dm, MACPPC_BOOT_BLOCK_BLOCKSIZE, 0) !=
	    MACPPC_BOOT_BLOCK_BLOCKSIZE) {
		warn("Can't write sector 0 of `%s'", params->filesystem);
		return (0);
	}

		/* block 1: Apple Partition Map */
	memset(&pme, 0, sizeof(pme));
	pme.pmSig =		htobe16(APPLE_PART_MAP_ENTRY_MAGIC);
	pme.pmMapBlkCnt =	htobe32(2);
	pme.pmPyPartStart =	htobe32(1);
	pme.pmPartBlkCnt =	htobe32(2);
	pme.pmDataCnt =		htobe32(2);
	strlcpy(pme.pmPartName, "Apple", sizeof(pme.pmPartName));
	strlcpy(pme.pmPartType, "Apple_partition_map", sizeof(pme.pmPartType));
	pme.pmPartStatus =	htobe32(0x37);
	if (pwrite(params->fsfd, &pme, MACPPC_BOOT_BLOCK_BLOCKSIZE,
	    1 * MACPPC_BOOT_BLOCK_BLOCKSIZE) != MACPPC_BOOT_BLOCK_BLOCKSIZE) {
		warn("Can't write Apple Partition Map into sector 1 of `%s'",
		    params->filesystem);
		return (0);
	}

		/* block 2: NetBSD partition */
	memset(&pme, 0, sizeof(pme));
	pme.pmSig =		htobe16(APPLE_PART_MAP_ENTRY_MAGIC);
	pme.pmMapBlkCnt =	htobe32(2);
	pme.pmPyPartStart =	htobe32(4);
	pme.pmPartBlkCnt =	htobe32(0x7fffffff);
	pme.pmDataCnt =		htobe32(0x7fffffff);
	strlcpy(pme.pmPartName, "NetBSD", sizeof(pme.pmPartName));
	strlcpy(pme.pmPartType, "NetBSD/macppc", sizeof(pme.pmPartType));
	pme.pmPartStatus =	htobe32(0x3b);
	pme.pmBootSize =	htobe32(roundup(params->s1stat.st_size, 512));
	pme.pmBootLoad =	htobe32(0x4000);
	pme.pmBootEntry =	htobe32(0x4000);
	strlcpy(pme.pmProcessor, "PowerPC", sizeof(pme.pmProcessor));
	if (pwrite(params->fsfd, &pme, MACPPC_BOOT_BLOCK_BLOCKSIZE,
	    2 * MACPPC_BOOT_BLOCK_BLOCKSIZE) != MACPPC_BOOT_BLOCK_BLOCKSIZE) {
		warn("Can't write Apple Partition Map into sector 2 of `%s'",
		    params->filesystem);
		return (0);
	}

	return (1);
}
