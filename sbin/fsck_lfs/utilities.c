/* $NetBSD: utilities.c,v 1.41 2015/08/12 18:28:00 dholland Exp $	 */

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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>

#define buf ubuf
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "segwrite.h"

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"
#include "exitvalues.h"

long diskreads, totalreads;	/* Disk cache statistics */

extern off_t locked_queue_bytes;

int
ftypeok(union lfs_dinode * dp)
{
	switch (lfs_dino_getmode(fs, dp) & LFS_IFMT) {

	case LFS_IFDIR:
	case LFS_IFREG:
	case LFS_IFBLK:
	case LFS_IFCHR:
	case LFS_IFLNK:
	case LFS_IFSOCK:
	case LFS_IFIFO:
		return (1);

	default:
		if (debug)
			pwarn("bad file type 0%o\n", lfs_dino_getmode(fs, dp));
		return (0);
	}
}

int
reply(const char *question)
{
	int persevere;
	char c;

	if (preen)
		err(EXIT_FAILURE, "INTERNAL ERROR: GOT TO reply()");
	persevere = !strcmp(question, "CONTINUE");
	pwarn("\n");
	if (!persevere && nflag) {
		printf("%s? no\n\n", question);
		return (0);
	}
	if (yflag || (persevere && nflag)) {
		printf("%s? yes\n\n", question);
		return (1);
	}
	do {
		printf("%s? [yn] ", question);
		(void) fflush(stdout);
		c = getc(stdin);
		while (c != '\n' && getc(stdin) != '\n')
			if (feof(stdin))
				return (0);
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	printf("\n");
	if (c == 'y' || c == 'Y')
		return (1);
	return (0);
}

static void
write_superblocks(void)
{
	if (debug)
		pwarn("writing superblocks with lfs_idaddr = 0x%jx\n",
			(uintmax_t)lfs_sb_getidaddr(fs));
	lfs_writesuper(fs, lfs_sb_getsboff(fs, 0));
	lfs_writesuper(fs, lfs_sb_getsboff(fs, 1));
	fsmodified = 1;
}

void
ckfini(int markclean)
{
	if (locked_queue_bytes > 0) {
		if (preen || reply("WRITE CHANGES TO DISK") == 1) {
			if (preen == 0)
				pwarn("WRITING CHANGES TO DISK\n");
			lfs_segwrite(fs, SEGM_CKP);
			fsdirty = 0;
			fsmodified = 1;
		}
	}

	if (!nflag && (lfs_sb_getpflags(fs) & LFS_PF_CLEAN) == 0) {
		lfs_sb_setpflags(fs, lfs_sb_getpflags(fs) | LFS_PF_CLEAN);
		fsmodified = 1;
	}

	if (fsmodified && (preen || reply("UPDATE SUPERBLOCKS"))) {
		sbdirty();
		write_superblocks();
	}
	if (markclean && fsmodified) {
		/*
		 * Mark the file system as clean, and sync the superblock.
		 */
		if (preen)
			pwarn("MARKING FILE SYSTEM CLEAN\n");
		else if (!reply("MARK FILE SYSTEM CLEAN"))
			markclean = 0;
		if (markclean) {
			lfs_sb_setpflags(fs, lfs_sb_getpflags(fs) | LFS_PF_CLEAN);
			sbdirty();
			write_superblocks();
			if (!preen)
				printf(
					"\n***** FILE SYSTEM MARKED CLEAN *****\n");
		}
	}

	if (debug)
		bufstats();
	(void) close(fsreadfd);
}
/*
 * Free a previously allocated block
 */
void
freeblk(daddr_t blkno, long frags)
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	(void) pass4check(&idesc);
}
/*
 * Find a pathname
 */
void
getpathname(char *namebuf, size_t namebuflen, ino_t curdir, ino_t ino)
{
	int len;
	char *cp;
	struct inodesc idesc;
	static int busy = 0;

	if (curdir == ino && ino == ULFS_ROOTINO) {
		(void) strlcpy(namebuf, "/", namebuflen);
		return;
	}
	if (busy ||
	    (statemap[curdir] != DSTATE && statemap[curdir] != DFOUND)) {
		(void) strlcpy(namebuf, "?", namebuflen);
		return;
	}
	busy = 1;
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_fix = IGNORE;
	cp = &namebuf[MAXPATHLEN - 1];
	*cp = '\0';
	if (curdir != ino) {
		idesc.id_parent = curdir;
		goto namelookup;
	}
	while (ino != ULFS_ROOTINO) {
		idesc.id_number = ino;
		idesc.id_func = findino;
		idesc.id_name = "..";
		if ((ckinode(ginode(ino), &idesc) & FOUND) == 0)
			break;
namelookup:
		idesc.id_number = idesc.id_parent;
		idesc.id_parent = ino;
		idesc.id_func = findname;
		idesc.id_name = namebuf;
		if (ginode(idesc.id_number) == NULL)
			break;
		if ((ckinode(ginode(idesc.id_number), &idesc) & FOUND) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		memcpy(cp, namebuf, (size_t) len);
		*--cp = '/';
		if (cp < &namebuf[LFS_MAXNAMLEN])
			break;
		ino = idesc.id_number;
	}
	busy = 0;
	if (ino != ULFS_ROOTINO)
		*--cp = '?';
	memcpy(namebuf, cp, (size_t) (&namebuf[MAXPATHLEN] - cp));
}

/*
 * determine whether an inode should be fixed.
 */
int
dofix(struct inodesc * idesc, const char *msg)
{

	switch (idesc->id_fix) {

	case DONTKNOW:
		if (idesc->id_type == DATA)
			direrror(idesc->id_number, msg);
		else
			pwarn("%s", msg);
		if (preen) {
			printf(" (SALVAGED)\n");
			idesc->id_fix = FIX;
			return (ALTERED);
		}
		if (reply("SALVAGE") == 0) {
			idesc->id_fix = NOFIX;
			return (0);
		}
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		return (ALTERED);

	case NOFIX:
	case IGNORE:
		return (0);

	default:
		err(EEXIT, "UNKNOWN INODESC FIX MODE %d", idesc->id_fix);
	}
	/* NOTREACHED */
}
