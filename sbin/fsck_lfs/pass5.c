/* $NetBSD: pass5.c,v 1.4 2000/05/23 01:48:55 perseant Exp $	 */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>

#include <string.h>
#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern SEGUSE  *seg_table;

void
pass5()
{
	SEGUSE         *su;
	struct bufarea *bp;
	int             i;

	/*
	 * Check segment holdings against actual holdings.  Check for
	 * "clean" segments that contain live data.
	 */
	for (i = 0; i < sblock.lfs_nseg; i++) {
		su = lfs_gseguse(i, &bp);
		if (!(su->su_flags & SEGUSE_DIRTY) &&
		    seg_table[i].su_nbytes > 0) {
			pwarn("%d bytes contained in 'clean' segment %d\n",
			      seg_table[i].su_nbytes, i);
			if (preen || reply("fix")) {
				su->su_flags |= SEGUSE_DIRTY;
				dirty(bp);
			}
		}
		if ((su->su_flags & SEGUSE_DIRTY) &&
		    su->su_nbytes != seg_table[i].su_nbytes) {
			pwarn("segment %d claims %d bytes but has %d\n",
			      i, su->su_nbytes, seg_table[i].su_nbytes);
			if (preen || reply("fix")) {
				su->su_nbytes = seg_table[i].su_nbytes;
				dirty(bp);
			}
		}
		bp->b_flags &= ~B_INUSE;
	}
}
