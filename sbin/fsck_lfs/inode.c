/* $NetBSD: inode.c,v 1.21 2003/09/19 08:31:58 itojun Exp $	 */

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

/*
 * Copyright (c) 1997, 1998
 *      Konrad Schroder.  All rights reserved.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <err.h>
#ifndef SMALL
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

extern SEGUSE *seg_table;
extern ufs_daddr_t *din_table;

static int iblock(struct inodesc *, long, u_int64_t);
int blksreqd(struct lfs *, int);
int lfs_maxino(void);

/*
 * Get a dinode of a given inum.
 * XXX combine this function with vget.
 */
struct ufs1_dinode *
ginode(ino_t ino)
{
	struct uvnode *vp;
	struct ubuf *bp;
	IFILE *ifp;

	vp = vget(fs, ino);
	if (vp == NULL)
		return NULL;

	if (din_table[ino] == 0x0) {
		LFS_IENTRY(ifp, fs, ino, bp);
		din_table[ino] = ifp->if_daddr;
		seg_table[dtosn(fs, ifp->if_daddr)].su_nbytes += DINODE1_SIZE;
		brelse(bp);
	}
	return (VTOI(vp)->i_din.ffs1_din);
}

/*
 * Check validity of held blocks in an inode, recursing through all blocks.
 */
int
ckinode(struct ufs1_dinode *dp, struct inodesc *idesc)
{
	ufs_daddr_t *ap, lbn;
	long ret, n, ndb, offset;
	struct ufs1_dinode dino;
	u_int64_t remsize, sizepb;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp, *thisvp;

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = dp->di_size;
	mode = dp->di_mode & IFMT;
	if (mode == IFBLK || mode == IFCHR ||
	    (mode == IFLNK && (dp->di_size < fs->lfs_maxsymlinklen ||
		    (fs->lfs_maxsymlinklen == 0 &&
			dp->di_blocks == 0))))
		return (KEEPON);
	dino = *dp;
	ndb = howmany(dino.di_size, fs->lfs_bsize);

	thisvp = vget(fs, idesc->id_number);
	for (lbn = 0; lbn < NDADDR; lbn++) {
		ap = dino.di_db + lbn;
		if (thisvp)
			idesc->id_numfrags =
				numfrags(fs, VTOI(thisvp)->i_lfs_fragsize[lbn]);
		else {
			if (--ndb == 0 && (offset = blkoff(fs, dino.di_size)) != 0) {
				idesc->id_numfrags =
			    	numfrags(fs, fragroundup(fs, offset));
			} else
				idesc->id_numfrags = fs->lfs_frag;
		}
		if (*ap == 0) {
			if (idesc->id_type == DATA && ndb >= 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					dp = VTOD(vp);
					dp->di_size = (ap - &dino.di_db[0]) *
					    fs->lfs_bsize;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
				}
			}
			continue;
		}
		idesc->id_blkno = *ap;
		idesc->id_lblkno = ap - &dino.di_db[0];
		if (idesc->id_type == ADDR) {
			ret = (*idesc->id_func) (idesc);
		} else
			ret = dirscan(idesc);
		idesc->id_lblkno = 0;
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = fs->lfs_frag;
	remsize = dino.di_size - fs->lfs_bsize * NDADDR;
	sizepb = fs->lfs_bsize;
	for (ap = &dino.di_ib[0], n = 1; n <= NIADDR; ap++, n++) {
		if (*ap) {
			idesc->id_blkno = *ap;
			ret = iblock(idesc, n, remsize);
			if (ret & STOP)
				return (ret);
		} else {
			if (idesc->id_type == DATA && remsize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					dp = VTOD(vp);
					dp->di_size -= remsize;
					remsize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
					break;
				}
			}
		}
		sizepb *= NINDIR(fs);
		remsize -= sizepb;
	}
	return (KEEPON);
}

static int
iblock(struct inodesc *idesc, long ilevel, u_int64_t isize)
{
	ufs_daddr_t *ap, *aplim;
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
	if (chkrange(idesc->id_blkno, fragstofsb(fs, idesc->id_numfrags)))
		return (SKIP);

	devvp = fs->lfs_unlockvp;
	bread(devvp, fsbtodb(fs, idesc->id_blkno), fs->lfs_bsize, NOCRED, &bp);
	ilevel--;
	for (sizepb = fs->lfs_bsize, i = 0; i < ilevel; i++)
		sizepb *= NINDIR(fs);
	if (isize > sizepb * NINDIR(fs))
		nif = NINDIR(fs);
	else
		nif = howmany(isize, sizepb);
	if (idesc->id_func == pass1check && nif < NINDIR(fs)) {
		aplim = ((ufs_daddr_t *) bp->b_data) + NINDIR(fs);
		for (ap = ((ufs_daddr_t *) bp->b_data) + nif; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
			(void) sprintf(buf, "PARTIALLY TRUNCATED INODE I=%u",
			    idesc->id_number);
			if (dofix(idesc, buf)) {
				*ap = 0;
				++diddirty;
			}
		}
	}
	aplim = ((ufs_daddr_t *) bp->b_data) + nif;
	for (ap = ((ufs_daddr_t *) bp->b_data); ap < aplim; ap++) {
		if (*ap) {
			idesc->id_blkno = *ap;
			if (ilevel == 0)
				n = (*func) (idesc);
			else
				n = iblock(idesc, ilevel, isize);
			if (n & STOP) {
				if (diddirty)
					VOP_BWRITE(bp);
				else
					brelse(bp);
				return (n);
			}
		} else {
			if (idesc->id_type == DATA && isize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					vp = vget(fs, idesc->id_number);
					VTOI(vp)->i_ffs1_size -= isize;
					isize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty(VTOI(vp));
					if (diddirty)
						VOP_BWRITE(bp);
					else
						brelse(bp);
					return (STOP);
				}
			}
		}
		isize -= sizepb;
	}
	if (diddirty)
		VOP_BWRITE(bp);
	else
		brelse(bp);
	return (KEEPON);
}
/*
 * Check that a block in a legal block number.
 * Return 0 if in range, 1 if out of range.
 */
int
chkrange(daddr_t blk, int cnt)
{
	if (blk < sntod(fs, 0)) {
		return (1);
	}
	if (blk > maxfsblock) {
		return (1);
	}
	if (blk + cnt < sntod(fs, 0)) {
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
cacheino(struct ufs1_dinode * dp, ino_t inumber)
{
	struct inoinfo *inp;
	struct inoinfo **inpp, **ninpsort;
	unsigned int blks;

	blks = howmany(dp->di_size, fs->lfs_bsize);
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
		inp->i_parent = (ino_t) 0;
	inp->i_dotdot = (ino_t) 0;
	inp->i_number = inumber;
	inp->i_isize = dp->di_size;

	inp->i_numblks = blks * sizeof(ufs_daddr_t);
	memcpy(&inp->i_blks[0], &dp->di_db[0], (size_t) inp->i_numblks);
	if (inplast == listmax) {
		ninpsort = (struct inoinfo **) realloc((char *) inpsort,
		    (unsigned) (listmax + 100) * sizeof(struct inoinfo *));
		if (ninpsort == NULL)
			err(8, "cannot increase directory list\n");
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
	register struct inoinfo *inp;

	for (inp = inphead[inumber % numdirs]; inp; inp = inp->i_nexthash) {
		if (inp->i_number != inumber)
			continue;
		return (inp);
	}
	err(8, "cannot find inode %d\n", inumber);
	return ((struct inoinfo *) 0);
}

/*
 * Clean up all the inode cache structure.
 */
void
inocleanup()
{
	register struct inoinfo **inpp;

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
	ip->i_flag |= IN_MODIFIED;
}

void
clri(struct inodesc * idesc, char *type, int flag)
{
	struct ubuf *bp;
	IFILE *ifp;
	struct uvnode *vp;

	vp = vget(fs, idesc->id_number);
	if (flag == 1) {
		pwarn("%s %s", type,
		      (VTOI(vp)->i_ffs1_mode & IFMT) == IFDIR ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if (flag == 2 || preen || reply("CLEAR") == 1) {
		if (preen && flag != 2)
			printf(" (CLEARED)\n");
		n_files--;
		(void) ckinode(VTOD(vp), idesc);
		clearinode(VTOD(vp));
		statemap[idesc->id_number] = USTATE;
		inodirty(VTOI(vp));

		/* Send cleared inode to the free list */

		LFS_IENTRY(ifp, fs, idesc->id_number, bp);
		ifp->if_daddr = LFS_UNUSED_DADDR;
		ifp->if_nextfree = fs->lfs_freehd;
		fs->lfs_freehd = idesc->id_number;
		sbdirty();
		VOP_BWRITE(bp);
	}
}

int
findname(struct inodesc * idesc)
{
	register struct direct *dirp = idesc->id_dirp;

	if (dirp->d_ino != idesc->id_parent)
		return (KEEPON);
	memcpy(idesc->id_name, dirp->d_name, (size_t) dirp->d_namlen + 1);
	return (STOP | FOUND);
}

int
findino(struct inodesc * idesc)
{
	register struct direct *dirp = idesc->id_dirp;

	if (dirp->d_ino == 0)
		return (KEEPON);
	if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
	    dirp->d_ino >= ROOTINO && dirp->d_ino < maxino) {
		idesc->id_parent = dirp->d_ino;
		return (STOP | FOUND);
	}
	return (KEEPON);
}

void
pinode(ino_t ino)
{
	register struct ufs1_dinode *dp;
	register char *p;
	struct passwd *pw;
	time_t t;

	printf(" I=%u ", ino);
	if (ino < ROOTINO || ino >= maxino)
		return;
	dp = ginode(ino);
	if (dp) {
		printf(" OWNER=");
#ifndef SMALL
		if ((pw = getpwuid((int) dp->di_uid)) != 0)
			printf("%s ", pw->pw_name);
		else
#endif
			printf("%u ", (unsigned) dp->di_uid);
		printf("MODE=%o\n", dp->di_mode);
		if (preen)
			printf("%s: ", cdevname());
		printf("SIZE=%llu ", (unsigned long long) dp->di_size);
		t = dp->di_mtime;
		p = ctime(&t);
		printf("MTIME=%12.12s %4.4s ", &p[4], &p[20]);
	}
}

void
blkerror(ino_t ino, char *type, daddr_t blk)
{

	pfatal("%lld %s I=%u", (long long) blk, type, ino);
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
		err(8, "BAD STATE %d TO BLKERR\n", statemap[ino]);
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
	struct ufs1_dinode *dp;
	time_t t;
	struct uvnode *vp;
	struct ubuf *bp;

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
	vp = vget(fs, ino);
	dp = (VTOI(vp)->i_din.ffs1_din);
	bp = getblk(vp, 0, fs->lfs_fsize);
	VOP_BWRITE(bp);
	dp->di_mode = type;
	(void) time(&t);
	dp->di_atime = t;
	dp->di_mtime = dp->di_ctime = dp->di_atime;
	dp->di_size = fs->lfs_fsize;
	dp->di_blocks = btofsb(fs, fs->lfs_fsize);
	n_files++;
	inodirty(VTOI(vp));
	typemap[ino] = IFTODT(type);
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
	clearinode(VTOI(vp)->i_din.ffs1_din);
	inodirty(VTOI(vp));
	statemap[ino] = USTATE;

	n_files--;
}
