/*	$NetBSD: utilities.c,v 1.68 2023/01/14 12:12:50 christos Exp $	*/

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
__RCSID("$NetBSD: utilities.c,v 1.68 2023/01/14 12:12:50 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/quota2.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"
#include "exitvalues.h"

long	diskreads, totalreads;	/* Disk cache statistics */

static void rwerror(const char *, daddr_t);

int
ftypeok(union dinode *dp)
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
reply(const char *question)
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
bufinit(void)
{
	struct bufarea *bp;
	long bufcnt, i;
	char *bufp;

	pbp = pdirbp = (struct bufarea *)0;
	bufp = aligned_alloc(DEV_BSIZE, (unsigned int)sblock->fs_bsize);
	if (bufp == 0)
		errexit("cannot allocate buffer pool");
	cgblk.b_un.b_buf = bufp;
	initbarea(&cgblk);
#ifndef NO_APPLE_UFS
	bufp = aligned_alloc(DEV_BSIZE, (unsigned int)APPLEUFS_LABEL_SIZE);
	if (bufp == 0)
		errexit("cannot allocate buffer pool");
	appleufsblk.b_un.b_buf = bufp;
	initbarea(&appleufsblk);
#endif
	bufhead.b_next = bufhead.b_prev = &bufhead;
	bufcnt = MAXBUFSPACE / sblock->fs_bsize;
	if (bufcnt < MINBUFS)
		bufcnt = MINBUFS;
	for (i = 0; i < bufcnt; i++) {
		bp = malloc(sizeof(struct bufarea));
		bufp = aligned_alloc(DEV_BSIZE, (unsigned int)sblock->fs_bsize);
		if (bp == NULL || bufp == NULL) {
			if (i >= MINBUFS) {
				if (bp)
					free(bp);
				if (bufp)
					free(bufp);
				break;
			}
			errexit("cannot allocate buffer pool");
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
getdatablk(daddr_t blkno, long size)
{
	struct bufarea *bp;

	for (bp = bufhead.b_next; bp != &bufhead; bp = bp->b_next)
		if (bp->b_bno == FFS_FSBTODB(sblock, blkno))
			goto foundit;
	for (bp = bufhead.b_prev; bp != &bufhead; bp = bp->b_prev)
		if ((bp->b_flags & B_INUSE) == 0)
			break;
	if (bp == &bufhead)
		errexit("deadlocked buffer pool");
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
getblk(struct bufarea *bp, daddr_t blk, long size)
{
	daddr_t dblk;

	dblk = FFS_FSBTODB(sblock, blk);
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
flush(int fd, struct bufarea *bp)
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
		    FFS_FSBTODB(sblock, sblock->fs_csaddr + j * sblock->fs_frag),
		    size);
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
	}
}

static void
rwerror(const char *mesg, daddr_t blk)
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %lld", mesg, (long long)blk);
	if (reply("CONTINUE") == 0)
		exit(FSCK_EXIT_CHECK_FAILED);
}

void
ckfini(int noint)
{
	struct bufarea *bp, *nbp;
	int cnt = 0;

	if (!noint) {
		if (doinglevel2)
			return;
		markclean = 0;
	}

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
#ifndef NO_APPLE_UFS
	flush(fswritefd, &appleufsblk);
	free(appleufsblk.b_un.b_buf);
#endif
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
		errexit("Panic: lost %d buffers", bufhead.b_size - cnt);
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
			flush(fswritefd, &sblk);
			if (!preen)
				printf(
				    "\n***** FILE SYSTEM MARKED CLEAN *****\n");
		}
	}
	if (doing2ea) {
		printf("ENABLING EXTATTR SUPPORT\n");
		is_ufs2ea = 1;
		sbdirty();
		flush(fswritefd, &sblk);
	}
	if (doing2noea) {
		printf("DISABLING EXTATTR SUPPORT\n");
		is_ufs2ea = 0;
		sbdirty();
		flush(fswritefd, &sblk);
	}
	if (debug)
		printf("cache missed %ld of %ld (%d%%)\n", diskreads,
		    totalreads, (int)(diskreads * 100 / totalreads));
	cleanup_wapbl();
	(void)close(fsreadfd);
	(void)close(fswritefd);
}

int
bread(int fd, char *buf, daddr_t blk, long size)
{
	char *cp;
	int i, errs;
	off_t offset;

	offset = blk;
	offset *= dev_bsize;
	if ((pread(fd, buf, (int)size, offset) == size) &&
	    read_wapbl(buf, size, blk) == 0)
		return (0);
	rwerror("READ", blk);
	errs = 0;
	memset(buf, 0, (size_t)size);
	printf("THE FOLLOWING DISK SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < size; i += secsize, cp += secsize) {
		if (pread(fd, cp, (int)secsize, offset + i) != secsize) {
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
bwrite(int fd, char *buf, daddr_t blk, long size)
{
	int i;
	char *cp;
	off_t offset;

	if (fd < 0)
		return;
	offset = blk;
	offset *= dev_bsize;
	if (pwrite(fd, buf, (int)size, offset) == size) {
		fsmodified = 1;
		return;
	}
	rwerror("WRITE", blk);
	printf("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < size; i += dev_bsize, cp += dev_bsize)
		if (pwrite(fd, cp, (int)dev_bsize, offset + i) != dev_bsize)
			printf(" %lld,", (long long)(blk + i / dev_bsize));
	printf("\n");
	return;
}

/*
 * allocate a data block with the specified number of fragments
 */
daddr_t
allocblk(long frags)
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
			if (frags == sblock->fs_frag) {
				cgp->cg_cs.cs_nbfree--;
				sblock->fs_cstotal.cs_nbfree--;
				sblock->fs_cs(fs, cg).cs_nbfree--;
				ffs_clusteracct(sblock, cgp,
				    ffs_fragstoblks(sblock, baseblk), -1);
			} else {
				cgp->cg_cs.cs_nffree -= frags;
				sblock->fs_cstotal.cs_nffree -= frags;
				sblock->fs_cs(fs, cg).cs_nffree -= frags;
			}
			sbdirty();
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
freeblk(daddr_t blkno, long frags)
{
	struct inodesc idesc;

	memset(&idesc, 0, sizeof(idesc));
	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	(void)pass4check(&idesc);
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
	struct inostat *info;

	if (curdir == ino && ino == UFS_ROOTINO) {
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
	while (ino != UFS_ROOTINO) {
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
		if (cp < &namebuf[FFS_MAXNAMLEN])
			break;
		ino = idesc.id_number;
	}
	busy = 0;
	if (ino != UFS_ROOTINO)
		*--cp = '?';
	memmove(namebuf, cp, (size_t)(&namebuf[MAXPATHLEN] - cp));
}

/*
 * determine whether an inode should be fixed.
 */
int
dofix(struct inodesc *idesc, const char *msg)
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
		errexit("UNKNOWN INODESC FIX MODE %d", idesc->id_fix);
	}
	/* NOTREACHED */
	return (0);
}

void
copyback_cg(struct bufarea *blk)
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
	size_t iloff;

	if (inum > maxino)
		errexit("inoinfo: inumber %llu out of range",
		    (unsigned long long)inum);
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
			errexit("cannot allocate space for compat store");
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

struct uquot *
find_uquot(struct uquot_hash *uq_hash, uint32_t uid, int alloc)
{
	struct uquot *uq;
	SLIST_FOREACH(uq, &uq_hash[uid & q2h_hash_mask], uq_entries) {
		if (uq->uq_uid == uid)
			return uq;
	}
	if (!alloc)
		return NULL;
	uq = malloc(sizeof(struct uquot));
	if (uq == NULL)
		errexit("cannot allocate quota entry");
	memset(uq, 0, sizeof(struct uquot));
	uq->uq_uid = uid;
	SLIST_INSERT_HEAD(&uq_hash[uid & q2h_hash_mask], uq, uq_entries);
	return uq;
}

void
remove_uquot(struct uquot_hash *uq_hash, struct uquot *uq)
{
	SLIST_REMOVE(&uq_hash[uq->uq_uid & q2h_hash_mask],
	    uq, uquot, uq_entries);
}

void
update_uquot(ino_t inum, uid_t uid, gid_t gid, int64_t bchange, int64_t ichange)
{
	/* simple uquot cache: remember the last used */
	static struct uquot *uq_u = NULL;
	static struct uquot *uq_g = NULL;

	if (inum < UFS_ROOTINO)
		return;
	if (is_journal_inode(inum))
		return;
	if (is_quota_inode(inum))
		return;
	
	if (uquot_user_hash == NULL)
		return;
		
	if (uq_u == NULL || uq_u->uq_uid != uid)
		uq_u = find_uquot(uquot_user_hash, uid, 1);
	uq_u->uq_b += bchange;
	uq_u->uq_i += ichange;
	if (uq_g == NULL || uq_g->uq_uid != gid)
		uq_g = find_uquot(uquot_group_hash, gid, 1);
	uq_g->uq_b += bchange;    
	uq_g->uq_i += ichange;
}

int
is_quota_inode(ino_t inum)
{

	if ((sblock->fs_flags & FS_DOQUOTA2) == 0)
		return 0;

	if (sblock->fs_quota_magic != Q2_HEAD_MAGIC)
		return 0;
	
	if (sblock->fs_quotafile[USRQUOTA] == inum)
		return 1;

	if (sblock->fs_quotafile[GRPQUOTA] == inum) 
		return 1;

	return 0;
}
