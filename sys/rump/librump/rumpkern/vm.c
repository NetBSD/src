/*	$NetBSD: vm.c,v 1.16.2.2 2007/09/03 14:45:39 yamt Exp $	*/

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

/*
 * Virtual memory emulation routines.  Contents:
 *  + UBC
 *  + anon objects & pager
 *  + vnode objects & pager
 *  + misc support routines
 *  + kmem
 */

/*
 * XXX: we abuse pg->uanon for the virtual address of the storage
 * for each page.  phys_addr would fit the job description better,
 * except that it will create unnecessary lossage on some platforms
 * due to not being a pointer type.
 */

#include <sys/param.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_readahead.h>

#include <machine/pmap.h>

#include "rump_private.h"
#include "rumpuser.h"

/* dumdidumdum */
#define len2npages(off, len)						\
  (((((len) + PAGE_MASK) & ~(PAGE_MASK)) >> PAGE_SHIFT)			\
    + (((off & PAGE_MASK) + (len & PAGE_MASK)) > PAGE_SIZE))

static int ubc_winvalid;
static struct uvm_object *ubc_uobj;
static off_t ubc_offset;
static int ubc_flags;

struct uvm_pagerops uvm_vnodeops;
struct uvm_pagerops aobj_pager;
struct uvmexp uvmexp;
struct uvm uvm;

struct vmspace rump_vmspace;
struct vm_map rump_vmmap;

/*
 * vm pages 
 */

struct vm_page *
rumpvm_makepage(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	pg = rumpuser_malloc(sizeof(struct vm_page), 0);
	memset(pg, 0, sizeof(struct vm_page));
	TAILQ_INSERT_TAIL(&uobj->memq, pg, listq);
	pg->offset = off;
	pg->uobject = uobj;

	pg->uanon = (void *)rumpuser_malloc(PAGE_SIZE, 0);
	memset((void *)pg->uanon, 0, PAGE_SIZE);
	pg->flags = PG_CLEAN;

	return pg;
}

void
rumpvm_freepage(struct vm_page *pg)
{
	struct uvm_object *uobj = pg->uobject;

	TAILQ_REMOVE(&uobj->memq, pg, listq);
	rumpuser_free((void *)pg->uanon);
	rumpuser_free(pg);
}

struct rumpva {
	vaddr_t addr;
	struct vm_page *pg;

	LIST_ENTRY(rumpva) entries;
};
static LIST_HEAD(, rumpva) rvahead = LIST_HEAD_INITIALIZER(rvahead);

void
rumpvm_enterva(vaddr_t addr, struct vm_page *pg)
{
	struct rumpva *rva;

	rva = rumpuser_malloc(sizeof(struct rumpva), 0);
	rva->addr = addr;
	rva->pg = pg;
	LIST_INSERT_HEAD(&rvahead, rva, entries);
}

void
rumpvm_flushva()
{
	struct rumpva *rva;

	while ((rva = LIST_FIRST(&rvahead)) != NULL) {
		LIST_REMOVE(rva, entries);
		rumpuser_free(rva);
	}
}

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

/*
 * Anon object stuff
 */

static int
ao_get(struct uvm_object *uobj, voff_t off, struct vm_page **pgs,
	int *npages, int centeridx, vm_prot_t access_type,
	int advice, int flags)
{
	struct vm_page *pg;
	int i;

	if (centeridx)
		panic("%s: centeridx != 0 not supported", __func__);

	/* loop over pages */
	off = trunc_page(off);
	for (i = 0; i < *npages; i++) {
		pg = uvm_pagelookup(uobj, off + (i << PAGE_SHIFT));
		if (pg) {
			pgs[i] = pg;
		} else {
			pg = rumpvm_makepage(uobj, off + (i << PAGE_SHIFT));
			pgs[i] = pg;
		}
	}

	return 0;

}

static int
ao_put(struct uvm_object *uobj, voff_t start, voff_t stop, int flags)
{
	struct vm_page *pg;

	/* we only free all pages for now */
	if ((flags & PGO_FREE) == 0 || (flags & PGO_ALLPAGES) == 0)
		return 0;

	while ((pg = TAILQ_FIRST(&uobj->memq)) != NULL)
		rumpvm_freepage(pg);

	return 0;
}

struct uvm_object *
uao_create(vsize_t size, int flags)
{
	struct uvm_object *uobj;

	uobj = rumpuser_malloc(sizeof(struct uvm_object), 0);
	memset(uobj, 0, sizeof(struct uvm_object));
	uobj->pgops = &aobj_pager;
	TAILQ_INIT(&uobj->memq);

	return uobj;
}

void
uao_detach(struct uvm_object *uobj)
{

	ao_put(uobj, 0, 0, PGO_ALLPAGES | PGO_FREE);
	rumpuser_free(uobj);
}

/*
 * UBC
 */

int
rump_ubc_magic_uiomove(size_t n, struct uio *uio)
{
	int npages = len2npages(uio->uio_offset, n);
	struct vm_page *pgs[npages];
	int i, rv;

	if (ubc_winvalid == 0)
		panic("%s: ubc window not allocated", __func__);

	memset(pgs, 0, sizeof(pgs));
	rv = ubc_uobj->pgops->pgo_get(ubc_uobj, ubc_offset,
	    pgs, &npages, 0, 0, 0, 0);
	if (rv)
		return rv;

	for (i = 0; i < npages; i++) {
		size_t xfersize;
		off_t pageoff;

		pageoff = uio->uio_offset & PAGE_MASK;
		xfersize = MIN(MIN(n, PAGE_SIZE), PAGE_SIZE-pageoff);
		uiomove((uint8_t *)pgs[i]->uanon + pageoff, xfersize, uio);
		if (uio->uio_rw == UIO_WRITE)
			pgs[i]->flags &= ~PG_CLEAN;
		ubc_offset += xfersize;
		n -= xfersize;
	}

	return 0;
}

void *
ubc_alloc(struct uvm_object *uobj, voff_t offset, vsize_t *lenp, int advice,
	int flags)
{
	vsize_t reallen;

	/* XXX: only one window, but that's ok for now */
	if (ubc_winvalid == 1)
		panic("%s: ubc window already allocated", __func__);

	printf("UBC_ALLOC offset 0x%x\n", (int)offset);
	ubc_uobj = uobj;
	ubc_offset = offset;
	reallen = round_page(*lenp);
	ubc_flags = flags;

	ubc_winvalid = 1;

	return RUMP_UBC_MAGIC_WINDOW;
}

void
ubc_release(void *va, int flags)
{

	ubc_winvalid = 0;
}

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo,
	int advice, int flags)
{
	void *win;
	vsize_t len;

	while (todo > 0) {
		len = todo;

		win = ubc_alloc(uobj, uio->uio_offset, &len, 0, flags);
		rump_ubc_magic_uiomove(len, uio);
		ubc_release(win, 0);

		todo -= len;
	}
	return 0;
}


/*
 * Misc routines
 */

void
rumpvm_init()
{

	uvm_vnodeops.pgo_get = vn_get;
	uvm_vnodeops.pgo_put = vn_put;
	aobj_pager.pgo_get = ao_get;
	aobj_pager.pgo_put = ao_put;

	uvmexp.free = 1024*1024; /* XXX */
	uvm.pagedaemon_lwp = NULL; /* doesn't match curlwp */
}

void
uvm_pageactivate(struct vm_page *pg)
{

	/* nada */
}

void
uvm_page_unbusy(struct vm_page **pgs, int npgs)
{

	/* nada */
}

void
uvm_pagewire(struct vm_page *pg)
{

	/* nada */
}

void
uvm_pageunwire(struct vm_page *pg)
{

	/* nada */
}

vaddr_t
uvm_pagermapin(struct vm_page **pps, int npages, int flags)
{

	panic("%s: unimplemented", __func__);
}

struct vm_page *
uvm_pagelookup(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	TAILQ_FOREACH(pg, &uobj->memq, listq)
		if (pg->offset == off)
			return pg;

	return NULL;
}

struct vm_page *
uvm_pageratop(vaddr_t va)
{
	struct rumpva *rva;

	LIST_FOREACH(rva, &rvahead, entries)
		if (rva->addr == va)
			return rva->pg;

	panic("%s: va %llu", __func__, (unsigned long long)va);
}

void
uvm_estimatepageable(int *active, int *inactive)
{

	*active = 0;
	*inactive = 0;
	panic("%s: unimplemented", __func__);
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

	if ((bp->b_flags & (B_READ | B_NOCACHE)) == 0 && bioopsp)
		bioopsp->io_pageiodone(bp);
}

void
uvm_vnp_setsize(struct vnode *vp, voff_t newsize)
{

	vp->v_size = vp->v_writesize = newsize;
}

void
uvm_vnp_setwritesize(struct vnode *vp, voff_t newsize)
{

	vp->v_writesize = newsize;
}

void
uvm_vnp_zerorange(struct vnode *vp, off_t off, size_t len)
{
	int maxpages = MIN(32, round_page(len) >> PAGE_SHIFT);
	struct vm_page *pgs[maxpages];
	struct uvm_object *uobj = &vp->v_uobj;
	int rv, npages, i;

	while (len) {
		npages = MIN(maxpages, round_page(len) >> PAGE_SHIFT);
		memset(pgs, 0, npages * sizeof(struct vm_page *));
		rv = uobj->pgops->pgo_get(uobj, off, pgs, &npages, 0, 0, 0, 0);
		assert(npages > 0);

		for (i = 0; i < npages; i++) {
			uint8_t *start;
			size_t chunkoff, chunklen;

			chunkoff = off & PAGE_MASK;
			chunklen = MIN(PAGE_SIZE - chunkoff, len);
			start = (uint8_t *)pgs[i]->uanon + chunkoff;

			memset(start, 0, chunklen);
			pgs[i]->flags &= PG_CLEAN;

			off += chunklen;
			len -= chunklen;
		}
	}

	return;
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

	return (vp->v_flag & VONWORKLST) == 0;
}

/*
 * Kmem
 */

void *
kmem_alloc(size_t size, km_flag_t kmflag)
{

	return rumpuser_malloc(size, kmflag == KM_NOSLEEP);
}

void *
kmem_zalloc(size_t size, km_flag_t kmflag)
{
	void *rv;

	rv = kmem_alloc(size, kmflag);
	if (rv)
		memset(rv, 0, size);

	return rv;
}

void
kmem_free(void *p, size_t size)
{

	rumpuser_free(p);
}

/*
 * UVM km
 */

vaddr_t
uvm_km_alloc(struct vm_map *map, vsize_t size, vsize_t align, uvm_flag_t flags)
{
	void *rv;

	rv = rumpuser_malloc(size, flags & (UVM_KMF_CANFAIL | UVM_KMF_NOWAIT));
	if (rv && flags & UVM_KMF_ZERO)
		memset(rv, 0, size);

	return (vaddr_t)rv;
}

void
uvm_km_free(struct vm_map *map, vaddr_t vaddr, vsize_t size, uvm_flag_t flags)
{

	rumpuser_free((void *)vaddr);
}

struct vm_map *
uvm_km_suballoc(struct vm_map *map, vaddr_t *minaddr, vaddr_t *maxaddr,
	vsize_t size, int pageable, bool fixed, struct vm_map_kernel *submap)
{

	return (struct vm_map *)417416;
}
