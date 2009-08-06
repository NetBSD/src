/* $NetBSD: fdfs.c,v 1.7 2009/08/06 00:51:55 pooka Exp $	 */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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

/*
 * Buffer cache routines for a file-descriptor backed filesystem.
 * This is part of lfs_cleanerd so there is also a "segment pointer" that
 * we can make buffers out of without duplicating memory or reading the data
 * again.
 */

#include <err.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "vnode.h"
#include "bufcache.h"
#include "fdfs.h"
#include "kernelops.h"

/*
 * Return a "vnode" interface to a given file descriptor.
 */
struct uvnode *
fd_vget(int fd, int bsize, int segsize, int nseg)
{
	struct fdfs *fs;
	struct uvnode *vp;
	int i;

	fs = (struct fdfs *)malloc(sizeof(*fs));
	if (fs == NULL)
		return NULL;
	if (segsize > 0) {
		fs->fd_bufp = (struct fd_buf *)malloc(nseg *
						      sizeof(struct fd_buf));
		if (fs->fd_bufp == NULL) {
			free(fs);
			return NULL;
		}
		for (i = 0; i < nseg; i++) {
			fs->fd_bufp[i].start = 0x0;
			fs->fd_bufp[i].end = 0x0;
			fs->fd_bufp[i].buf = (char *)malloc(segsize);
			if (fs->fd_bufp[i].buf == NULL) {
				while (--i >= 0)
					free(fs->fd_bufp[i].buf);
				free(fs->fd_bufp);
				free(fs);
				return NULL;
			}
		}
	} else
		fs->fd_bufp = NULL;

	fs->fd_fd = fd;
	fs->fd_bufc = nseg;
	fs->fd_bufi = 0;
	fs->fd_bsize = bsize;
	fs->fd_ssize = segsize;

	vp = (struct uvnode *) malloc(sizeof(*vp));
	if (vp == NULL) {
		if (fs->fd_bufp) {
			for (i = nseg - 1; i >= 0; i--)
				free(fs->fd_bufp[i].buf);
			free(fs->fd_bufp);
		}
		free(fs);
		return NULL;
	}
	memset(vp, 0, sizeof(*vp));
	vp->v_fd = fd;
	vp->v_fs = fs;
	vp->v_usecount = 0;
	vp->v_strategy_op = fd_vop_strategy;
	vp->v_bwrite_op = fd_vop_bwrite;
	vp->v_bmap_op = fd_vop_bmap;
	LIST_INIT(&vp->v_cleanblkhd);
	LIST_INIT(&vp->v_dirtyblkhd);
	vp->v_data = NULL;

	return vp;
}

/*
 * Deallocate a vnode.
 */
void
fd_reclaim(struct uvnode *vp)
{
	int i;
	struct ubuf *bp;
	struct fdfs *fs;

	while ((bp = LIST_FIRST(&vp->v_dirtyblkhd)) != NULL) {
		bremfree(bp);
		buf_destroy(bp);
	}
	while ((bp = LIST_FIRST(&vp->v_cleanblkhd)) != NULL) {
		bremfree(bp);
		buf_destroy(bp);
	}

	fs = (struct fdfs *)vp->v_fs;
	for (i = 0; i < fs->fd_bufc; i++)
		free(fs->fd_bufp[i].buf);
	free(fs->fd_bufp);
	free(fs);
	memset(vp, 0, sizeof(vp));
}

/*
 * We won't be using that last segment after all.
 */
void
fd_release(struct uvnode *vp)
{
	--((struct fdfs *)vp->v_fs)->fd_bufi;
}

/*
 * Reset buffer pointer to first buffer.
 */
void
fd_release_all(struct uvnode *vp)
{
	((struct fdfs *)vp->v_fs)->fd_bufi = 0;
}

/*
 * Prepare a segment buffer which we will expect to read from.
 * We never increment fd_bufi unless we have succeeded to allocate the space,
 * if necessary, and have read the segment.
 */
int
fd_preload(struct uvnode *vp, daddr_t start)
{
	struct fdfs *fs = (struct fdfs *)vp->v_fs;
	struct fd_buf *t;
	int r;

	/* We might need to allocate more buffers. */
	if (fs->fd_bufi == fs->fd_bufc) {
		++fs->fd_bufc;
		syslog(LOG_DEBUG, "increasing number of segment buffers to %d",
			fs->fd_bufc);
		t = realloc(fs->fd_bufp, fs->fd_bufc * sizeof(struct fd_buf));
		if (t == NULL) {
			syslog(LOG_NOTICE, "failed resizing table to %d\n",
				fs->fd_bufc);
			return -1;
		}
		fs->fd_bufp = t;
		fs->fd_bufp[fs->fd_bufi].start = 0x0;
		fs->fd_bufp[fs->fd_bufi].end = 0x0;
		fs->fd_bufp[fs->fd_bufi].buf = (char *)malloc(fs->fd_ssize);
		if (fs->fd_bufp[fs->fd_bufi].buf == NULL) {
			syslog(LOG_NOTICE, "failed to allocate buffer #%d\n",
				fs->fd_bufc);
			--fs->fd_bufc;
			return -1;
		}
	}

	/* Read the current buffer. */
	fs->fd_bufp[fs->fd_bufi].start = start;
	fs->fd_bufp[fs->fd_bufi].end =	 start + fs->fd_ssize / fs->fd_bsize;

	if ((r = kops.ko_pread(fs->fd_fd, fs->fd_bufp[fs->fd_bufi].buf,
		       (size_t)fs->fd_ssize, start * fs->fd_bsize)) < 0) {
		syslog(LOG_ERR, "preload to segment buffer %d", fs->fd_bufi);
		return r;
	}

	fs->fd_bufi = fs->fd_bufi + 1;
	return 0;
}

/*
 * Get a pointer to a block contained in one of the segment buffers,
 * as if from bread() but avoiding the buffer cache.
 */
char *
fd_ptrget(struct uvnode *vp, daddr_t bn)
{
	int i;
	struct fdfs *fs;

	fs = (struct fdfs *)vp->v_fs;
	for (i = 0; i < fs->fd_bufc; i++) {
		if (bn >= fs->fd_bufp[i].start && bn < fs->fd_bufp[i].end) {
			return fs->fd_bufp[i].buf +
				(bn - fs->fd_bufp[i].start) * fs->fd_bsize;
		}
	}
	return NULL;
}

/*
 * Strategy routine.  We can read from the segment buffer if requested.
 */
int
fd_vop_strategy(struct ubuf * bp)
{
	struct fdfs *fs;
	char *cp;
	int count;

	fs = (struct fdfs *)bp->b_vp->v_fs;
	if (bp->b_flags & B_READ) {
		if ((cp = fd_ptrget(bp->b_vp, bp->b_blkno)) != NULL) {
			free(bp->b_data);
			bp->b_data = cp;
			bp->b_flags |= (B_DONTFREE | B_DONE);
			return 0;
		}
		count = kops.ko_pread(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
			      bp->b_blkno * fs->fd_bsize);
		if (count == bp->b_bcount)
			bp->b_flags |= B_DONE;
	} else {
		count = kops.ko_pwrite(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
			       bp->b_blkno * fs->fd_bsize);
		if (count == 0) {
			perror("pwrite");
			return -1;
		}
		bp->b_flags &= ~B_DELWRI;
		reassignbuf(bp, bp->b_vp);
	}
	return 0;
}

/*
 * Delayed write.
 */
int
fd_vop_bwrite(struct ubuf * bp)
{
	bp->b_flags |= B_DELWRI;
	reassignbuf(bp, bp->b_vp);
	brelse(bp, 0);
	return 0;
}

/*
 * Map lbn to disk address.  Since we are using the file
 * descriptor as the "disk", the disk address is meaningless
 * and we just return the block address.
 */
int
fd_vop_bmap(struct uvnode * vp, daddr_t lbn, daddr_t * daddrp)
{
	*daddrp = lbn;
	return 0;
}
