/*	$NetBSD: dir.c,v 1.54 2013/01/22 09:39:11 dholland Exp $	*/

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
static char sccsid[] = "@(#)dir.c	8.8 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: dir.c,v 1.54 2013/01/22 09:39:11 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

const char	*lfname = "lost+found";
int	lfmode = 01700;
ino_t	lfdir;
struct	dirtemplate emptydir = {
	.dot_ino = 0,
	.dot_reclen = DIRBLKSIZ,
};
struct	dirtemplate dirhead = {
	.dot_ino = 0,
	.dot_reclen = 12,
	.dot_type = DT_DIR,
	.dot_namlen = 1,
	.dot_name = ".",
	.dotdot_ino = 0,
	.dotdot_reclen = DIRBLKSIZ - 12,
	.dotdot_type = DT_DIR,
	.dotdot_namlen = 2,
	.dotdot_name = "..",
};
struct	odirtemplate odirhead = {
	.dot_ino = 0,
	.dot_reclen = 12,
	.dot_namlen = 1,
	.dot_name = ".",
	.dotdot_ino = 0,
	.dotdot_reclen = DIRBLKSIZ - 12,
	.dotdot_namlen = 2,
	.dotdot_name = "..",
};

static int chgino(struct  inodesc *);
static int dircheck(struct inodesc *, struct direct *);
static int expanddir(union dinode *, char *);
static void freedir(ino_t, ino_t);
static struct direct *fsck_readdir(struct inodesc *);
static struct bufarea *getdirblk(daddr_t, long);
static int lftempname(char *, ino_t);
static int mkentry(struct inodesc *);
void reparent(ino_t, ino_t);

/*
 * Propagate connected state through the tree.
 */
void
propagate(ino_t inumber)
{
	struct inoinfo *inp;

	inp = getinoinfo(inumber);

	for (;;) {
		inoinfo(inp->i_number)->ino_state = DMARK;
		if (inp->i_child &&
		    inoinfo(inp->i_child->i_number)->ino_state != DMARK)
			inp = inp->i_child;
		else if (inp->i_number == inumber)
			break;
		else if (inp->i_sibling)
			inp = inp->i_sibling;
		else
			inp = getinoinfo(inp->i_parent);
	}

	for (;;) {
		inoinfo(inp->i_number)->ino_state = DFOUND;
		if (inp->i_child &&
		    inoinfo(inp->i_child->i_number)->ino_state != DFOUND)
			inp = inp->i_child;
		else if (inp->i_number == inumber)
			break;
		else if (inp->i_sibling)
			inp = inp->i_sibling;
		else
			inp = getinoinfo(inp->i_parent);
	}
}

void
reparent(ino_t inumber, ino_t parent)
{
	struct inoinfo *inp, *pinp;

	inp = getinoinfo(inumber);
	inp->i_parent = inp->i_dotdot = parent;
	pinp = getinoinfo(parent);
	inp->i_sibling = pinp->i_child;
	pinp->i_child = inp;
	propagate(inumber);
}

/*
 * Scan each entry in a directory block.
 */
int
dirscan(struct inodesc *idesc)
{
	struct direct *dp;
	struct bufarea *bp;
	int dsize, n;
	long blksiz;
#if DIRBLKSIZ > APPLEUFS_DIRBLKSIZ
	char dbuf[DIRBLKSIZ];
#else
	char dbuf[APPLEUFS_DIRBLKSIZ];
#endif

	if (idesc->id_type != DATA)
		errexit("wrong type to dirscan %d", idesc->id_type);
	if (idesc->id_entryno == 0 &&
	    (idesc->id_filesize & (dirblksiz - 1)) != 0)
		idesc->id_filesize = roundup(idesc->id_filesize, dirblksiz);
	blksiz = idesc->id_numfrags * sblock->fs_fsize;
	if (chkrange(idesc->id_blkno, idesc->id_numfrags)) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}

	/*
	 * If we are are swapping byte order in directory entries, just swap
	 * this block and return.
	 */
	if (do_dirswap) {
		int off;
		bp = getdirblk(idesc->id_blkno, blksiz);
		for (off = 0; off < blksiz; off += iswap16(dp->d_reclen)) {
			dp = (struct direct *)(bp->b_un.b_buf + off);
			dp->d_ino = bswap32(dp->d_ino);
			dp->d_reclen = bswap16(dp->d_reclen);
			if (!newinofmt) {
				u_int8_t tmp = dp->d_namlen;
				dp->d_namlen = dp->d_type;
				dp->d_type = tmp;
			}
			if (dp->d_reclen == 0)
				break;
		}
		dirty(bp);
		idesc->id_filesize -= blksiz;
		return (idesc->id_filesize > 0 ? KEEPON : STOP);
	}

	idesc->id_loc = 0;
	for (dp = fsck_readdir(idesc); dp != NULL; dp = fsck_readdir(idesc)) {
		dsize = iswap16(dp->d_reclen);
		if (dsize > (int)sizeof dbuf)
			dsize = sizeof dbuf;
		memmove(dbuf, dp, (size_t)dsize);
#		if (BYTE_ORDER == LITTLE_ENDIAN)
			if (!newinofmt && !needswap) {
#		else
			if (!newinofmt && needswap) {
#		endif
				struct direct *tdp = (struct direct *)dbuf;
				u_char tmp;

				tmp = tdp->d_namlen;
				tdp->d_namlen = tdp->d_type;
				tdp->d_type = tmp;
			}
		idesc->id_dirp = (struct direct *)dbuf;
		if ((n = (*idesc->id_func)(idesc)) & ALTERED) {
#			if (BYTE_ORDER == LITTLE_ENDIAN)
				if (!newinofmt && !doinglevel2 && !needswap) {
#			else
				if (!newinofmt && !doinglevel2 && needswap) {
#			endif
					struct direct *tdp;
					u_char tmp;

					tdp = (struct direct *)dbuf;
					tmp = tdp->d_namlen;
					tdp->d_namlen = tdp->d_type;
					tdp->d_type = tmp;
				}
			bp = getdirblk(idesc->id_blkno, blksiz);
			memmove(bp->b_un.b_buf + idesc->id_loc - dsize, dbuf,
			    (size_t)dsize);
			dirty(bp);
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
fsck_readdir(struct inodesc *idesc)
{
	struct direct *dp, *ndp;
	struct bufarea *bp;
	long size, blksiz, fix, dploc;

	blksiz = idesc->id_numfrags * sblock->fs_fsize;
	bp = getdirblk(idesc->id_blkno, blksiz);
	if (idesc->id_loc % dirblksiz == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (struct direct *)(bp->b_un.b_buf + idesc->id_loc);
		if (dircheck(idesc, dp))
			goto dpok;
		if (idesc->id_fix == IGNORE)
			return (0);
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bp = getdirblk(idesc->id_blkno, blksiz);
		dp = (struct direct *)(bp->b_un.b_buf + idesc->id_loc);
		dp->d_reclen = iswap16(dirblksiz);
		dp->d_ino = 0;
		dp->d_type = 0;
		dp->d_namlen = 0;
		dp->d_name[0] = '\0';
		if (fix)
			dirty(bp);
		else 
			markclean = 0;
		idesc->id_loc += dirblksiz;
		idesc->id_filesize -= dirblksiz;
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz)
		return NULL;
	dploc = idesc->id_loc;
	dp = (struct direct *)(bp->b_un.b_buf + dploc);
	idesc->id_loc += iswap16(dp->d_reclen);
	idesc->id_filesize -= iswap16(dp->d_reclen);
	if ((idesc->id_loc % dirblksiz) == 0)
		return (dp);
	ndp = (struct direct *)(bp->b_un.b_buf + idesc->id_loc);
	if (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	    dircheck(idesc, ndp) == 0) {
		size = dirblksiz - (idesc->id_loc % dirblksiz);
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		if (idesc->id_fix == IGNORE)
			return (0);
		fix = dofix(idesc, "DIRECTORY CORRUPTED");
		bp = getdirblk(idesc->id_blkno, blksiz);
		dp = (struct direct *)(bp->b_un.b_buf + dploc);
		dp->d_reclen = iswap16(iswap16(dp->d_reclen) + size);
		if (fix)
			dirty(bp);
		else 
			markclean = 0;
	}
	return (dp);
}

/*
 * Verify that a directory entry is valid.
 * This is a superset of the checks made in the kernel.
 */
static int
dircheck(struct inodesc *idesc, struct direct *dp)
{
	int size;
	char *cp;
	u_char namlen, type;
	int spaceleft;

	spaceleft = dirblksiz - (idesc->id_loc % dirblksiz);
	if (iswap32(dp->d_ino) >= maxino ||
	    dp->d_reclen == 0 ||
	    iswap16(dp->d_reclen) > spaceleft ||
	    (iswap16(dp->d_reclen) & 0x3) != 0) 
		return (0);
	if (dp->d_ino == 0)
		return (1);
	size = DIRSIZ(!newinofmt, dp, needswap);
#	if (BYTE_ORDER == LITTLE_ENDIAN)
		if (!newinofmt && !needswap) {
#	else
		if (!newinofmt && needswap) {
#	endif
			type = dp->d_namlen;
			namlen = dp->d_type;
		} else {
			namlen = dp->d_namlen;
			type = dp->d_type;
		}
	if (iswap16(dp->d_reclen) < size ||
	    idesc->id_filesize < size ||
	    /* namlen > MAXNAMLEN || */
	    type > 15)
		return (0);
	for (cp = dp->d_name, size = 0; size < namlen; size++)
		if (*cp == '\0' || (*cp++ == '/'))
			return (0);
	if (*cp != '\0')
		return (0);
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
	union dinode *dp;
	char pathbuf[MAXPATHLEN + 1];
	uint16_t mode;

	pwarn("%s ", errmesg);
	pinode(ino);
	printf("\n");
	getpathname(pathbuf, sizeof(pathbuf), cwd, ino);
	if (ino < UFS_ROOTINO || ino > maxino) {
		pfatal("NAME=%s\n", pathbuf);
		return;
	}
	dp = ginode(ino);
	if (ftypeok(dp)) {
		mode = DIP(dp, mode);
		pfatal("%s=%s\n",
		    (iswap16(mode) & IFMT) == IFDIR ? "DIR" : "FILE", pathbuf);
	}
	else
		pfatal("NAME=%s\n", pathbuf);
}

void
adjust(struct inodesc *idesc, int lcnt)
{
	union dinode *dp;
	int16_t nlink;
	int saveresolved;

	dp = ginode(idesc->id_number);
	nlink = iswap16(DIP(dp, nlink));
	if (nlink == lcnt) {
		/*
		 * If we have not hit any unresolved problems, are running
		 * in preen mode, and are on a file system using soft updates,
		 * then just toss any partially allocated files.
		 */
		if (resolved && preen && usedsoftdep) {
			clri(idesc, "UNREF", 1);
			return;
		} else {
			/*
			 * The file system can be marked clean even if
			 * a file is not linked up, but is cleared.
			 * Hence, resolved should not be cleared when
			 * linkup is answered no, but clri is answered yes.
			 */
			saveresolved = resolved;
			if (linkup(idesc->id_number, (ino_t)0, NULL) == 0) {
				resolved = saveresolved;
				clri(idesc, "UNREF", 0);
				return;
			}
			/*
			 * Account for the new reference created by linkup().
			 */
			dp = ginode(idesc->id_number);
			lcnt--;
		}
	}
	if (lcnt != 0) {
		pwarn("LINK COUNT %s", (lfdir == idesc->id_number) ? lfname :
			((iswap16(DIP(dp, mode)) & IFMT) == IFDIR ?
			"DIR" : "FILE"));
		pinode(idesc->id_number);
		printf(" COUNT %d SHOULD BE %d",
			nlink, nlink - lcnt);
		if (preen || usedsoftdep) {
			if (lcnt < 0) {
				printf("\n");
				pfatal("LINK COUNT INCREASING");
			}
			if (preen)
				printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			DIP_SET(dp, nlink, iswap16(nlink - lcnt));
			inodirty();
		} else 
			markclean = 0;
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
	if (iswap16(dirp->d_reclen) - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = iswap16(iswap16(dirp->d_reclen) - oldlen);
	dirp->d_reclen = iswap16(oldlen);
	dirp = (struct direct *)(((char *)dirp) + oldlen);
	/* ino to be entered is in id_parent */
	dirp->d_ino = iswap32(idesc->id_parent);
	dirp->d_reclen = newent.d_reclen;
	if (newinofmt)
		dirp->d_type = inoinfo(idesc->id_parent)->ino_type;
	else
		dirp->d_type = 0;
	dirp->d_namlen = newent.d_namlen;
	memmove(dirp->d_name, idesc->id_name, (size_t)newent.d_namlen + 1);
#	if (BYTE_ORDER == LITTLE_ENDIAN)
		/*
		 * If the entry was split, dirscan() will only reverse the byte
		 * order of the original entry, and not the new one, before
		 * writing it back out.  So, we reverse the byte order here if
		 * necessary.
		 */
		if (oldlen != 0 && !newinofmt && !doinglevel2 && !needswap) {
#	else
		if (oldlen != 0 && !newinofmt && !doinglevel2 && needswap) {
#	endif
			u_char tmp;

			tmp = dirp->d_namlen;
			dirp->d_namlen = dirp->d_type;
			dirp->d_type = tmp;
		}
	return (ALTERED|STOP);
}

static int
chgino(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;

	if (memcmp(dirp->d_name, idesc->id_name, (int)dirp->d_namlen + 1))
		return (KEEPON);
	dirp->d_ino = iswap32(idesc->id_parent);
	if (newinofmt)
		dirp->d_type = inoinfo(idesc->id_parent)->ino_type;
	else
		dirp->d_type = 0;
	return (ALTERED|STOP);
}

int
linkup(ino_t orphan, ino_t parentdir, char *name)
{
	union dinode *dp;
	int lostdir;
	ino_t oldlfdir;
	struct inodesc idesc;
	char tempname[BUFSIZ];
	int16_t nlink;
	uint16_t mode;

	memset(&idesc, 0, sizeof(struct inodesc));
	dp = ginode(orphan);
	mode = iswap16(DIP(dp, mode));
	nlink = iswap16(DIP(dp, nlink));
	lostdir = (mode & IFMT) == IFDIR;
	pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	pinode(orphan);
	if (preen  && DIP(dp, size) == 0)
		return (0);
	if (preen)
		printf(" (RECONNECTED)\n");
	else
		if (reply("RECONNECT") == 0) {
			markclean = 0;
			return (0);
		}
	if (parentdir != 0)
		inoinfo(parentdir)->ino_linkcnt++;
	if (lfdir == 0) {
		dp = ginode(UFS_ROOTINO);
		idesc.id_name = lfname;
		idesc.id_type = DATA;
		idesc.id_func = findino;
		idesc.id_number = UFS_ROOTINO;
		idesc.id_uid = iswap32(DIP(dp, uid));
		idesc.id_gid = iswap32(DIP(dp, gid));
		if ((ckinode(dp, &idesc) & FOUND) != 0) {
			lfdir = idesc.id_parent;
		} else {
			pwarn("NO lost+found DIRECTORY");
			if (preen || reply("CREATE")) {
				lfdir = allocdir(UFS_ROOTINO, (ino_t)0, lfmode);
				if (lfdir != 0) {
					if (makeentry(UFS_ROOTINO, lfdir, lfname) != 0) {
						numdirs++;
						if (preen)
							printf(" (CREATED)\n");
					} else {
						freedir(lfdir, UFS_ROOTINO);
						lfdir = 0;
						if (preen)
							printf("\n");
					}
				}
				if (lfdir != 0) {
					reparent(lfdir, UFS_ROOTINO);
				}
			}
		}
		if (lfdir == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			markclean = 0;
			return (0);
		}
	}
	dp = ginode(lfdir);
	mode = DIP(dp, mode);
	mode = iswap16(mode);
	if ((mode & IFMT) != IFDIR) {
		pfatal("lost+found IS NOT A DIRECTORY");
		if (reply("REALLOCATE") == 0) {
			markclean = 0;
			return (0);
		}
		oldlfdir = lfdir;
		lfdir = allocdir(UFS_ROOTINO, (ino_t)0, lfmode);
		if (lfdir == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			markclean = 0;
			return (0);
		}
		if ((changeino(UFS_ROOTINO, lfname, lfdir) & ALTERED) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			markclean = 0;
			return (0);
		}
		inodirty();
		reparent(lfdir, UFS_ROOTINO);
		idesc.id_type = ADDR;
		idesc.id_func = pass4check;
		idesc.id_number = oldlfdir;
		adjust(&idesc, inoinfo(oldlfdir)->ino_linkcnt + 1);
		inoinfo(oldlfdir)->ino_linkcnt = 0;
		dp = ginode(lfdir);
	}
	if (inoinfo(lfdir)->ino_state != DFOUND) {
		pfatal("SORRY. NO lost+found DIRECTORY\n\n");
		markclean = 0;
		return (0);
	}
	(void)lftempname(tempname, orphan);
	if (makeentry(lfdir, orphan, (name ? name : tempname)) == 0) {
		pfatal("SORRY. NO SPACE IN lost+found DIRECTORY");
		printf("\n\n");
		markclean = 0;
		return (0);
	}
	inoinfo(orphan)->ino_linkcnt--;
	if (lostdir) {
		if ((changeino(orphan, "..", lfdir) & ALTERED) == 0 &&
		    parentdir != (ino_t)-1)
			(void)makeentry(orphan, lfdir, "..");
		dp = ginode(lfdir);
		nlink = DIP(dp, nlink);
		DIP_SET(dp, nlink, iswap16(iswap16(nlink) + 1));
		inodirty();
		inoinfo(lfdir)->ino_linkcnt++;
		reparent(orphan, lfdir);
		pwarn("DIR I=%llu CONNECTED. ", (unsigned long long)orphan);
		if (parentdir != (ino_t)-1)
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
	union dinode *dp;

	dp = ginode(dir);
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = chgino;
	idesc.id_number = dir;
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	idesc.id_parent = newnum;	/* new value for name */
	idesc.id_uid = iswap32(DIP(dp, uid));
	idesc.id_gid = iswap32(DIP(dp, gid));
	return (ckinode(dp, &idesc));
}

/*
 * make an entry in a directory
 */
int
makeentry(ino_t parent, ino_t ino, const char *name)
{
	union dinode *dp;
	struct inodesc idesc;
	char pathbuf[MAXPATHLEN + 1];
	
	if (parent < UFS_ROOTINO || parent >= maxino ||
	    ino < UFS_ROOTINO || ino >= maxino)
		return (0);
	dp = ginode(parent);
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = mkentry;
	idesc.id_number = parent;
	idesc.id_parent = ino;	/* this is the inode to enter */
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	idesc.id_uid = iswap32(DIP(dp, uid));
	idesc.id_gid = iswap32(DIP(dp, gid));
	if (iswap64(DIP(dp, size)) % dirblksiz) {
		DIP_SET(dp, size,
		    iswap64(roundup(iswap64(DIP(dp, size)), dirblksiz)));
		inodirty();
	}
	if ((ckinode(dp, &idesc) & ALTERED) != 0)
		return (1);
	getpathname(pathbuf, sizeof(pathbuf), parent, parent);
	dp = ginode(parent);
	if (expanddir(dp, pathbuf) == 0)
		return (0);
	update_uquot(idesc.id_number, idesc.id_uid, idesc.id_gid,
	    btodb(sblock->fs_bsize), 0);
	return (ckinode(dp, &idesc) & ALTERED);
}

/*
 * Attempt to expand the size of a directory
 */
static int
expanddir(union dinode *dp, char *name)
{
	daddr_t lastbn, newblk, dirblk;
	struct bufarea *bp;
	char *cp;
#if DIRBLKSIZ > APPLEUFS_DIRBLKSIZ
	char firstblk[DIRBLKSIZ];
#else
	char firstblk[APPLEUFS_DIRBLKSIZ];
#endif
	struct ufs1_dinode *dp1 = NULL;
	struct ufs2_dinode *dp2 = NULL;

	if (is_ufs2)
		dp2 = &dp->dp2;
	else
		dp1 = &dp->dp1;

	lastbn = lblkno(sblock, iswap64(DIP(dp, size)));
	if (lastbn >= UFS_NDADDR - 1 || DIP(dp, db[lastbn]) == 0 ||
	    DIP(dp, size) == 0)
		return (0);
	if ((newblk = allocblk(sblock->fs_frag)) == 0)
		return (0);
	if (is_ufs2) {
		dp2->di_db[lastbn + 1] = dp2->di_db[lastbn];
		dp2->di_db[lastbn] = iswap64(newblk);
		dp2->di_size = iswap64(iswap64(dp2->di_size)+sblock->fs_bsize);
		dp2->di_blocks = iswap64(iswap64(dp2->di_blocks) +
		    btodb(sblock->fs_bsize));
		dirblk = iswap64(dp2->di_db[lastbn + 1]);
	} else {
		dp1->di_db[lastbn + 1] = dp1->di_db[lastbn];
		dp1->di_db[lastbn] = iswap32((int32_t)newblk);
		dp1->di_size = iswap64(iswap64(dp1->di_size)+sblock->fs_bsize);
		dp1->di_blocks = iswap32(iswap32(dp1->di_blocks) +
		    btodb(sblock->fs_bsize));
		dirblk = iswap32(dp1->di_db[lastbn + 1]);
	}
	bp = getdirblk(dirblk, sblksize(sblock, (daddr_t)DIP(dp, size), lastbn + 1));
	if (bp->b_errs)
		goto bad;
	memmove(firstblk, bp->b_un.b_buf, dirblksiz);
	bp = getdirblk(newblk, sblock->fs_bsize);
	if (bp->b_errs)
		goto bad;
	memmove(bp->b_un.b_buf, firstblk, dirblksiz);
	emptydir.dot_reclen = iswap16(dirblksiz);
	for (cp = &bp->b_un.b_buf[dirblksiz];
	     cp < &bp->b_un.b_buf[sblock->fs_bsize];
	     cp += dirblksiz)
		memmove(cp, &emptydir, sizeof emptydir);
	dirty(bp);
	bp = getdirblk(dirblk, sblksize(sblock, (daddr_t)DIP(dp, size), lastbn + 1));
	if (bp->b_errs)
		goto bad;
	memmove(bp->b_un.b_buf, &emptydir, sizeof emptydir);
	pwarn("NO SPACE LEFT IN %s", name);
	if (preen)
		printf(" (EXPANDED)\n");
	else if (reply("EXPAND") == 0)
		goto bad;
	dirty(bp);
	inodirty();
	return (1);
bad:
	if (is_ufs2) {
		dp2->di_db[lastbn] = dp2->di_db[lastbn + 1];
		dp2->di_db[lastbn + 1] = 0;
		dp2->di_size = iswap64(iswap64(dp2->di_size)-sblock->fs_bsize);
		dp2->di_blocks = iswap64(iswap64(dp2->di_blocks) -
		    btodb(sblock->fs_bsize));
	} else {
		dp1->di_db[lastbn] = dp1->di_db[lastbn + 1];
		dp1->di_db[lastbn + 1] = 0;
		dp1->di_size = iswap64(iswap64(dp1->di_size)-sblock->fs_bsize);
		dp1->di_blocks = iswap32(iswap32(dp1->di_blocks) -
		    btodb(sblock->fs_bsize));
	}
	freeblk(newblk, sblock->fs_frag);
	markclean = 0;
	return (0);
}

/*
 * allocate a new directory
 */
ino_t
allocdir(ino_t parent, ino_t request, int mode)
{
	ino_t ino;
	char *cp;
	union dinode *dp;
	struct bufarea *bp;
	struct inoinfo *inp;
	struct dirtemplate *dirp;
	daddr_t dirblk;

	ino = allocino(request, IFDIR|mode);
	if (ino < UFS_ROOTINO)
		return 0;
	update_uquot(ino, 0, 0, btodb(sblock->fs_fsize), 1);
	dirhead.dot_reclen = iswap16(12);
	dirhead.dotdot_reclen = iswap16(dirblksiz - 12);
	odirhead.dot_reclen = iswap16(12);
	odirhead.dotdot_reclen = iswap16(dirblksiz - 12);
	odirhead.dot_namlen = iswap16(1);
	odirhead.dotdot_namlen = iswap16(2);
	if (newinofmt)
		dirp = &dirhead;
	else
		dirp = (struct dirtemplate *)&odirhead;
	dirp->dot_ino = iswap32(ino);
	dirp->dotdot_ino = iswap32(parent);
	dp = ginode(ino);
	dirblk = is_ufs2 ? iswap64(dp->dp2.di_db[0])
		    : iswap32(dp->dp1.di_db[0]);
	bp = getdirblk(dirblk, sblock->fs_fsize);
	if (bp->b_errs) {
		freeino(ino);
		return (0);
	}
	memmove(bp->b_un.b_buf, dirp, sizeof(struct dirtemplate));
	emptydir.dot_reclen = iswap16(dirblksiz);
	for (cp = &bp->b_un.b_buf[dirblksiz];
	     cp < &bp->b_un.b_buf[sblock->fs_fsize];
	     cp += dirblksiz)
		memmove(cp, &emptydir, sizeof emptydir);
	dirty(bp);
	DIP_SET(dp, nlink, iswap16(2));
	inodirty();
	if (ino == UFS_ROOTINO) {
		inoinfo(ino)->ino_linkcnt = iswap16(DIP(dp, nlink));
		cacheino(dp, ino);
		return(ino);
	}
	if (inoinfo(parent)->ino_state != DSTATE &&
	    inoinfo(parent)->ino_state != DFOUND) {
		freeino(ino);
		return (0);
	}
	cacheino(dp, ino);
	inp = getinoinfo(ino);
	inp->i_parent = parent;
	inp->i_dotdot = parent;
	inoinfo(ino)->ino_state = inoinfo(parent)->ino_state;
	if (inoinfo(ino)->ino_state == DSTATE) {
		inoinfo(ino)->ino_linkcnt = iswap16(DIP(dp, nlink));
		inoinfo(parent)->ino_linkcnt++;
	}
	dp = ginode(parent);
	DIP_SET(dp, nlink, iswap16(iswap16(DIP(dp, nlink)) + 1));
	inodirty();
	return (ino);
}

/*
 * free a directory inode
 */
static void
freedir(ino_t ino, ino_t parent)
{
	union dinode *dp;

	if (ino != parent) {
		dp = ginode(parent);
		DIP_SET(dp, nlink, iswap16(iswap16(DIP(dp, nlink)) - 1));
		inodirty();
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

/*
 * Get a directory block.
 * Insure that it is held until another is requested.
 */
static struct bufarea *
getdirblk(daddr_t blkno, long size)
{

	if (pdirbp != 0)
		pdirbp->b_flags &= ~B_INUSE;
	pdirbp = getdatablk(blkno, size);
	return (pdirbp);
}
