/*	$NetBSD: pass2.c,v 1.36 2003/08/07 10:04:21 agc Exp $	*/

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
static char sccsid[] = "@(#)pass2.c	8.9 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: pass2.c,v 1.36 2003/08/07 10:04:21 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

#define MINDIRSIZE	(sizeof (struct dirtemplate))

static int blksort __P((const void *, const void *));
static int pass2check __P((struct inodesc *));

void
pass2()
{
	union dinode *dp;
	struct inoinfo **inpp, *inp, *pinp;
	struct inoinfo **inpend;
	struct inostat *rinfo, *info;
	struct inodesc curino;
	union dinode dino;
	int i, maxblk;
	char pathbuf[MAXPATHLEN + 1];

	rinfo = inoinfo(ROOTINO);
	switch (rinfo->ino_state) {

	case USTATE:
		pfatal("ROOT INODE UNALLOCATED");
		if (reply("ALLOCATE") == 0) {
			markclean = 0;
			ckfini();
			exit(EEXIT);
		}
		if (allocdir(ROOTINO, ROOTINO, 0755) != ROOTINO)
			errx(EEXIT, "CANNOT ALLOCATE ROOT INODE");
		break;

	case DCLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		if (reply("REALLOCATE")) {
			freeino(ROOTINO);
			if (allocdir(ROOTINO, ROOTINO, 0755) != ROOTINO)
				errx(EEXIT, "CANNOT ALLOCATE ROOT INODE");
			break;
		}
		markclean = 0;
		if (reply("CONTINUE") == 0) {
			ckfini();
			exit(EEXIT);
		}
		break;

	case FSTATE:
	case FCLEAR:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("REALLOCATE")) {
			freeino(ROOTINO);
			if (allocdir(ROOTINO, ROOTINO, 0755) != ROOTINO)
				errx(EEXIT, "CANNOT ALLOCATE ROOT INODE");
			break;
		}
		if (reply("FIX") == 0) {
			markclean = 0;
			ckfini();
			exit(EEXIT);
		}
		dp = ginode(ROOTINO);
		DIP(dp, mode) =
		    iswap16((iswap16(DIP(dp, mode)) & ~IFMT) | IFDIR);
		inodirty();
		break;

	case DSTATE:
		break;

	default:
		errx(EEXIT, "BAD STATE %d FOR ROOT INODE", rinfo->ino_state);
	}
	if (newinofmt) {
		info = inoinfo(WINO);
		info->ino_state = FSTATE;
		info->ino_type = DT_WHT;
	}
	/*
	 * Sort the directory list into disk block order.
	 */
	qsort((char *)inpsort, (size_t)inplast, sizeof *inpsort, blksort);
	/*
	 * Check the integrity of each directory.
	 */
	memset(&curino, 0, sizeof(struct inodesc));
	curino.id_type = DATA;
	curino.id_func = pass2check;
	inpend = &inpsort[inplast];
	for (inpp = inpsort; inpp < inpend; inpp++) {
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 2: dir %ld of %d (%d%%)\n", cdevname(),
			    (long)(inpp - inpsort), (int)inplast, 
			    (int)((inpp - inpsort) * 100 / inplast));
			got_siginfo = 0;
		}
		inp = *inpp;
		if (inp->i_isize == 0)
			continue;
		if (inp->i_isize < MINDIRSIZE) {
			direrror(inp->i_number, "DIRECTORY TOO SHORT");
			inp->i_isize = roundup(MINDIRSIZE, dirblksiz);
			if (reply("FIX") == 1) {
				dp = ginode(inp->i_number);
				DIP(dp, size) = iswap64(inp->i_isize);
				inodirty();
			} else
				markclean = 0;
		} else if ((inp->i_isize & (dirblksiz - 1)) != 0) {
			getpathname(pathbuf, sizeof(pathbuf), inp->i_number,
			    inp->i_number);
			if (usedsoftdep)
				pfatal("%s %s: LENGTH %lld NOT MULTIPLE OF %d",
					"DIRECTORY", pathbuf,
					(long long)inp->i_isize, dirblksiz);
			else
				pwarn("%s %s: LENGTH %lld NOT MULTIPLE OF %d",
					"DIRECTORY", pathbuf,
					(long long)inp->i_isize, dirblksiz);
			if (preen)
				printf(" (ADJUSTED)\n");
			inp->i_isize = roundup(inp->i_isize, dirblksiz);
			if (preen || reply("ADJUST") == 1) {
				dp = ginode(inp->i_number);
				DIP(dp, size) = iswap64(inp->i_isize);
				inodirty();
			} else
				markclean = 0;
		}
		memset(&dino, 0, sizeof dino);
		dp = &dino;
		if (!is_ufs2) {
			dp->dp1.di_mode = iswap16(IFDIR);
			dp->dp1.di_size = iswap64(inp->i_isize);
			maxblk = inp->i_numblks < NDADDR ? inp->i_numblks :
			    NDADDR;
			for (i = 0; i < maxblk; i++)
				dp->dp1.di_db[i] = inp->i_blks[i];
			if (inp->i_numblks > NDADDR) {
				for (i = 0; i < NIADDR; i++)
					dp->dp1.di_ib[i] =
					    inp->i_blks[NDADDR + i];
			}
		} else {
			dp->dp2.di_mode = iswap16(IFDIR);
			dp->dp2.di_size = iswap64(inp->i_isize);
			maxblk = inp->i_numblks < NDADDR ? inp->i_numblks :
			    NDADDR;
			for (i = 0; i < maxblk; i++)
				dp->dp2.di_db[i] = inp->i_blks[i];
			if (inp->i_numblks > NDADDR) {
				for (i = 0; i < NIADDR; i++)
					dp->dp2.di_ib[i] =
					    inp->i_blks[NDADDR + i];
			}
		}
		curino.id_number = inp->i_number;
		curino.id_parent = inp->i_parent;
		(void)ckinode(&dino, &curino);
	}

	/*
	 * Byte swapping in directory entries, if needed, has been done.
	 * Now rescan dirs for pass2check()
	 */
	if (do_dirswap) { 
		do_dirswap = 0;
		for (inpp = inpsort; inpp < inpend; inpp++) {
			inp = *inpp;
			if (inp->i_isize == 0)
				continue;
			memset(&dino, 0, sizeof dino);
			if (!is_ufs2) {
				dino.dp1.di_mode = iswap16(IFDIR);
				dino.dp1.di_size = iswap64(inp->i_isize);
				for (i = 0; i < inp->i_numblks; i++)
					dino.dp1.di_db[i] = inp->i_blks[i];
			} else {
				dino.dp2.di_mode = iswap16(IFDIR);
				dino.dp2.di_size = iswap64(inp->i_isize);
				for (i = 0; i < inp->i_numblks; i++)
					dino.dp2.di_db[i] = inp->i_blks[i];
			}
			curino.id_number = inp->i_number;
			curino.id_parent = inp->i_parent;
			(void)ckinode(&dino, &curino);
		}
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
		    inp->i_dotdot == (ino_t)-1)
			continue;
		info = inoinfo(inp->i_parent);
		if (inp->i_dotdot == 0) {
			inp->i_dotdot = inp->i_parent;
			fileerror(inp->i_parent, inp->i_number, "MISSING '..'");
			if (reply("FIX") == 0) {
				markclean = 0;
				continue;
			}
			(void)makeentry(inp->i_number, inp->i_parent, "..");
			info->ino_linkcnt--;
			continue;
		}
		fileerror(inp->i_parent, inp->i_number,
		    "BAD INODE NUMBER FOR '..'");
		if (reply("FIX") == 0) {
			markclean = 0;
			continue;
		}
		inoinfo(inp->i_dotdot)->ino_linkcnt++;
		info->ino_linkcnt--;
		inp->i_dotdot = inp->i_parent;
		(void)changeino(inp->i_number, "..", inp->i_parent);
	}
	/*
	 * Create a list of children for each directory.
	 */
	inpend = &inpsort[inplast];
	for (inpp = inpsort; inpp < inpend; inpp++) {
		inp = *inpp;
		info = inoinfo(inp->i_number);
		inp->i_child = inp->i_sibling = inp->i_parentp = 0;
		if (info->ino_state == DFOUND)
			info->ino_state = DSTATE;
	}
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
	/*
	 * Mark all the directories that can be found from the root.
	 */
	propagate(ROOTINO);
}

static int
pass2check(idesc)
	struct inodesc *idesc;
{
	struct direct *dirp = idesc->id_dirp;
	struct inoinfo *inp;
	struct inostat *info;
	int n, entrysize, ret = 0;
	union dinode *dp;
	char *errmsg;
	struct direct proto;
	char namebuf[MAXPATHLEN + 1];
	char pathbuf[MAXPATHLEN + 1];

	/*
	 * If converting, set directory entry type.
	 */
	if (!is_ufs2 && doinglevel2 && iswap32(dirp->d_ino) > 0 &&
	    iswap32(dirp->d_ino) < maxino) {
		dirp->d_type = inoinfo(iswap32(dirp->d_ino))->ino_type;
		ret |= ALTERED;
	}
	/* 
	 * check for "."
	 */
	if (idesc->id_entryno != 0)
		goto chk1;
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, ".") == 0) {
		if (iswap32(dirp->d_ino) != idesc->id_number) {
			direrror(idesc->id_number, "BAD INODE NUMBER FOR '.'");
			dirp->d_ino = iswap32(idesc->id_number);
			if (reply("FIX") == 1)
				ret |= ALTERED;
			else
				markclean = 0;
		}
		if (newinofmt && dirp->d_type != DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '.'");
			dirp->d_type = DT_DIR;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			else
				markclean = 0;
		}
		goto chk1;
	}
	direrror(idesc->id_number, "MISSING '.'");
	proto.d_ino = iswap32(idesc->id_number);
	if (newinofmt)
		proto.d_type = DT_DIR;
	else
		proto.d_type = 0;
	proto.d_namlen = 1;
	(void)strlcpy(proto.d_name, ".", sizeof(proto.d_name));
#	if BYTE_ORDER == LITTLE_ENDIAN
		if (!newinofmt && !needswap) {
#	else
		if (!newinofmt && needswap) {
#	endif
			u_char tmp;

			tmp = proto.d_type;
			proto.d_type = proto.d_namlen;
			proto.d_namlen = tmp;
		}
	entrysize = DIRSIZ(0, &proto, 0);
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") != 0) {
		pfatal("CANNOT FIX, FIRST ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
		markclean = 0;
	} else if (iswap16(dirp->d_reclen) < entrysize) {
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
		markclean = 0;
	} else if (iswap16(dirp->d_reclen) < 2 * entrysize) {
		proto.d_reclen = dirp->d_reclen;
		memmove(dirp, &proto, (size_t)entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
		else
			markclean = 0;
	} else {
		n = iswap16(dirp->d_reclen) - entrysize;
		proto.d_reclen = iswap16(entrysize);
		memmove(dirp, &proto, (size_t)entrysize);
		idesc->id_entryno++;
		inoinfo(iswap32(dirp->d_ino))->ino_linkcnt--;
		dirp = (struct direct *)((char *)(dirp) + entrysize);
		memset(dirp, 0, (size_t)n);
		dirp->d_reclen = iswap16(n);
		if (reply("FIX") == 1)
			ret |= ALTERED;
		else
			markclean = 0;
	}
chk1:
	if (idesc->id_entryno > 1)
		goto chk2;
	inp = getinoinfo(idesc->id_number);
	proto.d_ino = iswap32(inp->i_parent);
	if (newinofmt)
		proto.d_type = DT_DIR;
	else
		proto.d_type = 0;
	proto.d_namlen = 2;
	(void)strlcpy(proto.d_name, "..", sizeof(proto.d_name));
#if BYTE_ORDER == LITTLE_ENDIAN
	if (!newinofmt && !needswap) {
#else
	if (!newinofmt && needswap) {
#endif
		u_char tmp;

		tmp = proto.d_type;
		proto.d_type = proto.d_namlen;
		proto.d_namlen = tmp;
	}
	entrysize = DIRSIZ(0, &proto, 0);
	if (idesc->id_entryno == 0) {
		n = DIRSIZ(0, dirp, 0);
		if (iswap16(dirp->d_reclen) < n + entrysize)
			goto chk2;
		proto.d_reclen = iswap16(iswap16(dirp->d_reclen) - n);
		dirp->d_reclen = iswap16(n);
		idesc->id_entryno++;
		inoinfo(iswap32(dirp->d_ino))->ino_linkcnt--;
		dirp = (struct direct *)((char *)(dirp) + n);
		memset(dirp, 0, (size_t)iswap16(proto.d_reclen));
		dirp->d_reclen = proto.d_reclen;
	}
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") == 0) {
		inp->i_dotdot = iswap32(dirp->d_ino);
		if (newinofmt && dirp->d_type != DT_DIR) {
			direrror(idesc->id_number, "BAD TYPE VALUE FOR '..'");
			dirp->d_type = DT_DIR;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			else
				markclean = 0;
		}
		goto chk2;
	}
	if (iswap32(dirp->d_ino) != 0 && strcmp(dirp->d_name, ".") != 0) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
		inp->i_dotdot = (ino_t)-1;
		markclean = 0;
	} else if (iswap16(dirp->d_reclen) < entrysize) {
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
		inp->i_dotdot = (ino_t)-1;
		markclean = 0;
	} else if (inp->i_parent != 0) {
		/*
		 * We know the parent, so fix now.
		 */
		inp->i_dotdot = inp->i_parent;
		fileerror(inp->i_parent, idesc->id_number, "MISSING '..'");
		proto.d_reclen = dirp->d_reclen;
		memmove(dirp, &proto, (size_t)entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
		else
			markclean = 0;
	}
	idesc->id_entryno++;
	if (dirp->d_ino != 0)
		inoinfo(iswap32(dirp->d_ino))->ino_linkcnt--;
	return (ret|KEEPON);
chk2:
	if (dirp->d_ino == 0)
		return (ret|KEEPON);
	if (dirp->d_namlen <= 2 &&
	    dirp->d_name[0] == '.' &&
	    idesc->id_entryno >= 2) {
		if (dirp->d_namlen == 1) {
			direrror(idesc->id_number, "EXTRA '.' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			else
				markclean = 0;
			return (KEEPON | ret);
		}
		if (dirp->d_name[1] == '.') {
			direrror(idesc->id_number, "EXTRA '..' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			else
				markclean = 0;
			return (KEEPON | ret);
		}
	}
	idesc->id_entryno++;
	n = 0;
	if (iswap32(dirp->d_ino) > maxino) {
		fileerror(idesc->id_number, dirp->d_ino, "I OUT OF RANGE");
		n = reply("REMOVE");
		if (n == 0)
			markclean = 0;
	} else if (newinofmt &&
		   ((iswap32(dirp->d_ino) == WINO && dirp->d_type != DT_WHT) ||
		    (iswap32(dirp->d_ino) != WINO && dirp->d_type == DT_WHT))) {
		fileerror(idesc->id_number, iswap32(dirp->d_ino), "BAD WHITEOUT ENTRY");
		dirp->d_ino = iswap32(WINO);
		dirp->d_type = DT_WHT;
		if (reply("FIX") == 1)
			ret |= ALTERED;
		else
			markclean = 0;
	} else {
again:
		info = inoinfo(iswap32(dirp->d_ino));
		switch (info->ino_state) {
		case USTATE:
			if (idesc->id_entryno <= 2)
				break;
			fileerror(idesc->id_number, iswap32(dirp->d_ino), "UNALLOCATED");
			n = reply("REMOVE");
			if (n == 0)
				markclean = 0;
			break;

		case DCLEAR:
		case FCLEAR:
			if (idesc->id_entryno <= 2)
				break;
			if (info->ino_state == FCLEAR)
				errmsg = "DUP/BAD";
			else if (!preen && !usedsoftdep)
				errmsg = "ZERO LENGTH DIRECTORY";
			else {
				n = 1;
				break;
			}
			fileerror(idesc->id_number, iswap32(dirp->d_ino), errmsg);
			if ((n = reply("REMOVE")) == 1)
				break;
			dp = ginode(iswap32(dirp->d_ino));
			info->ino_state =
			    (iswap16(DIP(dp, mode)) & IFMT) == IFDIR ? DSTATE : FSTATE;
			info->ino_linkcnt = iswap16(DIP(dp, nlink));
			goto again;

		case DSTATE:
		case DFOUND:
			inp = getinoinfo(iswap32(dirp->d_ino));
			if (inp->i_parent != 0 && idesc->id_entryno > 2) {
				getpathname(pathbuf, sizeof(pathbuf),
				    idesc->id_number, idesc->id_number);
				getpathname(namebuf, sizeof(namebuf),
				    iswap32(dirp->d_ino), iswap32(dirp->d_ino));
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
			if (newinofmt && dirp->d_type != info->ino_type) {
				fileerror(idesc->id_number, iswap32(dirp->d_ino),
				    "BAD TYPE VALUE");
				dirp->d_type = info->ino_type;
				if (reply("FIX") == 1)
					ret |= ALTERED;
				else
					markclean = 0;
			}
			info->ino_linkcnt--;
			break;

		default:
			errx(EEXIT, "BAD STATE %d FOR INODE I=%d",
			    info->ino_state, iswap32(dirp->d_ino));
		}
	}
	if (n == 0)
		return (ret|KEEPON);
	dirp->d_ino = 0;
	return (ret|KEEPON|ALTERED);
}

/*
 * Routine to sort disk blocks.
 */
static int
blksort(arg1, arg2)
	const void *arg1, *arg2;
{

	return ((*(struct inoinfo **)arg1)->i_blks[0] -
		(*(struct inoinfo **)arg2)->i_blks[0]);
}
