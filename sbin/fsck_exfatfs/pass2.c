/*	$NetBSD: pass2.c,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <util.h>

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/queue.h>

#define buf ubuf
#define vnode uvnode
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>

#include "bufcache.h"
#include "vnode.h"
#include "fsutil.h"
#include "fsck_exfatfs.h"
#include "pass2.h"

void
pass2(struct exfatfs *fs, uint8_t *observed_bitmap)
{
	uint32_t base, off;
	struct buf *bp = NULL;
	daddr_t lbn, endlbn;
	size_t size = bitmap_discontiguous ? SECSIZE(fs) : MAXPHYS;
	int modified;
	
	if (!Pflag) {
		fprintf(stderr, "** Phase 2 - Verify allocation bitmap\n");
	}
	
	assert(fs->xf_bitmapvp != NULL);

	endlbn = howmany(fs->xf_ClusterCount, SECSIZE(fs) * NBBY);
	for (lbn = 0; lbn < endlbn; lbn += size / SECSIZE(fs)) {
		if (size > (size_t) (endlbn - lbn) * SECSIZE(fs)) {
			size = (endlbn - lbn) * SECSIZE(fs);
			if (debug)
				pwarn("At lbn %lu, switch to block size %zd\n",
					(unsigned long)lbn, size);
		}
		if (debug)
			pwarn("check bitmap block %lu size %zd\n",
				(unsigned long)lbn, size);
		bread(fs->xf_bitmapvp, lbn, size, 0, &bp);

		/* Check the quick way first */
		base = lbn * SECSIZE(fs) * NBBY;
		if (memcmp(bp->b_data, observed_bitmap + base / NBBY, size) == 0) {
			brelse(bp, 0);
			continue;
		}

		/* There is some difference; find out what it is */
		for (off = 0; off < size  * NBBY; ++off) {
			if (base + off >= fs->xf_ClusterCount)
				break;
			if (isset(bp->b_data, off) == isset(observed_bitmap, base + off))
				continue;
			++problems;
			if (isset(observed_bitmap, base + off)) {
				pwarn("AT PHYSICAL DISK ADDRESS 0x%lx size %zu offset %lu/%lu\n", bp->b_blkno, size, (unsigned long)off, (unsigned long)(size * NBBY));
				pwarn("UNALLOCATED CLUSTER %lu IN USE\n", (unsigned long)base + off + 2);
				if (Pflag || reply("ALLOCATE") == 1) {
					setbit(bp->b_data, off - base);
					modified = 1;
				}
			} else {
				pwarn("AT PHYSICAL DISK ADDRESS 0x%lx size %zu offset %lu/%lu\n", bp->b_blkno, size, (unsigned long)off, (unsigned long)(size * NBBY));
				pwarn("ALLOCATED CLUSTER %lu NOT IN USE\n", (unsigned long)base + off + 2);
				if (Pflag || reply("CLEAR") == 1) {
					clrbit(bp->b_data, off - base);
					modified = 1;
				}
			}
		}

		if (modified)
			bwrite(bp);
		else
			brelse(bp, 0);
	}
}
