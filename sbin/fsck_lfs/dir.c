/* $NetBSD: dir.c,v 1.16 2005/06/27 02:48:28 christos Exp $	 */

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

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/lfs/lfs.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

const char *lfname = "lost+found";
int lfmode = 01700;
struct dirtemplate emptydir = {0, DIRBLKSIZ};
struct dirtemplate dirhead = {
	0, 12, DT_DIR, 1, ".",
	0, DIRBLKSIZ - 12, DT_DIR, 2, ".."
};
struct odirtemplate odirhead = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

static int expanddir(struct uvnode *, struct ufs1_dinode *, char *);
static void freedir(ino_t, ino_t);
static struct direct *fsck_readdir(struct uvnode *, struct inodesc *);
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
		    inp->i_number == ROOTINO)
			continue;
		pinp = getinoinfo(inp->i_parent);
		inp->i_parentp = pinp;
		inp->i_sibling = pinp->i_child;
		pinp->i_child = inp;
	}
	inp = getinoinfo(ROOTINO);
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
	struct direct *dp;
	struct ubuf *bp;
	int dsize, n;
	long blksiz;
	char dbuf[DIRBLKSIZ];
	struct uvnode *vp;

	if (idesc->id_type != DATA)
		errexit("wrong type to dirscan %d\n", idesc->id_type);
	if (idesc->id_entryno == 0 &&
	    (idesc->id_filesize & (DIRBLKSIZ - 1)) != 0)
		idesc->id_filesize = roundup(idesc->id_filesize, DIRBLKSIZ);
	blksiz = idesc->id_numfrags * fs->lfs_fsize;
	if (chkrange(idesc->id_blkno, fragstofsb(fs, idesc->id_numfrags))) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}
	idesc->id_loc = 0;

	vp = vget(fs, idesc->id_number);
	for (dp = fsck_readdir(vp, idesc); dp != NULL;
	    dp = fsck_readdir(vp, idesc)) {
		dsize = dp->d_reclen;
		memcpy(dbuf, dp, (size_t) dsize);
		idesc->id_dirp = (struct direct *) dbuf;
		if ((n = (*idesc->id_func) (idesc)) & ALTERED) {
			bread(vp, idesc->id_lblkno, blksiz, NOCRED, &bp);
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
static struct direct *
fsck_readdir(struct uvnode *vp, struct inodesc *idesc)
{
	struct direct *dp, *ndp;
	struct ubuf *bp;
	long size, blksiz, fix, dploc;

	blksiz = idesc->id_numfrags * fs->lfs_fsize;
	bread(vp, idesc->id_lblkno, blksiz, NOCRED, &bp);
	if (idesc->id_loc % DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (struct direct *) (bp->b_data + idesc->id_loc);
		if (dircheck(idesc, dp))
			goto dpok;
		brelse(bp);
		if (idesc->id_fix == IGNORE)
			return (0);
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bread(vp, idesc->id_lblkno, blksiz, NOCRED, &bp);
		dp = (struct direct *) (bp->b_data + idesc->id_loc);
		dp->d_reclen = DIRBLKSIZ;
		dp->d_ino = 0;
		dp->d_type = 0;
		dp->d_namlen = 0;
		dp->d_name[0] = '\0';
		if (fix)
			VOP_BWRITE(bp);
		else
			brelse(bp);
		idesc->id_loc += DIRBLKSIZ;
		idesc->id_filesize -= DIRBLKSIZ;
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz) {
		brelse(bp);
		return NULL;
	}
	dploc = idesc->id_loc;
	dp = (struct direct *) (bp->b_data + dploc);
	idesc->id_loc += dp->d_reclen;
	idesc->id_filesize -= dp->d_reclen;
	if ((idesc->id_loc % DIRBLKSIZ) == 0) {
		brelse(bp);
		return dp;
	}
	ndp = (struct direct *) (bp->b_data + idesc->id_loc);
	if (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	    dircheck(idesc, ndp) == 0) {
		brelse(bp);
		size = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		if (idesc->id_fix == IGNORE)
			return 0;
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bread(vp, idesc->id_lblkno, blksiz, NOCRED, &bp);
		dp = (struct direct *) (bp->b_data + dploc);
		dp->d_reclen += size;
		if (fix)
			VOP_BWRITE(bp);
		else
			brelse(bp);
	} else
		brelse(bp);

	return (dp);
}

/*
 * Verify that a directory entry is valid.
 * This is a superset of the checks made in the kernel.
 */
int
dircheck(struct inodesc *idesc, struct direct *dp)
{
	int size;
	char *cp;
	u_char namlen, type;
	int spaceleft;

	spaceleft = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
	if (dp->d_ino >= maxino ||
	    dp->d_reclen == 0 ||
	    dp->d_reclen > spaceleft ||
	    (dp->d_reclen & 0x3) != 0) {
		pwarn("ino too large, reclen=0, reclen>space, or reclen&3!=0\n");
		pwarn("dp->d_ino = 0x%x\tdp->d_reclen = 0x%x\n",
		    dp->d_ino, dp->d_reclen);
		pwarn("maxino = 0x%x\tspaceleft = 0x%x\n", maxino, spaceleft);
		return (0);
	}
	if (dp->d_ino == 0)
		return (1);
	size = DIRSIZ(0, dp, 0);
	namlen = dp->d_namlen;
	type = dp->d_type;
	if (dp->d_reclen < size ||
	    idesc->id_filesize < size ||
	/* namlen > MAXNAMLEN || */
	    type > 15) {
		printf("reclen<size, filesize<size, namlen too large, or type>15\n");
		return (0);
	}
	for (cp = dp->d_name, size = 0; size < namlen; size++)
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
direrror(ino_t ino, char *errmesg)
{

	fileerror(ino, ino, errmesg);
}

void
fileerror(ino_t cwd, ino_t ino, char *errmesg)
{
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp;

	pwarn("%s ", errmesg);
	pinode(ino);
	printf("\n");
	getpathname(pathbuf, sizeof(pathbuf), cwd, ino);
	if (ino < ROOTINO || ino >= maxino) {
		pfatal("NAME=%s\n", pathbuf);
		return;
	}
	vp = vget(fs, ino);
	if (vp == NULL)
		pfatal("INO is NULL\n");
	else {
		if (ftypeok(VTOD(vp)))
			pfatal("%s=%s\n",
			    (VTOI(vp)->i_ffs1_mode & IFMT) == IFDIR ?
			    "DIR" : "FILE", pathbuf);
		else
			pfatal("NAME=%s\n", pathbuf);
	}
}

void
adjust(struct inodesc *idesc, short lcnt)
{
	struct uvnode *vp;
	struct ufs1_dinode *dp;

	vp = vget(fs, idesc->id_number);
	dp = VTOD(vp);
	if (dp->di_nlink == lcnt) {
		if (linkup(idesc->id_number, (ino_t) 0) == 0)
			clri(idesc, "UNREF", 0);
	} else {
		pwarn("LINK COUNT %s", (lfdir == idesc->id_number) ? lfname :
		    ((dp->di_mode & IFMT) == IFDIR ? "DIR" : "FILE"));
		pinode(idesc->id_number);
		printf(" COUNT %d SHOULD BE %d",
		    dp->di_nlink, dp->di_nlink - lcnt);
		if (preen) {
			if (lcnt < 0) {
				printf("\n");
				pfatal("LINK COUNT INCREASING");
			}
			printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			dp->di_nlink -= lcnt;
			inodirty(VTOI(vp));
		}
	}
}

static int
mkentry(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;
	struct direct newent;
	int newlen, oldlen;

	newent.d_namlen = strlen(idesc->id_name);
	newlen = DIRSIZ(0, &newent, 0);
	if (dirp->d_ino != 0)
		oldlen = DIRSIZ(0, dirp, 0);
	else
		oldlen = 0;
	if (dirp->d_reclen - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = dirp->d_reclen - oldlen;
	dirp->d_reclen = oldlen;
	dirp = (struct direct *) (((char *) dirp) + oldlen);
	dirp->d_ino = idesc->id_parent;	/* ino to be entered is in id_parent */
	dirp->d_reclen = newent.d_reclen;
	dirp->d_type = typemap[idesc->id_parent];
	dirp->d_namlen = newent.d_namlen;
	memcpy(dirp->d_name, idesc->id_name, (size_t) dirp->d_namlen + 1);
	return (ALTERED | STOP);
}

static int
chgino(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;

	if (memcmp(dirp->d_name, idesc->id_name, (int) dirp->d_namlen + 1))
		return (KEEPON);
	dirp->d_ino = idesc->id_parent;
	dirp->d_type = typemap[idesc->id_parent];
	return (ALTERED | STOP);
}

int
linkup(ino_t orphan, ino_t parentdir)
{
	struct ufs1_dinode *dp;
	int lostdir;
	ino_t oldlfdir;
	struct inodesc idesc;
	char tempname[BUFSIZ];
	struct uvnode *vp;

	memset(&idesc, 0, sizeof(struct inodesc));
	vp = vget(fs, orphan);
	dp = VTOD(vp);
	lostdir = (dp->di_mode & IFMT) == IFDIR;
	pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	pinode(orphan);
	if (preen && dp->di_size == 0)
		return (0);
	if (preen)
		printf(" (RECONNECTED)\n");
	else if (reply("RECONNECT") == 0)
		return (0);
	if (lfdir == 0) {
		dp = ginode(ROOTINO);
		idesc.id_name = lfname;
		idesc.id_type = DATA;
		idesc.id_func = findino;
		idesc.id_number = ROOTINO;
		if ((ckinode(dp, &idesc) & FOUND) != 0) {
			lfdir = idesc.id_parent;
		} else {
			pwarn("NO lost+found DIRECTORY");
			if (preen || reply("CREATE")) {
				lfdir = allocdir(ROOTINO, (ino_t) 0, lfmode);
				if (lfdir != 0) {
					if (makeentry(ROOTINO, lfdir, lfname) != 0) {
						if (preen)
							printf(" (CREATED)\n");
					} else {
						freedir(lfdir, ROOTINO);
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
	if ((dp->di_mode & IFMT) != IFDIR) {
		pfatal("lost+found IS NOT A DIRECTORY");
		if (reply("REALLOCATE") == 0)
			return (0);
		oldlfdir = lfdir;
		if ((lfdir = allocdir(ROOTINO, (ino_t) 0, lfmode)) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		if ((changeino(ROOTINO, lfname, lfdir) & ALTERED) == 0) {
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
		VTOI(vp)->i_ffs1_nlink++;
		inodirty(VTOI(vp));
		lncntp[lfdir]++;
		pwarn("DIR I=%u CONNECTED. ", orphan);
		if (parentdir != (ino_t) - 1)
			printf("PARENT WAS I=%u\n", parentdir);
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
	struct ufs1_dinode *dp;
	struct inodesc idesc;
	char pathbuf[MAXPATHLEN + 1];
	struct uvnode *vp;

	if (parent < ROOTINO || parent >= maxino ||
	    ino < ROOTINO || ino >= maxino)
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
	if (dp->di_size % DIRBLKSIZ) {
		dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
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
expanddir(struct uvnode *vp, struct ufs1_dinode *dp, char *name)
{
	daddr_t lastbn;
	struct ubuf *bp;
	char *cp, firstblk[DIRBLKSIZ];

	lastbn = lblkno(fs, dp->di_size);
	if (lastbn >= NDADDR - 1 || dp->di_db[lastbn] == 0 || dp->di_size == 0)
		return (0);
	dp->di_db[lastbn + 1] = dp->di_db[lastbn];
	dp->di_db[lastbn] = 0;
	bp = getblk(vp, lastbn, fs->lfs_bsize);
	VOP_BWRITE(bp);
	dp->di_size += fs->lfs_bsize;
	dp->di_blocks += btofsb(fs, fs->lfs_bsize);
	bread(vp, dp->di_db[lastbn + 1],
	    (long) dblksize(fs, dp, lastbn + 1), NOCRED, &bp);
	if (bp->b_flags & B_ERROR)
		goto bad;
	memcpy(firstblk, bp->b_data, DIRBLKSIZ);
	bread(vp, lastbn, fs->lfs_bsize, NOCRED, &bp);
	if (bp->b_flags & B_ERROR)
		goto bad;
	memcpy(bp->b_data, firstblk, DIRBLKSIZ);
	for (cp = &bp->b_data[DIRBLKSIZ];
	    cp < &bp->b_data[fs->lfs_bsize];
	    cp += DIRBLKSIZ)
		memcpy(cp, &emptydir, sizeof emptydir);
	VOP_BWRITE(bp);
	bread(vp, dp->di_db[lastbn + 1],
	    (long) dblksize(fs, dp, lastbn + 1), NOCRED, &bp);
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
	dp->di_db[lastbn] = dp->di_db[lastbn + 1];
	dp->di_db[lastbn + 1] = 0;
	dp->di_size -= fs->lfs_bsize;
	dp->di_blocks -= btofsb(fs, fs->lfs_bsize);
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
	struct ufs1_dinode *dp;
	struct ubuf *bp;
	struct dirtemplate *dirp;
	struct uvnode *vp;

	ino = allocino(request, IFDIR | mode);
	dirp = &dirhead;
	dirp->dot_ino = ino;
	dirp->dotdot_ino = parent;
	vp = vget(fs, ino);
	dp = VTOD(vp);
	bread(vp, dp->di_db[0], fs->lfs_fsize, NOCRED, &bp);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		freeino(ino);
		return (0);
	}
	memcpy(bp->b_data, dirp, sizeof(struct dirtemplate));
	for (cp = &bp->b_data[DIRBLKSIZ];
	    cp < &bp->b_data[fs->lfs_fsize];
	    cp += DIRBLKSIZ)
		memcpy(cp, &emptydir, sizeof emptydir);
	VOP_BWRITE(bp);
	dp->di_nlink = 2;
	inodirty(VTOI(vp));
	if (ino == ROOTINO) {
		lncntp[ino] = dp->di_nlink;
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
		lncntp[ino] = dp->di_nlink;
		lncntp[parent]++;
	}
	vp = vget(fs, parent);
	dp = VTOD(vp);
	dp->di_nlink++;
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
		VTOI(vp)->i_ffs1_nlink--;
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
