/*	$NetBSD: vm_vfs.c,v 1.7 2009/08/04 20:01:06 pooka Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm_vfs.c,v 1.7 2009/08/04 20:01:06 pooka Exp $");

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

/*
 * release resources held during async io.  this is almost the
 * same as uvm_aio_aiodone() from uvm_pager.c and only lacks the
 * call to uvm_aio_aiodone_pages(): unbusies pages directly here.
 */
void
uvm_aio_aiodone(struct buf *bp)
{
	int i, npages = bp->b_bufsize >> PAGE_SHIFT;
	struct vm_page **pgs;
	vaddr_t va;

	pgs = kmem_alloc(npages * sizeof(*pgs), KM_SLEEP);
	for (i = 0; i < npages; i++) {
		va = (vaddr_t)bp->b_data + (i << PAGE_SHIFT);
		pgs[i] = uvm_pageratop(va);
	}

	uvm_pagermapout((vaddr_t)bp->b_data, npages);
	uvm_page_unbusy(pgs, npages);

	if (BUF_ISWRITE(bp) && (bp->b_cflags & BC_AGE) != 0) {
		mutex_enter(bp->b_objlock);
		vwakeup(bp);
		mutex_exit(bp->b_objlock);
	}

	putiobuf(bp);

	kmem_free(pgs, npages * sizeof(*pgs));
}

struct uvm_ractx *
uvm_ra_allocctx(void)
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

void
uvm_vnp_setsize(struct vnode *vp, voff_t newsize)
{

	mutex_enter(&vp->v_interlock);
	vp->v_size = vp->v_writesize = newsize;
	mutex_exit(&vp->v_interlock);
}

void
uvm_vnp_setwritesize(struct vnode *vp, voff_t newsize)
{

	mutex_enter(&vp->v_interlock);
	vp->v_writesize = newsize;
	mutex_exit(&vp->v_interlock);
}

void
uvm_vnp_zerorange(struct vnode *vp, off_t off, size_t len)
{
	struct uvm_object *uobj = &vp->v_uobj;
	struct vm_page **pgs;
	int maxpages = MIN(32, round_page(len) >> PAGE_SHIFT);
	int rv, npages, i;

	pgs = kmem_zalloc(maxpages * sizeof(pgs), KM_SLEEP);
	while (len) {
		npages = MIN(maxpages, round_page(len) >> PAGE_SHIFT);
		memset(pgs, 0, npages * sizeof(struct vm_page *));
		mutex_enter(&uobj->vmobjlock);
		rv = uobj->pgops->pgo_get(uobj, off, pgs, &npages, 0, 0, 0, 0);
		KASSERT(npages > 0);

		for (i = 0; i < npages; i++) {
			uint8_t *start;
			size_t chunkoff, chunklen;

			chunkoff = off & PAGE_MASK;
			chunklen = MIN(PAGE_SIZE - chunkoff, len);
			start = (uint8_t *)pgs[i]->uanon + chunkoff;

			memset(start, 0, chunklen);
			pgs[i]->flags &= ~PG_CLEAN;

			off += chunklen;
			len -= chunklen;
		}
		uvm_page_unbusy(pgs, npages);
	}
	kmem_free(pgs, maxpages * sizeof(pgs));

	return;
}

/*
 * UBC
 */

/* dumdidumdum */
#define len2npages(off, len)						\
  (((((len) + PAGE_MASK) & ~(PAGE_MASK)) >> PAGE_SHIFT)			\
    + (((off & PAGE_MASK) + (len & PAGE_MASK)) > PAGE_SIZE))

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo,
	int advice, int flags)
{
	struct vm_page **pgs;
	int npages = len2npages(uio->uio_offset, todo);
	size_t pgalloc;
	int i, rv, pagerflags;

	pgalloc = npages * sizeof(pgs);
	pgs = kmem_zalloc(pgalloc, KM_SLEEP);

	pagerflags = PGO_SYNCIO | PGO_NOBLOCKALLOC | PGO_NOTIMESTAMP;
	if (flags & UBC_WRITE)
		pagerflags |= PGO_PASTEOF;
	if (flags & UBC_FAULTBUSY)
		pagerflags |= PGO_OVERWRITE;

	do {
		mutex_enter(&uobj->vmobjlock);
		rv = uobj->pgops->pgo_get(uobj, uio->uio_offset, pgs, &npages,
		    0, VM_PROT_READ | VM_PROT_WRITE, 0, pagerflags);
		if (rv)
			goto out;

		for (i = 0; i < npages; i++) {
			size_t xfersize;
			off_t pageoff;

			pageoff = uio->uio_offset & PAGE_MASK;
			xfersize = MIN(MIN(todo, PAGE_SIZE), PAGE_SIZE-pageoff);
			uiomove((uint8_t *)pgs[i]->uanon + pageoff,
			    xfersize, uio);
			if (uio->uio_rw == UIO_WRITE)
				pgs[i]->flags &= ~PG_CLEAN;
			todo -= xfersize;
		}
		uvm_page_unbusy(pgs, npages);
	} while (todo);

 out:
	kmem_free(pgs, pgalloc);
	return rv;
}
