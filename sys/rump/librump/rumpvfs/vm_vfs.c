/*	$NetBSD: vm_vfs.c,v 1.28.2.1 2011/06/23 14:20:29 cherry Exp $	*/

/*
 * Copyright (c) 2008-2011 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: vm_vfs.c,v 1.28.2.1 2011/06/23 14:20:29 cherry Exp $");

#include <sys/param.h>

#include <sys/buf.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

/*
 * release resources held during async io.  this is almost the
 * same as uvm_aio_aiodone() from uvm_pager.c and only lacks the
 * call to uvm_aio_aiodone_pages(): unbusies pages directly here.
 */
void
uvm_aio_aiodone(struct buf *bp)
{
	struct uvm_object *uobj;
	int i, npages = bp->b_bufsize >> PAGE_SHIFT;
	struct vm_page **pgs;
	vaddr_t va;
	int pageout = 0;

	KASSERT(npages > 0);
	pgs = kmem_alloc(npages * sizeof(*pgs), KM_SLEEP);
	for (i = 0; i < npages; i++) {
		va = (vaddr_t)bp->b_data + (i << PAGE_SHIFT);
		pgs[i] = uvm_pageratop(va);
		if (pgs[i]->flags & PG_PAGEOUT) {
			KASSERT((pgs[i]->flags & PG_FAKE) == 0);
			pageout++;
			pgs[i]->flags &= ~PG_PAGEOUT;
			pgs[i]->flags |= PG_RELEASED;
		}
	}

	uvm_pagermapout((vaddr_t)bp->b_data, npages);

	/* get uobj because we need it after pages might be recycled */
	uobj = pgs[0]->uobject;
	KASSERT(uobj);

	mutex_enter(uobj->vmobjlock);
	mutex_enter(&uvm_pageqlock);
	uvm_page_unbusy(pgs, npages);
	mutex_exit(&uvm_pageqlock);
	mutex_exit(uobj->vmobjlock);

	uvm_pageout_done(pageout);

	if (BUF_ISWRITE(bp) && (bp->b_cflags & BC_AGE) != 0) {
		mutex_enter(bp->b_objlock);
		vwakeup(bp);
		mutex_exit(bp->b_objlock);
	}

	putiobuf(bp);

	kmem_free(pgs, npages * sizeof(*pgs));
}

void
uvm_aio_biodone(struct buf *bp)
{

	uvm_aio_aiodone(bp);
}

/*
 * UBC
 */

#define PAGERFLAGS (PGO_SYNCIO | PGO_NOBLOCKALLOC | PGO_NOTIMESTAMP)

void
ubc_zerorange(struct uvm_object *uobj, off_t off, size_t len, int flags)
{
	struct vm_page **pgs;
	struct uvm_object *pguobj;
	int maxpages = MIN(32, round_page(len) >> PAGE_SHIFT);
	int rv, npages, i;

	if (maxpages == 0)
		return;

	pgs = kmem_alloc(maxpages * sizeof(pgs), KM_SLEEP);
	mutex_enter(uobj->vmobjlock);
	while (len) {
		npages = MIN(maxpages, round_page(len) >> PAGE_SHIFT);
		memset(pgs, 0, npages * sizeof(struct vm_page *));
		rv = uobj->pgops->pgo_get(uobj, trunc_page(off),
		    pgs, &npages, 0, VM_PROT_READ | VM_PROT_WRITE,
		    0, PAGERFLAGS | PGO_PASTEOF);
		KASSERT(npages > 0);

		for (i = 0, pguobj = NULL; i < npages; i++) {
			struct vm_page *pg;
			uint8_t *start;
			size_t chunkoff, chunklen;

			pg = pgs[i];
			if (pg == NULL)
				break;
			if (pguobj == NULL)
				pguobj = pg->uobject;
			KASSERT(pguobj == pg->uobject);

			chunkoff = off & PAGE_MASK;
			chunklen = MIN(PAGE_SIZE - chunkoff, len);
			start = (uint8_t *)pg->uanon + chunkoff;

			memset(start, 0, chunklen);
			pg->flags &= ~PG_CLEAN;

			off += chunklen;
			len -= chunklen;
		}
		mutex_enter(pguobj->vmobjlock);
		uvm_page_unbusy(pgs, npages);
		if (pguobj != uobj) {
			mutex_exit(pguobj->vmobjlock);
			mutex_enter(uobj->vmobjlock);
		}
	}
	mutex_exit(uobj->vmobjlock);
	kmem_free(pgs, maxpages * sizeof(pgs));

	return;
}

#define len2npages(off, len)						\
    ((round_page(off+len) - trunc_page(off)) >> PAGE_SHIFT)

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo,
	int advice, int flags)
{
	struct vm_page **pgs;
	struct uvm_object *pguobj;
	int npages = len2npages(uio->uio_offset, todo);
	size_t pgalloc;
	int i, rv, pagerflags;
	vm_prot_t prot;

	pgalloc = npages * sizeof(pgs);
	pgs = kmem_alloc(pgalloc, KM_SLEEP);

	pagerflags = PAGERFLAGS;
	if (flags & UBC_WRITE)
		pagerflags |= PGO_PASTEOF;
	if (flags & UBC_FAULTBUSY)
		pagerflags |= PGO_OVERWRITE;

	prot = VM_PROT_READ;
	if (flags & UBC_WRITE)
		prot |= VM_PROT_WRITE;

	mutex_enter(uobj->vmobjlock);
	do {
		npages = len2npages(uio->uio_offset, todo);
		memset(pgs, 0, pgalloc);
		rv = uobj->pgops->pgo_get(uobj, trunc_page(uio->uio_offset),
		    pgs, &npages, 0, prot, 0, pagerflags);
		if (rv)
			goto out;

		for (i = 0, pguobj = NULL; i < npages; i++) {
			struct vm_page *pg;
			size_t xfersize;
			off_t pageoff;

			pg = pgs[i];
			if (pg == NULL)
				break;
			if (pguobj == NULL)
				pguobj = pg->uobject;
			KASSERT(pguobj == pg->uobject);

			pageoff = uio->uio_offset & PAGE_MASK;
			xfersize = MIN(MIN(todo, PAGE_SIZE), PAGE_SIZE-pageoff);
			KASSERT(xfersize > 0);
			rv = uiomove((uint8_t *)pg->uanon + pageoff,
			    xfersize, uio);
			if (rv) {
				mutex_enter(pguobj->vmobjlock);
				uvm_page_unbusy(pgs, npages);
				mutex_exit(pguobj->vmobjlock);
				goto out;
			}
			if (uio->uio_rw == UIO_WRITE)
				pg->flags &= ~(PG_CLEAN | PG_FAKE);
			todo -= xfersize;
		}
		mutex_enter(pguobj->vmobjlock);
		uvm_page_unbusy(pgs, npages);
		if (pguobj != uobj) {
			mutex_exit(pguobj->vmobjlock);
			mutex_enter(uobj->vmobjlock);
		}
	} while (todo);
	mutex_exit(uobj->vmobjlock);

 out:
	kmem_free(pgs, pgalloc);
	return rv;
}
