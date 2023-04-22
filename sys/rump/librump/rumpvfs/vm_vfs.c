/*	$NetBSD: vm_vfs.c,v 1.42 2023/04/22 13:53:53 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vm_vfs.c,v 1.42 2023/04/22 13:53:53 riastradh Exp $");

#include <sys/param.h>

#include <sys/buf.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

void
uvm_aio_aiodone_pages(struct vm_page **pgs, int npages, bool write, int error)
{
	struct uvm_object *uobj = pgs[0]->uobject;
	struct vm_page *pg;
	int i;

	rw_enter(uobj->vmobjlock, RW_WRITER);
	for (i = 0; i < npages; i++) {
		pg = pgs[i];
		KASSERT((pg->flags & PG_PAGEOUT) == 0 ||
			(pg->flags & PG_FAKE) == 0);

		if (pg->flags & PG_FAKE) {
			KASSERT(!write);
			pg->flags &= ~PG_FAKE;
			KASSERT(uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_CLEAN);
			uvm_pagelock(pg);
			uvm_pageenqueue(pg);
			uvm_pageunlock(pg);
		}

	}
	uvm_page_unbusy(pgs, npages);
	rw_exit(uobj->vmobjlock);
}

/*
 * Release resources held during async io.
 */
void
uvm_aio_aiodone(struct buf *bp)
{
	struct uvm_object *uobj = NULL;
	int npages = bp->b_bufsize >> PAGE_SHIFT;
	struct vm_page **pgs;
	vaddr_t va;
	int i, error;
	bool write;

	error = bp->b_error;
	write = BUF_ISWRITE(bp);

	KASSERT(npages > 0);
	pgs = kmem_alloc(npages * sizeof(*pgs), KM_SLEEP);
	for (i = 0; i < npages; i++) {
		va = (vaddr_t)bp->b_data + (i << PAGE_SHIFT);
		pgs[i] = uvm_pageratop(va);

		if (uobj == NULL) {
			uobj = pgs[i]->uobject;
			KASSERT(uobj != NULL);
		} else {
			KASSERT(uobj == pgs[i]->uobject);
		}
	}
	uvm_pagermapout((vaddr_t)bp->b_data, npages);

	uvm_aio_aiodone_pages(pgs, npages, write, error);

	if (write && (bp->b_cflags & BC_AGE) != 0) {
		mutex_enter(bp->b_objlock);
		vwakeup(bp);
		mutex_exit(bp->b_objlock);
	}

	putiobuf(bp);

	kmem_free(pgs, npages * sizeof(*pgs));
}
