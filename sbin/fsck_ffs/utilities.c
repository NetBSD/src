/*	$NetBSD: utilities.c,v 1.45.2.1 2004/04/27 17:51:23 jdc Exp $	*/

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
static char sccsid[] = "@(#)utilities.c	8.6 (Berkeley) 5/19/95";
#else
__RCSID("$NetBSD: utilities.c,v 1.45.2.1 2004/04/27 17:51:23 jdc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"

long	diskreads, totalreads;	/* Disk cache statistics */

static void rwerror __P((char *, daddr_t));

extern int returntosingle;

int
ftypeok(dp)
	union dinode *dp;
{
	switch (iswap16(DIP(dp, mode)) & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
	case IFIFO:
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", iswap16(DIP(dp, mode)));
		return (0);
	}
}

int
reply(question)
	char *question;
{
	int persevere;
	char c;

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	persevere = !strcmp(question, "CONTINUE");
	printf("\n");
	if (!persevere && (nflag || fswritefd < 0)) {
		printf("%s? no\n\n", question);
		resolved = 0;
		return (0);
	}
	if (yflag || (persevere && nflag)) {
		printf("%s? yes\n\n", question);
		return (1);
	}
	do	{
		printf("%s? [yn] ", question);
		(void) fflush(stdout);
		c = getc(stdin);
		while (c != '\n' && getc(stdin) != '\n') {
			if (feof(stdin)) {
				resolved = 0;
				return (0);
			}
		}
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	printf("\n");
	if (c == 'y' || c == 'Y')
		return (1);
	resolved = 0;
	return (0);
}

/*
 * Malloc buffers and set up cache.
 */
void
bufinit()
{
	struct bufarea *bp;
	long bufcnt, i;
	char *bufp;

	pbp = pdirbp = (struct bufarea *)0;
	bufp = malloc((unsigned int)sblock->fs_bsize);
	if (bufp == 0)
		errx(EEXIT, "cannot allocate buffer pool");
	cgblk.b_un.b_buf = bufp;
	initbarea(&cgblk);
	bufp = malloc((unsigned int)APPLEUFS_LABEL_SIZE);
	if (bufp == 0)
		errx(EEXIT, "cannot allocate buffer pool");
	appleufsblk.b_un.b_buf = bufp;
	initbarea(&appleufsblk);
	bufhead.b_next = bufhead.b_prev = &bufhead;
	bufcnt = MAXBUFSPACE / sblock->fs_bsize;
	if (bufcnt < MINBUFS)
		bufcnt = MINBUFS;
	for (i = 0; i < bufcnt; i++) {
		bp = (struct bufarea *)malloc(sizeof(struct bufarea));
		bufp = malloc((unsigned int)sblock->fs_bsize);
		if (bp == NULL || bufp == NULL) {
			if (i >= MINBUFS)
				break;
			errx(EEXIT, "cannot allocate buffer pool");
		}
		bp->b_un.b_buf = bufp;
		bp->b_prev = &bufhead;
		bp->b_next = bufhead.b_next;
		bufhead.b_next->b_prev = bp;
		bufhead.b_next = bp;
		initbarea(bp);
	}
	bufhead.b_size = i;	/* save number of buffers */
}

/*
 * Manage a cache of directory blocks.
 */
struct bufarea *
getdatablk(blkno, size)
	daddr_t blkno;
	long size;
{
	struct bufarea *bp;

	for (bp = bufhead.b_next; bp != &bufhead; bp = bp->b_next)
		if (bp->b_bno == fsbtodb(sblock, blkno))
			goto foundit;
	for (bp = bufhead.b_prev; bp != &bufhead; bp = bp->b_prev)
		if ((bp->b_flags & B_INUSE) == 0)
			break;
	if (bp == &bufhead)
		errx(EEXIT, "deadlocked buffer pool");
	/* fall through */
foundit:
	getblk(bp, blkno, size);
	bp->b_prev->b_next = bp->b_next;
	bp->b_next->b_prev = bp->b_prev;
	bp->b_prev = &bufhead;
	bp->b_next = bufhead.b_next;
	bufhead.b_next->b_prev = bp;
	bufhead.b_next = bp;
	bp->b_flags |= B_INUSE;
	return (bp);
}

void
getblk(bp, blk, size)
	struct bufarea *bp;
	daddr_t blk;
	long size;
{
	daddr_t dblk;

	dblk = fsbtodb(sblock, blk);
	totalreads++;
	if (bp->b_bno != dblk) {
		flush(fswritefd, bp);
		diskreads++;
		bp->b_errs = bread(fsreadfd, bp->b_un.b_buf, dblk, size);
		bp->b_bno = dblk;
		bp->b_size = size;
	}
}

void
flush(fd, bp)
	int fd;
	struct bufarea *bp;
{
	int i, j;
	struct csum *ccsp;

	if (!bp->b_dirty)
		return;
	if (bp->b_errs != 0)
		pfatal("WRITING %sZERO'ED BLOCK %lld TO DISK\n",
		    (bp->b_errs == bp->b_size / dev_bsize) ? "" : "PARTIALLY ",
		    (long long)bp->b_bno);
	bp->b_dirty = 0;
	bp->b_errs = 0;
	bwrite(fd, bp->b_un.b_buf, bp->b_bno, (long)bp->b_size);
	if (bp != &sblk)
		return;
	for (i = 0, j = 0; i < sblock->fs_cssize; i += sblock->fs_bsize, j++) {
		int size = sblock->fs_cssize - i < sblock->fs_bsize ?
			sblock->fs_cssize - i : sblock->fs_bsize;
		ccsp = (struct csum *)((char *)sblock->fs_csp + i);
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
		bwrite(fswritefd, (char *)ccsp,
		    fsbtodb(sblock, sblock->fs_csaddr + j * sblock->fs_frag),
		    size);
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
	}
}

static void
rwerror(mesg, blk)
	char *mesg;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %lld", mesg, (long long)blk);
	if (reply("CONTINUE") == 0)
		exit(EEXIT);
}

void
ckfini()
{
	struct bufarea *bp, *nbp;
	int ofsmodified, cnt = 0;

	if (fswritefd < 0) {
		(void)close(fsreadfd);
		return;
	}
	flush(fswritefd, &sblk);
	if (havesb && bflag != 0 &&
	    (preen || reply("UPDATE STANDARD SUPERBLOCK"))) {
		if (preen)
			pwarn("UPDATING STANDARD SUPERBLOCK\n");
		if (!is_ufs2 && (sblock->fs_old_flags & FS_FLAGS_UPDATED) == 0)
			sblk.b_bno = SBLOCK_UFS1 / dev_bsize;
		else
			sblk.b_bno = sblock->fs_sblockloc / dev_bsize;
		sbdirty();
		flush(fswritefd, &sblk);
	}
	flush(fswritefd, &appleufsblk);
	free(appleufsblk.b_un.b_buf);
	flush(fswritefd, &cgblk);
	free(cgblk.b_un.b_buf);
	for (bp = bufhead.b_prev; bp && bp != &bufhead; bp = nbp) {
		cnt++;
		flush(fswritefd, bp);
		nbp = bp->b_prev;
		free(bp->b_un.b_buf);
		free((char *)bp);
	}
	if (bufhead.b_size != cnt)
		errx(EEXIT, "Panic: lost %d buffers", bufhead.b_size - cnt);
	pbp = pdirbp = (struct bufarea *)0;
	if (markclean && (sblock->fs_clean & FS_ISCLEAN) == 0) {
		/*
		 * Mark the file system as clean, and sync the superblock.
		 */
		if (preen)
			pwarn("MARKING FILE SYSTEM CLEAN\n");
		else if (!reply("MARK FILE SYSTEM CLEAN"))
			markclean = 0;
		if (markclean) {
			sblock->fs_clean = FS_ISCLEAN;
			sblock->fs_pendingblocks = 0;
			sblock->fs_pendinginodes = 0;
			sbdirty();
			ofsmodified = fsmodified;
			flush(fswritefd, &sblk);
#if LITE2BORKEN
			fsmodified = ofsmodified;
#endif
			if (!preen)
				printf(
				    "\n***** FILE SYSTEM MARKED CLEAN *****\n");
		}
	}
	if (debug)
		printf("cache missed %ld of %ld (%d%%)\n", diskreads,
		    totalreads, (int)(diskreads * 100 / totalreads));
	(void)close(fsreadfd);
	(void)close(fswritefd);
}

int
bread(fd, buf, blk, size)
	int fd;
	char *buf;
	daddr_t blk;
	long size;
{
	char *cp;
	int i, errs;
	off_t offset;

	offset = blk;
	offset *= dev_bsize;
	if (lseek(fd, offset, 0) < 0)
		rwerror("SEEK", blk);
	else if (read(fd, buf, (int)size) == size)
		return (0);
	rwerror("READ", blk);
	if (lseek(fd, offset, 0) < 0)
		rwerror("SEEK", blk);
	errs = 0;
	memset(buf, 0, (size_t)size);
	printf("THE FOLLOWING DISK SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < size; i += secsize, cp += secsize) {
		if (read(fd, cp, (int)secsize) != secsize) {
			(void)lseek(fd, offset + i + secsize, 0);
			if (secsize != dev_bsize && dev_bsize != 1)
				printf(" %lld (%lld),",
				    (long long)((blk*dev_bsize + i) / secsize),
				    (long long)(blk + i / dev_bsize));
			else
				printf(" %lld,",
				    (long long)(blk + i / dev_bsize));
			errs++;
		}
	}
	printf("\n");
	return (errs);
}

void
bwrite(fd, buf, blk, size)
	int fd;
	char *buf;
	daddr_t blk;
	long size;
{
	int i;
	char *cp;
	off_t offset;

	if (fd < 0)
		return;
	offset = blk;
	offset *= dev_bsize;
	if (lseek(fd, offset, 0) < 0)
		rwerror("SEEK", blk);
	else if (write(fd, buf, (int)size) == size) {
		fsmodified = 1;
		return;
	}
	rwerror("WRITE", blk);
	if (lseek(fd, offset, 0) < 0)
		rwerror("SEEK", blk);
	printf("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < size; i += dev_bsize, cp += dev_bsize)
		if (write(fd, cp, (int)dev_bsize) != dev_bsize) {
			(void)lseek(fd, offset + i + dev_bsize, 0);
			printf(" %lld,", (long long)(blk + i / dev_bsize));
		}
	printf("\n");
	return;
}

/*
 * allocate a data block with the specified number of fragments
 */
daddr_t
allocblk(frags)
	long frags;
{
	int i, j, k, cg, baseblk;
	struct cg *cgp = cgrp;

	if (frags <= 0 || frags > sblock->fs_frag)
		return (0);
	for (i = 0; i < maxfsblock - sblock->fs_frag; i += sblock->fs_frag) {
		for (j = 0; j <= sblock->fs_frag - frags; j++) {
			if (testbmap(i + j))
				continue;
			for (k = 1; k < frags; k++)
				if (testbmap(i + j + k))
					break;
			if (k < frags) {
				j += k;
				continue;
			}
			cg = dtog(sblock, i + j);
			getblk(&cgblk, cgtod(sblock, cg), sblock->fs_cgsize);
			memcpy(cgp, cgblk.b_un.b_cg, sblock->fs_cgsize);
			if ((doswap && !needswap) || (!doswap && needswap))
				ffs_cg_swap(cgblk.b_un.b_cg, cgp, sblock);
			if (!cg_chkmagic(cgp, 0))
				pfatal("CG %d: ALLOCBLK: BAD MAGIC NUMBER\n",
				    cg);
			baseblk = dtogd(sblock, i + j);
			for (k = 0; k < frags; k++) {
				setbmap(i + j + k);
				clrbit(cg_blksfree(cgp, 0), baseblk + k);
			}
			n_blks += frags;
			if (frags == sblock->fs_frag)
				cgp->cg_cs.cs_nbfree--;
			else
				cgp->cg_cs.cs_nffree -= frags;
			cgdirty();
			return (i + j);
		}
	}
	return (0);
}

/*
 * Free a previously allocated block
 */
void
freeblk(blkno, frags)
	daddr_t blkno;
	long frags;
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	(void)pass4check(&idesc);
}

/*
 * Find a pathname
 */
void
getpathname(namebuf, namebuflen, curdir, ino)
	char *namebuf;
	size_t namebuflen;
	ino_t curdir, ino;
{
	int len;
	char *cp;
	struct inodesc idesc;
	static int busy = 0;
	struct inostat *info;

	if (curdir == ino && ino == ROOTINO) {
		(void)strlcpy(namebuf, "/", namebuflen);
		return;
	}
	info = inoinfo(curdir);
	if (busy || (info->ino_state != DSTATE && info->ino_state != DFOUND)) {
		(void)strlcpy(namebuf, "?", namebuflen);
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
	while (ino != ROOTINO) {
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
		if ((ckinode(ginode(idesc.id_number), &idesc)&FOUND) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		memmove(cp, namebuf, (size_t)len);
		*--cp = '/';
		if (cp < &namebuf[MAXNAMLEN])
			break;
		ino = idesc.id_number;
	}
	busy = 0;
	if (ino != ROOTINO)
		*--cp = '?';
	memmove(namebuf, cp, (size_t)(&namebuf[MAXPATHLEN] - cp));
}

void
catch(sig)
	int sig;
{
	if (!doinglevel2) {
		markclean = 0;
		ckfini();
	}
	exit(12);
}

/*
 * When preening, allow a single quit to signal
 * a special exit after filesystem checks complete
 * so that reboot sequence may be interrupted.
 */
void
catchquit(sig)
	int sig;
{
	int errsave = errno;

	printf("returning to single-user after file system check\n");
	returntosingle = 1;
	(void)signal(SIGQUIT, SIG_DFL);
	errno = errsave;
}

/*
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
void
voidquit(sig)
	int sig;
{
	int errsave = errno;

	sleep(1);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_DFL);
	errno = errsave;
}

/*
 * determine whether an inode should be fixed.
 */
int
dofix(idesc, msg)
	struct inodesc *idesc;
	char *msg;
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
		errx(EEXIT, "UNKNOWN INODESC FIX MODE %d", idesc->id_fix);
	}
	/* NOTREACHED */
	return (0);
}

void
copyback_cg(blk)
	struct bufarea *blk;
{

	memcpy(blk->b_un.b_cg, cgrp, sblock->fs_cgsize);
	if (needswap)
		ffs_cg_swap(cgrp, blk->b_un.b_cg, sblock);
}

void
infohandler(int sig)
{
	got_siginfo = 1;
}

/*
 * Look up state information for an inode.
 */
struct inostat *
inoinfo(ino_t inum)
{
	static struct inostat unallocated = { USTATE, 0, 0 };
	struct inostatlist *ilp;
	int iloff;

	if (inum > maxino)
		errx(EEXIT, "inoinfo: inumber %d out of range", inum);
	ilp = &inostathead[inum / sblock->fs_ipg];
	iloff = inum % sblock->fs_ipg;
	if (iloff >= ilp->il_numalloced)
		return (&unallocated);
	return (&ilp->il_stat[iloff]);
}

void
sb_oldfscompat_read(struct fs *fs, struct fs **fssave)
{
	if ((fs->fs_magic != FS_UFS1_MAGIC) ||
	    (fs->fs_old_flags & FS_FLAGS_UPDATED))
		return;

	/* Save a copy of fields that may be modified for compatibility */
	if (fssave) {
		if (!*fssave)
			*fssave = malloc(sizeof(struct fs));
		if (!*fssave)
			errx(EEXIT, "cannot allocate space for compat store");
		memmove(*fssave, fs, sizeof(struct fs));

		if (debug)
			printf("detected ufs1 superblock not yet updated for ufs2 kernels\n");

		if (doswap) {
			uint16_t postbl[256];
			int i, n;

			if (fs->fs_old_postblformat == FS_42POSTBLFMT)
				n = 256;
			else
				n = 128;

			/* extract the postbl from the unswapped superblock */
			if (!needswap)
				ffs_sb_swap(*fssave, *fssave);
			memmove(postbl, (&(*fssave)->fs_old_postbl_start),
			    n * sizeof(postbl[0]));
			if (!needswap)
				ffs_sb_swap(*fssave, *fssave);

			/* Now swap it */
			for (i=0; i < n; i++)
				postbl[i] = bswap16(postbl[i]);

			/* And put it back such that it will get correctly
			 * unscrambled if it is swapped again on the way out
			 */
			if (needswap)
				ffs_sb_swap(*fssave, *fssave);
			memmove((&(*fssave)->fs_old_postbl_start), postbl,
			    n * sizeof(postbl[0]));
			if (needswap)
				ffs_sb_swap(*fssave, *fssave);
		}

	}

	/* These fields will be overwritten by their
	 * original values in fs_oldfscompat_write, so it is harmless
	 * to modify them here.
	 */
	fs->fs_cstotal.cs_ndir =
	    fs->fs_old_cstotal.cs_ndir;
	fs->fs_cstotal.cs_nbfree =
	    fs->fs_old_cstotal.cs_nbfree;
	fs->fs_cstotal.cs_nifree =
	    fs->fs_old_cstotal.cs_nifree;
	fs->fs_cstotal.cs_nffree =
	    fs->fs_old_cstotal.cs_nffree;
	
	fs->fs_maxbsize = fs->fs_bsize;
	fs->fs_time = fs->fs_old_time;
	fs->fs_size = fs->fs_old_size;
	fs->fs_dsize = fs->fs_old_dsize;
	fs->fs_csaddr = fs->fs_old_csaddr;
	fs->fs_sblockloc = SBLOCK_UFS1;

	fs->fs_flags = fs->fs_old_flags;

	if (fs->fs_old_postblformat == FS_42POSTBLFMT) {
		fs->fs_old_nrpos = 8;
		fs->fs_old_npsect = fs->fs_old_nsect;
		fs->fs_old_interleave = 1;
		fs->fs_old_trackskew = 0;
	}
}

void
sb_oldfscompat_write(struct fs *fs, struct fs *fssave)
{
	if ((fs->fs_magic != FS_UFS1_MAGIC) ||
	    (fs->fs_old_flags & FS_FLAGS_UPDATED))
		return;

	fs->fs_old_flags = fs->fs_flags;
	fs->fs_old_time = fs->fs_time;
	fs->fs_old_cstotal.cs_ndir = fs->fs_cstotal.cs_ndir;
	fs->fs_old_cstotal.cs_nbfree = fs->fs_cstotal.cs_nbfree;
	fs->fs_old_cstotal.cs_nifree = fs->fs_cstotal.cs_nifree;
	fs->fs_old_cstotal.cs_nffree = fs->fs_cstotal.cs_nffree;

	fs->fs_flags = fssave->fs_flags;

	if (fs->fs_old_postblformat == FS_42POSTBLFMT) {
		fs->fs_old_nrpos = fssave->fs_old_nrpos;
		fs->fs_old_npsect = fssave->fs_old_npsect;
		fs->fs_old_interleave = fssave->fs_old_interleave;
		fs->fs_old_trackskew = fssave->fs_old_trackskew;
	}

	memmove(&fs->fs_old_postbl_start, &fssave->fs_old_postbl_start,
	    ((fs->fs_old_postblformat == FS_42POSTBLFMT) ?
	    512 : 256));
}
