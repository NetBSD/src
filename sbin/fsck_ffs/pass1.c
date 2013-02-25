/*	$NetBSD: pass1.c,v 1.49.8.1 2013/02/25 00:28:06 tls Exp $	*/

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
static char sccsid[] = "@(#)pass1.c	8.6 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: pass1.c,v 1.49.8.1 2013/02/25 00:28:06 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"
#include "exitvalues.h"

static daddr_t badblk;
static daddr_t dupblk;
static void checkinode(ino_t, struct inodesc *);
static ino_t lastino;

void
pass1(void)
{
	ino_t inumber, inosused, ninosused, ii;
	size_t inospace;
	int c;
	daddr_t i, cgd;
	struct inodesc idesc;
	struct cg *cgp = cgrp;
	struct inostat *info;
	uint8_t *cp;

	/*
	 * Set file system reserved blocks in used block map.
	 */
	for (c = 0; c < sblock->fs_ncg; c++) {
		cgd = cgdmin(sblock, c);
		if (c == 0)
			i = cgbase(sblock, c);
		else
			i = cgsblock(sblock, c);
		for (; i < cgd; i++)
			setbmap(i);
	}
	i = sblock->fs_csaddr;
	cgd = i + howmany(sblock->fs_cssize, sblock->fs_fsize);
	for (; i < cgd; i++)
		setbmap(i);
	/*
	 * Find all allocated blocks.
	 */
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_func = pass1check;
	n_files = n_blks = 0;
	for (c = 0; c < sblock->fs_ncg; c++) {
		inumber = c * sblock->fs_ipg;
		setinodebuf(inumber);
		getblk(&cgblk, cgtod(sblock, c), sblock->fs_cgsize);
		memcpy(cgp, cgblk.b_un.b_cg, sblock->fs_cgsize);
		if((doswap && !needswap) || (!doswap && needswap))
			ffs_cg_swap(cgblk.b_un.b_cg, cgp, sblock);
		if (is_ufs2)
			inosused = cgp->cg_initediblk;
		else
			inosused = sblock->fs_ipg;
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 1: cyl group %d of %d (%d%%)\n",
			    cdevname(), c, sblock->fs_ncg,
			    c * 100 / sblock->fs_ncg);
			got_siginfo = 0;
		}
#ifdef PROGRESS
		progress_bar(cdevname(), preen ? NULL : "phase 1",
			    c, sblock->fs_ncg);
#endif /* PROGRESS */
		/*
		 * If we are using soft updates, then we can trust the
		 * cylinder group inode allocation maps to tell us which
		 * inodes are allocated. We will scan the used inode map
		 * to find the inodes that are really in use, and then
		 * read only those inodes in from disk.
		 */
		if (preen && usedsoftdep) {
			if (!cg_chkmagic(cgp, 0))
				pfatal("CG %d: BAD MAGIC NUMBER\n", c);
			cp = &cg_inosused(cgp, 0)[(inosused - 1) / CHAR_BIT];
			for ( ; inosused > 0; inosused -= CHAR_BIT, cp--) {
				if (*cp == 0)
					continue;
				for (i = 1 << (CHAR_BIT - 1); i > 0; i >>= 1) {
					if (*cp & i)
						break;
					inosused--;
				}
				break;
			}
#ifdef notdef
			if (inosused < 0)
				inosused = 0;
#endif
		}
		/*
		 * Allocate inoinfo structures for the allocated inodes.
		 */
		inostathead[c].il_numalloced = inosused;
		if (inosused == 0) {
			inostathead[c].il_stat = 0;
			continue;
		}
		inospace = inosused * sizeof(*info);
		if (inospace / sizeof(*info) != inosused) {
			pfatal("too many inodes %llu\n", (unsigned long long)
			    inosused);
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		info = malloc(inospace);
		if (info == NULL) {
			pfatal("cannot alloc %zu bytes for inoinfo\n",
			    inospace);
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		(void)memset(info, 0, inospace);
		inostathead[c].il_stat = info;
		/*
		 * Scan the allocated inodes.
		 */
		for (ii = 0; ii < inosused; ii++, inumber++) {
			if (inumber < UFS_ROOTINO) {
				(void)getnextinode(inumber);
				continue;
			}
			checkinode(inumber, &idesc);
		}
		lastino += 1;
		if (inosused < (ino_t)sblock->fs_ipg || inumber == lastino)
			continue;
		/*
		 * If we were not able to determine in advance which inodes
		 * were in use, then reduce the size of the inoinfo structure
		 * to the size necessary to describe the inodes that we
		 * really found.
		 */
		if (lastino < (c * (ino_t)sblock->fs_ipg))
			ninosused = 0;
		else
			ninosused = lastino - (c * sblock->fs_ipg);
		inostathead[c].il_numalloced = ninosused;
		if (ninosused == 0) {
			free(inostathead[c].il_stat);
			inostathead[c].il_stat = 0;
			continue;
		}
		if (ninosused != inosused) {
			struct inostat *ninfo;
			size_t ninospace = ninosused * sizeof(*ninfo);
			if (ninospace / sizeof(*info) != ninosused) {
				pfatal("too many inodes %llu\n",
				    (unsigned long long)ninosused);
				exit(FSCK_EXIT_CHECK_FAILED);
			}
			ninfo = realloc(info, ninospace);
			if (ninfo == NULL) {
				pfatal("cannot realloc %zu bytes to %zu "
				    "for inoinfo\n", inospace, ninospace);
				exit(FSCK_EXIT_CHECK_FAILED);
			}
			if (ninosused > inosused)
				(void)memset(&ninfo[inosused], 0, ninospace - inospace);
			inostathead[c].il_stat = ninfo;
		}
	}
#ifdef PROGRESS
	if (!preen)
		progress_done();
#endif /* PROGRESS */
	freeinodebuf();
	do_blkswap = 0; /* has been done */
}

static void
checkinode(ino_t inumber, struct inodesc *idesc)
{
	union dinode *dp;
	struct zlncnt *zlnp;
	daddr_t ndb;
	int j;
	mode_t mode;
	u_int64_t size, kernmaxfilesize;
	int64_t blocks;
	char symbuf[MAXBSIZE];
	struct inostat *info;
	uid_t uid;
	gid_t gid;

	dp = getnextinode(inumber);
	info = inoinfo(inumber);
	mode = iswap16(DIP(dp, mode)) & IFMT;
	size = iswap64(DIP(dp, size));
	uid = iswap32(DIP(dp, uid));
	gid = iswap32(DIP(dp, gid));
	if (mode == 0) {
		if ((is_ufs2 && 
		    (memcmp(dp->dp2.di_db, ufs2_zino.di_db,
			UFS_NDADDR * sizeof(int64_t)) ||
		    memcmp(dp->dp2.di_ib, ufs2_zino.di_ib,
			UFS_NIADDR * sizeof(int64_t))))
		    ||
		    (!is_ufs2 && 
		    (memcmp(dp->dp1.di_db, ufs1_zino.di_db,
			UFS_NDADDR * sizeof(int32_t)) ||
		    memcmp(dp->dp1.di_ib, ufs1_zino.di_ib,
			UFS_NIADDR * sizeof(int32_t)))) ||
		    mode || size) {
			pfatal("PARTIALLY ALLOCATED INODE I=%llu",
			    (unsigned long long)inumber);
			if (reply("CLEAR") == 1) {
				dp = ginode(inumber);
				clearinode(dp);
				inodirty();
			} else
				markclean = 0;
		}
		info->ino_state = USTATE;
		return;
	}
	lastino = inumber;
	/* This should match the file size limit in ffs_mountfs(). */
	if (is_ufs2)
		kernmaxfilesize = sblock->fs_maxfilesize;
	else
		kernmaxfilesize = (u_int64_t)0x80000000 * sblock->fs_bsize - 1;
	if (size > kernmaxfilesize  || size + sblock->fs_bsize - 1 < size ||
	    (mode == IFDIR && size > MAXDIRSIZE)) {
		if (debug)
			printf("bad size %llu:",(unsigned long long)size);
		goto unknown;
	}
	if (!preen && mode == IFMT && reply("HOLD BAD BLOCK") == 1) {
		dp = ginode(inumber);
		DIP_SET(dp, size, iswap64(sblock->fs_fsize));
		size = sblock->fs_fsize;
		DIP_SET(dp, mode, iswap16(IFREG|0600));
		inodirty();
	}
	ndb = howmany(size, sblock->fs_bsize);
	if (ndb < 0) {
		if (debug)
			printf("bad size %llu ndb %lld:",
				(unsigned long long)size, (long long)ndb);
		goto unknown;
	}
	if (mode == IFBLK || mode == IFCHR)
		ndb++;
	if (mode == IFLNK) {
		/*
		 * Note that the old fastlink format always had di_blocks set
		 * to 0.  Other than that we no longer use the `spare' field
		 * (which is now the extended uid) for sanity checking, the
		 * new format is the same as the old.  We simply ignore the
		 * conversion altogether.  - mycroft, 19MAY1994
		 */
		if (!is_ufs2 && doinglevel2 &&
		    size > 0 && size < UFS1_MAXSYMLINKLEN &&
		    DIP(dp, blocks) != 0) {
			if (bread(fsreadfd, symbuf,
			    fsbtodb(sblock, iswap32(DIP(dp, db[0]))),
			    (long)secsize) != 0)
				errexit("cannot read symlink");
			if (debug) {
				symbuf[size] = 0;
				printf("convert symlink %llu(%s) "
				    "of size %lld\n",
				    (unsigned long long)inumber, symbuf,
				    (unsigned long long)size);
			}
			dp = ginode(inumber);
			memmove(dp->dp1.di_db, symbuf, (long)size);
			DIP_SET(dp, blocks, 0);
			inodirty();
		}
		/*
		 * Fake ndb value so direct/indirect block checks below
		 * will detect any garbage after symlink string.
		 */
		if ((sblock->fs_maxsymlinklen < 0) ||
		    (size < (uint64_t)sblock->fs_maxsymlinklen) ||
		    (isappleufs && (size < APPLEUFS_MAXSYMLINKLEN)) ||
		    (sblock->fs_maxsymlinklen == 0 && DIP(dp, blocks) == 0)) {
			if (is_ufs2)
				ndb = howmany(size, sizeof(int64_t));
			else	
				ndb = howmany(size, sizeof(int32_t));
			if (ndb > UFS_NDADDR) {
				j = ndb - UFS_NDADDR;
				for (ndb = 1; j > 1; j--)
					ndb *= NINDIR(sblock);
				ndb += UFS_NDADDR;
			}
		}
	}
	if (ndb < UFS_NDADDR) {
	    for (j = ndb; j < UFS_NDADDR; j++)
		if (DIP(dp, db[j]) != 0) {
		    if (debug) {
			if (!is_ufs2)
			    printf("bad direct addr ix %d: %d [ndb %lld]\n",
				j, iswap32(dp->dp1.di_db[j]),
				(long long)ndb);
			else
			    printf("bad direct addr ix %d: %lld [ndb %lld]\n",
				j, (long long)iswap64(dp->dp2.di_db[j]),
				(long long)ndb);
		    }
		    goto unknown;
		}
	}

	for (j = 0, ndb -= UFS_NDADDR; ndb > 0; j++)
		ndb /= NINDIR(sblock);

	for (; j < UFS_NIADDR; j++)
		if (DIP(dp, ib[j]) != 0) {
		    if (debug) {
			if (!is_ufs2)
			    printf("bad indirect addr: %d\n",
				iswap32(dp->dp1.di_ib[j]));
			else
			    printf("bad indirect addr: %lld\n",
				(long long)iswap64(dp->dp2.di_ib[j]));
		    }
		    goto unknown;
		}
	if (ftypeok(dp) == 0)
		goto unknown;
	n_files++;
	info->ino_linkcnt = iswap16(DIP(dp, nlink));
	if (info->ino_linkcnt <= 0) {
		zlnp = (struct zlncnt *)malloc(sizeof *zlnp);
		if (zlnp == NULL) {
			markclean = 0;
			pfatal("LINK COUNT TABLE OVERFLOW");
			if (reply("CONTINUE") == 0) {
				ckfini(1);
				exit(FSCK_EXIT_CHECK_FAILED);
			}
		} else {
			zlnp->zlncnt = inumber;
			zlnp->next = zlnhead;
			zlnhead = zlnp;
		}
	}
	if (mode == IFDIR) {
		if (size == 0)
			info->ino_state = DCLEAR;
		else
			info->ino_state = DSTATE;
		cacheino(dp, inumber);
		countdirs++;
	} else
		info->ino_state = FSTATE;
	info->ino_type = IFTODT(mode);
	if (!is_ufs2 && doinglevel2 &&
	    (iswap16(dp->dp1.di_ouid) != (u_short)-1 ||
		iswap16(dp->dp1.di_ogid) != (u_short)-1)) {
		dp = ginode(inumber);
		dp->dp1.di_uid = iswap32(iswap16(dp->dp1.di_ouid));
		dp->dp1.di_ouid = iswap16(-1);
		dp->dp1.di_gid = iswap32(iswap16(dp->dp1.di_ogid));
		dp->dp1.di_ogid = iswap16(-1);
		inodirty();
	}
	badblk = dupblk = 0;
	idesc->id_number = inumber;
	idesc->id_uid = iswap32(DIP(dp, uid));
	idesc->id_gid = iswap32(DIP(dp, gid));
	if (iswap32(DIP(dp, flags)) & SF_SNAPSHOT)
		idesc->id_type = SNAP;
	else
		idesc->id_type = ADDR;
	(void)ckinode(dp, idesc);
#ifdef notyet
	if (is_ufs2 && iswap32(dp->dp2.di_extsize) > 0) {
		int ret, offset;
		idesc->id_type = ADDR;
		ndb = howmany(iswap32(dp->dp2.di_extsize), sblock->fs_bsize);
		for (j = 0; j < UFS_NXADDR; j++) {
			if (--ndb == 0 &&
			    (offset = blkoff(sblock, iswap32(dp->dp2.di_extsize))) != 0)
				idesc->id_numfrags = numfrags(sblock,
				    fragroundup(sblock, offset));
			else
				idesc->id_numfrags = sblock->fs_frag;
			if (dp->dp2.di_extb[j] == 0)
				continue;
			idesc->id_blkno = iswap64(dp->dp2.di_extb[j]);
			ret = (*idesc->id_func)(idesc);
			if (ret & STOP)
				break;
		}
	}
#endif
	idesc->id_entryno *= btodb(sblock->fs_fsize);
	if (is_ufs2)
		blocks = iswap64(dp->dp2.di_blocks);
	else
		blocks = iswap32(dp->dp1.di_blocks);
	if (blocks != idesc->id_entryno) {
		pwarn("INCORRECT BLOCK COUNT I=%llu (%lld should be %lld)",
		    (unsigned long long)inumber, (long long)blocks,
		    (long long)idesc->id_entryno);
		if (preen)
			printf(" (CORRECTED)\n");
		else if (reply("CORRECT") == 0) {
			markclean = 0;
			return;
		}
		dp = ginode(inumber);
		if (is_ufs2)
			dp->dp2.di_blocks = iswap64(idesc->id_entryno);
		else
			dp->dp1.di_blocks = iswap32((int32_t)idesc->id_entryno);
		inodirty();
	}
	if (idesc->id_type != SNAP)
		update_uquot(inumber, idesc->id_uid, idesc->id_gid,
		    idesc->id_entryno, 1);
	return;
unknown:
	pfatal("UNKNOWN FILE TYPE I=%llu", (unsigned long long)inumber);
	info->ino_state = FCLEAR;
	if (reply("CLEAR") == 1) {
		info->ino_state = USTATE;
		dp = ginode(inumber);
		clearinode(dp);
		inodirty();
	} else
		markclean = 0;
}

int
pass1check(struct inodesc *idesc)
{
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;
	struct dups *dlp;
	struct dups *new;

	if (idesc->id_type == SNAP) {
		if (blkno == BLK_NOCOPY || blkno == BLK_SNAP)
			return (KEEPON);
	}
	if ((anyout = chkrange(blkno, idesc->id_numfrags)) != 0) {
		blkerror(idesc->id_number, "BAD", blkno);
		if (badblk++ >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%llu",
			    (unsigned long long)idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0) {
				markclean = 0;
				ckfini(1);
				exit(FSCK_EXIT_CHECK_FAILED);
			}
			return (STOP);
		}
	}
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (anyout && chkrange(blkno, 1)) {
			res = SKIP;
		} else if (!testbmap(blkno)) {
			n_blks++;
			setbmap(blkno);
		} else {
			blkerror(idesc->id_number, "DUP", blkno);
			if (dupblk++ >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%llu",
				    (unsigned long long)idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0) {
					markclean = 0;
					ckfini(1);
					exit(FSCK_EXIT_CHECK_FAILED);
				}
				return (STOP);
			}
			new = (struct dups *)malloc(sizeof(struct dups));
			if (new == NULL) {
				markclean = 0;
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0) {
					markclean = 0;
					ckfini(1);
					exit(FSCK_EXIT_CHECK_FAILED);
				}
				return (STOP);
			}
			new->dup = blkno;
			if (muldup == 0) {
				duplist = muldup = new;
				new->next = 0;
			} else {
				new->next = muldup->next;
				muldup->next = new;
			}
			for (dlp = duplist; dlp != muldup; dlp = dlp->next)
				if (dlp->dup == blkno)
					break;
			if (dlp == muldup && dlp->dup != blkno)
				muldup = new;
		}
		/*
		 * count the number of blocks found in id_entryno
		 */
		idesc->id_entryno++;
	}
	return (res);
}
