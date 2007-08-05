/*	$NetBSD: buffercache.c,v 1.1.2.2 2007/08/05 22:28:08 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include "rumpuser.h"

int
bread(struct vnode *vp, daddr_t blkno, int size, struct kauth_cred *cred,
	struct buf **bpp)
{
	struct buf *bp;

	bp = getblk(vp, blkno, size, 0, 0);
	bp->b_flags = B_READ;
	VOP_STRATEGY(vp, bp);

	*bpp = bp;
	return 0;
}

int
breadn(struct vnode *vp, daddr_t blkno, int size, daddr_t *rablks,
	int *rasizes, int nrablks, struct kauth_cred *cred, struct buf **bpp)
{

	return bread(vp, blkno, size, cred, bpp);
}

int
bwrite(struct buf *bp)
{

	bp->b_flags &= ~B_READ;
	VOP_STRATEGY(bp->b_vp, bp);
	brelse(bp);

	return 0;
}

void
bawrite(struct buf *bp)
{

	bwrite(bp);
}

void
bdwrite(struct buf *bp)
{

	bwrite(bp);
}

void
brelse(struct buf *bp)
{

	rumpuser_free(bp->b_data);
	rumpuser_free(bp);
}

struct buf *
getblk(struct vnode *vp, daddr_t blkno, int size, int slpflag, int slptimeo)
{
	struct buf *bp;

	bp = rumpuser_malloc(sizeof(struct buf), 0);
	memset(bp, 0x55, sizeof(struct buf));
	bp->b_data = rumpuser_malloc(size, 0);
	bp->b_blkno = bp->b_lblkno = blkno;
	bp->b_bcount = bp->b_bufsize = size;
	bp->b_vp = vp;

	return bp;
}

struct buf *
incore(struct vnode *vp, daddr_t blkno)
{

	return NULL;
}

void
allocbuf(struct buf *bp, int size, int preserve)
{

	bp->b_bcount = bp->b_bufsize = size;
	bp->b_data = rumpuser_realloc(bp->b_data, size, 0);
}

void
biodone(struct buf *bp)
{

	/* nada */
}

/* everything is always instantly done */
int
biowait(struct buf *bp)
{

	return 0;
}

struct buf *
getiobuf()
{

	panic("%s: not implemented", __func__);
}

struct buf *
getiobuf_nowait()
{

	panic("%s: not implemented", __func__);
}

void
putiobuf(struct buf *bp)
{

	panic("%s: not implemented", __func__);
}
