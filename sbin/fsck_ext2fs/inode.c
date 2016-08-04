/*	$NetBSD: inode.c,v 1.37 2016/08/04 17:43:47 jdolecek Exp $	*/

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
 * Copyright (c) 1997 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)inode.c	8.5 (Berkeley) 2/8/95";
#else
__RCSID("$NetBSD: inode.c,v 1.37 2016/08/04 17:43:47 jdolecek Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ext2fs/ext2fs_dinode.h>
#include <ufs/ext2fs/ext2fs_dir.h>
#include <ufs/ext2fs/ext2fs.h>

#include <ufs/ufs/dinode.h> /* for IFMT & friends */
#ifndef SMALL
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

/*
 * CG is stored in fs byte order in memory, so we can't use ino_to_fsba
 * here.
 */

#define fsck_ino_to_fsba(fs, x)                      \
	(fs2h32((fs)->e2fs_gd[ino_to_cg(fs, x)].ext2bgd_i_tables) + \
	(((x)-1) % (fs)->e2fs.e2fs_ipg)/(fs)->e2fs_ipb)


static ino_t startinum;

static int iblock(struct inodesc *, long, u_int64_t);

static int setlarge(void);

static int sethuge(void);

static int
setlarge(void)
{
	if (sblock.e2fs.e2fs_rev < E2FS_REV1) {
		pfatal("LARGE FILES UNSUPPORTED ON REVISION 0 FILESYSTEMS");
		return 0;
	}
	if (!(sblock.e2fs.e2fs_features_rocompat & EXT2F_ROCOMPAT_LARGEFILE)) {
		if (preen)
			pwarn("SETTING LARGE FILE INDICATOR\n");
		else if (!reply("SET LARGE FILE INDICATOR"))
			return 0;
		sblock.e2fs.e2fs_features_rocompat |= EXT2F_ROCOMPAT_LARGEFILE;
		sbdirty();
	}
	return 1;
}

static int
sethuge(void)
{
	if (sblock.e2fs.e2fs_rev < E2FS_REV1) {
		pfatal("HUGE FILES UNSUPPORTED ON REVISION 0 FILESYSTEMS");
		return 0;
	}
	if (!(sblock.e2fs.e2fs_features_rocompat & EXT2F_ROCOMPAT_HUGE_FILE)) {
		if (preen)
			pwarn("SETTING HUGE FILE FEATURE\n");
		else if (!reply("SET HUGE FILE FEATURE"))
			return 0;
		sblock.e2fs.e2fs_features_rocompat |= EXT2F_ROCOMPAT_HUGE_FILE;
		sbdirty();
	}
	return 1;
}

u_int64_t
inosize(struct ext2fs_dinode *dp)
{
	u_int64_t size = fs2h32(dp->e2di_size);

	if ((fs2h16(dp->e2di_mode) & IFMT) == IFREG)
		size |= (u_int64_t)fs2h32(dp->e2di_size_high) << 32;
	if (size > INT32_MAX)
		(void)setlarge();
	return size;
}

void
inossize(struct ext2fs_dinode *dp, u_int64_t size)
{
	if ((fs2h16(dp->e2di_mode) & IFMT) == IFREG) {
		dp->e2di_size_high = h2fs32(size >> 32);
		if (size > INT32_MAX)
			if (!setlarge())
				return;
	} else if (size > INT32_MAX) {
		pfatal("TRYING TO SET FILESIZE TO %llu ON MODE %x FILE\n",
		    (unsigned long long)size, fs2h16(dp->e2di_mode) & IFMT);
		return;
	}
	dp->e2di_size = h2fs32(size);
}

int
ckinode(struct ext2fs_dinode *dp, struct inodesc *idesc)
{
	u_int32_t *ap;
	long ret, n, ndb;
	struct ext2fs_dinode dino;
	u_int64_t remsize, sizepb;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = inosize(dp);
	mode = fs2h16(dp->e2di_mode) & IFMT;
	if (mode == IFBLK || mode == IFCHR || mode == IFIFO ||
	    (mode == IFLNK && (inosize(dp) < EXT2_MAXSYMLINKLEN)))
		return (KEEPON);
	dino = *dp;
	ndb = howmany(inosize(&dino), sblock.e2fs_bsize);
	for (ap = &dino.e2di_blocks[0]; ap < &dino.e2di_blocks[EXT2FS_NDADDR];
	    ap++,ndb--) {
		idesc->id_numfrags = 1;
		if (*ap == 0) {
			if (idesc->id_type == DATA && ndb > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					inossize(dp,
					    (ap - &dino.e2di_blocks[0]) *
					    sblock.e2fs_bsize);
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
				}
			}
			continue;
		}
		idesc->id_blkno = fs2h32(*ap);
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = 1;
	remsize = inosize(&dino) - sblock.e2fs_bsize * EXT2FS_NDADDR;
	sizepb = sblock.e2fs_bsize;
	for (ap = &dino.e2di_blocks[EXT2FS_NDADDR], n = 1; n <= EXT2FS_NIADDR; ap++, n++) {
		if (*ap) {
			idesc->id_blkno = fs2h32(*ap);
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
					dp = ginode(idesc->id_number);
					inossize(dp, inosize(dp) - remsize);
					remsize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
					break;
				}
			}
		}
		sizepb *= EXT2_NINDIR(&sblock);
		remsize -= sizepb;
	}
	return (KEEPON);
}

static int
iblock(struct inodesc *idesc, long ilevel, u_int64_t isize)
{
	/* XXX ondisk32 */
	int32_t *ap;
	int32_t *aplim;
	struct bufarea *bp;
	int i, n, (*func)(struct inodesc *);
	size_t nif;
	u_int64_t sizepb;
	char buf[BUFSIZ];
	char pathbuf[MAXPATHLEN + 1];
	struct ext2fs_dinode *dp;

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (chkrange(idesc->id_blkno, idesc->id_numfrags))
		return (SKIP);
	bp = getdatablk(idesc->id_blkno, sblock.e2fs_bsize);
	ilevel--;
	for (sizepb = sblock.e2fs_bsize, i = 0; i < ilevel; i++)
		sizepb *= EXT2_NINDIR(&sblock);
	if (isize > sizepb * EXT2_NINDIR(&sblock))
		nif = EXT2_NINDIR(&sblock);
	else
		nif = howmany(isize, sizepb);
	if (idesc->id_func == pass1check &&
		nif < EXT2_NINDIR(&sblock)) {
		aplim = &bp->b_un.b_indir[EXT2_NINDIR(&sblock)];
		for (ap = &bp->b_un.b_indir[nif]; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
			(void)snprintf(buf, sizeof(buf),
			    "PARTIALLY TRUNCATED INODE I=%llu",
			    (unsigned long long)idesc->id_number);
			if (dofix(idesc, buf)) {
				*ap = 0;
				dirty(bp);
			}
		}
		flush(fswritefd, bp);
	}
	aplim = &bp->b_un.b_indir[nif];
	for (ap = bp->b_un.b_indir; ap < aplim; ap++) {
		if (*ap) {
			idesc->id_blkno = fs2h32(*ap);
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
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					inossize(dp, inosize(dp) - isize);
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
chkrange(daddr_t blk, int cnt)
{
	int c, overh;

	if ((unsigned int)(blk + cnt) > maxfsblock)
		return (1);
	c = dtog(&sblock, blk);
	overh = cgoverhead(c);
	if (blk < sblock.e2fs.e2fs_bpg * c + overh +
	    sblock.e2fs.e2fs_first_dblock) {
		if ((blk + cnt) > sblock.e2fs.e2fs_bpg * c + overh +
		    sblock.e2fs.e2fs_first_dblock) {
			if (debug) {
				printf("blk %lld < cgdmin %d;",
				    (long long)blk,
				    sblock.e2fs.e2fs_bpg * c + overh +
				    sblock.e2fs.e2fs_first_dblock);
				printf(" blk + cnt %lld > cgsbase %d\n",
				    (long long)(blk + cnt),
				    sblock.e2fs.e2fs_bpg * c +
				    overh + sblock.e2fs.e2fs_first_dblock);
			}
			return (1);
		}
	} else {
		if ((blk + cnt) > sblock.e2fs.e2fs_bpg * (c + 1) + overh +
		    sblock.e2fs.e2fs_first_dblock) {
			if (debug)  {
				printf("blk %lld >= cgdmin %d;",
				    (long long)blk,
				    sblock.e2fs.e2fs_bpg * c + overh +
				    sblock.e2fs.e2fs_first_dblock);
				printf(" blk + cnt %lld > cgdmax %d\n",
				    (long long)(blk+cnt),
				    sblock.e2fs.e2fs_bpg * (c + 1) +
				    overh + sblock.e2fs.e2fs_first_dblock);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * General purpose interface for reading inodes.
 */
struct ext2fs_dinode *
ginode(ino_t inumber)
{
	daddr_t iblk;
	struct ext2fs_dinode *dp;

	if ((inumber < EXT2_FIRSTINO &&
	     inumber != EXT2_ROOTINO &&
	     !(inumber == EXT2_RESIZEINO &&
	       (sblock.e2fs.e2fs_features_compat & EXT2F_COMPAT_RESIZE) != 0))
		|| inumber > maxino)
		errexit("bad inode number %llu to ginode",
		    (unsigned long long)inumber);
	if (startinum == 0 ||
	    inumber < startinum || inumber >= startinum + sblock.e2fs_ipb) {
		iblk = fsck_ino_to_fsba(&sblock, inumber);
		if (pbp != 0)
			pbp->b_flags &= ~B_INUSE;
		pbp = getdatablk(iblk, sblock.e2fs_bsize);
		startinum =
		    ((inumber - 1) / sblock.e2fs_ipb) * sblock.e2fs_ipb + 1;
	}
	dp = (struct ext2fs_dinode *)(pbp->b_un.b_buf +
	    EXT2_DINODE_SIZE(&sblock) * ino_to_fsbo(&sblock, inumber));

	return dp;
}

/*
 * Special purpose version of ginode used to optimize first pass
 * over all the inodes in numerical order.
 */
ino_t nextino, lastinum;
long readcnt, readpercg, fullcnt, inobufsize, partialcnt, partialsize;
char *inodebuf;

struct ext2fs_dinode *
getnextinode(ino_t inumber)
{
	long size;
	daddr_t dblk;
	struct ext2fs_dinode *dp;
	static char *bp;

	if (inumber != nextino++ || inumber > maxino)
		errexit("bad inode number %llu to nextinode",
		    (unsigned long long)inumber);
	if (inumber >= lastinum) {
		readcnt++;
		dblk = EXT2_FSBTODB(&sblock, fsck_ino_to_fsba(&sblock, lastinum));
		if (readcnt % readpercg == 0) {
			size = partialsize;
			lastinum += partialcnt;
		} else {
			size = inobufsize;
			lastinum += fullcnt;
		}
		(void)bread(fsreadfd, inodebuf, dblk, size);
		bp = inodebuf;
	}
	dp = (struct ext2fs_dinode *)bp;
	bp += EXT2_DINODE_SIZE(&sblock);

	return dp;
}

void
resetinodebuf(void)
{

	startinum = 0;
	nextino = 1;
	lastinum = 1;
	readcnt = 0;
	inobufsize = ext2_blkroundup(&sblock, INOBUFSIZE);
	fullcnt = inobufsize / EXT2_DINODE_SIZE(&sblock);
	readpercg = sblock.e2fs.e2fs_ipg / fullcnt;
	partialcnt = sblock.e2fs.e2fs_ipg % fullcnt;
	partialsize = partialcnt * EXT2_DINODE_SIZE(&sblock);
	if (partialcnt != 0) {
		readpercg++;
	} else {
		partialcnt = fullcnt;
		partialsize = inobufsize;
	}
	if (inodebuf == NULL &&
	    (inodebuf = malloc((unsigned int)inobufsize)) == NULL)
		errexit("Cannot allocate space for inode buffer");
	while (nextino < EXT2_ROOTINO)
		(void)getnextinode(nextino);
}

void
freeinodebuf(void)
{

	if (inodebuf != NULL)
		free(inodebuf);
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
cacheino(struct ext2fs_dinode *dp, ino_t inumber)
{
	struct inoinfo *inp;
	struct inoinfo **inpp;
	unsigned int blks;

	blks = howmany(inosize(dp), sblock.e2fs_bsize);
	if (blks > EXT2FS_NDADDR)
		blks = EXT2FS_NDADDR + EXT2FS_NIADDR;
	/* XXX ondisk32 */
	inp = malloc(sizeof(*inp) + (blks - 1) * sizeof(int32_t));
	if (inp == NULL)
		return;
	inpp = &inphead[inumber % numdirs];
	inp->i_nexthash = *inpp;
	*inpp = inp;
	inp->i_child = inp->i_sibling = inp->i_parentp = 0;
	if (inumber == EXT2_ROOTINO)
		inp->i_parent = EXT2_ROOTINO;
	else
		inp->i_parent = (ino_t)0;
	inp->i_dotdot = (ino_t)0;
	inp->i_number = inumber;
	inp->i_isize = inosize(dp);
	/* XXX ondisk32 */
	inp->i_numblks = blks * sizeof(int32_t);
	memcpy(&inp->i_blks[0], &dp->e2di_blocks[0], (size_t)inp->i_numblks);
	if (inplast == listmax) {
		listmax += 100;
		inpsort = (struct inoinfo **)realloc((char *)inpsort,
		    (unsigned int)listmax * sizeof(struct inoinfo *));
		if (inpsort == NULL)
			errexit("cannot increase directory list");
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
	errexit("cannot find inode %llu", (unsigned long long)inumber);
	return ((struct inoinfo *)0);
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
		free(*inpp);
	free(inphead);
	free(inpsort);
	inphead = inpsort = NULL;
}
	
void
inodirty(void)
{
	
	dirty(pbp);
}

void
clri(struct inodesc *idesc, const char *type, int flag)
{
	struct ext2fs_dinode *dp;

	dp = ginode(idesc->id_number);
	if (flag == 1) {
		pwarn("%s %s", type,
		    (fs2h16(dp->e2di_mode) & IFMT) == IFDIR ? "DIR" : "FILE");
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
	}
}

int
findname(struct inodesc *idesc)
{
	struct ext2fs_direct *dirp = idesc->id_dirp;
	u_int16_t namlen = dirp->e2d_namlen;
	/* from utilities.c namebuf[] variable */
	char *buf = __UNCONST(idesc->id_name);
	if (namlen > MAXPATHLEN) {
		/* XXX: Prevent overflow but don't fix */
		namlen = MAXPATHLEN;
	}

	if (fs2h32(dirp->e2d_ino) != idesc->id_parent)
		return (KEEPON);
	(void)memcpy(buf, dirp->e2d_name, (size_t)namlen);
	buf[namlen] = '\0';
	return (STOP|FOUND);
}

int
findino(struct inodesc *idesc)
{
	struct ext2fs_direct *dirp = idesc->id_dirp;
	u_int32_t ino = fs2h32(dirp->e2d_ino);

	if (ino == 0)
		return (KEEPON);
	if (strcmp(dirp->e2d_name, idesc->id_name) == 0 &&
	    (ino == EXT2_ROOTINO || ino >= EXT2_FIRSTINO) 
		&& ino <= maxino) {
		idesc->id_parent = ino;
		return (STOP|FOUND);
	}
	return (KEEPON);
}

void
pinode(ino_t ino)
{
	struct ext2fs_dinode *dp;
	struct passwd *pw;
	uid_t uid;

	printf(" I=%llu ", (unsigned long long)ino);
	if ((ino < EXT2_FIRSTINO && ino != EXT2_ROOTINO) || ino > maxino)
		return;
	dp = ginode(ino);
	uid = fs2h16(dp->e2di_uid);
	if (sblock.e2fs.e2fs_rev > E2FS_REV0)
		uid |= fs2h16(dp->e2di_uid_high) << 16;
	printf(" OWNER=");
#ifndef SMALL
	if (Uflag && (pw = getpwuid(uid)) != 0)
		printf("%s ", pw->pw_name);
	else
#endif
		printf("%u ", (unsigned int)uid);
	printf("MODE=%o\n", fs2h16(dp->e2di_mode));
	if (preen)
		printf("%s: ", cdevname());
	printf("SIZE=%llu ", (long long)inosize(dp));
	printf("MTIME=%s ", print_mtime(fs2h32(dp->e2di_mtime)));
}

void
blkerror(ino_t ino, const char *type, daddr_t blk)
{

	pfatal("%lld %s I=%llu", (long long)blk, type, (unsigned long long)ino);
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
		errexit("BAD STATE %d TO BLKERR", statemap[ino]);
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
	struct ext2fs_dinode *dp;
	time_t t;

	if (request == 0)
		request = EXT2_ROOTINO;
	else if (statemap[request] != USTATE)
		return (0);
	for (ino = request; ino < maxino; ino++) {
		if ((ino > EXT2_ROOTINO) && (ino < EXT2_FIRSTINO))
			continue;
		if (statemap[ino] == USTATE)
			break;
	}
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
	dp->e2di_blocks[0] = h2fs32(allocblk());
	if (dp->e2di_blocks[0] == 0) {
		statemap[ino] = USTATE;
		return (0);
	}
	dp->e2di_mode = h2fs16(type);
	(void)time(&t);
	dp->e2di_atime = h2fs32(t);
	dp->e2di_mtime = dp->e2di_ctime = dp->e2di_atime;
	dp->e2di_dtime = 0;
	inossize(dp, sblock.e2fs_bsize);
	inosnblock(dp, btodb(sblock.e2fs_bsize));
	n_files++;
	inodirty();
	typemap[ino] = E2IFTODT(type);
	return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino_t ino)
{
	struct inodesc idesc;
	struct ext2fs_dinode *dp;

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

uint64_t
inonblock(struct ext2fs_dinode *dp)
{
	uint64_t nblock;

	/* XXX check for EXT2_HUGE_FILE without EXT2F_ROCOMPAT_HUGE_FILE? */

	nblock = fs2h32(dp->e2di_nblock);

	if ((sblock.e2fs.e2fs_features_rocompat & EXT2F_ROCOMPAT_HUGE_FILE)) {
		nblock |= (uint64_t)fs2h16(dp->e2di_nblock_high) << 32;
		if (fs2h32(dp->e2di_flags) & EXT2_HUGE_FILE) {
			nblock = EXT2_FSBTODB(&sblock, nblock);
		}
	}

	return nblock;
}

void
inosnblock(struct ext2fs_dinode *dp, uint64_t nblock)
{
	uint32_t flags;

	flags = fs2h32(dp->e2di_flags);

	if (nblock <= 0xffffffffULL) {
		flags &= ~EXT2_HUGE_FILE;
		dp->e2di_flags = h2fs32(flags);
		dp->e2di_nblock = h2fs32(nblock);
		return;
	}

	sethuge();

	if (nblock <= 0xffffffffffffULL) {
		flags &= ~EXT2_HUGE_FILE;
		dp->e2di_flags = h2fs32(flags);
		dp->e2di_nblock = h2fs32(nblock);
		dp->e2di_nblock_high = h2fs16((nblock >> 32));
		return;
	}

	if (EXT2_DBTOFSB(&sblock, nblock) <= 0xffffffffffffULL) {
		flags |= EXT2_HUGE_FILE;
		dp->e2di_flags = h2fs32(flags);
		dp->e2di_nblock = h2fs32(EXT2_DBTOFSB(&sblock, nblock));
		dp->e2di_nblock_high = h2fs16((EXT2_DBTOFSB(&sblock, nblock) >> 32));
		return;
	}

	pfatal("trying to set nblocks higher than representable");

	return;
}
