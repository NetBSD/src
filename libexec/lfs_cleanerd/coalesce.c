/*      $NetBSD: coalesce.c,v 1.18.6.2 2013/01/23 00:05:27 yamt Exp $  */

/*-
 * Copyright (c) 2002, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <errno.h>
#include <err.h>

#include <syslog.h>

#include "bufcache.h"
#include "vnode.h"
#include "cleaner.h"
#include "kernelops.h"

extern int debug, do_mmap;

int log2int(int n)
{
	int log;

	log = 0;
	while (n > 0) {
		++log;
		n >>= 1;
	}
	return log - 1;
}

enum coalesce_returncodes {
	COALESCE_OK = 0,
	COALESCE_NOINODE,
	COALESCE_TOOSMALL,
	COALESCE_BADSIZE,
	COALESCE_BADBLOCKSIZE,
	COALESCE_NOMEM,
	COALESCE_BADBMAPV,
	COALESCE_BADMARKV,
	COALESCE_NOTWORTHIT,
	COALESCE_NOTHINGLEFT,
	COALESCE_EIO,

	COALESCE_MAXERROR
};

const char *coalesce_return[] = {
	"Successfully coalesced",
	"File not in use or inode not found",
	"Not large enough to coalesce",
	"Negative size",
	"Not enough blocks to account for size",
	"Malloc failed",
	"LFCNBMAPV failed",
	"Not broken enough to fix",
	"Too many blocks not found",
	"Too many blocks found in active segments",
	"I/O error",

	"No such error"
};

static struct ufs1_dinode *
get_dinode(struct clfs *fs, ino_t ino)
{
	IFILE *ifp;
	daddr_t daddr;
	struct ubuf *bp;
	struct ufs1_dinode *dip, *r;

	lfs_ientry(&ifp, fs, ino, &bp);
	daddr = ifp->if_daddr;
	brelse(bp, 0);

	if (daddr == 0x0)
		return NULL;

	bread(fs->clfs_devvp, daddr, fs->lfs_ibsize, NOCRED, 0, &bp);
	for (dip = (struct ufs1_dinode *)bp->b_data;
	     dip < (struct ufs1_dinode *)(bp->b_data + fs->lfs_ibsize); dip++)
		if (dip->di_inumber == ino) {
			r = (struct ufs1_dinode *)malloc(sizeof(*r));
			if (r == NULL)
				break;
			memcpy(r, dip, sizeof(*r));
			brelse(bp, 0);
			return r;
		}
	brelse(bp, 0);
	return NULL;
}

/*
 * Find out if this inode's data blocks are discontinuous; if they are,
 * rewrite them using markv.  Return the number of inodes rewritten.
 */
static int
clean_inode(struct clfs *fs, ino_t ino)
{
	BLOCK_INFO *bip = NULL, *tbip;
	CLEANERINFO cip;
	struct ubuf *bp;
	struct ufs1_dinode *dip;
	struct clfs_seguse *sup;
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;
	daddr_t toff;
	int i;
	int nb, onb, noff;
	int retval;
	int bps;

	dip = get_dinode(fs, ino);
	if (dip == NULL)
		return COALESCE_NOINODE;

	/* Compute file block size, set up for bmapv */
	onb = nb = lblkno(fs, dip->di_size);

	/* XXX for now, don't do any file small enough to have fragments */
	if (nb < UFS_NDADDR) {
		free(dip);
		return COALESCE_TOOSMALL;
	}

	/* Sanity checks */
#if 0	/* di_size is uint64_t -- this is a noop */
	if (dip->di_size < 0) {
		dlog("ino %d, negative size (%" PRId64 ")", ino, dip->di_size);
		free(dip);
		return COALESCE_BADSIZE;
	}
#endif
	if (nb > dip->di_blocks) {
		dlog("ino %d, computed blocks %d > held blocks %d", ino, nb,
		     dip->di_blocks);
		free(dip);
		return COALESCE_BADBLOCKSIZE;
	}

	bip = (BLOCK_INFO *)malloc(sizeof(BLOCK_INFO) * nb);
	if (bip == NULL) {
		syslog(LOG_WARNING, "ino %llu, %d blocks: %m",
		    (unsigned long long)ino, nb);
		free(dip);
		return COALESCE_NOMEM;
	}
	for (i = 0; i < nb; i++) {
		memset(bip + i, 0, sizeof(BLOCK_INFO));
		bip[i].bi_inode = ino;
		bip[i].bi_lbn = i;
		bip[i].bi_version = dip->di_gen;
		/* Don't set the size, but let lfs_bmap fill it in */
	}
	lim.blkiov = bip;
	lim.blkcnt = nb;
	if (kops.ko_fcntl(fs->clfs_ifilefd, LFCNBMAPV, &lim) < 0) { 
		syslog(LOG_WARNING, "%s: coalesce: LFCNBMAPV: %m",
		       fs->lfs_fsmnt);
		retval = COALESCE_BADBMAPV;
		goto out;
	}
#if 0
	for (i = 0; i < nb; i++) {
		printf("bi_size = %d, bi_ino = %d, "
		    "bi_lbn = %d, bi_daddr = %d\n",
		    bip[i].bi_size, bip[i].bi_inode, bip[i].bi_lbn,
		    bip[i].bi_daddr);
	}
#endif
	noff = toff = 0;
	for (i = 1; i < nb; i++) {
		if (bip[i].bi_daddr != bip[i - 1].bi_daddr + fs->lfs_frag)
			++noff;
		toff += abs(bip[i].bi_daddr - bip[i - 1].bi_daddr
		    - fs->lfs_frag) >> fs->lfs_fbshift;
	}

	/*
	 * If this file is not discontinuous, there's no point in rewriting it.
	 *
	 * Explicitly allow a certain amount of discontinuity, since large
	 * files will be broken among segments and medium-sized files
	 * can have a break or two and it's okay.
	 */
	if (nb <= 1 || noff == 0 || noff < log2int(nb) ||
	    segtod(fs, noff) * 2 < nb) {
		retval = COALESCE_NOTWORTHIT;
		goto out;
	} else if (debug)
		syslog(LOG_DEBUG, "ino %llu total discontinuity "
		    "%d (%lld) for %d blocks", (unsigned long long)ino,
		    noff, (long long)toff, nb);

	/* Search for blocks in active segments; don't move them. */
	for (i = 0; i < nb; i++) {
		if (bip[i].bi_daddr <= 0)
			continue;
		sup = &fs->clfs_segtab[dtosn(fs, bip[i].bi_daddr)];
		if (sup->flags & SEGUSE_ACTIVE)
			bip[i].bi_daddr = LFS_UNUSED_DADDR; /* 0 */
	}

	/*
	 * Get rid of any blocks we've marked dead.  If this is an older
	 * kernel that doesn't have bmapv fill in the block sizes, we'll
	 * toss everything here. 
	 */
	onb = nb;
	toss_old_blocks(fs, &bip, &nb, NULL);
	nb = i;

	/*
	 * We may have tossed enough blocks that it is no longer worthwhile
	 * to rewrite this inode.
	 */
	if (nb == 0 || onb - nb > log2int(onb)) {
		if (debug)
			syslog(LOG_DEBUG, "too many blocks tossed, not rewriting");
		retval = COALESCE_NOTHINGLEFT;
		goto out;
	}

	/*
	 * We are going to rewrite this inode.
	 * For any remaining blocks, read in their contents.
	 */
	for (i = 0; i < nb; i++) {
		bip[i].bi_bp = malloc(bip[i].bi_size);
		if (bip[i].bi_bp == NULL) {
			syslog(LOG_WARNING, "allocate block buffer size=%d: %m",
			    bip[i].bi_size);
			retval = COALESCE_NOMEM;
			goto out;
		}

		if (kops.ko_pread(fs->clfs_devfd, bip[i].bi_bp, bip[i].bi_size,
			  fsbtob(fs, bip[i].bi_daddr)) < 0) {
			retval = COALESCE_EIO;
			goto out;
		}
	}
	if (debug)
		syslog(LOG_DEBUG, "ino %llu markv %d blocks",
		    (unsigned long long)ino, nb);

	/*
	 * Write in segment-sized chunks.  If at any point we'd write more
	 * than half of the available segments, sleep until that's not
	 * true any more.
	 */
	bps = segtod(fs, 1);
	for (tbip = bip; tbip < bip + nb; tbip += bps) {
		do {
			bread(fs->lfs_ivnode, 0, fs->lfs_bsize, NOCRED, 0, &bp);
			cip = *(CLEANERINFO *)bp->b_data;
			brelse(bp, B_INVAL);

			if (cip.clean < 4) /* XXX magic number 4 */
				kops.ko_fcntl(fs->clfs_ifilefd,
				    LFCNSEGWAIT, NULL);
		} while(cip.clean < 4);

		lim.blkiov = tbip;
		lim.blkcnt = (tbip + bps < bip + nb ? bps : nb % bps);
		if (kops.ko_fcntl(fs->clfs_ifilefd, LFCNMARKV, &lim) < 0) {
			retval = COALESCE_BADMARKV;
			goto out;
		}
	}

	retval = COALESCE_OK;
out:
	free(dip);
	if (bip) {
		for (i = 0; i < onb; i++)
			if (bip[i].bi_bp)
				free(bip[i].bi_bp);
		free(bip);
	}
	return retval;
}

/*
 * Try coalescing every inode in the filesystem.
 * Return the number of inodes actually altered.
 */
int clean_all_inodes(struct clfs *fs)
{
	int i, r, maxino;
	int totals[COALESCE_MAXERROR];
	struct stat st;

	memset(totals, 0, sizeof(totals));

	fstat(fs->clfs_ifilefd, &st);
	maxino = fs->lfs_ifpb * (st.st_size >> fs->lfs_bshift) -
		fs->lfs_segtabsz - fs->lfs_cleansz;

	for (i = 0; i < maxino; i++) {
		r = clean_inode(fs, i);
		++totals[r];
	}

	for (i = 0; i < COALESCE_MAXERROR; i++)
		if (totals[i])
			syslog(LOG_DEBUG, "%s: %d", coalesce_return[i],
			       totals[i]);
	
	return totals[COALESCE_OK];
}

/*
 * Fork a child process to coalesce this fs.
 */
int
fork_coalesce(struct clfs *fs)
{
	static pid_t childpid;
	int num;

	/*
	 * If already running a coalescing child, don't start a new one.
	 */
	if (childpid) {
		if (waitpid(childpid, NULL, WNOHANG) == childpid)
			childpid = 0;
	}
	if (childpid && kill(childpid, 0) >= 0) {
		/* already running a coalesce process */
		if (debug)
			syslog(LOG_DEBUG, "coalescing already in progress");
		return 0;
	}

	/*
	 * Fork a child and let the child coalease
	 */
	childpid = fork();
	if (childpid < 0) {
		syslog(LOG_ERR, "%s: fork to coaleasce: %m", fs->lfs_fsmnt);
		return 0;
	} else if (childpid == 0) {
		syslog(LOG_NOTICE, "%s: new coalescing process, pid %d",
		       fs->lfs_fsmnt, getpid());
		num = clean_all_inodes(fs);
		syslog(LOG_NOTICE, "%s: coalesced %d discontiguous inodes",
		       fs->lfs_fsmnt, num);
		exit(0);
	}

	return 0;
}
