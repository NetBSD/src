/*      $NetBSD: coalesce.c,v 1.9 2003/04/02 10:39:22 fvdl Exp $  */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "clean.h"

extern int debug, do_mmap;

static int
tossdead(const void *client, const void *a, const void *b)
{
	return (((BLOCK_INFO *)a)->bi_daddr <= 0 ||
		((BLOCK_INFO *)a)->bi_size == 0);
}

static int log2int(int n)
{
	int log;

	log = 0;
	while (n > 0) {
		++log;
		n /= 2;
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
	COALESCE_NOTWORTHIT,
	COALESCE_NOTHINGLEFT,
	COALESCE_NOTHINGLEFT2,
	COALESCE_EIO,

	COALESCE_MAXERROR
};

char *coalesce_return[] = {
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

/*
 * Find out if this inode's data blocks are discontinuous; if they are,
 * rewrite them using markv.  Return the number of inodes rewritten.
 */
int clean_inode(struct fs_info *fsp, ino_t ino)
{
	int i, error;
	BLOCK_INFO *bip = NULL, *tbip;
	struct ufs1_dinode *dip;
	int nb, onb, noff;
	daddr_t toff;
	struct lfs *lfsp;
	int bps;
	SEGUSE *sup;
	int retval;

	lfsp = &fsp->fi_lfs;

        dip = get_dinode(fsp, ino);
	if (dip == NULL)
		return COALESCE_NOINODE;

	/* Compute file block size, set up for bmapv */
	onb = nb = lblkno(lfsp, dip->di_size);

	/* XXX for now, don't do any file small enough to have fragments */
	if (nb < NDADDR)
		return COALESCE_TOOSMALL;

	/* Sanity checks */
	if (dip->di_size < 0) {
		if (debug)
			syslog(LOG_DEBUG, "ino %d, negative size (%lld)",
				ino, (long long)dip->di_size);
		return COALESCE_BADSIZE;
	}
	if (nb > dip->di_blocks) {
		if (debug)
			syslog(LOG_DEBUG, "ino %d, computed blocks %d > held blocks %d",
				ino, nb, dip->di_blocks);
		return COALESCE_BADBLOCKSIZE;
	}

	bip = (BLOCK_INFO *)malloc(sizeof(BLOCK_INFO) * nb);
	if (bip == NULL) {
		syslog(LOG_WARNING, "ino %d, %d blocks: %m", ino, nb);
		return COALESCE_NOMEM;
	}
	for (i = 0; i < nb; i++) {
		memset(bip + i, 0, sizeof(BLOCK_INFO));
		bip[i].bi_inode = ino;
		bip[i].bi_lbn = i;
		bip[i].bi_version = dip->di_gen;
		/* Don't set the size, but let lfs_bmap fill it in */
	}
	if ((error = lfs_bmapv_emul(ifile_fd, bip, nb)) < 0) { 
                syslog(LOG_WARNING, "LFCNBMAPV: %m");
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
		if (bip[i].bi_daddr != bip[i - 1].bi_daddr + lfsp->lfs_frag)
			++noff;
		toff += abs(bip[i].bi_daddr - bip[i - 1].bi_daddr
		    - lfsp->lfs_frag) >> lfsp->lfs_fbshift;
	}

	/*
	 * If this file is not discontinuous, there's no point in rewriting it.
         *
         * Explicitly allow a certain amount of discontinuity, since large
         * files will be broken among segments and medium-sized files
         * can have a break or two and it's okay.
	 */
	if (nb <= 1 || noff == 0 || noff < log2int(nb) ||
	    segtod(lfsp, noff) * 2 < nb) {
		retval = COALESCE_NOTWORTHIT;
		goto out;
	} else if (debug)
		syslog(LOG_DEBUG, "ino %d total discontinuity "
			"%d (%lld) for %d blocks", ino, noff,
			(long long)toff, nb);

	/* Search for blocks in active segments; don't move them. */
	for (i = 0; i < nb; i++) {
		if (bip[i].bi_daddr <= 0)
			continue;
		sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep,
				dtosn(lfsp, bip[i].bi_daddr));
		if (sup->su_flags & SEGUSE_ACTIVE)
			bip[i].bi_daddr = LFS_UNUSED_DADDR; /* 0 */
	}
        /*
	 * Get rid of any we've marked dead.  If this is an older
	 * kernel that doesn't have bmapv fill in the block
	 * sizes, we'll toss everything here.
	 */
	toss(bip, &nb, sizeof(BLOCK_INFO), tossdead, NULL);
        if (nb && tossdead(NULL, bip + nb - 1, NULL))
                --nb;
        if (nb == 0) {
		retval = COALESCE_NOTHINGLEFT;
		goto out;
	}

	/*
	 * We may have tossed enough blocks that it is no longer worthwhile
	 * to rewrite this inode.
	 */
	if (onb - nb > log2int(onb)) {
		if (debug)
			syslog(LOG_DEBUG, "too many blocks tossed, not rewriting");
		return COALESCE_NOTHINGLEFT2;
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
                if (get_rawblock(fsp, bip[i].bi_bp, bip[i].bi_size,
		    bip[i].bi_daddr) != bip[i].bi_size) {
			retval = COALESCE_EIO;
			goto out;
		}
	}
	if (debug)
		syslog(LOG_DEBUG, "ino %d markv %d blocks", ino, nb);

	/*
	 * Write in segment-sized chunks.  If at any point we'd write more
	 * than half of the available segments, sleep until that's not
	 * true any more.
	 */
	bps = segtod(lfsp, 1);
	for (tbip = bip; tbip < bip + nb; tbip += bps) {
		while (fsp->fi_cip->clean < 4) {
			lfs_segwait_emul(ifile_fd, NULL);
			reread_fs_info(fsp, do_mmap);
			/* XXX start over? */
		}
		lfs_markv_emul(ifile_fd, tbip,
                          (tbip + bps < bip + nb ? bps : nb % bps));
	}

	retval = COALESCE_OK;
out:
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
int clean_all_inodes(struct fs_info *fsp)
{
	int i, r;
	int totals[COALESCE_MAXERROR];

	memset(totals, 0, sizeof(totals));
	for (i = 0; i < fsp->fi_ifile_count; i++) {
		r = clean_inode(fsp, i);
		++totals[r];
	}

	for (i = 0; i < COALESCE_MAXERROR; i++)
		if (totals[i])
			syslog(LOG_DEBUG, "%s: %d", coalesce_return[i],
				totals[i]);

	return totals[COALESCE_OK];
}

int fork_coalesce(struct fs_info *fsp)
{
	static pid_t childpid;
	int num;

	reread_fs_info(fsp, do_mmap);

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
	childpid = fork();
	if (childpid < 0) {
		syslog(LOG_ERR, "fork: %m");
		return 0;
	} else if (childpid == 0) {
		syslog(LOG_NOTICE, "new coalescing process, pid %d", getpid());
		num = clean_all_inodes(fsp);
		syslog(LOG_NOTICE, "coalesced %d discontiguous inodes", num);
		exit(0);
	}
	return 0;
}
