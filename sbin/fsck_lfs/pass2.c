/* $NetBSD: pass2.c,v 1.17.48.1 2013/02/25 00:28:07 tls Exp $	 */

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
#include <sys/mount.h>
#include <sys/buf.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/lfs/lfs.h>

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

#define MINDIRSIZE	(sizeof (struct dirtemplate))

static int pass2check(struct inodesc *);
static int blksort(const void *, const void *);

void
pass2(void)
{
	struct ufs1_dinode *dp;
	struct uvnode *vp;
	struct inoinfo **inpp, *inp;
	struct inoinfo **inpend;
	struct inodesc curino;
	struct ufs1_dinode dino;
	char pathbuf[MAXPATHLEN + 1];

	switch (statemap[UFS_ROOTINO]) {

	case USTATE:
		pfatal("ROOT INODE UNALLOCATED");
		if (reply("ALLOCATE") == 0)
			err(EEXIT, "%s", "");
		if (allocdir(UFS_ROOTINO, UFS_ROOTINO, 0755) != UFS_ROOTINO)
			err(EEXIT, "CANNOT ALLOCATE ROOT INODE\n");
		break;

	case DCLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		if (reply("REALLOCATE")) {
			freeino(UFS_ROOTINO);
			if (allocdir(UFS_ROOTINO, UFS_ROOTINO, 0755) != UFS_ROOTINO)
				err(EEXIT, "CANNOT ALLOCATE ROOT INODE\n");
			break;
		}
		if (reply("CONTINUE") == 0)
			err(EEXIT, "%s", "");
		break;

	case FSTATE:
	case FCLEAR:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("REALLOCATE")) {
			freeino(UFS_ROOTINO);
			if (allocdir(UFS_ROOTINO, UFS_ROOTINO, 0755) != UFS_ROOTINO)
				err(EEXIT, "CANNOT ALLOCATE ROOT INODE\n");
			break;
		}
		if (reply("FIX") == 0)
			errx(EEXIT, "%s", "");
		vp = vget(fs, UFS_ROOTINO);
		dp = VTOD(vp);
		dp->di_mode &= ~IFMT;
		dp->di_mode |= IFDIR;
		inodirty(VTOI(vp));
		break;

	case DSTATE:
		break;

	default:
		errx(EEXIT, "BAD STATE %d FOR ROOT INODE\n", statemap[UFS_ROOTINO]);
	}
	statemap[UFS_WINO] = FSTATE;
	typemap[UFS_WINO] = DT_WHT;
	/*
	 * Sort the directory list into disk block order.
	 */
	qsort((char *) inpsort, (size_t) inplast, sizeof *inpsort, blksort);
	/*
	 * Check the integrity of each directory.
	 */
	memset(&curino, 0, sizeof(struct inodesc));
	curino.id_type = DATA;
	curino.id_func = pass2check;
	inpend = &inpsort[inplast];
	for (inpp = inpsort; inpp < inpend; inpp++) {
		inp = *inpp;
		if (inp->i_isize == 0)
			continue;
		if (inp->i_isize < MINDIRSIZE) {
			direrror(inp->i_number, "DIRECTORY TOO SHORT");
			inp->i_isize = roundup(MINDIRSIZE, DIRBLKSIZ);
			if (reply("FIX") == 1) {
				vp = vget(fs, inp->i_number);
				dp = VTOD(vp);
				dp->di_size = inp->i_isize;
				inodirty(VTOI(vp));
			}
		} else if ((inp->i_isize & (DIRBLKSIZ - 1)) != 0) {
			getpathname(pathbuf, sizeof(pathbuf), inp->i_number,
			    inp->i_number);
			pwarn("DIRECTORY %s: LENGTH %lu NOT MULTIPLE OF %d",
			    pathbuf, (unsigned long) inp->i_isize, DIRBLKSIZ);
			if (preen)
				printf(" (ADJUSTED)\n");
			inp->i_isize = roundup(inp->i_isize, DIRBLKSIZ);
			if (preen || reply("ADJUST") == 1) {
				vp = vget(fs, inp->i_number);
				dp = VTOD(vp);
				dp->di_size = inp->i_isize;
				inodirty(VTOI(vp));
			}
		}
		memset(&dino, 0, sizeof(struct ufs1_dinode));
		dino.di_mode = IFDIR;
		dino.di_size = inp->i_isize;
		memcpy(&dino.di_db[0], &inp->i_blks[0], (size_t) inp->i_numblks);
		curino.id_number = inp->i_number;
		curino.id_parent = inp->i_parent;
		(void) ckinode(&dino, &curino);
	}
	/*
	 * Now that the parents of all directories have been found,
	 * make another pass to verify the value of `..'
	 */
	for (inpp = inpsort; inpp < inpend; inpp++) {
		inp = *inpp;
		if (inp->i_parent == 0 || inp->i_isize == 0)
			continue;
		if (inp->i_dotdot == inp->i_parent ||
		    inp->i_dotdot == (ino_t) - 1)
			continue;
		if (inp->i_dotdot == 0) {
			inp->i_dotdot = inp->i_parent;
			fileerror(inp->i_parent, inp->i_number, "MISSING '..'");
			if (reply("FIX") == 0)
				continue;
			(void) makeentry(inp->i_number, inp->i_parent, "..");
			lncntp[inp->i_parent]--;
			continue;
		}
		fileerror(inp->i_parent, inp->i_number,
		    "BAD INODE NUMBER FOR '..'");
		if (reply("FIX") == 0)
			continue;
		lncntp[inp->i_dotdot]++;
		lncntp[inp->i_parent]--;
		inp->i_dotdot = inp->i_parent;
		(void) changeino(inp->i_number, "..", inp->i_parent);
	}
	/*
	 * Mark all the directories that can be found from the root.
	 */
	propagate();
}

static int
pass2check(struct inodesc * idesc)
{
	struct direct *dirp = idesc->id_dirp;
	struct inoinfo *inp;
	int n, entrysize, ret = 0;
	struct ufs1_dinode *dp;
	const char *errmsg;
	struct direct proto;
	char namebuf[MAXPATHLEN + 1];
	char pathbuf[MAXPATHLEN + 1];

	/*
	 * check for "."
	 */
	if (idesc->id_entryno != 0)
		goto chk1;
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, ".") == 0) {
		if (dirp->d_ino != idesc->id_number) {
			direrror(idesc->id_number, "BAD INODE NUMBER FOR '.'");
			dirp->d_ino = idesc->id_number;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		if (dirp->d_type != DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '.'");
			dirp->d_type = DT_DIR;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		goto chk1;
	}
	direrror(idesc->id_number, "MISSING '.'");
	proto.d_ino = idesc->id_number;
	proto.d_type = DT_DIR;
	proto.d_namlen = 1;
	(void) strlcpy(proto.d_name, ".", sizeof(proto.d_name));
	entrysize = DIRSIZ(0, &proto, 0);
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") != 0) {
		pfatal("CANNOT FIX, FIRST ENTRY IN DIRECTORY CONTAINS %s\n",
		    dirp->d_name);
	} else if (dirp->d_reclen < entrysize) {
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
	} else if (dirp->d_reclen < 2 * entrysize) {
		proto.d_reclen = dirp->d_reclen;
		memcpy(dirp, &proto, (size_t) entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	} else {
		n = dirp->d_reclen - entrysize;
		proto.d_reclen = entrysize;
		memcpy(dirp, &proto, (size_t) entrysize);
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (struct direct *) ((char *) (dirp) + entrysize);
		memset(dirp, 0, (size_t) n);
		dirp->d_reclen = n;
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
chk1:
	if (idesc->id_entryno > 1)
		goto chk2;
	inp = getinoinfo(idesc->id_number);
	proto.d_ino = inp->i_parent;
	proto.d_type = DT_DIR;
	proto.d_namlen = 2;
	(void) strlcpy(proto.d_name, "..", sizeof(proto.d_name));
	entrysize = DIRSIZ(0, &proto, 0);
	if (idesc->id_entryno == 0) {
		n = DIRSIZ(0, dirp, 0);
		if (dirp->d_reclen < n + entrysize)
			goto chk2;
		proto.d_reclen = dirp->d_reclen - n;
		dirp->d_reclen = n;
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (struct direct *) ((char *) (dirp) + n);
		memset(dirp, 0, (size_t) proto.d_reclen);
		dirp->d_reclen = proto.d_reclen;
	}
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") == 0) {
		inp->i_dotdot = dirp->d_ino;
		if (dirp->d_type != DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '..'");
			dirp->d_type = DT_DIR;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		goto chk2;
	}
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, ".") != 0) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n",
		    dirp->d_name);
		inp->i_dotdot = (ino_t) - 1;
	} else if (dirp->d_reclen < entrysize) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
		inp->i_dotdot = (ino_t) - 1;
	} else if (inp->i_parent != 0) {
		/*
		 * We know the parent, so fix now.
		 */
		inp->i_dotdot = inp->i_parent;
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		proto.d_reclen = dirp->d_reclen;
		memcpy(dirp, &proto, (size_t) entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
	idesc->id_entryno++;
	if (dirp->d_ino != 0)
		lncntp[dirp->d_ino]--;
	return (ret | KEEPON);
chk2:
	if (dirp->d_ino == 0)
		return (ret | KEEPON);
	if (dirp->d_namlen <= 2 &&
	    dirp->d_name[0] == '.' &&
	    idesc->id_entryno >= 2) {
		if (dirp->d_namlen == 1) {
			direrror(idesc->id_number, "EXTRA '.' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			return (KEEPON | ret);
		}
		if (dirp->d_name[1] == '.') {
			direrror(idesc->id_number, "EXTRA '..' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			return (KEEPON | ret);
		}
	}
	idesc->id_entryno++;
	n = 0;
	if (dirp->d_ino >= maxino) {
		fileerror(idesc->id_number, dirp->d_ino, "I OUT OF RANGE");
		n = reply("REMOVE");
	} else if (dirp->d_ino == LFS_IFILE_INUM &&
	    idesc->id_number == UFS_ROOTINO) {
		if (dirp->d_type != DT_REG) {
			fileerror(idesc->id_number, dirp->d_ino,
			    "BAD TYPE FOR IFILE");
			dirp->d_type = DT_REG;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
	} else if (((dirp->d_ino == UFS_WINO && (dirp->d_type != DT_WHT)) ||
		(dirp->d_ino != UFS_WINO && dirp->d_type == DT_WHT))) {
		fileerror(idesc->id_number, dirp->d_ino, "BAD WHITEOUT ENTRY");
		dirp->d_ino = UFS_WINO;
		dirp->d_type = DT_WHT;
		if (reply("FIX") == 1)
			ret |= ALTERED;
	} else {
again:
		switch (statemap[dirp->d_ino]) {
		case USTATE:
			if (idesc->id_entryno <= 2)
				break;
			fileerror(idesc->id_number, dirp->d_ino, "UNALLOCATED");
			n = reply("REMOVE");
			break;

		case DCLEAR:
		case FCLEAR:
			if (idesc->id_entryno <= 2)
				break;
			if (statemap[dirp->d_ino] == FCLEAR)
				errmsg = "DUP/BAD";
			else if (!preen)
				errmsg = "ZERO LENGTH DIRECTORY";
			else {
				n = 1;
				break;
			}
			fileerror(idesc->id_number, dirp->d_ino, errmsg);
			if ((n = reply("REMOVE")) == 1)
				break;
			dp = ginode(dirp->d_ino);
			statemap[dirp->d_ino] =
			    (dp->di_mode & IFMT) == IFDIR ? DSTATE : FSTATE;
			lncntp[dirp->d_ino] = dp->di_nlink;
			goto again;

		case DSTATE:
		case DFOUND:
			inp = getinoinfo(dirp->d_ino);
			if (inp->i_parent != 0 && idesc->id_entryno > 2) {
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				getpathname(namebuf, sizeof(namebuf),
				    dirp->d_ino, dirp->d_ino);
				pwarn("%s %s %s\n", pathbuf,
				    "IS AN EXTRANEOUS HARD LINK TO DIRECTORY",
				    namebuf);
				if (preen)
					printf(" (IGNORED)\n");
				else if ((n = reply("REMOVE")) == 1)
					break;
			}
			if (idesc->id_entryno > 2)
				inp->i_parent = idesc->id_number;
			/* fall through */

		case FSTATE:
			if (dirp->d_type != typemap[dirp->d_ino]) {
				fileerror(idesc->id_number, dirp->d_ino,
				    "BAD TYPE VALUE");
				if (debug)
					pwarn("dir has %d, typemap has %d\n",
						dirp->d_type, typemap[dirp->d_ino]);
				dirp->d_type = typemap[dirp->d_ino];
				if (reply("FIX") == 1)
					ret |= ALTERED;
			}
			lncntp[dirp->d_ino]--;
			break;

		default:
			errx(EEXIT, "BAD STATE %d FOR INODE I=%d",
			    statemap[dirp->d_ino], dirp->d_ino);
		}
	}
	if (n == 0)
		return (ret | KEEPON);
	dirp->d_ino = 0;
	return (ret | KEEPON | ALTERED);
}
/*
 * Routine to sort disk blocks.
 */
static int
blksort(const void *inpp1, const void *inpp2)
{
	return ((*(const struct inoinfo *const *) inpp1)->i_blks[0] -
	    (*(const struct inoinfo *const *) inpp2)->i_blks[0]);
}
