/*      $NetBSD: coalesce.c,v 1.1 2002/06/06 00:56:50 perseant Exp $  */

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

int	lfs_bmapv(fsid_t *, BLOCK_INFO_15 *, int);
int	lfs_markv(fsid_t *, BLOCK_INFO_15 *, int);

extern int debug;

static int
tossdead(const void *client, const void *a, const void *b)
{
	return (((BLOCK_INFO_15 *)a)->bi_daddr == LFS_UNUSED_DADDR ||
		((BLOCK_INFO_15 *)a)->bi_size == 0);
}

/*
 * Find out if this inode's data blocks are discontinuous; if they are,
 * rewrite them using lfs_markv.  Return the number of inodes rewritten.
 */
int clean_inode(struct fs_info *fsp, ino_t ino)
{
	int i, error;
	IFILE *ifp;
	BLOCK_INFO_15 *bip, *tbip;
	struct dinode *dip;
	int nb, noff;
	ufs_daddr_t toff;
	struct lfs *lfsp;
	int bps;
        SEGUSE *sup;

	lfsp = &fsp->fi_lfs;

        dip = get_dinode(fsp, ino);
	if (dip == NULL)
		return 0;

	/* Compute file block size, set up for lfs_bmapv */
	nb = btofsb(lfsp, dip->di_size);
	if (nb > dip->di_blocks) {
		syslog(LOG_WARNING, "ino %d, computed blocks %d > held blocks %d",
			ino, nb, dip->di_blocks);
		return -1;
	}
	bip = (BLOCK_INFO_15 *)malloc(sizeof(BLOCK_INFO_15) * nb);
	if (bip == NULL) {
		syslog(LOG_WARNING, "ino %d, %d blocks: %m", ino, nb);
		return -1;
	}
	for (i = 0; i < nb; i++) {
		memset(bip + i, 0, sizeof(BLOCK_INFO_15));
		bip[i].bi_inode = ino;
		bip[i].bi_lbn = i;
		bip[i].bi_version = ifp->if_version;
		/* Don't set the size, but let lfs_bmap fill it in */
	}
	if ((error = lfs_bmapv(&fsp->fi_statfsp->f_fsid, bip, nb)) < 0) { 
                syslog(LOG_WARNING, "lfs_bmapv");
		free(bip);
		return -1;
	}
	noff = toff = 0;
	for (i = 1; i < nb; i++) {
		if (bip[i].bi_daddr != bip[i - 1].bi_daddr + 1)
			++noff;
		toff += abs(bip[i].bi_daddr - bip[i - 1].bi_daddr - 1);
	}

	/*
	 * If this file is not discontinuous, there's no point in rewriting it.
         *
         * Explicitly allow a certain amount of discontinuity, since large
         * files will be broken among segments and medium-sized files
         * can have a break or two and it's okay.
	 */
	if (nb <= 1 || noff == 0 || (1 << (noff + 1)) < nb ||
	    segtod(lfsp, noff) * 2< nb) {
		free(bip);
		return 0;
	} else if (debug)
		syslog(LOG_DEBUG, "ino %d total discontinuity "
			"%d (%d) for %d blocks", ino, noff, toff, nb);

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
	 * kernel that doesn't have lfs_bmapv fill in the block
	 * sizes, we'll toss everything here.
	 */
	toss(bip, &nb, sizeof(BLOCK_INFO_15), tossdead, NULL);
        if (nb && tossdead(NULL, bip + nb - 1, NULL))
                --nb;
        if (nb == 0) {
		free(bip);
		return 0;
	}

	/*
	 * If we're going to write more blocks than half of the available
	 * segments, don't do it.
	 */
	if (segtod(lfsp, fsp->fi_cip->clean - 1) / 2 < nb) {
		syslog(LOG_WARNING, "not rewriting ino %d, "
			"not enough free segments", ino);
		free(bip);
		return 0;
	}

        /*
	 * We are going to rewrite this inode.
	 * For any remaining blocks, read in their contents.
	 */
	for (i = 0; i < nb; i++) {
		bip[i].bi_bp = malloc(bip[i].bi_size);
                get_rawblock(fsp, bip[i].bi_bp, bip[i].bi_size, bip[i].bi_daddr);
	}
	if (debug)
		syslog(LOG_DEBUG, "ino %d markv %d blocks", ino, nb);

	/* Write in segment-sized chunks */
	bps = segtod(lfsp, 1);
	for (tbip = bip; tbip < bip + nb; tbip += bps) {
		lfs_markv(&fsp->fi_statfsp->f_fsid, tbip,
                          (tbip + bps < bip + nb ? bps : nb % bps));
	}

	for (i = 0; i < nb; i++)
		if (bip[i].bi_bp)
			free(bip[i].bi_bp);
	free(bip);
	return 1;
}

/*
 * Try coalescing every inode in the filesystem.
 * Return the number of inodes actually altered.
 */
int clean_all_inodes(struct fs_info *fsp)
{
	int i;
	int r, tot;

	tot = 0;
	for (i = 0; i < fsp->fi_ifile_count; i++) {
		r = clean_inode(fsp, i);
		if (r > 0)
			tot += r;
	}
	return tot;
}

int fork_coalesce(struct fs_info *fsp)
{
	static pid_t childpid;

	if (childpid) {
     		if (waitpid(childpid, NULL, WNOHANG) == childpid)
			childpid = 0;
	}
	if (childpid && kill(childpid, 0) >= 0) {
		/* already running a coalesce process */
		return 0;
	}
	childpid = fork();
	if (childpid < 0) {
		syslog(LOG_ERR, "fork: %m");
		return 0;
	} else if (childpid == 0) {
		clean_all_inodes(fsp);
		exit(0);
	}
	return 0;
}
