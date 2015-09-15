/* $NetBSD: dir.c,v 1.40 2015/09/15 15:01:22 dholland Exp $	 */

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

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

const char *lfname = "lost+found";
int lfmode = 01700;
struct lfs_dirtemplate emptydir = {
	.dot_ino = 0,
	.dot_reclen = LFS_DIRBLKSIZ,
};
struct lfs_dirtemplate dirhead = {
	.dot_ino = 0,
	.dot_reclen = 12,
	.dot_type = LFS_DT_DIR,
	.dot_namlen = 1,
	.dot_name = ".",
	.dotdot_ino = 0,
	.dotdot_reclen = LFS_DIRBLKSIZ - 12,
	.dotdot_type = LFS_DT_DIR,
	.dotdot_namlen = 2,
	.dotdot_name = ".."
};
#if 0
struct lfs_odirtemplate odirhead = {
	.dot_ino = 0,
	.dot_reclen = 12,
	.dot_namlen = 1,
	.dot_name = ".",
	.dotdot_ino = 0,
	.dotdot_reclen = LFS_DIRBLKSIZ - 12,
	.dotdot_namlen = 2,
	.dotdot_name = ".."
};
#endif

static int expanddir(struct uvnode *, union lfs_dinode *, char *);
static void freedir(ino_t, ino_t);
static struct lfs_direct *fsck_readdir(struct uvnode *, struct inodesc *);
static int lftempname(char *, ino_t);
static int mkentry(struct inodesc *);
static int chgino(struct inodesc *);

/*
 * Propagate connected state through the tree.
 */
void
propagate(void)
{
	struct inoinfo **inpp, *inp, *pinp;
	struct inoinfo **inpend;

	/*
	 * Create a list of children for each directory.
	 */
	inpend = &inpsort[inplast];
	for (inpp = inpsort; inpp < inpend; inpp++) {
		inp = *inpp;
		if (inp->i_parent == 0 ||
		    inp->i_number == ULFS_ROOTINO)
			continue;
		pinp = getinoinfo(inp->i_parent);
		inp->i_parentp = pinp;
		inp->i_sibling = pinp->i_child;
		pinp->i_child = inp;
	}
	inp = getinoinfo(ULFS_ROOTINO);
	while (inp) {
		statemap[inp->i_number] = DFOUND;
		if (inp->i_child &&
		    statemap[inp->i_child->i_number] == DSTATE)
			inp = inp->i_child;
		else if (inp->i_sibling)
			inp = inp->i_sibling;
		else
			inp = inp->i_parentp;
	}
}

/*
 * Scan each entry in a directory block.
 */
int
dirscan(struct inodesc *idesc)
{
	struct lfs_direct *dp;
	struct ubuf *bp;
	int dsize, n;
	long blksiz;
	char dbuf[LFS_DIRBLKSIZ];
	struct uvnode *vp;

	if (idesc->id_type != DATA)
		errexit("wrong type to dirscan %d", idesc->id_type);
	if (idesc->id_entryno == 0 &&
	    (idesc->id_filesize & (LFS_DIRBLKSIZ - 1)) != 0)
		idesc->id_filesize = roundup(idesc->id_filesize, LFS_DIRBLKSIZ);
	blksiz = idesc->id_numfrags * lfs_sb_getfsize(fs);
	if (chkrange(idesc->id_blkno, idesc->id_numfrags)) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}
	idesc->id_loc = 0;

	vp = vget(fs, idesc->id_number);
	for (dp = fsck_readdir(vp, idesc); dp != NULL;
	    dp = fsck_readdir(vp, idesc)) {
		dsize = lfs_dir_getreclen(fs, dp);
		memcpy(dbuf, dp, (size_t) dsize);
		idesc->id_dirp = (struct lfs_direct *) dbuf;
		if ((n = (*idesc->id_func) (idesc)) & ALTERED) {
			bread(vp, idesc->id_lblkno, blksiz, 0, &bp);
			memcpy(bp->b_data + idesc->id_loc - dsize, dbuf,
			    (size_t) dsize);
			VOP_BWRITE(bp);
			sbdirty();
		}
		if (n & STOP)
			return (n);
	}
	return (idesc->id_filesize > 0 ? KEEPON : STOP);
}

/*
 * get next entry in a directory.
 */
static struct lfs_direct *
fsck_readdir(struct uvnode *vp, struct inodesc *idesc)
{
	struct lfs_direct *dp, *ndp;
	struct ubuf *bp;
	long size, blksiz, fix, dploc;

	blksiz = idesc->id_numfrags * lfs_sb_getfsize(fs);
	bread(vp, idesc->id_lblkno, blksiz, 0, &bp);
	if (idesc->id_loc % LFS_DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (struct lfs_direct *) (bp->b_data + idesc->id_loc);
		if (dircheck(idesc, dp))
			goto dpok;
		brelse(bp, 0);
		if (idesc->id_fix == IGNORE)
			return (0);
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bread(vp, idesc->id_lblkno, blksiz, 0, &bp);
		dp = (struct lfs_direct *) (bp->b_data + idesc->id_loc);
		lfs_dir_setino(fs, dp, 0);
		lfs_dir_settype(fs, dp, LFS_DT_UNKNOWN);
		lfs_dir_setnamlen(fs, dp, 0);
		lfs_dir_setreclen(fs, dp, LFS_DIRBLKSIZ);
		/* for now at least, don't zero the old contents */
		dp->d_name[0] = '\0';
		if (fix)
			VOP_BWRITE(bp);
		else
			brelse(bp, 0);
		idesc->id_loc += LFS_DIRBLKSIZ;
		idesc->id_filesize -= LFS_DIRBLKSIZ;
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz) {
		brelse(bp, 0);
		return NULL;
	}
	dploc = idesc->id_loc;
	dp = (struct lfs_direct *) (bp->b_data + dploc);
	idesc->id_loc += lfs_dir_getreclen(fs, dp);
	idesc->id_filesize -= lfs_dir_getreclen(fs, dp);
	if ((idesc->id_loc % LFS_DIRBLKSIZ) == 0) {
		brelse(bp, 0);
		return dp;
	}
	ndp = (struct lfs_direct *) (bp->b_data + idesc->id_loc);
	if (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	    dircheck(idesc, ndp) == 0) {
		brelse(bp, 0);
		size = LFS_DIRBLKSIZ - (idesc->id_loc % LFS_DIRBLKSIZ);
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		if (idesc->id_fix == IGNORE)
			return 0;
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bread(vp, idesc->id_lblkno, blksiz, 0, &bp);
		dp = (struct lfs_direct *) (bp->b_data + dploc);
		lfs_dir_setreclen(fs, dp, lfs_dir_getreclen(fs, dp) + size);
		if (fix)
			VOP_BWRITE(bp);
		else
			brelse(bp, 0);
	} else
		brelse(bp, 0);

	return (dp);
}

/*
 * Verify that a directory entry is valid.
 * This is a superset of the checks made in the kernel.
 */
int
dircheck(struct inodesc *idesc, struct lfs_direct *dp)
{
	int size;
	const char *cp;
	u_char namlen, type;
	int spaceleft;

	spaceleft = LFS_DIRBLKSIZ - (idesc->id_loc % LFS_DIRBLKSIZ);
	if (lfs_dir_getino(fs, dp) >= maxino ||
	    lfs_dir_getreclen(fs, dp) == 0 ||
	    lfs_dir_getreclen(fs, dp) > spaceleft ||
	    (lfs_dir_getreclen(fs, dp) & 0x3) != 0) {
		pwarn("ino too large, reclen=0, reclen>space, or reclen&3!=0\n");
		pwarn("dp->d_ino = 0x%jx\tdp->d_reclen = 0x%x\n",
		    (uintmax_t)lfs_dir_getino(fs, dp),
		    lfs_dir_getreclen(fs, dp));
		pwarn("maxino = %ju\tspaceleft = 0x%x\n",
		    (uintmax_t)maxino, spaceleft);
		return (0);
	}
	if (lfs_dir_getino(fs, dp) == 0)
		return (1);
	size = LFS_DIRSIZ(fs, dp);
	namlen = lfs_dir_getnamlen(fs, dp);
	type = lfs_dir_gettype(fs, dp);
	if (lfs_dir_getreclen(fs, dp) < size ||
	    idesc->id_filesize < size ||
	/* namlen > MAXNAMLEN || */
	    type > 15) {
		printf("reclen<size, filesize<size, namlen too large, or type>15\n");
		return (0);
	}
	cp = dp->d_name;
	for (size = 0; size < namlen; size++)
		if (*cp == '\0' || (*cp++ == '/')) {
			printf("name contains NUL or /\n");
			return (0);
		}
	if (*cp != '\0') {
		printf("name size misstated\n");
		return (0);
	}
	return (1);
}

void
direrror(ino_t ino, const char *errmesg)
{

	fileerror(ino, ino, errmesg);
}

void
fileerror(ino_t cwd, ino_t ino, const char *errmesg)
{
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp;

	pwarn("%s ", errmesg);
	pinode(ino);
	printf("\n");
	pwarn("PARENT=%lld\n", (long long)cwd);
	getpathname(pathbuf, sizeof(pathbuf), cwd, ino);
	if (ino < ULFS_ROOTINO || ino >= maxino) {
		pfatal("NAME=%s\n", pathbuf);
		return;
	}
	vp = vget(fs, ino);
	if (vp == NULL)
		pfatal("INO is NULL\n");
	else {
		if (ftypeok(VTOD(vp)))
			pfatal("%s=%s\n",
			    (lfs_dino_getmode(fs, VTOI(vp)->i_din) & LFS_IFMT) == LFS_IFDIR ?
			    "DIR" : "FILE", pathbuf);
		else
			pfatal("NAME=%s\n", pathbuf);
	}
}

void
adjust(struct inodesc *idesc, short lcnt)
{
	struct uvnode *vp;
	union lfs_dinode *dp;

	/*
	 * XXX: (1) since lcnt is apparently a delta, rename it; (2)
	 * why is it a value to *subtract*? that is unnecessarily
	 * confusing.
	 */

	vp = vget(fs, idesc->id_number);
	dp = VTOD(vp);
	if (lfs_dino_getnlink(fs, dp) == lcnt) {
		if (linkup(idesc->id_number, (ino_t) 0) == 0)
			clri(idesc, "UNREF", 0);
	} else {
		pwarn("LINK COUNT %s", (lfdir == idesc->id_number) ? lfname :
		    ((lfs_dino_getmode(fs, dp) & LFS_IFMT) == LFS_IFDIR ? "DIR" : "FILE"));
		pinode(idesc->id_number);
		printf(" COUNT %d SHOULD BE %d",
		    lfs_dino_getnlink(fs, dp), lfs_dino_getnlink(fs, dp) - lcnt);
		if (preen) {
			if (lcnt < 0) {
				printf("\n");
				pfatal("LINK COUNT INCREASING");
			}
			printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			lfs_dino_setnlink(fs, dp,
			    lfs_dino_getnlink(fs, dp) - lcnt);
			inodirty(VTOI(vp));
		}
	}
}

static int
mkentry(struct inodesc *idesc)
{
	struct lfs_direct *dirp = idesc->id_dirp;
	unsigned namlen;
	unsigned newreclen, oldreclen;

	/* figure the length needed for id_name */
	namlen = strlen(idesc->id_name);
	newreclen = LFS_DIRECTSIZ(namlen);

	/* find the minimum record length for the existing name */
	if (lfs_dir_getino(fs, dirp) != 0)
		oldreclen = LFS_DIRSIZ(fs, dirp);
	else
		oldreclen = 0;

	/* Can we insert here? */
	if (lfs_dir_getreclen(fs, dirp) - oldreclen < newreclen)
		return (KEEPON);

	/* Divide the record; all but oldreclen goes to the new record */
	newreclen = lfs_dir_getreclen(fs, dirp) - oldreclen;
	lfs_dir_setreclen(fs, dirp, oldreclen);

	/* advance the pointer to the new record */
	dirp = LFS_NEXTDIR(fs, dirp);

	/* write record; ino to be entered is in id_parent */
	lfs_dir_setino(fs, dirp, idesc->id_parent);
	lfs_dir_setreclen(fs, dirp, newreclen);
	lfs_dir_settype(fs, dirp, typemap[idesc->id_parent]);
	lfs_dir_setnamlen(fs, dirp, namlen);
	memcpy(dirp->d_name, idesc->id_name, (size_t)namlen + 1);
	return (ALTERED | STOP);
}

static int
chgino(struct inodesc *idesc)
{
	struct lfs_direct *dirp = idesc->id_dirp;
	int namlen;

	namlen = lfs_dir_getnamlen(fs, dirp);
	if (memcmp(dirp->d_name, idesc->id_name, namlen + 1))
		return (KEEPON);
	lfs_dir_setino(fs, dirp, idesc->id_parent);
	lfs_dir_settype(fs, dirp, typemap[idesc->id_parent]);
	return (ALTERED | STOP);
}

int
linkup(ino_t orphan, ino_t parentdir)
{
	union lfs_dinode *dp;
	int lostdir;
	ino_t oldlfdir;
	struct inodesc idesc;
	char tempname[BUFSIZ];
	struct uvnode *vp;

	memset(&idesc, 0, sizeof(struct inodesc));
	vp = vget(fs, orphan);
	dp = VTOD(vp);
	lostdir = (lfs_dino_getmode(fs, dp) & LFS_IFMT) == LFS_IFDIR;
	pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	pinode(orphan);
	if (preen && lfs_dino_getsize(fs, dp) == 0)
		return (0);
	if (preen)
		printf(" (RECONNECTED)\n");
	else if (reply("RECONNECT") == 0)
		return (0);
	if (lfdir == 0) {
		dp = ginode(ULFS_ROOTINO);
		idesc.id_name = lfname;
		idesc.id_type = DATA;
		idesc.id_func = findino;
		idesc.id_number = ULFS_ROOTINO;
		if ((ckinode(dp, &idesc) & FOUND) != 0) {
			lfdir = idesc.id_parent;
		} else {
			pwarn("NO lost+found DIRECTORY");
			if (preen || reply("CREATE")) {
				lfdir = allocdir(ULFS_ROOTINO, (ino_t) 0, lfmode);
				if (lfdir != 0) {
					if (makeentry(ULFS_ROOTINO, lfdir, lfname) != 0) {
						if (preen)
							printf(" (CREATED)\n");
					} else {
						freedir(lfdir, ULFS_ROOTINO);
						lfdir = 0;
						if (preen)
							printf("\n");
					}
				}
			}
		}
		if (lfdir == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY");
			printf("\n\n");
			return (0);
		}
	}
	vp = vget(fs, lfdir);
	dp = VTOD(vp);
	if ((lfs_dino_getmode(fs, dp) & LFS_IFMT) != LFS_IFDIR) {
		pfatal("lost+found IS NOT A DIRECTORY");
		if (reply("REALLOCATE") == 0)
			return (0);
		oldlfdir = lfdir;
		if ((lfdir = allocdir(ULFS_ROOTINO, (ino_t) 0, lfmode)) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		if ((changeino(ULFS_ROOTINO, lfname, lfdir) & ALTERED) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		inodirty(VTOI(vp));
		idesc.id_type = ADDR;
		idesc.id_func = pass4check;
		idesc.id_number = oldlfdir;
		adjust(&idesc, lncntp[oldlfdir] + 1);
		lncntp[oldlfdir] = 0;
		vp = vget(fs, lfdir);
		dp = VTOD(vp);
	}
	if (statemap[lfdir] != DFOUND) {
		pfatal("SORRY. NO lost+found DIRECTORY\n\n");
		return (0);
	}
	(void) lftempname(tempname, orphan);
	if (makeentry(lfdir, orphan, tempname) == 0) {
		pfatal("SORRY. NO SPACE IN lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
	lncntp[orphan]--;
	if (lostdir) {
		if ((changeino(orphan, "..", lfdir) & ALTERED) == 0 &&
		    parentdir != (ino_t) - 1)
			(void) makeentry(orphan, lfdir, "..");
		vp = vget(fs, lfdir);
		lfs_dino_setnlink(fs, VTOI(vp)->i_din,
		    lfs_dino_getnlink(fs, VTOI(vp)->i_din) + 1);
		inodirty(VTOI(vp));
		lncntp[lfdir]++;
		pwarn("DIR I=%llu CONNECTED. ", (unsigned long long)orphan);
		if (parentdir != (ino_t) - 1)
			printf("PARENT WAS I=%llu\n",
			    (unsigned long long)parentdir);
		if (preen == 0)
			printf("\n");
	}
	return (1);
}

/*
 * fix an entry in a directory.
 */
int
changeino(ino_t dir, const char *name, ino_t newnum)
{
	struct inodesc idesc;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = chgino;
	idesc.id_number = dir;
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	idesc.id_parent = newnum;	/* new value for name */

	return (ckinode(ginode(dir), &idesc));
}

/*
 * make an entry in a directory
 */
int
makeentry(ino_t parent, ino_t ino, const char *name)
{
	union lfs_dinode *dp;
	struct inodesc idesc;
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp;
	uint64_t size;

	if (parent < ULFS_ROOTINO || parent >= maxino ||
	    ino < ULFS_ROOTINO || ino >= maxino)
		return (0);
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = mkentry;
	idesc.id_number = parent;
	idesc.id_parent = ino;	/* this is the inode to enter */
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	vp = vget(fs, parent);
	dp = VTOD(vp);
	size = lfs_dino_getsize(fs, dp);
	if (size % LFS_DIRBLKSIZ) {
		size = roundup(size, LFS_DIRBLKSIZ);
		lfs_dino_setsize(fs, dp, size);
		inodirty(VTOI(vp));
	}
	if ((ckinode(dp, &idesc) & ALTERED) != 0)
		return (1);
	getpathname(pathbuf, sizeof(pathbuf), parent, parent);
	vp = vget(fs, parent);
	dp = VTOD(vp);
	if (expanddir(vp, dp, pathbuf) == 0)
		return (0);
	return (ckinode(dp, &idesc) & ALTERED);
}

/*
 * Attempt to expand the size of a directory
 */
static int
expanddir(struct uvnode *vp, union lfs_dinode *dp, char *name)
{
	daddr_t lastbn;
	struct ubuf *bp;
	char *cp, firstblk[LFS_DIRBLKSIZ];

	lastbn = lfs_lblkno(fs, lfs_dino_getsize(fs, dp));
	if (lastbn >= ULFS_NDADDR - 1 || lfs_dino_getdb(fs, dp, lastbn) == 0 ||
	    lfs_dino_getsize(fs, dp) == 0)
		return (0);
	lfs_dino_setdb(fs, dp, lastbn + 1, lfs_dino_getdb(fs, dp, lastbn));
	lfs_dino_setdb(fs, dp, lastbn, 0);
	bp = getblk(vp, lastbn, lfs_sb_getbsize(fs));
	VOP_BWRITE(bp);
	lfs_dino_setsize(fs, dp,
	    lfs_dino_getsize(fs, dp) + lfs_sb_getbsize(fs));
	lfs_dino_setblocks(fs, dp,
	    lfs_dino_getblocks(fs, dp) + lfs_btofsb(fs, lfs_sb_getbsize(fs)));
	bread(vp, lfs_dino_getdb(fs, dp, lastbn + 1),
	    (long) lfs_dblksize(fs, dp, lastbn + 1), 0, &bp);
	if (bp->b_flags & B_ERROR)
		goto bad;
	memcpy(firstblk, bp->b_data, LFS_DIRBLKSIZ);
	bread(vp, lastbn, lfs_sb_getbsize(fs), 0, &bp);
	if (bp->b_flags & B_ERROR)
		goto bad;
	memcpy(bp->b_data, firstblk, LFS_DIRBLKSIZ);
	for (cp = &bp->b_data[LFS_DIRBLKSIZ];
	    cp < &bp->b_data[lfs_sb_getbsize(fs)];
	    cp += LFS_DIRBLKSIZ)
		memcpy(cp, &emptydir, sizeof emptydir);
	VOP_BWRITE(bp);
	bread(vp, lfs_dino_getdb(fs, dp, lastbn + 1),
	    (long) lfs_dblksize(fs, dp, lastbn + 1), 0, &bp);
	if (bp->b_flags & B_ERROR)
		goto bad;
	memcpy(bp->b_data, &emptydir, sizeof emptydir);
	pwarn("NO SPACE LEFT IN %s", name);
	if (preen)
		printf(" (EXPANDED)\n");
	else if (reply("EXPAND") == 0)
		goto bad;
	VOP_BWRITE(bp);
	inodirty(VTOI(vp));
	return (1);
bad:
	lfs_dino_setdb(fs, dp, lastbn, lfs_dino_getdb(fs, dp, lastbn + 1));
	lfs_dino_setdb(fs, dp, lastbn + 1, 0);
	lfs_dino_setsize(fs, dp,
	    lfs_dino_getsize(fs, dp) - lfs_sb_getbsize(fs));
	lfs_dino_setblocks(fs, dp,
	    lfs_dino_getblocks(fs, dp) - lfs_btofsb(fs, lfs_sb_getbsize(fs)));
	return (0);
}

/*
 * allocate a new directory
 */
int
allocdir(ino_t parent, ino_t request, int mode)
{
	ino_t ino;
	char *cp;
	union lfs_dinode *dp;
	struct ubuf *bp;
	struct lfs_dirtemplate *dirp;
	struct uvnode *vp;

	ino = allocino(request, LFS_IFDIR | mode);
	dirp = &dirhead;
	dirp->dot_ino = ino;
	dirp->dotdot_ino = parent;
	vp = vget(fs, ino);
	dp = VTOD(vp);
	bread(vp, lfs_dino_getdb(fs, dp, 0), lfs_sb_getfsize(fs), 0, &bp);
	if (bp->b_flags & B_ERROR) {
		brelse(bp, 0);
		freeino(ino);
		return (0);
	}
	memcpy(bp->b_data, dirp, sizeof(struct lfs_dirtemplate));
	for (cp = &bp->b_data[LFS_DIRBLKSIZ];
	    cp < &bp->b_data[lfs_sb_getfsize(fs)];
	    cp += LFS_DIRBLKSIZ)
		memcpy(cp, &emptydir, sizeof emptydir);
	VOP_BWRITE(bp);
	lfs_dino_setnlink(fs, dp, 2);
	inodirty(VTOI(vp));
	if (ino == ULFS_ROOTINO) {
		lncntp[ino] = lfs_dino_getnlink(fs, dp);
		cacheino(dp, ino);
		return (ino);
	}
	if (statemap[parent] != DSTATE && statemap[parent] != DFOUND) {
		freeino(ino);
		return (0);
	}
	cacheino(dp, ino);
	statemap[ino] = statemap[parent];
	if (statemap[ino] == DSTATE) {
		lncntp[ino] = lfs_dino_getnlink(fs, dp);
		lncntp[parent]++;
	}
	vp = vget(fs, parent);
	dp = VTOD(vp);
	lfs_dino_setnlink(fs, dp, lfs_dino_getnlink(fs, dp) + 1);
	inodirty(VTOI(vp));
	return (ino);
}

/*
 * free a directory inode
 */
static void
freedir(ino_t ino, ino_t parent)
{
	struct uvnode *vp;

	if (ino != parent) {
		vp = vget(fs, parent);
		lfs_dino_setnlink(fs, VTOI(vp)->i_din,
		    lfs_dino_getnlink(fs, VTOI(vp)->i_din) - 1);
		inodirty(VTOI(vp));
	}
	freeino(ino);
}

/*
 * generate a temporary name for the lost+found directory.
 */
static int
lftempname(char *bufp, ino_t ino)
{
	ino_t in;
	char *cp;
	int namlen;

	cp = bufp + 2;
	for (in = maxino; in > 0; in /= 10)
		cp++;
	*--cp = 0;
	namlen = cp - bufp;
	in = ino;
	while (cp > bufp) {
		*--cp = (in % 10) + '0';
		in /= 10;
	}
	*cp = '#';
	return (namlen);
}
