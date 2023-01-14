/*	$NetBSD: pass4.c,v 1.30 2023/01/14 12:12:50 christos Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)pass4.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: pass4.c,v 1.30 2023/01/14 12:12:50 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"

void
pass4(void)
{
	ino_t inumber;
	struct zlncnt *zlnp;
	union dinode *dp;
	struct inodesc idesc;
	int n;
	size_t i;
	uint32_t cg;
	struct inostat *info;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_func = pass4check;

	for (cg = 0; cg < sblock->fs_ncg; cg++) {
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 4: cyl group %d of %d (%d%%)\n",
			    cdevname(), cg, sblock->fs_ncg,
			    cg * 100 / sblock->fs_ncg);
			got_siginfo = 0;
		}
#ifdef PROGRESS
		progress_bar(cdevname(), preen ? NULL : "phase 4",
			    cg, sblock->fs_ncg);
#endif /* PROGRESS */
		inumber = cg * sblock->fs_ipg;
		for (i = 0; i < inostathead[cg].il_numalloced; i++, inumber++) {
			if (inumber < UFS_ROOTINO)
				continue;
			dp = ginode(inumber);
			info = inoinfo(inumber);
			idesc.id_number = inumber;
			idesc.id_uid = iswap32(DIP(dp, uid));
			idesc.id_gid = iswap32(DIP(dp, gid));
			if (iswap32(DIP(dp, flags)) & SF_SNAPSHOT)
				idesc.id_type = SNAP;
			else
				idesc.id_type = ADDR;
			switch (info->ino_state) {
			case FSTATE:
			case DFOUND:
				n = info->ino_linkcnt;
				if (n) {
					if (is_journal_inode(inumber)) {
						if (debug)
							printf(
    "skipping unreferenced journal inode %" PRId64 "\n", inumber);
						break;
					} else if (is_quota_inode(inumber)) {
						if (debug)
							printf(
    "skipping unreferenced quota inode %" PRId64 "\n", inumber);
						break;
					} else {
						adjust(&idesc, (short)n);
					}
					break;
				}
				for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
					if (zlnp->zlncnt == inumber) {
						zlnp->zlncnt = zlnhead->zlncnt;
						zlnp = zlnhead;
						zlnhead = zlnhead->next;
						free((char *)zlnp);
						clri(&idesc, "UNREF", 1);
						break;
					}
				break;

			case DSTATE:
				clri(&idesc, "UNREF", 1);
				break;

			case DCLEAR:
				if (DIP(dp, size) == 0) {
					clri(&idesc, "ZERO LENGTH", 1);
					break;
				}
				/* fall through */
			case FCLEAR:
				clri(&idesc, "BAD/DUP", 1);
				break;

			case USTATE:
				break;

			default:
				errexit("BAD STATE %d FOR INODE I=%llu",
				    info->ino_state,
				    (unsigned long long)inumber);
			}
		}
	}
#ifdef PROGRESS
	if (!preen)
		progress_done();
#endif /* PROGRESS */
}

int
pass4check(struct inodesc *idesc)
{
	struct dups *dlp;
	int cg, nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;
	struct cg *cgp = cgrp;

	cg = dtog(sblock, blkno);
	getblk(&cgblk, cgtod(sblock, cg), sblock->fs_cgsize);
	memcpy(cgp, cgblk.b_un.b_cg, sblock->fs_cgsize);
	if ((doswap && !needswap) || (!doswap && needswap))
		ffs_cg_swap(cgblk.b_un.b_cg, cgp, sblock);
	if (!cg_chkmagic(cgp, 0)) {
		pwarn("CG %d: ALLOCBLK: BAD MAGIC NUMBER\n", cg);
		cgp = NULL;
	}

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (chkrange(blkno, 1)) {
			res = SKIP;
		} else if (testbmap(blkno)) {
			for (dlp = duplist; dlp; dlp = dlp->next) {
				if (dlp->dup != blkno)
					continue;
				dlp->dup = duplist->dup;
				dlp = duplist;
				duplist = duplist->next;
				free((char *)dlp);
				break;
			}
			if (dlp == 0) {
				clrbmap(blkno);
				if (cgp)
					setbit(cg_blksfree(cgp, 0),
					    dtogd(sblock, blkno));

				n_blks--;
				if (idesc->id_type != SNAP) {
					update_uquot(idesc->id_number,
					    idesc->id_uid, idesc->id_gid, 
					    -btodb(sblock->fs_fsize), 0);
				}
				if (idesc->id_numfrags != sblock->fs_frag &&
				    cgp) {
					cgp->cg_cs.cs_nffree ++;
					sblock->fs_cstotal.cs_nffree ++;
					sblock->fs_cs(fs, cg).cs_nffree ++;
				}
			}
		}
	}
	if (cgp && res != SKIP && idesc->id_numfrags == sblock->fs_frag) {
		cgp->cg_cs.cs_nbfree++;
		sblock->fs_cstotal.cs_nbfree++;
		sblock->fs_cs(fs, cg).cs_nbfree++;
		ffs_clusteracct(sblock, cgp,
		    ffs_fragstoblks(sblock, dtogd(sblock, idesc->id_blkno)), 1);
	}
	sbdirty();
	cgdirty();
	return (res);
}
