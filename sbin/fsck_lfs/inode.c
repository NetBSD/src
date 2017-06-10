/* $NetBSD: inode.c,v 1.69 2017/06/10 08:13:15 pgoyette Exp $	 */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>
#undef vnode

#include <err.h>
#ifndef SMALL
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

static int iblock(struct inodesc *, long, u_int64_t);
int blksreqd(struct lfs *, int);
int lfs_maxino(void);

/*
 * Get a dinode of a given inum.
 * XXX combine this function with vget.
 */
union lfs_dinode *
ginode(ino_t ino)
{
	struct uvnode *vp;
	struct ubuf *bp;
	IFILE *ifp;
	daddr_t daddr;
	unsigned segno;

	vp = vget(fs, ino);
	if (vp == NULL)
		return NULL;

	if (din_table[ino] == 0x0) {
		LFS_IENTRY(ifp, fs, ino, bp);
		daddr = lfs_if_getdaddr(fs, ifp);
		segno = lfs_dtosn(fs, daddr);
		din_table[ino] = daddr;
		seg_table[segno].su_nbytes += DINOSIZE(fs);
		brelse(bp, 0);
	}
	return VTOI(vp)->i_din;
}

/*
 * Check validity of held blocks in an inode, recursing through all blocks.
 */
int
ckinode(union lfs_dinode *dp, struct inodesc *idesc)
{
	daddr_t lbn, pbn;
	long ret, n, ndb, offset;
	union lfs_dinode dino;
	u_int64_t remsize, sizepb;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp, *thisvp;

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = lfs_dino_getsize(fs, dp);
	mode = lfs_dino_getmode(fs, dp) & LFS_IFMT;
	if (mode == LFS_IFBLK || mode == LFS_IFCHR ||
	    (mode == LFS_IFLNK && (lfs_dino_getsize(fs, dp) < lfs_sb_getmaxsymlinklen(fs) ||
		    (lfs_sb_getmaxsymlinklen(fs) == 0 &&
			lfs_dino_getblocks(fs, dp) == 0))))
		return (KEEPON);
	/* XXX is this safe if we're 32-bit? */
	dino = *dp;
	ndb = howmany(lfs_dino_getsize(fs, &dino), lfs_sb_getbsize(fs));

	thisvp = vget(fs, idesc->id_number);
	for (lbn = 0; lbn < ULFS_NDADDR; lbn++) {
		pbn = lfs_dino_getdb(fs, &dino, lbn);
		if (thisvp)
			idesc->id_numfrags =
				lfs_numfrags(fs, VTOI(thisvp)->i_lfs_fragsize[lbn]);
		else {
			if (--ndb == 0 && (offset = lfs_blkoff(fs, lfs_dino_getsize(fs, &dino))) != 0) {
				idesc->id_numfrags =
			    	lfs_numfrags(fs, lfs_fragroundup(fs, offset));
			} else
				idesc->id_numfrags = lfs_sb_getfrag(fs);
		}
		if (pbn == 0) {
			if (idesc->id_type == DATA && ndb >= 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s INO %lld: CONTAINS EMPTY BLOCKS [1]",
				    pathbuf, (long long)idesc->id_number);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					dp = VTOD(vp);
					lfs_dino_setsize(fs, dp,
					    lbn * lfs_sb_getbsize(fs));
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
				} else
					break;
			}
			continue;
		}
		idesc->id_blkno = pbn;
		idesc->id_lblkno = lbn;
		if (idesc->id_type == ADDR) {
			ret = (*idesc->id_func) (idesc);
		} else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = lfs_sb_getfrag(fs);
	remsize = lfs_dino_getsize(fs, &dino) - lfs_sb_getbsize(fs) * ULFS_NDADDR;
	sizepb = lfs_sb_getbsize(fs);
	for (n = 1; n <= ULFS_NIADDR; n++) {
		pbn = lfs_dino_getib(fs, &dino, n-1);
		if (pbn) {
			idesc->id_blkno = pbn;
			ret = iblock(idesc, n, remsize);
			if (ret & STOP)
				return (ret);
		} else {
			if (idesc->id_type == DATA && remsize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s INO %lld: CONTAINS EMPTY BLOCKS [2]",
				    pathbuf, (long long)idesc->id_number);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					dp = VTOD(vp);
					lfs_dino_setsize(fs, dp,
					    lfs_dino_getsize(fs, dp) - remsize);
					remsize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
					break;
				} else
					break;
			}
		}
		sizepb *= LFS_NINDIR(fs);
		remsize -= sizepb;
	}
	return (KEEPON);
}

static int
iblock(struct inodesc *idesc, long ilevel, u_int64_t isize)
{
	unsigned j, maxindir;
	daddr_t found;
	struct ubuf *bp;
	int i, n, (*func) (struct inodesc *), nif;
	u_int64_t sizepb;
	char pathbuf[MAXPATHLEN + 1], buf[BUFSIZ];
	struct uvnode *devvp, *vp;
	int diddirty = 0;

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		n = (*func) (idesc);
		if ((n & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (chkrange(idesc->id_blkno, idesc->id_numfrags))
		return (SKIP);

	devvp = fs->lfs_devvp;
	bread(devvp, LFS_FSBTODB(fs, idesc->id_blkno), lfs_sb_getbsize(fs),
	    0, &bp);
	ilevel--;
	for (sizepb = lfs_sb_getbsize(fs), i = 0; i < ilevel; i++)
		sizepb *= LFS_NINDIR(fs);
	if (isize > sizepb * LFS_NINDIR(fs))
		nif = LFS_NINDIR(fs);
	else
		nif = howmany(isize, sizepb);
	if (idesc->id_func == pass1check && nif < LFS_NINDIR(fs)) {
		maxindir = LFS_NINDIR(fs);
		for (j = nif; j < maxindir; j++) {
			found = lfs_iblock_get(fs, bp->b_data, j);
			if (found == 0)
				continue;
			(void)snprintf(buf, sizeof(buf),
			    "PARTIALLY TRUNCATED INODE I=%llu",
			    (unsigned long long)idesc->id_number);
			if (dofix(idesc, buf)) {
				lfs_iblock_set(fs, bp->b_data, j, 0);
				++diddirty;
			}
		}
	}
	maxindir = nif;
	for (j = 0; j < maxindir; j++) {
		found = lfs_iblock_get(fs, bp->b_data, j);
		if (found) {
			idesc->id_blkno = found;
			if (ilevel == 0) {
				/*
				 * dirscan needs lfs_lblkno.
				 */
				idesc->id_lblkno++;
				n = (*func) (idesc);
			} else {
				n = iblock(idesc, ilevel, isize);
			}
			if (n & STOP) {
				if (diddirty)
					VOP_BWRITE(bp);
				else
					brelse(bp, 0);
				return (n);
			}
		} else {
			if (idesc->id_type == DATA && isize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s INO %lld: CONTAINS EMPTY BLOCKS [3]",
				    pathbuf, (long long)idesc->id_number);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					lfs_dino_setsize(fs, VTOI(vp)->i_din,
					    lfs_dino_getsize(fs,
							     VTOI(vp)->i_din)
					    - isize);
					isize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
					if (diddirty)
						VOP_BWRITE(bp);
					else
						brelse(bp, 0);
					return (STOP);
				}
			}
		}
		isize -= sizepb;
	}
	if (diddirty)
		VOP_BWRITE(bp);
	else
		brelse(bp, 0);
	return (KEEPON);
}

/*
 * Check that a block in a legal block number.
 * Return 0 if in range, 1 if out of range.
 */
int
chkrange(daddr_t blk, int cnt)
{
	if (blk < lfs_sntod(fs, 0)) {
		return (1);
	}
	if (blk > maxfsblock) {
		return (1);
	}
	if (blk + cnt < lfs_sntod(fs, 0)) {
		return (1);
	}
	if (blk + cnt > maxfsblock) {
		return (1);
	}
	return (0);
}

/*
 * Routines to maintain information about directory inodes.
 * This is built during the first pass and used during the
 * second and third passes.
 *
 * Enter inodes into the cache.
 */
void
cacheino(union lfs_dinode *dp, ino_t inumber)
{
	struct inoinfo *inp;
	struct inoinfo **inpp, **ninpsort;
	unsigned int blks, i;

	blks = howmany(lfs_dino_getsize(fs, dp), lfs_sb_getbsize(fs));
	if (blks > ULFS_NDADDR)
		blks = ULFS_NDADDR + ULFS_NIADDR;
	inp = emalloc(sizeof(*inp) + (blks - 1) * sizeof(inp->i_blks[0]));
	inpp = &inphead[inumber % numdirs];
	inp->i_nexthash = *inpp;
	*inpp = inp;
	inp->i_child = inp->i_sibling = inp->i_parentp = 0;
	if (inumber == ULFS_ROOTINO)
		inp->i_parent = ULFS_ROOTINO;
	else
		inp->i_parent = (ino_t) 0;
	inp->i_dotdot = (ino_t) 0;
	inp->i_number = inumber;
	inp->i_isize = lfs_dino_getsize(fs, dp);

	inp->i_numblks = blks * sizeof(inp->i_blks[0]);
	for (i=0; i<blks && i<ULFS_NDADDR; i++) {
		inp->i_blks[i] = lfs_dino_getdb(fs, dp, i);
	}
	for (; i<blks; i++) {
		inp->i_blks[i] = lfs_dino_getib(fs, dp, i - ULFS_NDADDR);
	}
	if (inplast == listmax) {
		ninpsort = erealloc(inpsort,
		    (listmax + 100) * sizeof(struct inoinfo *));
		inpsort = ninpsort;
		listmax += 100;
	}
	inpsort[inplast++] = inp;
}

/*
 * Look up an inode cache structure.
 */
struct inoinfo *
getinoinfo(ino_t inumber)
{
	struct inoinfo *inp;

	for (inp = inphead[inumber % numdirs]; inp; inp = inp->i_nexthash) {
		if (inp->i_number != inumber)
			continue;
		return (inp);
	}
	err(EEXIT, "cannot find inode %llu", (unsigned long long)inumber);
	return ((struct inoinfo *) 0);
}

/*
 * Clean up all the inode cache structure.
 */
void
inocleanup(void)
{
	struct inoinfo **inpp;

	if (inphead == NULL)
		return;
	for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--)
		free((char *) (*inpp));
	free((char *) inphead);
	free((char *) inpsort);
	inphead = inpsort = NULL;
}

void
inodirty(struct inode *ip)
{
	ip->i_state |= IN_MODIFIED;
}

void
clri(struct inodesc * idesc, const char *type, int flag)
{
	struct uvnode *vp;

	vp = vget(fs, idesc->id_number);
	if (flag & 0x1) {
		pwarn("%s %s", type,
		      (lfs_dino_getmode(fs, VTOI(vp)->i_din) & LFS_IFMT) == LFS_IFDIR ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if ((flag & 0x2) || preen || reply("CLEAR") == 1) {
		if (preen && flag != 2)
			printf(" (CLEARED)\n");
		n_files--;
		(void) ckinode(VTOD(vp), idesc);
		clearinode(idesc->id_number);
		statemap[idesc->id_number] = USTATE;
		vnode_destroy(vp);
		return;
	}
	return;
}

void
clearinode(ino_t inumber)
{
	struct ubuf *bp;
	IFILE *ifp;
	daddr_t daddr;

	/* Send cleared inode to the free list */

	LFS_IENTRY(ifp, fs, inumber, bp);
	daddr = lfs_if_getdaddr(fs, ifp);
	if (daddr == LFS_UNUSED_DADDR) {
		brelse(bp, 0);
		return;
	}
	lfs_if_setdaddr(fs, ifp, LFS_UNUSED_DADDR);
	lfs_if_setnextfree(fs, ifp, lfs_sb_getfreehd(fs));
	lfs_sb_setfreehd(fs, inumber);
	sbdirty();
	VOP_BWRITE(bp);

	/*
	 * update segment usage.
	 */
	if (daddr != LFS_UNUSED_DADDR) {
		SEGUSE *sup;
		u_int32_t oldsn = lfs_dtosn(fs, daddr);

		seg_table[oldsn].su_nbytes -= DINOSIZE(fs);
		LFS_SEGENTRY(sup, fs, oldsn, bp);
		sup->su_nbytes -= DINOSIZE(fs);
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp);	/* Ifile */
	}
}

int
findname(struct inodesc * idesc)
{
	LFS_DIRHEADER *dirp = idesc->id_dirp;
	size_t len;
	char *buf;

	if (lfs_dir_getino(fs, dirp) != idesc->id_parent)
		return (KEEPON);
	len = lfs_dir_getnamlen(fs, dirp) + 1;
	if (len > MAXPATHLEN) {
		/* Truncate it but don't overflow the buffer */
		/* XXX: this case doesn't null-terminate the result */
		len = MAXPATHLEN;
	}
	/* this is namebuf with utils.h */
	buf = __UNCONST(idesc->id_name);
	(void)memcpy(buf, lfs_dir_nameptr(fs, dirp), len);
	return (STOP | FOUND);
}

int
findino(struct inodesc * idesc)
{
	LFS_DIRHEADER *dirp = idesc->id_dirp;
	ino_t ino;

	ino = lfs_dir_getino(fs, dirp);
	if (ino == 0)
		return (KEEPON);
	if (strcmp(lfs_dir_nameptr(fs, dirp), idesc->id_name) == 0 &&
	    ino >= ULFS_ROOTINO && ino < maxino) {
		idesc->id_parent = ino;
		return (STOP | FOUND);
	}
	return (KEEPON);
}

void
pinode(ino_t ino)
{
	union lfs_dinode *dp;
	struct passwd *pw;

	printf(" I=%llu ", (unsigned long long)ino);
	if (ino < ULFS_ROOTINO || ino >= maxino)
		return;
	dp = ginode(ino);
	if (dp) {
		printf(" OWNER=");
#ifndef SMALL
		if (Uflag && (pw = getpwuid(lfs_dino_getuid(fs, dp))) != 0)
			printf("%s ", pw->pw_name);
		else
#endif
			printf("%u ", (unsigned)lfs_dino_getuid(fs, dp));
		printf("MODE=%o\n", lfs_dino_getmode(fs, dp));
		if (preen)
			printf("%s: ", cdevname());
		printf("SIZE=%ju ", (uintmax_t) lfs_dino_getsize(fs, dp));
		printf("MTIME=%s ", print_mtime(lfs_dino_getmtime(fs, dp)));
	}
}

void
blkerror(ino_t ino, const char *type, daddr_t blk)
{

	pfatal("%lld %s I=%llu", (long long) blk, type,
	    (unsigned long long)ino);
	printf("\n");
	if (exitonfail)
		exit(1);
	switch (statemap[ino]) {

	case FSTATE:
		statemap[ino] = FCLEAR;
		return;

	case DSTATE:
		statemap[ino] = DCLEAR;
		return;

	case FCLEAR:
	case DCLEAR:
		return;

	default:
		err(EEXIT, "BAD STATE %d TO BLKERR", statemap[ino]);
		/* NOTREACHED */
	}
}

/*
 * allocate an unused inode
 */
ino_t
allocino(ino_t request, int type)
{
	ino_t ino;
	union lfs_dinode *dp;
	time_t t;
	struct uvnode *vp;
	struct ubuf *bp;

	if (request == 0)
		request = ULFS_ROOTINO;
	else if (statemap[request] != USTATE)
		return (0);
	for (ino = request; ino < maxino; ino++)
		if (statemap[ino] == USTATE)
			break;
	if (ino == maxino)
		extend_ifile(fs);

	switch (type & LFS_IFMT) {
	case LFS_IFDIR:
		statemap[ino] = DSTATE;
		break;
	case LFS_IFREG:
	case LFS_IFLNK:
		statemap[ino] = FSTATE;
		break;
	default:
		return (0);
	}
        vp = lfs_valloc(fs, ino);
	if (vp == NULL)
		return (0);
	dp = VTOI(vp)->i_din;
	bp = getblk(vp, 0, lfs_sb_getfsize(fs));
	VOP_BWRITE(bp);
	lfs_dino_setmode(fs, dp, type);
	(void) time(&t);
	lfs_dino_setatime(fs, dp, t);
	lfs_dino_setctime(fs, dp, t);
	lfs_dino_setmtime(fs, dp, t);
	lfs_dino_setsize(fs, dp, lfs_sb_getfsize(fs));
	lfs_dino_setblocks(fs, dp, lfs_btofsb(fs, lfs_sb_getfsize(fs)));
	n_files++;
	inodirty(VTOI(vp));
	typemap[ino] = LFS_IFTODT(type);
	return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino_t ino)
{
	struct inodesc idesc;
	struct uvnode *vp;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	vp = vget(fs, ino);
	(void) ckinode(VTOD(vp), &idesc);
	clearinode(ino);
	statemap[ino] = USTATE;
	vnode_destroy(vp);

	n_files--;
}
