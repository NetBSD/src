/*      $NetBSD: coalesce.c,v 1.33 2015/10/10 22:34:46 dholland Exp $  */

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
#include <assert.h>

#include <syslog.h>

#include "bufcache.h"
#include "vnode.h"
#include "cleaner.h"
#include "kernelops.h"

extern int debug, do_mmap;

/*
 * XXX return the arg to just int when/if we don't need it for
 * potentially huge block counts any more.
 */
static int
log2int(intmax_t n)
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

static union lfs_dinode *
get_dinode(struct clfs *fs, ino_t ino)
{
	IFILE *ifp;
	daddr_t daddr;
	struct ubuf *bp;
	union lfs_dinode *dip, *r;
	unsigned i;

	lfs_ientry(&ifp, fs, ino, &bp);
	daddr = lfs_if_getdaddr(fs, ifp);
	brelse(bp, 0);

	if (daddr == 0x0)
		return NULL;

	bread(fs->clfs_devvp, daddr, lfs_sb_getibsize(fs), 0, &bp);
	for (i = 0; i < LFS_INOPB(fs); i++) {
		dip = DINO_IN_BLOCK(fs, bp->b_data, i);
		if (lfs_dino_getinumber(fs, dip) == ino) {
			r = malloc(sizeof(*r));
			if (r == NULL)
				break;
			/*
			 * Don't just assign the union, as if we're
			 * 32-bit and it's the last inode in the block
			 * that will run off the end of the buffer.
			 */
			if (fs->lfs_is64) {
				r->u_64 = dip->u_64;
			} else {
				r->u_32 = dip->u_32;
			}
			brelse(bp, 0);
			return r;
		}
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
	union lfs_dinode *dip;
	struct clfs_seguse *sup;
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;
	daddr_t toff;
	int noff;
	blkcnt_t i, nb, onb;
	int retval;
	int bps;

	dip = get_dinode(fs, ino);
	if (dip == NULL)
		return COALESCE_NOINODE;

	/* Compute file block size, set up for bmapv */
	onb = nb = lfs_lblkno(fs, lfs_dino_getsize(fs, dip));

	/* XXX for now, don't do any file small enough to have fragments */
	if (nb < ULFS_NDADDR) {
		free(dip);
		return COALESCE_TOOSMALL;
	}

	/* Sanity checks */
#if 0	/* di_size is uint64_t -- this is a noop */
	if (lfs_dino_getsize(fs, dip) < 0) {
		dlog("ino %d, negative size (%" PRId64 ")", ino,
		     lfs_dino_getsize(fs, dip));
		free(dip);
		return COALESCE_BADSIZE;
	}
#endif
	if (nb > lfs_dino_getblocks(fs, dip)) {
		dlog("ino %ju, computed blocks %jd > held blocks %ju",
		     (uintmax_t)ino, (intmax_t)nb,
		     (uintmax_t)lfs_dino_getblocks(fs, dip));
		free(dip);
		return COALESCE_BADBLOCKSIZE;
	}

	/*
	 * XXX: We should really coalesce really large files in
	 * chunks, as there's substantial diminishing returns and
	 * mallocing huge amounts of memory just to get those returns
	 * is pretty silly. But that requires a big rework of this
	 * code. (On the plus side though then we can stop worrying
	 * about block counts > 2^31.)
	 */

	/* ugh, no DADDR_T_MAX */
	__CTASSERT(sizeof(daddr_t) == sizeof(int64_t));
	if (nb > INT64_MAX / sizeof(BLOCK_INFO)) {
		syslog(LOG_WARNING, "ino %ju, %jd blocks: array too large\n",
		       (uintmax_t)ino, (uintmax_t)nb);
		free(dip);
		return COALESCE_NOMEM;
	}

	bip = (BLOCK_INFO *)malloc(sizeof(BLOCK_INFO) * nb);
	if (bip == NULL) {
		syslog(LOG_WARNING, "ino %llu, %jd blocks: %s\n",
		    (unsigned long long)ino, (intmax_t)nb,
		    strerror(errno));
		free(dip);
		return COALESCE_NOMEM;
	}
	for (i = 0; i < nb; i++) {
		memset(bip + i, 0, sizeof(BLOCK_INFO));
		bip[i].bi_inode = ino;
		bip[i].bi_lbn = i;
		bip[i].bi_version = lfs_dino_getgen(fs, dip);
		/* Don't set the size, but let lfs_bmap fill it in */
	}
	/*
	 * The kernel also contains this check; but as lim.blkcnt is
	 * only 32 bits wide, we need to check ourselves too in case
	 * we'd otherwise truncate a value > 2^31, as that might
	 * succeed and create bizarre results.
	 */
	if (nb > LFS_MARKV_MAXBLKCNT) {
		syslog(LOG_WARNING, "%s: coalesce: LFCNBMAPV: Too large\n",
		       lfs_sb_getfsmnt(fs));
		retval = COALESCE_BADBMAPV;
		goto out;
	}
	lim.blkiov = bip;
	lim.blkcnt = nb;
	if (kops.ko_fcntl(fs->clfs_ifilefd, LFCNBMAPV, &lim) < 0) { 
		syslog(LOG_WARNING, "%s: coalesce: LFCNBMAPV: %m",
		       lfs_sb_getfsmnt(fs));
		retval = COALESCE_BADBMAPV;
		goto out;
	}
#if 0
	for (i = 0; i < nb; i++) {
		printf("bi_size = %d, bi_ino = %ju, "
		    "bi_lbn = %jd, bi_daddr = %jd\n",
		    bip[i].bi_size, (uintmax_t)bip[i].bi_inode,
		    (intmax_t)bip[i].bi_lbn,
		    (intmax_t)bip[i].bi_daddr);
	}
#endif
	noff = 0;
	toff = 0;
	for (i = 1; i < nb; i++) {
		if (bip[i].bi_daddr != bip[i - 1].bi_daddr + lfs_sb_getfrag(fs))
			++noff;
		toff += llabs(bip[i].bi_daddr - bip[i - 1].bi_daddr
		    - lfs_sb_getfrag(fs)) >> lfs_sb_getfbshift(fs);
	}

	/*
	 * If this file is not discontinuous, there's no point in rewriting it.
	 *
	 * Explicitly allow a certain amount of discontinuity, since large
	 * files will be broken among segments and medium-sized files
	 * can have a break or two and it's okay.
	 */
	if (nb <= 1 || noff == 0 || noff < log2int(nb) ||
	    lfs_segtod(fs, noff) * 2 < nb) {
		retval = COALESCE_NOTWORTHIT;
		goto out;
	} else if (debug)
		syslog(LOG_DEBUG, "ino %llu total discontinuity "
		    "%d (%jd) for %jd blocks", (unsigned long long)ino,
		    noff, (intmax_t)toff, (intmax_t)nb);

	/* Search for blocks in active segments; don't move them. */
	for (i = 0; i < nb; i++) {
		if (bip[i].bi_daddr <= 0)
			continue;
		sup = &fs->clfs_segtab[lfs_dtosn(fs, bip[i].bi_daddr)];
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
			syslog(LOG_WARNING, "allocate block buffer size=%d: %s\n",
			    bip[i].bi_size, strerror(errno));
			retval = COALESCE_NOMEM;
			goto out;
		}

		if (kops.ko_pread(fs->clfs_devfd, bip[i].bi_bp, bip[i].bi_size,
			  lfs_fsbtob(fs, bip[i].bi_daddr)) < 0) {
			retval = COALESCE_EIO;
			goto out;
		}
	}
	if (debug)
		syslog(LOG_DEBUG, "ino %ju markv %jd blocks",
		    (uintmax_t)ino, (intmax_t)nb);

	/*
	 * Write in segment-sized chunks.  If at any point we'd write more
	 * than half of the available segments, sleep until that's not
	 * true any more.
	 *
	 * XXX the pointer arithmetic in this loop is illegal; replace
	 * TBIP with an integer (blkcnt_t) offset.
	 */
	bps = lfs_segtod(fs, 1);
	for (tbip = bip; tbip < bip + nb; tbip += bps) {
		do {
			bread(fs->lfs_ivnode, 0, lfs_sb_getbsize(fs), 0, &bp);
			cip = *(CLEANERINFO *)bp->b_data;
			brelse(bp, B_INVAL);

			if (lfs_ci_getclean(fs, &cip) < 4) /* XXX magic number 4 */
				kops.ko_fcntl(fs->clfs_ifilefd,
				    LFCNSEGWAIT, NULL);
		} while (lfs_ci_getclean(fs, &cip) < 4);

		/*
		 * Note that although lim.blkcnt is 32 bits wide, bps
		 * (which is blocks-per-segment) is < 2^32 so the
		 * value assigned here is always in range.
		 */
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
	maxino = lfs_sb_getifpb(fs) * (st.st_size >> lfs_sb_getbshift(fs)) -
		lfs_sb_getsegtabsz(fs) - lfs_sb_getcleansz(fs);

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
		syslog(LOG_ERR, "%s: fork to coaleasce: %m", lfs_sb_getfsmnt(fs));
		return 0;
	} else if (childpid == 0) {
		syslog(LOG_NOTICE, "%s: new coalescing process, pid %d",
		       lfs_sb_getfsmnt(fs), getpid());
		num = clean_all_inodes(fs);
		syslog(LOG_NOTICE, "%s: coalesced %d discontiguous inodes",
		       lfs_sb_getfsmnt(fs), num);
		exit(0);
	}

	return 0;
}
