/*	$NetBSD: inode.c,v 1.41 2003/09/19 08:35:15 itojun Exp $	*/

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
static char sccsid[] = "@(#)inode.c	8.8 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: inode.c,v 1.41 2003/09/19 08:35:15 itojun Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#ifndef SMALL
#include <err.h>
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

static ino_t startinum;

static int iblock __P((struct inodesc *, long, u_int64_t));
static void swap_dinode1(union dinode *, int);
static void swap_dinode2(union dinode *, int);

int
ckinode(dp, idesc)
	union dinode *dp;
	struct inodesc *idesc;
{
	int ret, offset, i;
	union dinode dino;
	u_int64_t sizepb;
	int64_t remsize;
	daddr_t ndb;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = iswap64(DIP(dp, size));
	mode = iswap16(DIP(dp, mode)) & IFMT;
	if (mode == IFBLK || mode == IFCHR || (mode == IFLNK &&
	    (idesc->id_filesize < sblock->fs_maxsymlinklen ||
	    (isappleufs && (idesc->id_filesize < APPLEUFS_MAXSYMLINKLEN)) ||
	     (sblock->fs_maxsymlinklen == 0 && DIP(dp, blocks) == 0))))
		return (KEEPON);
	if (is_ufs2)
		dino.dp2 = dp->dp2;
	else
		dino.dp1 = dp->dp1;
	ndb = howmany(iswap64(DIP(&dino, size)), sblock->fs_bsize);
	for (i = 0; i < NDADDR; i++) {
		if (--ndb == 0 &&
		    (offset = blkoff(sblock, iswap64(DIP(&dino, size)))) != 0)
			idesc->id_numfrags =
				numfrags(sblock, fragroundup(sblock, offset));
		else
			idesc->id_numfrags = sblock->fs_frag;
		if (DIP(&dino, db[i]) == 0) {
			if (idesc->id_type == DATA && ndb >= 0) {
				/* An empty block in a directory XXX */
				markclean = 0;
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS I",
				    pathbuf);
				abort();
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					DIP(dp, size) = iswap64(i *
					    sblock->fs_bsize);
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
				}
			}
			continue;
		}
		if (is_ufs2)
			idesc->id_blkno = iswap64(dino.dp2.di_db[i]);
		else
			idesc->id_blkno = iswap32(dino.dp1.di_db[i]);
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = sblock->fs_frag;
	remsize = iswap64(DIP(&dino, size)) - sblock->fs_bsize * NDADDR;
	sizepb = sblock->fs_bsize;
	for (i = 0; i < NIADDR; i++) {
		if (DIP(&dino, ib[i])) {
			if (is_ufs2)
				idesc->id_blkno = iswap64(dino.dp2.di_ib[i]);
			else
				idesc->id_blkno = iswap32(dino.dp1.di_ib[i]);
			ret = iblock(idesc, i + 1, remsize);
			if (ret & STOP)
				return (ret);
		} else {
			if (idesc->id_type == DATA && remsize > 0) {
				/* An empty block in a directory XXX */
				markclean = 0;
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					DIP(dp, size) =
					    iswap64(iswap64(DIP(dp, size))
						- remsize);
					remsize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
					break;
				}
			}
		}
		sizepb *= NINDIR(sblock);
		remsize -= sizepb;
	}
	return (KEEPON);
}

static int
iblock(idesc, ilevel, isize)
	struct inodesc *idesc;
	long ilevel;
	u_int64_t isize;
{
	struct bufarea *bp;
	int i, n, (*func) __P((struct inodesc *)), nif;
	u_int64_t sizepb;
	char buf[BUFSIZ];
	char pathbuf[MAXPATHLEN + 1];
	union dinode *dp;

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (chkrange(idesc->id_blkno, idesc->id_numfrags))
		return (SKIP);
	bp = getdatablk(idesc->id_blkno, sblock->fs_bsize);
	ilevel--;
	for (sizepb = sblock->fs_bsize, i = 0; i < ilevel; i++)
		sizepb *= NINDIR(sblock);
	if (howmany(isize, sizepb) > NINDIR(sblock))
		nif = NINDIR(sblock);
	else
		nif = howmany(isize, sizepb);
	if (do_blkswap) { /* swap byte order of the whole blk */
		if (is_ufs2) {
			for (i = 0; i < nif; i++)
				bp->b_un.b_indir2[i] =
				    bswap64(bp->b_un.b_indir2[i]);
		} else {
			for (i = 0; i < nif; i++)
				bp->b_un.b_indir1[i] =
				    bswap32(bp->b_un.b_indir1[i]);
		}
		dirty(bp);
		flush(fswritefd, bp);
	}
	if (idesc->id_func == pass1check && nif < NINDIR(sblock)) {
		for (i = nif; i < NINDIR(sblock); i++) {
			if (IBLK(bp, i) == 0)
				continue;
			(void)snprintf(buf, sizeof(buf),
			    "PARTIALLY TRUNCATED INODE I=%u", idesc->id_number);
			if (dofix(idesc, buf)) {
				IBLK(bp, i) = 0;
				dirty(bp);
			} else
				markclean=  0;
		}
		flush(fswritefd, bp);
	}
	for (i = 0; i < nif; i++) {
		if (IBLK(bp, i)) {
			if (is_ufs2)
				idesc->id_blkno = iswap64(bp->b_un.b_indir2[i]);
			else
				idesc->id_blkno = iswap32(bp->b_un.b_indir1[i]);
			if (ilevel == 0)
				n = (*func)(idesc);
			else
				n = iblock(idesc, ilevel, isize);
			if (n & STOP) {
				bp->b_flags &= ~B_INUSE;
				return (n);
			}
		} else {
			if (idesc->id_type == DATA && isize > 0) {
				/* An empty block in a directory XXX */
				markclean=  0;
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					DIP(dp, size) =
					    iswap64(iswap64(DIP(dp, size))
						- isize);
					isize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
					bp->b_flags &= ~B_INUSE;
					return(STOP);
				}
			}
		}
		isize -= sizepb;
	}
	bp->b_flags &= ~B_INUSE;
	return (KEEPON);
}

/*
 * Check that a block in a legal block number.
 * Return 0 if in range, 1 if out of range.
 */
int
chkrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	int c;

	if ((unsigned)(blk + cnt) > maxfsblock)
		return (1);
	if (cnt > sblock->fs_frag ||
	    fragnum(sblock, blk) + cnt > sblock->fs_frag) {
		if (debug)
			printf("bad size: blk %lld, offset %d, size %d\n",
			    (long long)blk, (int)fragnum(sblock, blk), cnt);
	}
	c = dtog(sblock, blk);
	if (blk < cgdmin(sblock, c)) {
		if ((blk + cnt) > cgsblock(sblock, c)) {
			if (debug) {
				printf("blk %lld < cgdmin %lld;",
				    (long long)blk,
				    (long long)cgdmin(sblock, c));
				printf(" blk + cnt %lld > cgsbase %lld\n",
				    (long long)(blk + cnt),
				    (long long)cgsblock(sblock, c));
			}
			return (1);
		}
	} else {
		if ((blk + cnt) > cgbase(sblock, c+1)) {
			if (debug)  {
				printf("blk %lld >= cgdmin %lld;",
				    (long long)blk,
				    (long long)cgdmin(sblock, c));
				printf(" blk + cnt %lld > sblock->fs_fpg %d\n",
				    (long long)(blk+cnt), sblock->fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * General purpose interface for reading inodes.
 */
union dinode *
ginode(inumber)
	ino_t inumber;
{
	daddr_t iblk;
	int blkoff;

	if (inumber < ROOTINO || inumber > maxino)
		errx(EEXIT, "bad inode number %d to ginode", inumber);
	if (startinum == 0 ||
	    inumber < startinum || inumber >= startinum + INOPB(sblock)) {
		iblk = ino_to_fsba(sblock, inumber);
		if (pbp != 0)
			pbp->b_flags &= ~B_INUSE;
		pbp = getdatablk(iblk, sblock->fs_bsize);
		startinum = (inumber / INOPB(sblock)) * INOPB(sblock);
	}
	if (is_ufs2) {
		blkoff = (inumber % INOPB(sblock)) * DINODE2_SIZE;
		return ((union dinode *)((caddr_t)pbp->b_un.b_buf + blkoff));
	}
	blkoff = (inumber % INOPB(sblock)) * DINODE1_SIZE;
	return ((union dinode *)((caddr_t)pbp->b_un.b_buf + blkoff));
}

static void
swap_dinode1(union dinode *dp, int n)
{
	int i, j;
	struct ufs1_dinode *dp1;

	dp1 = (struct ufs1_dinode *)&dp->dp1;
	for (i = 0; i < n; i++, dp1++) {
		ffs_dinode1_swap(dp1, dp1);
		if ((iswap16(dp1->di_mode) & IFMT) != IFLNK ||
			(isappleufs && (iswap64(dp1->di_size) >
			  APPLEUFS_MAXSYMLINKLEN)) ||
			iswap64(dp1->di_size) > sblock->fs_maxsymlinklen) {
			for (j = 0; j < (NDADDR + NIADDR); j++)
			    dp1->di_db[j] = bswap32(dp1->di_db[j]);
		}
	}
}

static void
swap_dinode2(union dinode *dp, int n)
{
	int i, j;
	struct ufs2_dinode *dp2;

	dp2 = (struct ufs2_dinode *)&dp->dp2;
	for (i = 0; i < n; i++, dp2++) {
		ffs_dinode2_swap(dp2, dp2);
		if ((iswap16(dp2->di_mode) & IFMT) != IFLNK) {
			for (j = 0; j < (NDADDR + NIADDR + NXADDR); j++)
				dp2->di_extb[j] = bswap64(dp2->di_extb[j]);
		}
	}
}

/*
 * Special purpose version of ginode used to optimize first pass
 * over all the inodes in numerical order.
 */
ino_t nextino, lastinum, lastvalidinum;
long readcnt, readpercg, fullcnt, inobufsize, partialcnt, partialsize;
union dinode *inodebuf;

union dinode *
getnextinode(inumber)
	ino_t inumber;
{
	long size;
	daddr_t dblk;
	static union dinode *dp;
	union dinode *ret;

	if (inumber != nextino++ || inumber > lastvalidinum) {
		abort();
		errx(EEXIT, "bad inode number %d to nextinode", inumber);
	}

	if (inumber >= lastinum) {
		readcnt++;
		dblk = fsbtodb(sblock, ino_to_fsba(sblock, lastinum));
		if (readcnt % readpercg == 0) {
			size = partialsize;
			lastinum += partialcnt;
		} else {
			size = inobufsize;
			lastinum += fullcnt;
		}
		(void)bread(fsreadfd, (caddr_t)inodebuf, dblk, size);
		if (doswap) {
			if (is_ufs2)
				swap_dinode2(inodebuf, lastinum - inumber);
			else
				swap_dinode1(inodebuf, lastinum - inumber);
			bwrite(fswritefd, (char *)inodebuf, dblk, size);
		}
		dp = (union dinode *)inodebuf;
	}
	ret = dp;
	dp = (union dinode *)
	    ((char *)dp + (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE));
	return ret;
}

void
setinodebuf(inum)
	ino_t inum;
{

	if (inum % sblock->fs_ipg != 0)
		errx(EEXIT, "bad inode number %d to setinodebuf", inum);

	lastvalidinum = inum + sblock->fs_ipg - 1;
	startinum = 0;
	nextino = inum;
	lastinum = inum;
	readcnt = 0;
	if (inodebuf != NULL)
		return;
	inobufsize = blkroundup(sblock, INOBUFSIZE);
	fullcnt = inobufsize / (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE);
	readpercg = sblock->fs_ipg / fullcnt;
	partialcnt = sblock->fs_ipg % fullcnt;
	partialsize = partialcnt * (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE);
	if (partialcnt != 0) {
		readpercg++;
	} else {
		partialcnt = fullcnt;
		partialsize = inobufsize;
	}
	if (inodebuf == NULL &&
	    (inodebuf = malloc((unsigned)inobufsize)) == NULL)
		errx(EEXIT, "Cannot allocate space for inode buffer");
}

void
freeinodebuf()
{

	if (inodebuf != NULL)
		free((char *)inodebuf);
	inodebuf = NULL;
}

/*
 * Routines to maintain information about directory inodes.
 * This is built during the first pass and used during the
 * second and third passes.
 *
 * Enter inodes into the cache.
 */
void
cacheino(dp, inumber)
	union dinode *dp;
	ino_t inumber;
{
	struct inoinfo *inp;
	struct inoinfo **inpp, **ninpsort;
	unsigned int blks;
	int i;
	int64_t size;

	size = iswap64(DIP(dp, size));
	blks = howmany(size, sblock->fs_bsize);
	if (blks > NDADDR)
		blks = NDADDR + NIADDR;

	inp = (struct inoinfo *) malloc(sizeof(*inp) + (blks - 1)
					    * sizeof (int64_t));
	if (inp == NULL)
		return;
	inpp = &inphead[inumber % dirhash];
	inp->i_nexthash = *inpp;
	*inpp = inp;
	inp->i_child = inp->i_sibling = inp->i_parentp = 0;
	if (inumber == ROOTINO)
		inp->i_parent = ROOTINO;
	else
		inp->i_parent = (ino_t)0;
	inp->i_dotdot = (ino_t)0;
	inp->i_number = inumber;
	inp->i_isize = size;
	inp->i_numblks = blks;
	for (i = 0; i < (blks < NDADDR ? blks : NDADDR); i++)
		inp->i_blks[i] = DIP(dp, db[i]);
	if (blks > NDADDR)
		for (i = 0; i < NIADDR; i++)
			inp->i_blks[NDADDR + i] = DIP(dp, ib[i]);
	if (inplast == listmax) {
		ninpsort = (struct inoinfo **)realloc((char *)inpsort,
		    (unsigned)(listmax + 100) * sizeof(struct inoinfo *));
		if (inpsort == NULL)
			errx(EEXIT, "cannot increase directory list");
		inpsort = ninpsort;
		listmax += 100;
	}
	inpsort[inplast++] = inp;
}

/*
 * Look up an inode cache structure.
 */
struct inoinfo *
getinoinfo(inumber)
	ino_t inumber;
{
	struct inoinfo *inp;

	for (inp = inphead[inumber % dirhash]; inp; inp = inp->i_nexthash) {
		if (inp->i_number != inumber)
			continue;
		return (inp);
	}
	errx(EEXIT, "cannot find inode %d", inumber);
	return ((struct inoinfo *)0);
}

/*
 * Clean up all the inode cache structure.
 */
void
inocleanup()
{
	struct inoinfo **inpp;

	if (inphead == NULL)
		return;
	for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--)
		free((char *)(*inpp));
	free((char *)inphead);
	free((char *)inpsort);
	inphead = inpsort = NULL;
}
	
void
inodirty()
{
	
	dirty(pbp);
}

void
clri(idesc, type, flag)
	struct inodesc *idesc;
	char *type;
	int flag;
{
	union dinode *dp;

	dp = ginode(idesc->id_number);
	if (flag == 1) {
		pwarn("%s %s", type,
		    (iswap16(DIP(dp, mode)) & IFMT) == IFDIR ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if (preen || reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
		n_files--;
		(void)ckinode(dp, idesc);
		clearinode(dp);
		inoinfo(idesc->id_number)->ino_state = USTATE;
		inodirty();
	} else
		markclean=  0;
}

int
findname(idesc)
	struct inodesc *idesc;
{
	struct direct *dirp = idesc->id_dirp;

	if (iswap32(dirp->d_ino) != idesc->id_parent || idesc->id_entryno < 2) {
		idesc->id_entryno++;
		return (KEEPON);
	}
	memmove(idesc->id_name, dirp->d_name, (size_t)dirp->d_namlen + 1);
	return (STOP|FOUND);
}

int
findino(idesc)
	struct inodesc *idesc;
{
	struct direct *dirp = idesc->id_dirp;

	if (dirp->d_ino == 0)
		return (KEEPON);
	if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
	    iswap32(dirp->d_ino) >= ROOTINO && iswap32(dirp->d_ino) <= maxino) {
		idesc->id_parent = iswap32(dirp->d_ino);
		return (STOP|FOUND);
	}
	return (KEEPON);
}

int
clearentry(idesc)
      struct inodesc *idesc;
{
	struct direct *dirp = idesc->id_dirp;

	if (dirp->d_ino != idesc->id_parent || idesc->id_entryno < 2) {
		idesc->id_entryno++;
		return (KEEPON);
	}
	dirp->d_ino = 0;
	return (STOP|FOUND|ALTERED);
}

void
pinode(ino)
	ino_t ino;
{
	union dinode *dp;
	char *p;
	struct passwd *pw;
	time_t t;

	printf(" I=%u ", ino);
	if (ino < ROOTINO || ino > maxino)
		return;
	dp = ginode(ino);
	printf(" OWNER=");
#ifndef SMALL
	if ((pw = getpwuid((int)iswap32(DIP(dp, uid)))) != 0)
		printf("%s ", pw->pw_name);
	else
#endif
		printf("%u ", (unsigned)iswap32(DIP(dp, uid)));
	printf("MODE=%o\n", iswap16(DIP(dp, mode)));
	if (preen)
		printf("%s: ", cdevname());
	printf("SIZE=%llu ", (unsigned long long)iswap64(DIP(dp, size)));
	t = iswap32(DIP(dp, mtime));
	p = ctime(&t);
	printf("MTIME=%12.12s %4.4s ", &p[4], &p[20]);
}

void
blkerror(ino, type, blk)
	ino_t ino;
	char *type;
	daddr_t blk;
{
	struct inostat *info;

	pfatal("%lld %s I=%u", (long long)blk, type, ino);
	printf("\n");
	info = inoinfo(ino);
	switch (info->ino_state) {

	case FSTATE:
		info->ino_state = FCLEAR;
		return;

	case DSTATE:
		info->ino_state = DCLEAR;
		return;

	case FCLEAR:
	case DCLEAR:
		return;

	default:
		errx(EEXIT, "BAD STATE %d TO BLKERR", info->ino_state);
		/* NOTREACHED */
	}
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
	ino_t request;
	int type;
{
	ino_t ino;
	union dinode *dp;
	struct ufs1_dinode *dp1;
	struct ufs2_dinode *dp2;
	time_t t;
	struct cg *cgp = cgrp;
	int cg;
	struct inostat *info;

	if (request == 0)
		request = ROOTINO;
	else if (inoinfo(request)->ino_state != USTATE)
		return (0);
	for (ino = request; ino < maxino; ino++) {
		info = inoinfo(ino);
		if (info->ino_state == USTATE)
			break;
	}
	if (ino == maxino)
		return (0);
	cg = ino_to_cg(sblock, ino);
	getblk(&cgblk, cgtod(sblock, cg), sblock->fs_cgsize);
	memcpy(cgp, cgblk.b_un.b_cg, sblock->fs_cgsize);
	if ((doswap && !needswap) || (!doswap && needswap))
		ffs_cg_swap(cgblk.b_un.b_cg, cgp, sblock);
	if (!cg_chkmagic(cgp, 0))
		pfatal("CG %d: ALLOCINO: BAD MAGIC NUMBER\n", cg);
	if (doswap)
		cgdirty();
	setbit(cg_inosused(cgp, 0), ino % sblock->fs_ipg);
	cgp->cg_cs.cs_nifree--;
	switch (type & IFMT) {
	case IFDIR:
		info->ino_state = DSTATE;
		cgp->cg_cs.cs_ndir++;
		break;
	case IFREG:
	case IFLNK:
		info->ino_state = FSTATE;
		break;
	default:
		return (0);
	}
	cgdirty();
	dp = ginode(ino);
	if (is_ufs2) {
		dp2 = &dp->dp2;
		dp2->di_db[0] = iswap64(allocblk(1));
		if (dp2->di_db[0] == 0) {
			info->ino_state = USTATE;
			return (0);
		}
		dp2->di_mode = iswap16(type);
		dp2->di_flags = 0;
		(void)time(&t);
		dp2->di_atime = iswap64(t);
		dp2->di_mtime = dp2->di_ctime = dp2->di_atime;
		dp2->di_size = iswap64(sblock->fs_fsize);
		dp2->di_blocks = iswap64(btodb(sblock->fs_fsize));
	} else {
		dp1 = &dp->dp1;
		dp1->di_db[0] = iswap32(allocblk(1));
		if (dp1->di_db[0] == 0) {
			info->ino_state = USTATE;
			return (0);
		}
		dp1->di_mode = iswap16(type);
		dp1->di_flags = 0;
		(void)time(&t);
		dp1->di_atime = iswap32(t);
		dp1->di_mtime = dp1->di_ctime = dp1->di_atime;
		dp1->di_size = iswap64(sblock->fs_fsize);
		dp1->di_blocks = iswap32(btodb(sblock->fs_fsize));
	}
	n_files++;
	inodirty();
	if (newinofmt)
		info->ino_type = IFTODT(type);
	return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino)
	ino_t ino;
{
	struct inodesc idesc;
	union dinode *dp;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	dp = ginode(ino);
	(void)ckinode(dp, &idesc);
	clearinode(dp);
	inodirty();
	inoinfo(ino)->ino_state = USTATE;
	n_files--;
}
