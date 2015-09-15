/* $NetBSD: pass2.c,v 1.29 2015/09/15 15:01:22 dholland Exp $	 */

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

#define MINDIRSIZE	(sizeof (struct lfs_dirtemplate))

static int pass2check(struct inodesc *);
static int blksort(const void *, const void *);

void
pass2(void)
{
	union lfs_dinode *dp;
	struct uvnode *vp;
	struct inoinfo **inpp, *inp;
	struct inoinfo **inpend;
	struct inodesc curino;
	union lfs_dinode dino;
	char pathbuf[MAXPATHLEN + 1];
	uint16_t mode;
	unsigned ii;

	switch (statemap[ULFS_ROOTINO]) {

	case USTATE:
		pfatal("ROOT INODE UNALLOCATED");
		if (reply("ALLOCATE") == 0)
			err(EEXIT, "%s", "");
		if (allocdir(ULFS_ROOTINO, ULFS_ROOTINO, 0755) != ULFS_ROOTINO)
			err(EEXIT, "CANNOT ALLOCATE ROOT INODE");
		break;

	case DCLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		if (reply("REALLOCATE")) {
			freeino(ULFS_ROOTINO);
			if (allocdir(ULFS_ROOTINO, ULFS_ROOTINO, 0755) != ULFS_ROOTINO)
				err(EEXIT, "CANNOT ALLOCATE ROOT INODE");
			break;
		}
		if (reply("CONTINUE") == 0)
			err(EEXIT, "%s", "");
		break;

	case FSTATE:
	case FCLEAR:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("REALLOCATE")) {
			freeino(ULFS_ROOTINO);
			if (allocdir(ULFS_ROOTINO, ULFS_ROOTINO, 0755) != ULFS_ROOTINO)
				err(EEXIT, "CANNOT ALLOCATE ROOT INODE");
			break;
		}
		if (reply("FIX") == 0)
			errx(EEXIT, "%s", "");
		vp = vget(fs, ULFS_ROOTINO);
		dp = VTOD(vp);
		mode = lfs_dino_getmode(fs, dp);
		mode &= ~LFS_IFMT;
		mode |= LFS_IFDIR;
		lfs_dino_setmode(fs, dp, mode);
		inodirty(VTOI(vp));
		break;

	case DSTATE:
		break;

	default:
		errx(EEXIT, "BAD STATE %d FOR ROOT INODE", statemap[ULFS_ROOTINO]);
	}
	statemap[ULFS_WINO] = FSTATE;
	typemap[ULFS_WINO] = LFS_DT_WHT;
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
			inp->i_isize = roundup(MINDIRSIZE, LFS_DIRBLKSIZ);
			if (reply("FIX") == 1) {
				vp = vget(fs, inp->i_number);
				dp = VTOD(vp);
				lfs_dino_setsize(fs, dp, inp->i_isize);
				inodirty(VTOI(vp));
			}
		} else if ((inp->i_isize & (LFS_DIRBLKSIZ - 1)) != 0) {
			getpathname(pathbuf, sizeof(pathbuf), inp->i_number,
			    inp->i_number);
			pwarn("DIRECTORY %s: LENGTH %lu NOT MULTIPLE OF %d",
			    pathbuf, (unsigned long) inp->i_isize, LFS_DIRBLKSIZ);
			if (preen)
				printf(" (ADJUSTED)\n");
			inp->i_isize = roundup(inp->i_isize, LFS_DIRBLKSIZ);
			if (preen || reply("ADJUST") == 1) {
				vp = vget(fs, inp->i_number);
				dp = VTOD(vp);
				lfs_dino_setsize(fs, dp, inp->i_isize);
				inodirty(VTOI(vp));
			}
		}
		memset(&dino, 0, sizeof(dino));
		lfs_dino_setmode(fs, &dino, LFS_IFDIR);
		lfs_dino_setsize(fs, &dino, inp->i_isize);
		for (ii = 0; ii < inp->i_numblks / sizeof(inp->i_blks[0]) &&
			     ii < ULFS_NDADDR; ii++) {
			lfs_dino_setdb(fs, &dino, ii, inp->i_blks[ii]);
		}
		for (; ii < inp->i_numblks / sizeof(inp->i_blks[0]); ii++) {
			lfs_dino_setib(fs, &dino, ii - ULFS_NDADDR,
				       inp->i_blks[ii]);
		}
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
	struct lfs_direct *dirp = idesc->id_dirp;
	struct inoinfo *inp;
	int n, entrysize, ret = 0;
	union lfs_dinode *dp;
	const char *errmsg;
	struct lfs_direct proto;
	char namebuf[MAXPATHLEN + 1];
	char pathbuf[MAXPATHLEN + 1];

	/*
	 * check for "."
	 */
	if (idesc->id_entryno != 0)
		goto chk1;
	if (lfs_dir_getino(fs, dirp) != 0 && strcmp(dirp->d_name, ".") == 0) {
		if (lfs_dir_getino(fs, dirp) != idesc->id_number) {
			direrror(idesc->id_number, "BAD INODE NUMBER FOR '.'");
			if (reply("FIX") == 1) {
				lfs_dir_setino(fs, dirp, idesc->id_number);
				ret |= ALTERED;
			}
		}
		if (lfs_dir_gettype(fs, dirp) != LFS_DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '.'");
			if (reply("FIX") == 1) {
				lfs_dir_settype(fs, dirp, LFS_DT_DIR);
				ret |= ALTERED;
			}
		}
		goto chk1;
	}
	direrror(idesc->id_number, "MISSING '.'");
	lfs_dir_setino(fs, &proto, idesc->id_number);
	lfs_dir_settype(fs, &proto, LFS_DT_DIR);
	lfs_dir_setnamlen(fs, &proto, 1);
	entrysize = LFS_DIRECTSIZ(1);
	lfs_dir_setreclen(fs, &proto, entrysize);
	if (lfs_dir_getino(fs, dirp) != 0 && strcmp(dirp->d_name, "..") != 0) {
		pfatal("CANNOT FIX, FIRST ENTRY IN DIRECTORY CONTAINS %s\n",
		    dirp->d_name);
	} else if (lfs_dir_getreclen(fs, dirp) < entrysize) {
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
	} else if (lfs_dir_getreclen(fs, dirp) < 2 * entrysize) {
		/* convert this entry to a . entry */
		lfs_dir_setreclen(fs, &proto, lfs_dir_getreclen(fs, dirp));
		memcpy(dirp, &proto, sizeof(proto));
		/* 4 is entrysize - headersize (XXX: clean up) */
		(void) strlcpy(dirp->d_name, ".", 4);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	} else {
		/* split this entry and use the beginning for the . entry */
		n = lfs_dir_getreclen(fs, dirp) - entrysize;
		memcpy(dirp, &proto, sizeof(proto));
		/* XXX see case above */
		(void) strlcpy(dirp->d_name, ".", 4);
		idesc->id_entryno++;
		lncntp[lfs_dir_getino(fs, dirp)]--;
		dirp = LFS_NEXTDIR(fs, dirp);
		memset(dirp, 0, (size_t) n);
		lfs_dir_setreclen(fs, dirp, n);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
chk1:
	if (idesc->id_entryno > 1)
		goto chk2;
	inp = getinoinfo(idesc->id_number);
	lfs_dir_setino(fs, &proto, inp->i_parent);
	lfs_dir_settype(fs, &proto, LFS_DT_DIR);
	lfs_dir_setnamlen(fs, &proto, 2);
	entrysize = LFS_DIRECTSIZ(2);
	lfs_dir_setreclen(fs, &proto, entrysize);
	if (idesc->id_entryno == 0) {
		n = LFS_DIRSIZ(fs, dirp);
		if (lfs_dir_getreclen(fs, dirp) < n + entrysize)
			goto chk2;
		lfs_dir_setreclen(fs, &proto, lfs_dir_getreclen(fs, dirp) - n);
		lfs_dir_setreclen(fs, dirp, n);
		idesc->id_entryno++;
		lncntp[lfs_dir_getino(fs, dirp)]--;
		dirp = (struct lfs_direct *) ((char *) (dirp) + n);
		memset(dirp, 0, lfs_dir_getreclen(fs, &proto));
		lfs_dir_setreclen(fs, dirp, lfs_dir_getreclen(fs, &proto));
	}
	if (lfs_dir_getino(fs, dirp) != 0 && strcmp(dirp->d_name, "..") == 0) {
		inp->i_dotdot = lfs_dir_getino(fs, dirp);
		if (lfs_dir_gettype(fs, dirp) != LFS_DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '..'");
			lfs_dir_settype(fs, dirp, LFS_DT_DIR);
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		goto chk2;
	}
	if (lfs_dir_getino(fs, dirp) != 0 && strcmp(dirp->d_name, ".") != 0) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n",
		    dirp->d_name);
		inp->i_dotdot = (ino_t) - 1;
	} else if (lfs_dir_getreclen(fs, dirp) < entrysize) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
		inp->i_dotdot = (ino_t) - 1;
	} else if (inp->i_parent != 0) {
		/*
		 * We know the parent, so fix now.
		 */
		inp->i_dotdot = inp->i_parent;
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		lfs_dir_setreclen(fs, &proto, lfs_dir_getreclen(fs, dirp));
		memcpy(dirp, &proto, (size_t) entrysize);
		/* 4 is entrysize - headersize (XXX: clean up) */
		(void) strlcpy(proto.d_name, "..", 4);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
	idesc->id_entryno++;
	if (lfs_dir_getino(fs, dirp) != 0)
		lncntp[lfs_dir_getino(fs, dirp)]--;
	return (ret | KEEPON);
chk2:
	if (lfs_dir_getino(fs, dirp) == 0)
		return (ret | KEEPON);
	if (lfs_dir_getnamlen(fs, dirp) <= 2 &&
	    dirp->d_name[0] == '.' &&
	    idesc->id_entryno >= 2) {
		if (lfs_dir_getnamlen(fs, dirp) == 1) {
			direrror(idesc->id_number, "EXTRA '.' ENTRY");
			if (reply("FIX") == 1) {
				lfs_dir_setino(fs, dirp, 0);
				ret |= ALTERED;
			}
			return (KEEPON | ret);
		}
		if (dirp->d_name[1] == '.') {
			direrror(idesc->id_number, "EXTRA '..' ENTRY");
			if (reply("FIX") == 1) {
				lfs_dir_setino(fs, dirp, 0);
				ret |= ALTERED;
			}
			return (KEEPON | ret);
		}
	}
	idesc->id_entryno++;
	n = 0;
	if (lfs_dir_getino(fs, dirp) >= maxino) {
		fileerror(idesc->id_number, lfs_dir_getino(fs, dirp), "I OUT OF RANGE");
		n = reply("REMOVE");
	} else if (lfs_dir_getino(fs, dirp) == LFS_IFILE_INUM &&
	    idesc->id_number == ULFS_ROOTINO) {
		if (lfs_dir_gettype(fs, dirp) != LFS_DT_REG) {
			fileerror(idesc->id_number, lfs_dir_getino(fs, dirp),
			    "BAD TYPE FOR IFILE");
			if (reply("FIX") == 1) {
				lfs_dir_settype(fs, dirp, LFS_DT_REG);
				ret |= ALTERED;
			}
		}
	} else if (((lfs_dir_getino(fs, dirp) == ULFS_WINO && lfs_dir_gettype(fs, dirp) != LFS_DT_WHT) ||
		(lfs_dir_getino(fs, dirp) != ULFS_WINO && lfs_dir_gettype(fs, dirp) == LFS_DT_WHT))) {
		fileerror(idesc->id_number, lfs_dir_getino(fs, dirp), "BAD WHITEOUT ENTRY");
		if (reply("FIX") == 1) {
			lfs_dir_setino(fs, dirp, ULFS_WINO);
			lfs_dir_settype(fs, dirp, LFS_DT_WHT);
			ret |= ALTERED;
		}
	} else {
again:
		switch (statemap[lfs_dir_getino(fs, dirp)]) {
		case USTATE:
			if (idesc->id_entryno <= 2)
				break;
			fileerror(idesc->id_number, lfs_dir_getino(fs, dirp),
			    "UNALLOCATED");
			n = reply("REMOVE");
			break;

		case DCLEAR:
		case FCLEAR:
			if (idesc->id_entryno <= 2)
				break;
			if (statemap[lfs_dir_getino(fs, dirp)] == FCLEAR)
				errmsg = "DUP/BAD";
			else if (!preen)
				errmsg = "ZERO LENGTH DIRECTORY";
			else {
				n = 1;
				break;
			}
			fileerror(idesc->id_number, lfs_dir_getino(fs, dirp), errmsg);
			if ((n = reply("REMOVE")) == 1)
				break;
			dp = ginode(lfs_dir_getino(fs, dirp));
			statemap[lfs_dir_getino(fs, dirp)] =
			    (lfs_dino_getmode(fs, dp) & LFS_IFMT) == LFS_IFDIR ? DSTATE : FSTATE;
			lncntp[lfs_dir_getino(fs, dirp)] = lfs_dino_getnlink(fs, dp);
			goto again;

		case DSTATE:
		case DFOUND:
			inp = getinoinfo(lfs_dir_getino(fs, dirp));
			if (inp->i_parent != 0 && idesc->id_entryno > 2) {
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				getpathname(namebuf, sizeof(namebuf),
				    lfs_dir_getino(fs, dirp),
				    lfs_dir_getino(fs, dirp));
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
			if (lfs_dir_gettype(fs, dirp) != typemap[lfs_dir_getino(fs, dirp)]) {
				fileerror(idesc->id_number,
				    lfs_dir_getino(fs, dirp),
				    "BAD TYPE VALUE");
				if (debug)
					pwarn("dir has %d, typemap has %d\n",
						lfs_dir_gettype(fs, dirp), typemap[lfs_dir_getino(fs, dirp)]);
				lfs_dir_settype(fs, dirp, typemap[lfs_dir_getino(fs, dirp)]);
				if (reply("FIX") == 1)
					ret |= ALTERED;
			}
			lncntp[lfs_dir_getino(fs, dirp)]--;
			break;

		default:
			errx(EEXIT, "BAD STATE %d FOR INODE I=%ju",
			    statemap[lfs_dir_getino(fs, dirp)],
			    (uintmax_t)lfs_dir_getino(fs, dirp));
		}
	}
	if (n == 0)
		return (ret | KEEPON);
	lfs_dir_setino(fs, dirp, 0);
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
