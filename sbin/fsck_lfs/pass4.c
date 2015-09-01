/* $NetBSD: pass4.c,v 1.27 2015/09/01 06:15:02 dholland Exp $	 */

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>

#define vnode uvnode
#define buf ubuf
#define panic call_panic
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"

static int check_orphan(struct inodesc *idp);

static int
check_orphan(struct inodesc *idp)
{
	struct zlncnt *zlnp;
	ino_t inumber = idp->id_number;

	for (zlnp = orphead; zlnp; zlnp = zlnp->next) {
		if (zlnp->zlncnt == inumber) {
			/* Swap this with head */
			zlnp->zlncnt = orphead->zlncnt;
			zlnp = orphead;
			orphead = orphead->next;
			/* Free old head */
			free((char *) zlnp);
			clri(idp, "PROPERLY ORPHANED", 1);
			return 1;
		}
	}
	return 0;
}

void
pass4(void)
{
	ino_t inumber;
	struct zlncnt *zlnp;
	union lfs_dinode *dp;
	struct inodesc idesc;
	int n;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	for (inumber = ULFS_ROOTINO; inumber <= lastino; inumber++) {
		idesc.id_number = inumber;
		switch (statemap[inumber]) {

		case FSTATE:
		case DFOUND:
			n = lncntp[inumber];
			if (n)
				adjust(&idesc, (short) n);
			else {
				for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
					if (zlnp->zlncnt == inumber) {
						zlnp->zlncnt = zlnhead->zlncnt;
						zlnp = zlnhead;
						zlnhead = zlnhead->next;
						free((char *) zlnp);
						clri(&idesc, "UNREF", 1);
						break;
					}
			}
			break;

		case DSTATE:
			clri(&idesc, "UNREF", 1);
			break;

		case DCLEAR:
			if (check_orphan(&idesc))
				break;
			dp = ginode(inumber);
			if (lfs_dino_getsize(fs, dp) == 0) {
				const char * msg = (lncntp[inumber] ?
					"ZERO LENGTH" : "UNREF ZERO LENGTH");
				clri(&idesc, msg, 1);
				break;
			}
			clri(&idesc, "BAD/DUP", 1);
			break;

		case FCLEAR:
			if (check_orphan(&idesc))
				break;
			clri(&idesc, "BAD/DUP", 1);
			break;

		case USTATE:
			break;

		default:
			err(EEXIT, "BAD STATE %d FOR INODE I=%llu",
			    statemap[inumber], (unsigned long long)inumber);
		}
	}
}

int
pass4check(struct inodesc * idesc)
{
	struct dups *dlp;
	int ndblks, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;
	SEGUSE *sup;
	struct ubuf *bp;
	int sn;

	sn = lfs_dtosn(fs, blkno);
	for (ndblks = idesc->id_numfrags; ndblks > 0; blkno++, ndblks--) {
		if (chkrange(blkno, 1)) {
			res = SKIP;
		} else if (testbmap(blkno) || preen) {
			for (dlp = duplist; dlp; dlp = dlp->next) {
				if (dlp->dup != blkno)
					continue;
				dlp->dup = duplist->dup;
				dlp = duplist;
				duplist = duplist->next;
				free((char *) dlp);
				break;
			}
			if (dlp == 0) {
				clrbmap(blkno);
				LFS_SEGENTRY(sup, fs, sn, bp);
				sup->su_nbytes -= lfs_fsbtob(fs, 1);
				VOP_BWRITE(bp);
				seg_table[sn].su_nbytes -= lfs_fsbtob(fs, 1);
				lfs_sb_addbfree(fs, 1);
				n_blks--;
			}
		}
	}
	return (res);
}
