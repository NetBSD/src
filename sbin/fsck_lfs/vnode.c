/* $NetBSD: vnode.c,v 1.1 2003/03/28 08:09:55 perseant Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/queue.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bufcache.h"
#include "vnode.h"

struct uvnodelst vnodelist;
struct uvnodelst getvnodelist;
struct vgrlst    vgrlist;

int nvnodes;

/* Convert between inode pointers and vnode pointers. */
#ifndef VTOI
#define	VTOI(vp)	((struct inode *)(vp)->v_data)
#endif

/*
 * Raw device uvnode ops
 */

int
raw_vop_strategy(struct ubuf * bp)
{
	if (bp->b_flags & B_READ) {
		return pread(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    dbtob(bp->b_blkno));
	} else {
		return pwrite(bp->b_vp->v_fd, bp->b_data, bp->b_bcount,
		    dbtob(bp->b_blkno));
	}
}

int
raw_vop_bwrite(struct ubuf * bp)
{
	bp->b_flags &= ~(B_READ | B_DELWRI | B_DONE | B_ERROR);
	raw_vop_strategy(bp);
	brelse(bp);
	return 0;
}

int
raw_vop_bmap(struct uvnode * vp, daddr_t lbn, daddr_t * daddrp)
{
	*daddrp = lbn;
	return 0;
}

/* Register a fs-specific vget function */
void
register_vget(void *fs, struct uvnode *func(void *, ino_t))
{
	struct vget_reg *vgr;

	vgr = (struct vget_reg *)malloc(sizeof(*vgr));
	vgr->vgr_fs = fs;
	vgr->vgr_func = func;
	LIST_INSERT_HEAD(&vgrlist, vgr, vgr_list);
}

static struct uvnode *
VFS_VGET(void *fs, ino_t ino)
{
	struct vget_reg *vgr;

	LIST_FOREACH(vgr, &vgrlist, vgr_list) {
		if (vgr->vgr_fs == fs)
			return vgr->vgr_func(fs, ino);
	}
	return NULL;
}

void
vnode_destroy(struct uvnode *tossvp)
{
	struct ubuf *bp;

	--nvnodes;
	LIST_REMOVE(tossvp, v_getvnodes);
	LIST_REMOVE(tossvp, v_mntvnodes);
	LIST_FOREACH(bp, &tossvp->v_dirtyblkhd, b_vnbufs) {
		bremfree(bp);
		buf_destroy(bp);
	}
	LIST_FOREACH(bp, &tossvp->v_cleanblkhd, b_vnbufs) {
		bremfree(bp);
		buf_destroy(bp);
	}
	free(VTOI(tossvp)->inode_ext.lfs);
	memset(VTOI(tossvp), 0, sizeof(struct inode));
	free(tossvp->v_data);
	memset(tossvp, 0, sizeof(*tossvp));
	free(tossvp);
}

/*
 * Find a vnode in the cache; if not present, get it from the
 * filesystem-specific vget routine.
 */
struct uvnode *
vget(void *fs, ino_t ino)
{
	struct uvnode *vp, *tossvp;

	/* Look in the uvnode cache */
	tossvp = NULL;
	LIST_FOREACH(vp, &getvnodelist, v_getvnodes) {
		if (vp->v_fs != fs)
			continue;
		if (VTOI(vp)->i_number == ino) {
			LIST_REMOVE(vp, v_getvnodes);
			LIST_INSERT_HEAD(&getvnodelist, vp, v_getvnodes);
			break;
		}
		if (LIST_EMPTY(&vp->v_dirtyblkhd) &&
		    vp->v_usecount == 0 &&
		    !(vp->v_flag & VDIROP))
			tossvp = vp;
	}
	/* Don't let vnode list grow arbitrarily */
	if (nvnodes > VNODE_CACHE_SIZE && tossvp) {
		vnode_destroy(tossvp);
	}
	if (vp)
		return vp;

	return VFS_VGET(fs, ino);
}

void
vfs_init(void)
{
	nvnodes = 0;
	LIST_INIT(&vnodelist);
	LIST_INIT(&getvnodelist);
	LIST_INIT(&vgrlist);
}
