/*	$NetBSD: bfs_subr.c,v 1.3 2008/04/28 20:23:18 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: bfs_subr.c,v 1.3 2008/04/28 20:23:18 martin Exp $");
#ifdef _STANDALONE
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include "local.h"
#endif

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <machine/param.h>
#include <machine/sector.h>
#include <machine/bfs.h>

struct sector_io_ops __io_ops = {
	sector_read,
	sector_read_n,
	sector_write,
	sector_write_n
};

int
bfs_init(struct bfs **bfsp)
{
	int err, bfs_sector;

	device_attach(2, 0, -1);	/* HARDDISK UNIT 0 */

	if ((err = bfs_find(&bfs_sector)) != 0)
		return err;

	return bfs_init2(bfsp, bfs_sector, &__io_ops, false);
}

int
bfs_find(int *sector)
{
	static uint8_t buf[DEV_BSIZE];
	struct pdinfo_sector *pdinfo;
	struct vtoc_sector *vtoc;
	const struct ux_partition *bfs_partition;
	int start;

	/* Physical Disk INFOrmation */
	pdinfo = (struct pdinfo_sector *)buf;
	if (!pdinfo_sector(0, pdinfo) || !pdinfo_sanity(pdinfo))
		return ENOENT;

	/* `start' is logical sector start in UX partition table. */
	start = pdinfo->logical_sector;

	/* Volume Table Of Contents */
	vtoc = (struct vtoc_sector *)buf;
	if (!vtoc_sector(0, vtoc, start))
		return ENOENT;

	/* Boot File System partition */
	if ((bfs_partition = vtoc_find_bfs(vtoc)) == 0)
		return ENOENT;

	*sector = start + bfs_partition->start_sector;

	return 0;
}
