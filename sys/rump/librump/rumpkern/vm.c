/*	$NetBSD: vm.c,v 1.27 2008/01/03 02:48:03 pooka Exp $	*/

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

static int vn_get(struct uvm_object *, voff_t, struct vm_page **,
	int *, int, vm_prot_t, int, int);
static int vn_put(struct uvm_object *, voff_t, voff_t, int);
static int ao_get(struct uvm_object *, voff_t, struct vm_page **,
	int *, int, vm_prot_t, int, int);
static int ao_put(struct uvm_object *, voff_t, voff_t, int);

const struct uvm_pagerops uvm_vnodeops = {
	.pgo_get = vn_get,
	.pgo_put = vn_put,
};
const struct uvm_pagerops aobj_pager = {
	.pgo_get = ao_get,
	.pgo_put = ao_put,
};

kmutex_t uvm_pageqlock;

struct uvmexp uvmexp;
struct uvm uvm;

struct vmspace rump_vmspace;
struct vm_map rump_vmmap;

/*
 * vm pages 
 */

/* called with the object locked */
struct vm_page *
rumpvm_makepage(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	pg = kmem_zalloc(sizeof(struct vm_page), KM_SLEEP);
	pg->offset = off;
	pg->uobject = uobj;

	pg->uanon = (void *)kmem_zalloc(PAGE_SIZE, KM_SLEEP);
	pg->flags = PG_CLEAN|PG_BUSY|PG_FAKE;

	TAILQ_INSERT_TAIL(&uobj->memq, pg, listq);

	return pg;
}

/*
 * Release a page.
 *
 * Called with the vm object locked.
 */
void
uvm_pagefree(struct vm_page *pg)
{
	struct uvm_object *uobj = pg->uobject;

	if (pg->flags & PG_WANTED)
		wakeup(pg);

	TAILQ_REMOVE(&uobj->memq, pg, listq);
	kmem_free((void *)pg->uanon, PAGE_SIZE);
	kmem_free(pg, sizeof(*pg));
}

struct rumpva {
	vaddr_t addr;
	struct vm_page *pg;

	LIST_ENTRY(rumpva) entries;
};
static LIST_HEAD(, rumpva) rvahead = LIST_HEAD_INITIALIZER(rvahead);
static kmutex_t rvamtx;

void
rumpvm_enterva(vaddr_t addr, struct vm_page *pg)
{
	struct rumpva *rva;

	rva = kmem_alloc(sizeof(struct rumpva), KM_SLEEP);
	rva->addr = addr;
	rva->pg = pg;
	mutex_enter(&rvamtx);
	LIST_INSERT_HEAD(&rvahead, rva, entries);
	mutex_exit(&rvamtx);
}

void
rumpvm_flushva()
{
	struct rumpva *rva;

	mutex_enter(&rvamtx);
	while ((rva = LIST_FIRST(&rvahead)) != NULL) {
		LIST_REMOVE(rva, entries);
		kmem_free(rva, sizeof(*rva));
	}
	mutex_exit(&rvamtx);
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

	mutex_enter(&vp->v_interlock);
	return VOP_GETPAGES(vp, off, pgs, npages, centeridx, access_type,
	    advice, flags);
}

static int
vn_put(struct uvm_object *uobj, voff_t offlo, voff_t offhi, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;

	mutex_enter(&vp->v_interlock);
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
 retrylookup:
		pg = uvm_pagelookup(uobj, off + (i << PAGE_SHIFT));
		if (pg) {
			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				UVM_UNLOCK_AND_WAIT(pg, &uobj->vmobjlock, 0,
				    "aogetpg", 0);
				goto retrylookup;
			}
			pg->flags |= PG_BUSY;
			pgs[i] = pg;
		} else {
			pg = rumpvm_makepage(uobj, off + (i << PAGE_SHIFT));
			pgs[i] = pg;
		}
	}
	mutex_exit(&uobj->vmobjlock);

	return 0;

}

static int
ao_put(struct uvm_object *uobj, voff_t start, voff_t stop, int flags)
{
	struct vm_page *pg;

	/* we only free all pages for now */
	if ((flags & PGO_FREE) == 0 || (flags & PGO_ALLPAGES) == 0) {
		mutex_exit(&uobj->vmobjlock);
		return 0;
	}

	while ((pg = TAILQ_FIRST(&uobj->memq)) != NULL)
		uvm_pagefree(pg);
	mutex_exit(&uobj->vmobjlock);

	return 0;
}

struct uvm_object *
uao_create(vsize_t size, int flags)
{
	struct uvm_object *uobj;

	uobj = kmem_zalloc(sizeof(struct uvm_object), KM_SLEEP);
	uobj->pgops = &aobj_pager;
	TAILQ_INIT(&uobj->memq);
	mutex_init(&uobj->vmobjlock, MUTEX_DEFAULT, IPL_NONE);

	return uobj;
}

void
uao_detach(struct uvm_object *uobj)
{

	ao_put(uobj, 0, 0, PGO_ALLPAGES | PGO_FREE);
	kmem_free(uobj, sizeof(*uobj));
}

/*
 * UBC
 */

struct ubc_window {
	struct uvm_object	*uwin_obj;
	voff_t			uwin_off;
	uint8_t			*uwin_mem;
	size_t			uwin_mapsize;

	LIST_ENTRY(ubc_window)	uwin_entries;
};

static LIST_HEAD(, ubc_window) uwinlst = LIST_HEAD_INITIALIZER(uwinlst);
static kmutex_t uwinmtx;

int
rump_ubc_magic_uiomove(void *va, size_t n, struct uio *uio, int *rvp,
	struct ubc_window *uwinp)
{
	struct vm_page **pgs;
	int npages = len2npages(uio->uio_offset, n);
	size_t allocsize;
	int i, rv;

	if (uwinp == NULL) {
		mutex_enter(&uwinmtx);
		LIST_FOREACH(uwinp, &uwinlst, uwin_entries)
			if ((uint8_t *)va >= uwinp->uwin_mem
			    && (uint8_t *)va
			      < (uwinp->uwin_mem + uwinp->uwin_mapsize))
				break;
		mutex_exit(&uwinmtx);
		if (uwinp == NULL) {
			KASSERT(rvp != NULL);
			return 0;
		}
	}

	allocsize = npages * sizeof(pgs);
	pgs = kmem_zalloc(allocsize, KM_SLEEP);
	rv = uwinp->uwin_obj->pgops->pgo_get(uwinp->uwin_obj,
	    uwinp->uwin_off + ((uint8_t *)va - uwinp->uwin_mem),
	    pgs, &npages, 0, 0, 0, 0);
	if (rv)
		goto out;

	for (i = 0; i < npages; i++) {
		size_t xfersize;
		off_t pageoff;

		pageoff = uio->uio_offset & PAGE_MASK;
		xfersize = MIN(MIN(n, PAGE_SIZE), PAGE_SIZE-pageoff);
		uiomove((uint8_t *)pgs[i]->uanon + pageoff, xfersize, uio);
		if (uio->uio_rw == UIO_WRITE)
			pgs[i]->flags &= ~PG_CLEAN;
		n -= xfersize;
	}
	uvm_page_unbusy(pgs, npages);

 out:
	kmem_free(pgs, allocsize);
	if (rvp)
		*rvp = rv;
	return 1;
}

static struct ubc_window *
uwin_alloc(struct uvm_object *uobj, voff_t off, vsize_t len)
{
	struct ubc_window *uwinp; /* pronounced: you wimp! */

	uwinp = kmem_alloc(sizeof(struct ubc_window), KM_SLEEP);
	uwinp->uwin_obj = uobj;
	uwinp->uwin_off = off;
	uwinp->uwin_mapsize = len;
	uwinp->uwin_mem = kmem_alloc(len, KM_SLEEP);

	return uwinp;
}

static void
uwin_free(struct ubc_window *uwinp)
{

	kmem_free(uwinp->uwin_mem, uwinp->uwin_mapsize);
	kmem_free(uwinp, sizeof(struct ubc_window));
}

void *
ubc_alloc(struct uvm_object *uobj, voff_t offset, vsize_t *lenp, int advice,
	int flags)
{
	struct ubc_window *uwinp;

	uwinp = uwin_alloc(uobj, offset, *lenp);
	mutex_enter(&uwinmtx);
	LIST_INSERT_HEAD(&uwinlst, uwinp, uwin_entries);
	mutex_exit(&uwinmtx);

	DPRINTF(("UBC_ALLOC offset 0x%llx, uwin %p, mem %p\n",
	    (unsigned long long)offset, uwinp, uwinp->uwin_mem));
	
	return uwinp->uwin_mem;
}

void
ubc_release(void *va, int flags)
{
	struct ubc_window *uwinp;

	mutex_enter(&uwinmtx);
	LIST_FOREACH(uwinp, &uwinlst, uwin_entries)
		if ((uint8_t *)va >= uwinp->uwin_mem
		    && (uint8_t *)va < (uwinp->uwin_mem + uwinp->uwin_mapsize))
			break;
	mutex_exit(&uwinmtx);
	if (uwinp == NULL)
		panic("%s: releasing invalid window at %p", __func__, va);

	LIST_REMOVE(uwinp, uwin_entries);
	uwin_free(uwinp);
}

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo,
	int advice, int flags)
{
	struct ubc_window *uwinp;
	vsize_t len;

	while (todo > 0) {
		len = todo;

		uwinp = uwin_alloc(uobj, uio->uio_offset, len);
		rump_ubc_magic_uiomove(uwinp->uwin_mem, len, uio, NULL, uwinp);
		uwin_free(uwinp);

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

	uvmexp.free = 1024*1024; /* XXX */
	uvm.pagedaemon_lwp = NULL; /* doesn't match curlwp */

	mutex_init(&rvamtx, MUTEX_DEFAULT, 0);
	mutex_init(&uwinmtx, MUTEX_DEFAULT, 0);
	mutex_init(&uvm_pageqlock, MUTEX_DEFAULT, 0);
}

void
uvm_pageactivate(struct vm_page *pg)
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

/* Called with the vm object locked */
struct vm_page *
uvm_pagelookup(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	TAILQ_FOREACH(pg, &uobj->memq, listq) {
		if (pg->offset == off) {
			return pg;
		}
	}

	return NULL;
}

struct vm_page *
uvm_pageratop(vaddr_t va)
{
	struct rumpva *rva;

	mutex_enter(&rvamtx);
	LIST_FOREACH(rva, &rvahead, entries)
		if (rva->addr == va)
			break;
	mutex_exit(&rvamtx);

	if (rva == NULL)
		panic("%s: va %llu", __func__, (unsigned long long)va);

	return rva->pg;
}

void
uvm_page_unbusy(struct vm_page **pgs, int npgs)
{
	struct vm_page *pg;
	int i;

	for (i = 0; i < npgs; i++) {
		pg = pgs[i];
		if (pg == NULL)
			continue;

		KASSERT(pg->flags & PG_BUSY);
		if (pg->flags & PG_WANTED)
			wakeup(pg);
		pg->flags &= ~(PG_WANTED|PG_BUSY);
	}
}

void
uvm_estimatepageable(int *active, int *inactive)
{

	/* XXX: guessing game */
	*active = 1024;
	*inactive = 1024;
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

	if (((bp->b_flags | bp->b_cflags) & (B_READ | BC_NOCACHE)) == 0 && bioopsp)
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
		uvm_page_unbusy(pgs, npages);
	}
	kmem_free(pgs, maxpages * sizeof(pgs));

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

	return (vp->v_iflag & VI_ONWORKLST) == 0;
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

void
uvm_pageout_start(int npages)
{

	uvmexp.paging += npages;
}

void
uvm_pageout_done(int npages)
{

	uvmexp.paging -= npages;

	/*
	 * wake up either of pagedaemon or LWPs waiting for it.
	 */

	if (uvmexp.free <= uvmexp.reserve_kernel) {
		wakeup(&uvm.pagedaemon);
	} else {
		wakeup(&uvmexp.free);
	}
}
