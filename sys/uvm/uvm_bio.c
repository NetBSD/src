/*	$NetBSD: uvm_bio.c,v 1.1.2.3 1999/04/09 04:37:11 chs Exp $	*/

/* 
 * Copyright (c) 1998 Chuck Silvers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_uvmhist.h"

/*
 * uvm_bio.c: buffered i/o vnode mapping cache
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>

#include <uvm/uvm.h>

/*
 * global data structures
 */

/*
 * local functions
 */

static void	ubc_init __P((void));
static int	ubc_fault __P((struct uvm_faultinfo *, vaddr_t, 
			       vm_page_t *, int,
			       int, vm_fault_t, vm_prot_t, int));

static struct ubc_map *ubc_find_mapping __P((struct uvm_object *,
					     vaddr_t));

/*
 * local data structues
 */

#define UBC_HASH(uobj, offset) (((long)(uobj) / sizeof(struct uvm_object) + \
				 (offset) / MAXBSIZE) & ubc_object.hashmask)

/* XXX make this real eventually */
#define UBC_DEFAULT_READAHEAD_PAGES 1



struct ubc_map
{
	struct uvm_object *	uobj;		/* mapped object */
	vaddr_t			offset;		/* offset into uobj */
	int			refcount;	/* refcount on mapping */
	/* XXX refcount will turn into a rwlock when vnodes start
	   using their locks in shared mode. */

	vaddr_t			writeoff;	/* overwrite offset */
	vsize_t			writelen;	/* overwrite len */

	LIST_ENTRY(ubc_map)	hash;		/* hash table */
	TAILQ_ENTRY(ubc_map)	inactive;	/* inactive queue */
};


static struct ubc_object
{
	struct uvm_object uobj;		/* glue for uvm_map() */
	void *kva;			/* where ubc_object is mapped */
	struct ubc_map *umap;		/* array of ubc_map's */

	LIST_HEAD(, ubc_map) *hash;	/* hashtable for cached ubc_map's */
	u_long hashmask;		/* mask for hashtable */

	TAILQ_HEAD(, ubc_map) inactive;	/* inactive queue for ubc_map's */

} ubc_object;


struct uvm_pagerops ubc_pager =
{
	ubc_init,	/* init */
	NULL,		/* attach */
	NULL,		/* reference */
	NULL,		/* detach */
	ubc_fault,	/* fault */
	/* ... rest are NULL */
};


/* XXX */
static int ubc_nwins = 16;


/*
 * ubc_init
 *
 * init pager private data structures.
 */
static void
ubc_init()
{
	struct ubc_map *umap;
	int i;

	/*
	 * init ubc_object.
	 * alloc and init ubc_map's.
	 * init inactive queue.
	 * alloc and init hashtable.
	 * map in ubc_object.
	 */

	simple_lock_init(&ubc_object.uobj.vmobjlock);
	ubc_object.uobj.pgops = &ubc_pager;
	TAILQ_INIT(&ubc_object.uobj.memq);
	ubc_object.uobj.uo_npages = 0;
	ubc_object.uobj.uo_refs = UVM_OBJ_KERN;

	ubc_object.umap = malloc(ubc_nwins * sizeof(struct ubc_map),
				 M_TEMP, M_NOWAIT);
	if (ubc_object.umap == NULL) {
		panic("ubc_init: failed to allocate ubc_maps");
	}
	bzero(ubc_object.umap, ubc_nwins * sizeof(struct ubc_map));

	TAILQ_INIT(&ubc_object.inactive);
	for (umap = ubc_object.umap;
	     umap < &ubc_object.umap[ubc_nwins];
	     umap++) {
		TAILQ_INSERT_TAIL(&ubc_object.inactive, umap, inactive);
	}

	ubc_object.hash = hashinit(ubc_nwins / 4, M_TEMP, M_NOWAIT,
				   &ubc_object.hashmask);
	if (ubc_object.hash == NULL) {
		panic("ubc_init: failed to allocate hash\n");
	}

	for (i = 0; i <= ubc_object.hashmask; i++) {
		LIST_INIT(&ubc_object.hash[i]);
	}

	if (uvm_map(kernel_map, (vaddr_t *)&ubc_object.kva,
		    ubc_nwins * MAXBSIZE, &ubc_object.uobj, 0,
		    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
				UVM_ADV_RANDOM, UVM_FLAG_NOMERGE))
	    != KERN_SUCCESS) {
		panic("ubc_init: failed to map ubc_object\n");
	}

	/* XXX this shouldn't go here */
	{
		static struct uvm_history_ent ubchistbuf[200];
		ubchistbuf[0] = ubchistbuf[0];
		UVMHIST_INIT_STATIC(ubchist, ubchistbuf);
	}
}


/*
 * ubc_fault: fault routine for ubc mapping
 */
static int
ubc_fault(ufi, ign1, ign2, ign3, ign4, fault_type, access_type, flags)
	struct uvm_faultinfo *ufi;
	vaddr_t ign1;
	vm_page_t *ign2;
	int ign3, ign4;
	vm_fault_t fault_type;
	vm_prot_t access_type;
	int flags;
{
	struct uvm_object *uobj;
	struct uvm_vnode *uvn;
	struct ubc_map *umap;
	vaddr_t va, eva, ubc_offset, umap_offset;
	int i, rv, npages;
	struct vm_page *pages[MAXBSIZE >> PAGE_SHIFT];
	boolean_t retry;

	UVMHIST_FUNC("ubc_fault");  UVMHIST_CALLED(ubchist);

	va = ufi->orig_rvaddr;
	ubc_offset = va - (vaddr_t)ubc_object.kva;

#ifdef DEBUG
	if (ufi->entry->object.uvm_obj != &ubc_object.uobj) {
		panic("ubc_fault: not ubc_object");
	}
	if (ubc_offset >= ubc_nwins * MAXBSIZE) {
		panic("ubc_fault: fault addr 0x%lx outside ubc mapping", va);
	}
#endif

	UVMHIST_LOG(ubchist, "va 0x%lx ubc_offset 0x%lx at %d",
		    va, ubc_offset, access_type,0);

	umap = &ubc_object.umap[ubc_offset / MAXBSIZE];
	umap_offset = ubc_offset & (MAXBSIZE - 1);

#ifdef DIAGNOSTIC
	if (umap->refcount == 0) {
		panic("ubc_fault: umap %p has no refs", umap);
	}
#endif

	/* no umap locking needed since we have a ref on the umap */
	uobj = umap->uobj;
	uvn = (struct uvm_vnode *)uobj;
#ifdef DIAGNOSTIC
	if (uobj == NULL) {
		panic("ubc_fault: umap %p has null uobj", umap);
	}
#endif

	/* XXX limit npages by size of file? */
	npages = min(UBC_DEFAULT_READAHEAD_PAGES,
		     (MAXBSIZE - umap_offset) >> PAGE_SHIFT);

	/*
	 * no need to try with PGO_LOCKED...
	 * we don't need to have the map locked since we know that
	 * no one will mess with it until our reference is released.
	 */
	if (flags & PGO_LOCKED) {
		uvmfault_unlockall(ufi, NULL, &ubc_object.uobj, NULL);
		flags &= ~(PGO_LOCKED);
	}

	/*
	 * XXX
	 * uvn_get is currently pretty dumb about read-ahead,
	 * so right now this will only ever get the 1 page that we need.
	 */

again:
	/*
	 * XXX workaround for nfs.
	 * if we're writing, make sure that the vm system's notion
	 * of the vnode size is at least big enough to contain this write.
	 * this is because of the problem with nfs mentioned below.
	 * XXX this can happen for reading too, but there it really
	 * requires a second client.
	 */
	if (access_type == VM_PROT_WRITE &&
	    uvn->u_size < umap->writeoff + umap->writelen) {
		printf("ubc_fault: bumping size vp %p newsize 0x%x\n",
		       uobj, (int)(umap->writeoff + umap->writelen));
		uvm_vnp_setsize((struct vnode *)uobj,
				umap->writeoff + umap->writelen);
	}

	bzero(pages, sizeof pages);
	simple_lock(&uobj->vmobjlock);

UVMHIST_LOG(ubchist, "umap_offset 0x%x writeoff 0x%x writelen 0x%x u_size 0x%x",
	    (int)umap_offset, (int)umap->writeoff,
	    (int)umap->writelen, (int)uvn->u_size);

	if (access_type == VM_PROT_WRITE &&
	    umap_offset >= umap->writeoff &&
	    (umap_offset + PAGE_SIZE <= umap->writeoff + umap->writelen ||
	     umap_offset + PAGE_SIZE >= uvn->u_size - umap->offset)) {
UVMHIST_LOG(ubchist, "setting PGO_OVERWRITE", 0,0,0,0);
		flags |= PGO_OVERWRITE;
	}
	else { UVMHIST_LOG(ubchist, "NOT setting PGO_OVERWRITE", 0,0,0,0); }
	/* XXX be sure to zero any part of the page past EOF */

	/*
	 * XXX
	 * ideally we'd like to pre-fault all of the pages we're overwriting.
	 * so for PGO_OVERWRITE, we should call pgo_get() with all of the
	 * pages in [writeoff, writeoff+writesize] instead of just the one.
	 */

	UVMHIST_LOG(ubchist, "pgo_get vp %p offset 0x%x npages %d",
		    uobj, umap->offset + umap_offset, npages, 0);

	rv = uobj->pgops->pgo_get(uobj, umap->offset + umap_offset,
				  pages, &npages, 0, access_type, 0, flags);
	UVMHIST_LOG(ubchist, "pgo_get rv %d npages %d", rv, npages,0,0);

	switch (rv) {
	case VM_PAGER_OK:
		break;

#ifdef DIAGNOSTIC
	case VM_PAGER_PEND:
		panic("ubc_fault: pgo_get got PENDing on non-async I/O");
#endif

	case VM_PAGER_AGAIN:
		tsleep(&lbolt, PVM, "ubc_fault", 0);
		goto again;

	default:
		return rv;
	}

	if (npages == 0) {
		return VM_PAGER_OK;
	}


	retry = FALSE;
	va = ufi->orig_rvaddr;
	eva = ufi->orig_rvaddr + (npages << PAGE_SHIFT);

	UVMHIST_LOG(ubchist, "va 0x%lx eva 0x%lx", va, eva, 0,0);
	for (i = 0; va < eva; i++, va += PAGE_SIZE) {
		UVMHIST_LOG(ubchist, "pages[%d] = %p", i, pages[i],0,0);

		if (pages[i] == NULL || pages[i] == PGO_DONTCARE) {
			continue;
		}
		if (pages[i]->flags & PG_WANTED) {
			wakeup(pages[i]);
		}

#if 0
		/*
		 * since we have a reference on the uobj,
		 * it can't have been killed while we were sleeping.
		 */
#ifdef DIAGNOSTIC
		if (pages[i]->flags & PG_RELEASED) {
			panic("ubc_fault: pgo_get gave us a RELEASED page: "
			      "vp %p pg %p", uobj, pages[i]);
		}
#endif
#else
		/*
		 * unfortunately, there seems to be a way
		 * that we could get a PG_RELEASED page.
		 * if an nfs vnode that has a page in the cache is
		 * truncated by another client, we don't learn of
		 * the truncation until we try to read again.
		 * the read will succeed but it will also cause the
		 * page to become PG_RELEASED, which will trip us up.
		 * so if we get a PG_RELEASED page, free it and retry.
		 * 
		 * XXX this seems to happen even when we're the
		 * client that does the truncate.
		 */

		if (pages[i]->flags & PG_RELEASED) {
			simple_lock(&uobj->vmobjlock);
			rv = uobj->pgops->pgo_releasepg(pages[i], NULL);
#ifdef DIAGNOSTIC
			if (!rv) {
				panic("ubc_fault: object died");
			}
#endif
			simple_unlock(&uobj->vmobjlock);
			retry = TRUE;
			continue;
		}
#endif

		uvm_lock_pageq();
		uvm_pageactivate(pages[i]);
		uvm_unlock_pageq();

		/* uvmexp.fltnomap++; XXX? */
		pmap_enter(ufi->orig_map->pmap, va, VM_PAGE_TO_PHYS(pages[i]),
			   VM_PROT_ALL, FALSE);

		pages[i]->flags &= ~(PG_BUSY);
		UVM_PAGE_OWN(pages[i], NULL);
	}

	if (retry) {
		goto again;
	}

	return VM_PAGER_OK;
}



/*
 * local functions
 */
static struct ubc_map *
ubc_find_mapping(uobj, offset)
	struct uvm_object *uobj;
	vaddr_t offset;
{
	struct ubc_map *umap;

	for (umap = LIST_FIRST(&ubc_object.hash[UBC_HASH(uobj, offset)]);
	     umap != NULL;
	     umap = LIST_NEXT(umap, hash)) {
		if (umap->uobj == uobj && umap->offset == offset) {
			return umap;
		}
	}
	return NULL;
}


/*
 * ubc interface functions
 */

/*
 * ubc_alloc:  allocate a buffer mapping
 */
void *
ubc_alloc(uobj, offset, len, flags)
	struct uvm_object *uobj;
	vaddr_t offset;
	vsize_t len;
	int flags;
{
	int s;
	vaddr_t umap_offset, slot_offset;
	struct ubc_map *umap;
	UVMHIST_FUNC("ubc_alloc"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "uobj %p offset 0x%lx", uobj, offset,0,0);

	umap_offset = offset & ~(MAXBSIZE - 1);
	slot_offset = offset & (MAXBSIZE - 1);

#ifdef DIAGNOSTIC
	if (umap_offset != ((offset + len - 1) & ~(MAXBSIZE - 1))) {
		printf("uobj %p offset 0x%lx len 0x%lx flags 0x%x\n",
		       uobj, offset, len, flags);
		panic("ubc_alloc: region crosses window boundary");
	}
#endif

	/*
	 * the vnode is always locked here, so we don't need to add a ref.
	 * XXX make sure this is really true.
	 */

	s = splbio();

again:
	simple_lock(&ubc_object.uobj.vmobjlock);
	umap = ubc_find_mapping(uobj, umap_offset);
	if (umap == NULL) {
		vaddr_t va;

		umap = TAILQ_FIRST(&ubc_object.inactive);
		if (umap == NULL) {
			simple_unlock(&ubc_object.uobj.vmobjlock);
			tsleep(&lbolt, PVM, "ubc_alloc", 0);
			goto again;
		}

		/*
		 * remove from old hash (if any),
		 * add to new hash.
		 */

		if (umap->uobj != NULL) {
			LIST_REMOVE(umap, hash);
		}

		umap->uobj = uobj;
		umap->offset = umap_offset;

		LIST_INSERT_HEAD(&ubc_object.hash[UBC_HASH(uobj, umap_offset)],
				 umap, hash);

		va = (vaddr_t)(ubc_object.kva +
			       (umap - ubc_object.umap) * MAXBSIZE);
		pmap_remove(pmap_kernel(), va, va + MAXBSIZE);
	}

	if (umap->refcount == 0) {
		TAILQ_REMOVE(&ubc_object.inactive, umap, inactive);
	}

#ifdef DIAGNOSTIC
	if ((flags & UBC_WRITE) &&
	    (umap->writeoff || umap->writelen)) {
		panic("ubc_fault: concurrent writes vp %p", uobj);
	}
#endif
	if (flags & UBC_WRITE) {
		umap->writeoff = slot_offset;
		umap->writelen = len;
	}

	umap->refcount++;
	simple_unlock(&ubc_object.uobj.vmobjlock);
	splx(s);
	UVMHIST_LOG(ubchist, "umap %p refs %d va %p",
		    umap, umap->refcount,
		    ubc_object.kva + (umap - ubc_object.umap) * MAXBSIZE,0);

	return ubc_object.kva +
		(umap - ubc_object.umap) * MAXBSIZE + slot_offset;
}


void
ubc_release(va, wlen)
	void *va;
	vsize_t wlen;
{
	struct ubc_map *umap;
	struct uvm_object *uobj;
	int s;
	UVMHIST_FUNC("ubc_release"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "va %p", va,0,0,0);

	s = splbio();
	simple_lock(&ubc_object.uobj.vmobjlock);

	umap = &ubc_object.umap[(va - ubc_object.kva) / MAXBSIZE];
	uobj = umap->uobj;

#ifdef DIAGNOSTIC
	if (uobj == NULL) {
		panic("ubc_release: no mapping at va %p", va);
	}
#endif

	umap->writeoff = 0;
	umap->writelen = 0;
	umap->refcount--;
	if (umap->refcount == 0) {
		TAILQ_INSERT_TAIL(&ubc_object.inactive, umap, inactive);
	}
	UVMHIST_LOG(ubchist, "umap %p refs %d", umap, umap->refcount,0,0);
	simple_unlock(&ubc_object.uobj.vmobjlock);

	splx(s);
}


/*
 * XXX
 * we don't ever need to flush ubc mappings,
 * but this is here for now in case it's useful for debugging.
 */

void
ubc_flush(uobj, start, end)
	struct uvm_object *uobj;
	vaddr_t start, end;
{
	struct ubc_map *umap;
	int s;
	UVMHIST_FUNC("ubc_flush");  UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "uobj %p start 0x%lx end 0x%lx",
		    uobj, start, end,0);

	s = splbio(); 
	simple_lock(&ubc_object.uobj.vmobjlock);
	for (umap = ubc_object.umap;
	     umap < &ubc_object.umap[ubc_nwins];
	     umap++) {

		if (umap->uobj != uobj || 
		    umap->offset < start ||
		    (umap->offset >= end && end != 0)) {
			continue;
		}

		if (umap->refcount != 0) {
			panic("ubc_flush: flushing active "
			      "umap %p uobj %p", umap, uobj);
		}

		/*
		 * remove from hash,
		 * move to head of inactive queue.
		 */
		LIST_REMOVE(umap, hash);
		umap->uobj = NULL;
		TAILQ_REMOVE(&ubc_object.inactive, umap, inactive);
		TAILQ_INSERT_HEAD(&ubc_object.inactive, umap, inactive);
	}
	simple_unlock(&ubc_object.uobj.vmobjlock);
	splx(s);
}

void ubc_print(void);
void
ubc_print()
{
	struct ubc_map *umap;

	printf("addr        uobj        offset      refs  writeoff  writelen\n");
	for (umap = ubc_object.umap;
	     umap < &ubc_object.umap[ubc_nwins];
	     umap++) {
		if (umap->uobj == NULL) {
			continue;
		}

		printf("%p  %p  0x%08x  %2d    0x%04x    0x%04x\n",
		       ubc_object.kva + (umap - ubc_object.umap) * MAXBSIZE,
		       umap->uobj, (int)umap->offset, umap->refcount,
		       (int)umap->writeoff, (int)umap->writelen);
	}
}
