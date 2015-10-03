/* $NetBSD: pass1.c,v 1.45 2015/10/03 08:30:13 dholland Exp $	 */

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
#include <sys/buf.h>

#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>
#undef vnode

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

blkcnt_t badblkcount;
static blkcnt_t dupblkcount;
static int i_d_cmp(const void *, const void *);

struct ino_daddr {
	ino_t ino;
	daddr_t daddr;
};

static int
i_d_cmp(const void *va, const void *vb)
{
	const struct ino_daddr *a, *b;

	a = *((const struct ino_daddr *const *) va);
	b = *((const struct ino_daddr *const *) vb);

	if (a->daddr == b->daddr) {
		return (a->ino - b->ino);
	}
	if (a->daddr > b->daddr) {
		return 1;
	}
	return -1;
}

void
pass1(void)
{
	ino_t inumber;
	int i;
	struct inodesc idesc;
	union lfs_dinode *tinode;
	IFILE *ifp;
	struct ubuf *bp;
	struct ino_daddr **dins;

	/*
	 * Find all allocated blocks, initialize numdirs.
	 */
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	idesc.id_lblkno = 0;
	inumber = 0;
	n_files = n_blks = 0;

	if (debug)
		printf("creating sorted inode address table...\n");
	/* Sort by daddr */
	dins = ecalloc(maxino, sizeof(*dins));
	for (i = 0; i < maxino; i++) {
		dins[i] = emalloc(sizeof(**dins));
		dins[i]->ino = i;
		if (i == LFS_IFILE_INUM)
			dins[i]->daddr = lfs_sb_getidaddr(fs);
		else {
			LFS_IENTRY(ifp, fs, i, bp);
			dins[i]->daddr = lfs_if_getdaddr(fs, ifp);
			brelse(bp, 0);
		}
	}
	qsort(dins, maxino, sizeof(*dins), i_d_cmp);

	/* find a value for numdirs, fill in din_table */
	if (debug)
		printf("counting dirs...\n");
	numdirs = 0;
	for (i = 0; i < maxino; i++) {
		inumber = dins[i]->ino;
		if (inumber == 0 || dins[i]->daddr == 0)
			continue;
		tinode = ginode(inumber);
		if (tinode && (lfs_dino_getmode(fs, tinode) & LFS_IFMT) == LFS_IFDIR)
			numdirs++;
	}

	/* from setup.c */
	inplast = 0;
	listmax = numdirs + 10;
	inpsort = ecalloc(listmax, sizeof(struct inoinfo *));
	inphead = ecalloc(numdirs, sizeof(struct inoinfo *));
	if (debug)
		printf("counting blocks...\n");

	for (i = 0; i < maxino; i++) {
		inumber = dins[i]->ino;
		if (inumber == 0 || dins[i]->daddr == 0) {
			statemap[inumber] = USTATE;
			continue;
		}
		if (dins[i]->daddr != LFS_UNUSED_DADDR) {
			checkinode(inumber, &idesc);
		} else {
			statemap[inumber] = USTATE;
		}
		free(dins[i]);
	}
	free(dins);
}

static int
nonzero_db(union lfs_dinode *dip)
{
	unsigned i;

	for (i=0; i<ULFS_NDADDR; i++) {
		if (lfs_dino_getdb(fs, dip, i) != 0) {
			return 1;
		}
	}
	return 0;
}

static int
nonzero_ib(union lfs_dinode *dip)
{
	unsigned i;

	for (i=0; i<ULFS_NIADDR; i++) {
		if (lfs_dino_getib(fs, dip, i) != 0) {
			return 1;
		}
	}
	return 0;
}

void
checkinode(ino_t inumber, struct inodesc * idesc)
{
	union lfs_dinode *dp;
	struct uvnode  *vp;
	struct zlncnt *zlnp;
	struct ubuf *bp;
	IFILE *ifp;
	int64_t ndb, j;
	mode_t mode;

	vp = vget(fs, inumber);
	if (vp)
		dp = VTOD(vp);
	else
		dp = NULL;

	if (dp == NULL) {
		statemap[inumber] = USTATE;
		return;
	}
	mode = lfs_dino_getmode(fs, dp) & LFS_IFMT;

	/* XXX - LFS doesn't have this particular problem (?) */
	if (mode == 0) {
		if (nonzero_db(dp) || nonzero_ib(dp) ||
		    lfs_dino_getmode(fs, dp) || lfs_dino_getsize(fs, dp)) {
			pwarn("mode=o%o, ifmt=o%o\n", lfs_dino_getmode(fs, dp), mode);
			pfatal("PARTIALLY ALLOCATED INODE I=%llu",
			    (unsigned long long)inumber);
			if (reply("CLEAR") == 1) {
				vp = vget(fs, inumber);
				clearinode(inumber);
				vnode_destroy(vp);
			}
		}
		statemap[inumber] = USTATE;
		return;
	}
	lastino = inumber;
	if (/* lfs_dino_getsize(fs, dp) < 0 || */
	    lfs_dino_getsize(fs, dp) + lfs_sb_getbsize(fs) - 1 < lfs_dino_getsize(fs, dp)) {
		if (debug)
			printf("bad size %ju:",
			    (uintmax_t)lfs_dino_getsize(fs, dp));
		goto unknown;
	}
	if (!preen && mode == LFS_IFMT && reply("HOLD BAD BLOCK") == 1) {
		vp = vget(fs, inumber);
		dp = VTOD(vp);
		lfs_dino_setsize(fs, dp, lfs_sb_getfsize(fs));
		lfs_dino_setmode(fs, dp, LFS_IFREG | 0600);
		inodirty(VTOI(vp));
	}
	ndb = howmany(lfs_dino_getsize(fs, dp), lfs_sb_getbsize(fs));
	if (ndb < 0) {
		if (debug)
			printf("bad size %ju ndb %jd:",
			    (uintmax_t)lfs_dino_getsize(fs, dp), (intmax_t)ndb);
		goto unknown;
	}
	if (mode == LFS_IFBLK || mode == LFS_IFCHR)
		ndb++;
	if (mode == LFS_IFLNK) {
		/*
		 * Fake ndb value so direct/indirect block checks below
		 * will detect any garbage after symlink string.
		 */
		if (lfs_dino_getsize(fs, dp) < lfs_sb_getmaxsymlinklen(fs) ||
		    (lfs_sb_getmaxsymlinklen(fs) == 0 && lfs_dino_getblocks(fs, dp) == 0)) {
			ndb = howmany(lfs_dino_getsize(fs, dp), LFS_BLKPTRSIZE(fs));
			if (ndb > ULFS_NDADDR) {
				j = ndb - ULFS_NDADDR;
				for (ndb = 1; j > 1; j--)
					ndb *= LFS_NINDIR(fs);
				ndb += ULFS_NDADDR;
			}
		}
	}
	for (j = ndb; j < ULFS_NDADDR; j++)
		if (lfs_dino_getdb(fs, dp, j) != 0) {
			if (debug)
				printf("bad direct addr for size %ju lbn %jd: 0x%jx\n",
					(uintmax_t)lfs_dino_getsize(fs, dp),
					(intmax_t)j,
					(intmax_t)lfs_dino_getdb(fs, dp, j));
			goto unknown;
		}
	for (j = 0, ndb -= ULFS_NDADDR; ndb > 0; j++)
		ndb /= LFS_NINDIR(fs);
	for (; j < ULFS_NIADDR; j++)
		if (lfs_dino_getib(fs, dp, j) != 0) {
			if (debug)
				printf("bad indirect addr for size %ju # %jd: 0x%jx\n",
					(uintmax_t)lfs_dino_getsize(fs, dp),
					(intmax_t)j,
					(intmax_t)lfs_dino_getib(fs, dp, j));
			goto unknown;
		}
	if (ftypeok(dp) == 0)
		goto unknown;
	n_files++;
	lncntp[inumber] = lfs_dino_getnlink(fs, dp);
	if (lfs_dino_getnlink(fs, dp) <= 0) {
		zlnp = emalloc(sizeof *zlnp);
		zlnp->zlncnt = inumber;
		zlnp->next = zlnhead;
		zlnhead = zlnp;
	}
	if (mode == LFS_IFDIR) {
		if (lfs_dino_getsize(fs, dp) == 0)
			statemap[inumber] = DCLEAR;
		else
			statemap[inumber] = DSTATE;
		cacheino(dp, inumber);
	} else
		statemap[inumber] = FSTATE;

	/*
	 * Check for an orphaned file.  These happen when the cleaner has
	 * to rewrite blocks from a file whose directory operation (removal)
	 * is in progress.
	 */
	if (lfs_dino_getnlink(fs, dp) <= 0) {
		LFS_IENTRY(ifp, fs, inumber, bp);
		if (lfs_if_getnextfree(fs, ifp) == LFS_ORPHAN_NEXTFREE) {
			statemap[inumber] = (mode == LFS_IFDIR ? DCLEAR : FCLEAR);
			/* Add this to our list of orphans */
			zlnp = emalloc(sizeof *zlnp);
			zlnp->zlncnt = inumber;
			zlnp->next = orphead;
			orphead = zlnp;
		}
		brelse(bp, 0);
	}

	/* XXX: why VTOD(vp) instead of dp here? */

	typemap[inumber] = LFS_IFTODT(mode);
	badblkcount = dupblkcount = 0;
	idesc->id_number = inumber;
	(void) ckinode(VTOD(vp), idesc);
	if (lfs_dino_getblocks(fs, dp) != idesc->id_entryno) {
		pwarn("INCORRECT BLOCK COUNT I=%llu (%ju SHOULD BE %lld)",
		    (unsigned long long)inumber, lfs_dino_getblocks(fs, dp),
		    idesc->id_entryno);
		if (preen)
			printf(" (CORRECTED)\n");
		else if (reply("CORRECT") == 0)
			return;
		lfs_dino_setblocks(fs, VTOD(vp), idesc->id_entryno);
		inodirty(VTOI(vp));
	}
	return;
unknown:
	pfatal("UNKNOWN FILE TYPE I=%llu", (unsigned long long)inumber);
	statemap[inumber] = FCLEAR;
	if (reply("CLEAR") == 1) {
		statemap[inumber] = USTATE;
		vp = vget(fs, inumber);
		clearinode(inumber);
		vnode_destroy(vp);
	}
}

int
pass1check(struct inodesc *idesc)
{
	int res = KEEPON;
	int anyout, ndblks;
	daddr_t blkno = idesc->id_blkno;
	struct dups *dlp;
	struct dups *new;

	if ((anyout = chkrange(blkno, idesc->id_numfrags)) != 0) {
		blkerror(idesc->id_number, "BAD", blkno);
		if (badblkcount++ >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%llu",
			    (unsigned long long)idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				err(EEXIT, "%s", "");
			return (STOP);
		}
	} else if (!testbmap(blkno)) {
		seg_table[lfs_dtosn(fs, blkno)].su_nbytes += idesc->id_numfrags * lfs_sb_getfsize(fs);
	}
	for (ndblks = idesc->id_numfrags; ndblks > 0; blkno++, ndblks--) {
		if (anyout && chkrange(blkno, 1)) {
			res = SKIP;
		} else if (!testbmap(blkno)) {
			n_blks++;
#ifndef VERBOSE_BLOCKMAP
			setbmap(blkno);
#else
			setbmap(blkno, idesc->id_number);
#endif
		} else {
			blkerror(idesc->id_number, "DUP", blkno);
#ifdef VERBOSE_BLOCKMAP
			pwarn("(lbn %lld: Holder is %lld)\n",
				(long long)idesc->id_lblkno,
				(long long)testbmap(blkno));
#endif
			if (dupblkcount++ >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%llu",
				    (unsigned long long)idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					err(EEXIT, "%s", "");
				return (STOP);
			}
			new = emalloc(sizeof(struct dups));
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
