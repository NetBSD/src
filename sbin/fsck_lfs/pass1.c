/* $NetBSD: pass1.c,v 1.27 2006/10/16 03:21:34 christos Exp $	 */

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

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

SEGUSE *seg_table;
extern ufs_daddr_t *din_table;

ufs_daddr_t badblk;
static ufs_daddr_t dupblk;
static int i_d_cmp(const void *, const void *);

struct ino_daddr {
	ino_t ino;
	ufs_daddr_t daddr;
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
	struct ufs1_dinode *tinode;
	struct ifile *ifp;
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
	dins = (struct ino_daddr **) malloc(maxino * sizeof(*dins));
	if (dins == NULL)
		err(1, NULL);
	for (i = 0; i < maxino; i++) {
		dins[i] = malloc(sizeof(**dins));
		if (dins[i] == NULL)
			err(1, NULL);
		dins[i]->ino = i;
		if (i == fs->lfs_ifile)
			dins[i]->daddr = fs->lfs_idaddr;
		else {
			LFS_IENTRY(ifp, fs, i, bp);
			dins[i]->daddr = ifp->if_daddr;
			brelse(bp);
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
		if (tinode && (tinode->di_mode & IFMT) == IFDIR)
			numdirs++;
	}

	/* from setup.c */
	inplast = 0;
	listmax = numdirs + 10;
	inpsort = (struct inoinfo **) calloc((unsigned) listmax,
	    sizeof(struct inoinfo *));
	inphead = (struct inoinfo **) calloc((unsigned) numdirs,
	    sizeof(struct inoinfo *));
	if (inpsort == NULL || inphead == NULL) {
		printf("cannot alloc %lu bytes for inphead\n",
		    (unsigned long) numdirs * sizeof(struct inoinfo *));
		exit(1);
	}
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

void
checkinode(ino_t inumber, struct inodesc * idesc)
{
	struct ufs1_dinode *dp;
	struct uvnode  *vp;
	struct zlncnt *zlnp;
	struct ubuf *bp;
	IFILE *ifp;
	int ndb, j;
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
	mode = dp->di_mode & IFMT;

	/* XXX - LFS doesn't have this particular problem (?) */
	if (mode == 0) {
		if (memcmp(dp->di_db, zino.di_db, NDADDR * sizeof(ufs_daddr_t)) ||
		    memcmp(dp->di_ib, zino.di_ib, NIADDR * sizeof(ufs_daddr_t)) ||
		    dp->di_mode || dp->di_size) {
			pwarn("mode=o%o, ifmt=o%o\n", dp->di_mode, mode);
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
	if (/* dp->di_size < 0 || */
	    dp->di_size + fs->lfs_bsize - 1 < dp->di_size) {
		if (debug)
			printf("bad size %llu:",
			    (unsigned long long) dp->di_size);
		goto unknown;
	}
	if (!preen && mode == IFMT && reply("HOLD BAD BLOCK") == 1) {
		vp = vget(fs, inumber);
		dp = VTOD(vp);
		dp->di_size = fs->lfs_fsize;
		dp->di_mode = IFREG | 0600;
		inodirty(VTOI(vp));
	}
	ndb = howmany(dp->di_size, fs->lfs_bsize);
	if (ndb < 0) {
		if (debug)
			printf("bad size %llu ndb %d:",
			    (unsigned long long) dp->di_size, ndb);
		goto unknown;
	}
	if (mode == IFBLK || mode == IFCHR)
		ndb++;
	if (mode == IFLNK) {
		/*
		 * Fake ndb value so direct/indirect block checks below
		 * will detect any garbage after symlink string.
		 */
		if (dp->di_size < fs->lfs_maxsymlinklen ||
		    (fs->lfs_maxsymlinklen == 0 && dp->di_blocks == 0)) {
			ndb = howmany(dp->di_size, sizeof(ufs_daddr_t));
			if (ndb > NDADDR) {
				j = ndb - NDADDR;
				for (ndb = 1; j > 1; j--)
					ndb *= NINDIR(fs);
				ndb += NDADDR;
			}
		}
	}
	for (j = ndb; j < NDADDR; j++)
		if (dp->di_db[j] != 0) {
			if (debug)
				printf("bad direct addr for size %lld lbn %d: 0x%x\n",
					(long long)dp->di_size, j, (unsigned)dp->di_db[j]);
			goto unknown;
		}
	for (j = 0, ndb -= NDADDR; ndb > 0; j++)
		ndb /= NINDIR(fs);
	for (; j < NIADDR; j++)
		if (dp->di_ib[j] != 0) {
			if (debug)
				printf("bad indirect addr for size %lld # %d: 0x%x\n",
					(long long)dp->di_size, j, (unsigned)dp->di_ib[j]);
			goto unknown;
		}
	if (ftypeok(dp) == 0)
		goto unknown;
	n_files++;
	lncntp[inumber] = dp->di_nlink;
	if (dp->di_nlink <= 0) {
		zlnp = (struct zlncnt *) malloc(sizeof *zlnp);
		if (zlnp == NULL) {
			pfatal("LINK COUNT TABLE OVERFLOW");
			if (reply("CONTINUE") == 0)
				err(8, "%s", "");
		} else {
			zlnp->zlncnt = inumber;
			zlnp->next = zlnhead;
			zlnhead = zlnp;
		}
	}
	if (mode == IFDIR) {
		if (dp->di_size == 0)
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
	if (dp->di_nlink <= 0) {
		LFS_IENTRY(ifp, fs, inumber, bp);
		if (ifp->if_nextfree == LFS_ORPHAN_NEXTFREE) {
			statemap[inumber] = (mode == IFDIR ? DCLEAR : FCLEAR);
			/* Add this to our list of orphans */
			zlnp = (struct zlncnt *) malloc(sizeof *zlnp);
			if (zlnp == NULL) {
				pfatal("LINK COUNT TABLE OVERFLOW");
				if (reply("CONTINUE") == 0)
					err(8, "%s", "");
			} else {
				zlnp->zlncnt = inumber;
				zlnp->next = orphead;
				orphead = zlnp;
			}
		}
		brelse(bp);
	}

	typemap[inumber] = IFTODT(mode);
	badblk = dupblk = 0;
	idesc->id_number = inumber;
	(void) ckinode(VTOD(vp), idesc);
	if (dp->di_blocks != idesc->id_entryno) {
		pwarn("INCORRECT BLOCK COUNT I=%llu (%d SHOULD BE %d)",
		    (unsigned long long)inumber, dp->di_blocks,
		    idesc->id_entryno);
		if (preen)
			printf(" (CORRECTED)\n");
		else if (reply("CORRECT") == 0)
			return;
		VTOI(vp)->i_ffs1_blocks = idesc->id_entryno;
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

	if ((anyout = chkrange(blkno, fragstofsb(fs, idesc->id_numfrags))) != 0) {
		blkerror(idesc->id_number, "BAD", blkno);
		if (badblk++ >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%llu",
			    (unsigned long long)idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				err(8, "%s", "");
			return (STOP);
		}
	} else if (!testbmap(blkno)) {
		seg_table[dtosn(fs, blkno)].su_nbytes += idesc->id_numfrags * fs->lfs_fsize;
	}
	for (ndblks = fragstofsb(fs, idesc->id_numfrags); ndblks > 0; blkno++, ndblks--) {
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
			if (dupblk++ >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%llu",
				    (unsigned long long)idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					err(8, "%s", "");
				return (STOP);
			}
			new = (struct dups *) malloc(sizeof(struct dups));
			if (new == NULL) {
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					err(8, "%s", "");
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
