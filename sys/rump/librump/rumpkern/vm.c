/*	$NetBSD: vm.c,v 1.30.4.3 2009/08/19 18:48:30 yamt Exp $	*/

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
 *  + anon objects & pager
 *  + misc support routines
 *  + kmem
 */

/*
 * XXX: we abuse pg->uanon for the virtual address of the storage
 * for each page.  phys_addr would fit the job description better,
 * except that it will create unnecessary lossage on some platforms
 * due to not being a pointer type.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vm.c,v 1.30.4.3 2009/08/19 18:48:30 yamt Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/kmem.h>

#include <machine/pmap.h>

#include <rump/rumpuser.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_readahead.h>

#include "rump_private.h"

static int ao_get(struct uvm_object *, voff_t, struct vm_page **,
	int *, int, vm_prot_t, int, int);
static int ao_put(struct uvm_object *, voff_t, voff_t, int);

const struct uvm_pagerops aobj_pager = {
	.pgo_get = ao_get,
	.pgo_put = ao_put,
};

kmutex_t uvm_pageqlock;

struct uvmexp uvmexp;
struct uvm uvm;

struct vmspace rump_vmspace;
struct vm_map rump_vmmap;
static struct vm_map_kernel kmem_map_store;
struct vm_map *kmem_map = &kmem_map_store.vmk_map;
const struct rb_tree_ops uvm_page_tree_ops;

static struct vm_map_kernel kernel_map_store;
struct vm_map *kernel_map = &kernel_map_store.vmk_map;

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

	TAILQ_INSERT_TAIL(&uobj->memq, pg, listq.queue);
	uobj->uo_npages++;

	return pg;
}

/* these are going away very soon */
void rumpvm_enterva(vaddr_t addr, struct vm_page *pg) {}
void rumpvm_flushva(struct uvm_object *uobj) {}

struct vm_page *
uvm_pagealloc_strat(struct uvm_object *uobj, voff_t off, struct vm_anon *anon,
	int flags, int strat, int free_list)
{

	return rumpvm_makepage(uobj, off);
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

	uobj->uo_npages--;
	TAILQ_REMOVE(&uobj->memq, pg, listq.queue);
	kmem_free((void *)pg->uanon, PAGE_SIZE);
	kmem_free(pg, sizeof(*pg));
}

void
uvm_pagezero(struct vm_page *pg)
{

	pg->flags &= ~PG_CLEAN;
	memset((void *)pg->uanon, 0, PAGE_SIZE);
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

	mutex_enter(&uobj->vmobjlock);
	ao_put(uobj, 0, 0, PGO_ALLPAGES | PGO_FREE);
	mutex_destroy(&uobj->vmobjlock);
	kmem_free(uobj, sizeof(*uobj));
}

/*
 * Misc routines
 */

static kmutex_t pagermtx;

void
rumpvm_init(void)
{

	uvmexp.free = 1024*1024; /* XXX */
	uvm.pagedaemon_lwp = NULL; /* doesn't match curlwp */
	rump_vmspace.vm_map.pmap = pmap_kernel();

	mutex_init(&pagermtx, MUTEX_DEFAULT, 0);
	mutex_init(&uvm_pageqlock, MUTEX_DEFAULT, 0);

	kernel_map->pmap = pmap_kernel();
	callback_head_init(&kernel_map_store.vmk_reclaim_callback, IPL_VM);
	kmem_map->pmap = pmap_kernel();
	callback_head_init(&kmem_map_store.vmk_reclaim_callback, IPL_VM);
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

int
uvm_mmap(struct vm_map *map, vaddr_t *addr, vsize_t size, vm_prot_t prot,
	vm_prot_t maxprot, int flags, void *handle, voff_t off, vsize_t locklim)
{

	panic("%s: unimplemented", __func__);
}

struct pagerinfo {
	vaddr_t pgr_kva;
	int pgr_npages;
	struct vm_page **pgr_pgs;
	bool pgr_read;

	LIST_ENTRY(pagerinfo) pgr_entries;
};
static LIST_HEAD(, pagerinfo) pagerlist = LIST_HEAD_INITIALIZER(pagerlist);

/*
 * Pager "map" in routine.  Instead of mapping, we allocate memory
 * and copy page contents there.  Not optimal or even strictly
 * correct (the caller might modify the page contents after mapping
 * them in), but what the heck.  Assumes UVMPAGER_MAPIN_WAITOK.
 */
vaddr_t
uvm_pagermapin(struct vm_page **pgs, int npages, int flags)
{
	struct pagerinfo *pgri;
	vaddr_t curkva;
	int i;

	/* allocate structures */
	pgri = kmem_alloc(sizeof(*pgri), KM_SLEEP);
	pgri->pgr_kva = (vaddr_t)kmem_alloc(npages * PAGE_SIZE, KM_SLEEP);
	pgri->pgr_npages = npages;
	pgri->pgr_pgs = kmem_alloc(sizeof(struct vm_page *) * npages, KM_SLEEP);
	pgri->pgr_read = (flags & UVMPAGER_MAPIN_READ) != 0;

	/* copy contents to "mapped" memory */
	for (i = 0, curkva = pgri->pgr_kva;
	    i < npages;
	    i++, curkva += PAGE_SIZE) {
		/*
		 * We need to copy the previous contents of the pages to
		 * the window even if we are reading from the
		 * device, since the device might not fill the contents of
		 * the full mapped range and we will end up corrupting
		 * data when we unmap the window.
		 */
		memcpy((void*)curkva, pgs[i]->uanon, PAGE_SIZE);
		pgri->pgr_pgs[i] = pgs[i];
	}

	mutex_enter(&pagermtx);
	LIST_INSERT_HEAD(&pagerlist, pgri, pgr_entries);
	mutex_exit(&pagermtx);

	return pgri->pgr_kva;
}

/*
 * map out the pager window.  return contents from VA to page storage
 * and free structures.
 *
 * Note: does not currently support partial frees
 */
void
uvm_pagermapout(vaddr_t kva, int npages)
{
	struct pagerinfo *pgri;
	vaddr_t curkva;
	int i;

	mutex_enter(&pagermtx);
	LIST_FOREACH(pgri, &pagerlist, pgr_entries) {
		if (pgri->pgr_kva == kva)
			break;
	}
	KASSERT(pgri);
	if (pgri->pgr_npages != npages)
		panic("uvm_pagermapout: partial unmapping not supported");
	LIST_REMOVE(pgri, pgr_entries);
	mutex_exit(&pagermtx);

	if (pgri->pgr_read) {
		for (i = 0, curkva = pgri->pgr_kva;
		    i < pgri->pgr_npages;
		    i++, curkva += PAGE_SIZE) {
			memcpy(pgri->pgr_pgs[i]->uanon,(void*)curkva,PAGE_SIZE);
		}
	}

	kmem_free(pgri->pgr_pgs, npages * sizeof(struct vm_page *));
	kmem_free((void*)pgri->pgr_kva, npages * PAGE_SIZE);
	kmem_free(pgri, sizeof(*pgri));
}

/*
 * convert va in pager window to page structure.
 * XXX: how expensive is this (global lock, list traversal)?
 */
struct vm_page *
uvm_pageratop(vaddr_t va)
{
	struct pagerinfo *pgri;
	struct vm_page *pg = NULL;
	int i;

	mutex_enter(&pagermtx);
	LIST_FOREACH(pgri, &pagerlist, pgr_entries) {
		if (pgri->pgr_kva <= va
		    && va < pgri->pgr_kva + pgri->pgr_npages*PAGE_SIZE)
			break;
	}
	if (pgri) {
		i = (va - pgri->pgr_kva) >> PAGE_SHIFT;
		pg = pgri->pgr_pgs[i];
	}
	mutex_exit(&pagermtx);

	return pg;
}

/* Called with the vm object locked */
struct vm_page *
uvm_pagelookup(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	TAILQ_FOREACH(pg, &uobj->memq, listq.queue) {
		if (pg->offset == off) {
			return pg;
		}
	}

	return NULL;
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
		if (pg->flags & PG_RELEASED)
			uvm_pagefree(pg);
		else
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

struct vm_map_kernel *
vm_map_to_kernel(struct vm_map *map)
{

	return (struct vm_map_kernel *)map;
}

bool
vm_map_starved_p(struct vm_map *map)
{

	return false;
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

/* XXX: following two are unfinished because lwp's are not refcounted yet */
void
uvm_lwp_hold(struct lwp *l)
{

	atomic_inc_uint(&l->l_holdcnt);
}

void
uvm_lwp_rele(struct lwp *l)
{

	atomic_dec_uint(&l->l_holdcnt);
}

int
uvm_loan(struct vm_map *map, vaddr_t start, vsize_t len, void *v, int flags)
{

	panic("%s: unimplemented", __func__);
}

void
uvm_unloan(void *v, int npages, int flags)
{

	panic("%s: unimplemented", __func__);
}

int
uvm_loanuobjpages(struct uvm_object *uobj, voff_t pgoff, int orignpages,
	struct vm_page **opp)
{

	panic("%s: unimplemented", __func__);
}

void
uvm_object_printit(struct uvm_object *uobj, bool full,
	void (*pr)(const char *, ...))
{

	/* nada for now */
}

int
uvm_readahead(struct uvm_object *uobj, off_t off, off_t size)
{

	/* nada for now */
	return 0;
}

/*
 * Kmem
 */

#ifndef RUMP_USE_REAL_ALLOCATORS
void
kmem_init()
{

	/* nothing to do */
}

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
#endif /* RUMP_USE_REAL_ALLOCATORS */

/*
 * UVM km
 */

vaddr_t
uvm_km_alloc(struct vm_map *map, vsize_t size, vsize_t align, uvm_flag_t flags)
{
	void *rv;
	int alignbit, error;

	alignbit = 0;
	if (align) {
		alignbit = ffs(align)-1;
	}

	rv = rumpuser_anonmmap(size, alignbit, flags & UVM_KMF_EXEC, &error);
	if (rv == NULL) {
		if (flags & (UVM_KMF_CANFAIL | UVM_KMF_NOWAIT))
			return 0;
		else
			panic("uvm_km_alloc failed");
	}

	if (flags & UVM_KMF_ZERO)
		memset(rv, 0, size);

	return (vaddr_t)rv;
}

void
uvm_km_free(struct vm_map *map, vaddr_t vaddr, vsize_t size, uvm_flag_t flags)
{

	rumpuser_unmap((void *)vaddr, size);
}

struct vm_map *
uvm_km_suballoc(struct vm_map *map, vaddr_t *minaddr, vaddr_t *maxaddr,
	vsize_t size, int pageable, bool fixed, struct vm_map_kernel *submap)
{

	return (struct vm_map *)417416;
}

vaddr_t
uvm_km_alloc_poolpage(struct vm_map *map, bool waitok)
{

	return (vaddr_t)rumpuser_malloc(PAGE_SIZE, !waitok);
}

void
uvm_km_free_poolpage(struct vm_map *map, vaddr_t addr)
{

	rumpuser_unmap((void *)addr, PAGE_SIZE);
}

vaddr_t
uvm_km_alloc_poolpage_cache(struct vm_map *map, bool waitok)
{
	void *rv;
	int error;

	rv = rumpuser_anonmmap(PAGE_SIZE, PAGE_SHIFT, 0, &error);
	if (rv == NULL && waitok)
		panic("fixme: poolpage alloc failed");

	return (vaddr_t)rv;
}

void
uvm_km_free_poolpage_cache(struct vm_map *map, vaddr_t vaddr)
{

	rumpuser_unmap((void *)vaddr, PAGE_SIZE);
}

/*
 * Mapping and vm space locking routines.
 * XXX: these don't work for non-local vmspaces
 */
int
uvm_vslock(struct vmspace *vs, void *addr, size_t len, vm_prot_t access)
{

	KASSERT(vs == &rump_vmspace);
	return 0;
}

void
uvm_vsunlock(struct vmspace *vs, void *addr, size_t len)
{

	KASSERT(vs == &rump_vmspace);
}

void
vmapbuf(struct buf *bp, vsize_t len)
{

	bp->b_saveaddr = bp->b_data;
}

void
vunmapbuf(struct buf *bp, vsize_t len)
{

	bp->b_data = bp->b_saveaddr;
	bp->b_saveaddr = 0;
}

void
uvm_wait(const char *msg)
{

	/* nothing to wait for */
}

/*
 * page life cycle stuff.  it really doesn't exist, so just stubs.
 */

void
uvm_pageactivate(struct vm_page *pg)
{

	/* nada */
}

void
uvm_pagedeactivate(struct vm_page *pg)
{

	/* nada */
}

void
uvm_pagedequeue(struct vm_page *pg)
{

	/* nada*/
}

void
uvm_pageenqueue(struct vm_page *pg)
{

	/* nada */
}
