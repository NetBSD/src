/*	$NetBSD: inode.c,v 1.30 1998/10/23 01:13:33 thorpej Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__RCSID("$NetBSD: inode.c,v 1.30 1998/10/23 01:13:33 thorpej Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

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

int
ckinode(dp, idesc)
	struct dinode *dp;
	struct inodesc *idesc;
{
	ufs_daddr_t *ap;
	long ret, n, ndb, offset;
	struct dinode dino;
	u_int64_t sizepb;
	int64_t remsize;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = iswap64(dp->di_size);
	mode = iswap16(dp->di_mode) & IFMT;
	if (mode == IFBLK || mode == IFCHR || (mode == IFLNK &&
	    (idesc->id_filesize < sblock->fs_maxsymlinklen ||
	     (sblock->fs_maxsymlinklen == 0 && dp->di_blocks == 0))))
		return (KEEPON);
	dino = *dp;
	ndb = howmany(iswap64(dino.di_size), sblock->fs_bsize);
	for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++) {
		if (--ndb == 0 &&
			(offset = blkoff(sblock, iswap64(dino.di_size))) != 0)
			idesc->id_numfrags =
				numfrags(sblock, fragroundup(sblock, offset));
		else
			idesc->id_numfrags = sblock->fs_frag;
		if (*ap == 0) {
			if (idesc->id_type == DATA && ndb >= 0) {
				/* An empty block in a directory XXX */
				markclean = 0;
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->di_size = iswap64((ap - &dino.di_db[0]) *
					    sblock->fs_bsize);
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
				}
			}
			continue;
		}
		idesc->id_blkno = iswap32(*ap);
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = sblock->fs_frag;
	remsize = iswap64(dino.di_size) - sblock->fs_bsize * NDADDR;
	sizepb = sblock->fs_bsize;
	for (ap = &dino.di_ib[0], n = 1; n <= NIADDR; ap++, n++) {
		if (*ap) {
			idesc->id_blkno = iswap32(*ap);
			ret = iblock(idesc, n, remsize);
			if (ret & STOP)
				return (ret);
		} else {
			if (idesc->id_type == DATA && remsize > 0) {
				/* An empty block in a directory XXX */
				markclean = 0;
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->di_size = iswap64(iswap64(dp->di_size) - remsize);
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
	ufs_daddr_t *ap;
	ufs_daddr_t *aplim;
	struct bufarea *bp;
	int i, n, (*func) __P((struct inodesc *)), nif;
	u_int64_t sizepb;
	char buf[BUFSIZ];
	char pathbuf[MAXPATHLEN + 1];
	struct dinode *dp;

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
	if (isize > sizepb * NINDIR(sblock))
		nif = NINDIR(sblock);
	else
		nif = howmany(isize, sizepb);
	if (do_blkswap) { /* swap byte order of the whole blk */
		aplim = &bp->b_un.b_indir[nif];
		for (ap = bp->b_un.b_indir; ap < aplim; ap++)
			*ap = bswap32(*ap);
		dirty(bp);
		flush(fswritefd, bp);
	}
	if (idesc->id_func == pass1check && nif < NINDIR(sblock)) {
		aplim = &bp->b_un.b_indir[NINDIR(sblock)];
		for (ap = &bp->b_un.b_indir[nif]; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
			(void)snprintf(buf, sizeof(buf),
			    "PARTIALLY TRUNCATED INODE I=%u", idesc->id_number);
			if (dofix(idesc, buf)) {
				*ap = 0;
				dirty(bp);
			} else
				markclean=  0;
		}
		flush(fswritefd, bp);
	}
	aplim = &bp->b_un.b_indir[nif];
	for (ap = bp->b_un.b_indir; ap < aplim; ap++) {
		if (*ap) {
			idesc->id_blkno = iswap32(*ap);
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
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->di_size = iswap64(iswap64(dp->di_size) - isize);
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
	ufs_daddr_t blk;
	int cnt;
{
	int c;

	if ((unsigned)(blk + cnt) > maxfsblock)
		return (1);
	c = dtog(sblock, blk);
	if (blk < cgdmin(sblock, c)) {
		if ((blk + cnt) > cgsblock(sblock, c)) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, cgdmin(sblock, c));
				printf(" blk + cnt %d > cgsbase %d\n",
				    blk + cnt, cgsblock(sblock, c));
			}
			return (1);
		}
	} else {
		if ((blk + cnt) > cgbase(sblock, c+1)) {
			if (debug)  {
				printf("blk %d >= cgdmin %d;",
				    blk, cgdmin(sblock, c));
				printf(" blk + cnt %d > sblock->fs_fpg %d\n",
				    blk+cnt, sblock->fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * General purpose interface for reading inodes.
 */
struct dinode *
ginode(inumber)
	ino_t inumber;
{
	ufs_daddr_t iblk;
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
	blkoff = (inumber % INOPB(sblock)) * DINODE_SIZE;
	return ((struct dinode *)((caddr_t)pbp->b_un.b_buf + blkoff));
}

/*
 * Special purpose version of ginode used to optimize first pass
 * over all the inodes in numerical order.
 */
ino_t nextino, lastinum;
long readcnt, readpercg, fullcnt, inobufsize, partialcnt, partialsize;
struct dinode *inodebuf;

struct dinode *
getnextinode(inumber)
	ino_t inumber;
{
	long size;
	ufs_daddr_t dblk;
	static struct dinode *dp;

	if (inumber != nextino++ || inumber > maxino)
		errx(EEXIT, "bad inode number %d to nextinode", inumber);
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
		(void)bread(fsreadfd, (char *)inodebuf, dblk, size); /* ??? */
		if (doswap) {
			int i, j;
			for (i = inumber, dp  = inodebuf; i < lastinum; i++, dp++) {
				ffs_dinode_swap(dp, dp);
				/* ffs_dinode_swap() doesn't swap blocks addrs */
				if ((iswap16(dp->di_mode) & IFMT) != IFLNK ||
					iswap64(dp->di_size) > sblock->fs_maxsymlinklen) {
					for (j=0; j<NDADDR + NIADDR; j++)
						dp->di_db[j] = bswap32(dp->di_db[j]);
				}
			}
			bwrite(fswritefd, (char *)inodebuf, dblk, size);
		}
		dp = inodebuf;
	}
	return (dp++);
}

void
resetinodebuf()
{

	startinum = 0;
	nextino = 0;
	lastinum = 0;
	readcnt = 0;
	inobufsize = blkroundup(sblock, INOBUFSIZE);
	fullcnt = inobufsize / DINODE_SIZE;
	readpercg = sblock->fs_ipg / fullcnt;
	partialcnt = sblock->fs_ipg % fullcnt;
	partialsize = partialcnt * DINODE_SIZE;
	if (partialcnt != 0) {
		readpercg++;
	} else {
		partialcnt = fullcnt;
		partialsize = inobufsize;
	}
	if (inodebuf == NULL &&
	    (inodebuf = (struct dinode *)malloc((unsigned)inobufsize)) == NULL)
		errx(EEXIT, "Cannot allocate space for inode buffer");
	while (nextino < ROOTINO)
		(void)getnextinode(nextino);
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
	struct dinode *dp;
	ino_t inumber;
{
	struct inoinfo *inp;
	struct inoinfo **inpp;
	unsigned int blks;

	blks = howmany(iswap64(dp->di_size), sblock->fs_bsize);
	if (blks > NDADDR)
		blks = NDADDR + NIADDR;
	inp = (struct inoinfo *)
		malloc(sizeof(*inp) + (blks - 1) * sizeof(ufs_daddr_t));
	if (inp == NULL)
		return;
	inpp = &inphead[inumber % numdirs];
	inp->i_nexthash = *inpp;
	*inpp = inp;
	inp->i_child = inp->i_sibling = inp->i_parentp = 0;
	if (inumber == ROOTINO)
		inp->i_parent = ROOTINO;
	else
		inp->i_parent = (ino_t)0;
	inp->i_dotdot = (ino_t)0;
	inp->i_number = inumber;
	inp->i_isize = iswap64(dp->di_size);
	inp->i_numblks = blks * sizeof(ufs_daddr_t);
	memmove(&inp->i_blks[0], &dp->di_db[0], (size_t)inp->i_numblks);
	if (inplast == listmax) {
		listmax += 100;
		inpsort = (struct inoinfo **)realloc((char *)inpsort,
		    (unsigned)listmax * sizeof(struct inoinfo *));
		if (inpsort == NULL)
			errx(EEXIT, "cannot increase directory list");
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

	for (inp = inphead[inumber % numdirs]; inp; inp = inp->i_nexthash) {
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
	struct dinode *dp;

	dp = ginode(idesc->id_number);
	if (flag == 1) {
		pwarn("%s %s", type,
		    (iswap16(dp->di_mode) & IFMT) == IFDIR ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if (preen || reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
		n_files--;
		(void)ckinode(dp, idesc);
		clearinode(dp);
		statemap[idesc->id_number] = USTATE;
		inodirty();
	} else
		markclean=  0;
}

int
findname(idesc)
	struct inodesc *idesc;
{
	struct direct *dirp = idesc->id_dirp;

	if (iswap32(dirp->d_ino) != idesc->id_parent)
		return (KEEPON);
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

void
pinode(ino)
	ino_t ino;
{
	struct dinode *dp;
	char *p;
	struct passwd *pw;
	time_t t;

	printf(" I=%u ", ino);
	if (ino < ROOTINO || ino > maxino)
		return;
	dp = ginode(ino);
	printf(" OWNER=");
#ifndef SMALL
	if ((pw = getpwuid((int)iswap32(dp->di_uid))) != 0)
		printf("%s ", pw->pw_name);
	else
#endif
		printf("%u ", (unsigned)iswap32(dp->di_uid));
	printf("MODE=%o\n", iswap16(dp->di_mode));
	if (preen)
		printf("%s: ", cdevname());
	printf("SIZE=%qu ", (unsigned long long)iswap64(dp->di_size));
	t = iswap32(dp->di_mtime);
	p = ctime(&t);
	printf("MTIME=%12.12s %4.4s ", &p[4], &p[20]);
}

void
blkerror(ino, type, blk)
	ino_t ino;
	char *type;
	ufs_daddr_t blk;
{

	pfatal("%d %s I=%u", blk, type, ino);
	printf("\n");
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
		errx(EEXIT, "BAD STATE %d TO BLKERR", statemap[ino]);
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
	struct dinode *dp;
	time_t t;

	if (request == 0)
		request = ROOTINO;
	else if (statemap[request] != USTATE)
		return (0);
	for (ino = request; ino < maxino; ino++)
		if (statemap[ino] == USTATE)
			break;
	if (ino == maxino)
		return (0);
	switch (type & IFMT) {
	case IFDIR:
		statemap[ino] = DSTATE;
		break;
	case IFREG:
	case IFLNK:
		statemap[ino] = FSTATE;
		break;
	default:
		return (0);
	}
	dp = ginode(ino);
	dp->di_db[0] = iswap32(allocblk((long)1));
	if (dp->di_db[0] == 0) {
		statemap[ino] = USTATE;
		return (0);
	}
	dp->di_mode = iswap16(type);
	(void)time(&t);
	dp->di_atime = iswap32(t);
	dp->di_mtime = dp->di_ctime = dp->di_atime;
	dp->di_size = iswap64(sblock->fs_fsize);
	dp->di_blocks = iswap32(btodb(sblock->fs_fsize));
	n_files++;
	inodirty();
	if (newinofmt)
		typemap[ino] = IFTODT(type);
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
	struct dinode *dp;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	dp = ginode(ino);
	(void)ckinode(dp, &idesc);
	clearinode(dp);
	inodirty();
	statemap[ino] = USTATE;
	n_files--;
}
