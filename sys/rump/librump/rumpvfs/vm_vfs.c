/*	$NetBSD: vm_vfs.c,v 1.1 2008/11/19 14:10:49 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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

#include <sys/buf.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

static int vn_get(struct uvm_object *, voff_t, struct vm_page **,
	int *, int, vm_prot_t, int, int);
static int vn_put(struct uvm_object *, voff_t, voff_t, int);

const struct uvm_pagerops uvm_vnodeops = {
	.pgo_get = vn_get,
	.pgo_put = vn_put,
};

/*
 * vnode pager
 */

static int
vn_get(struct uvm_object *uobj, voff_t off, struct vm_page **pgs,
	int *npages, int centeridx, vm_prot_t access_type,
	int advice, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;

	return VOP_GETPAGES(vp, off, pgs, npages, centeridx, access_type,
	    advice, flags);
}

static int
vn_put(struct uvm_object *uobj, voff_t offlo, voff_t offhi, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;

	return VOP_PUTPAGES(vp, offlo, offhi, flags);
}

void
uvm_aio_biodone1(struct buf *bp)
{

	panic("%s: unimplemented", __func__);
}

void
uvm_aio_biodone(struct buf *bp)
{

	uvm_aio_aiodone(bp);
}

void
uvm_aio_aiodone(struct buf *bp)
{

	if (((bp->b_flags|bp->b_cflags) & (B_READ|BC_NOCACHE)) == 0 && bioopsp)
		bioopsp->io_pageiodone(bp);
}

struct uvm_ractx *
uvm_ra_allocctx()
{

	return NULL;
}

void
uvm_ra_freectx(struct uvm_ractx *ra)
{

	return;
}

bool
uvn_clean_p(struct uvm_object *uobj)
{
	struct vnode *vp = (void *)uobj;

	return (vp->v_iflag & VI_ONWORKLST) == 0;
}
