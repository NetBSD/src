/*	$NetBSD: pass3.c,v 1.1.2.1 2025/04/30 04:42:17 perseant Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
#include <fs/exfatfs/exfatfs_tables.h>

#include "bufcache.h"
#include "vnode.h"
#include "fsutil.h"
#include "fsck_exfatfs.h"
#include "defaults.h"
#include "pass3.h"

void
pass3(struct exfatfs *fs)
{
	uint16_t lc, uc_expected, uc_observed;
	size_t off, res, ucsize;
	struct ubuf *bp;
	uint16_t *uctable;

	if (!Pflag && !Qflag) {
		fprintf(stderr, "** Phase 3 - Check upcase table\n");
	}

	/*
	 * Verify the correct upcase values for the ASCII set.
	 * These are the only required upcase values.
	 */
	for (lc = 0; lc < 128; ++lc) {
		if (lc >= 'a' && lc <= 'z')
			uc_expected = lc - 0x20;
		else
			uc_expected = lc;
		uc_observed = exfatfs_upcase(fs, lc);
		if (uc_observed != uc_expected)
			break;
	}

	if (uc_observed != uc_expected) {
		if (Pflag)
			pfatal("UPCASE TBALE INCORRECT\n");

		default_upcase_table(&uctable, &ucsize);
		res = ucsize;
		pwarn("UPCASE TABLE INCORRECT\n");
		if (GET_DSE_DATALENGTH(VTOXI(fs->xf_upcasevp)) < res) {
			/* XXX we should be able to create a new file */
			pfatal("NO SPACE FOR NEW TABLE\n");
		}
		if (Pflag || reply("WRITE NEW TABLE") == 1) {
			off = 0;
			while (res > 0) {
				bp = getblk(fs->xf_upcasevp, off, EXFATFS_LSIZE(fs));
				memcpy(bp->b_data, ((char *)uctable) + off, MIN((size_t)EXFATFS_LSIZE(fs), res));
				bwrite(bp);
				off += EXFATFS_LSIZE(fs);
				res -= EXFATFS_LSIZE(fs);
			}
			fsdirty = 1;
		}
	}
}
