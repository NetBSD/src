/*	$NetBSD: uvm_amap.c,v 1.17.2.2 1999/02/25 04:06:53 chs Exp $	*/

/*
 * XXXCDC: "ROUGH DRAFT" QUALITY UVM PRE-RELEASE FILE!   
 *	   >>>USE AT YOUR OWN RISK, WORK IS NOT FINISHED<<<
 */
/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_amap.c,v 1.1.2.25 1998/02/06 22:49:23 chs Exp
 */

/*
 * uvm_amap.c: uvm amap ops
 */

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/pool.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>

#define UVM_AMAP		/* pull in uvm_amap.h functions */
#include <uvm/uvm.h>
#include <uvm/uvm_swap.h>


/*
 * anonblock_list: global list of anon blocks,
 * locked by swap_syscall_lock (since we never remove
 * anything from this list and we only add to it via swapctl()).
 */

struct anonblock {
	LIST_ENTRY(anonblock) list;
	int count;
	struct vm_anon *anons;
};


static LIST_HEAD(anonlist, anonblock) anonblock_list;


/*
 * pool for vm_amap_structures.
 */

struct pool uvm_amap_pool;

/*
 * local functions
 */

static struct vm_amap *amap_alloc1 __P((int, int, int));
static boolean_t anon_pagein __P((struct vm_anon *));

#ifdef VM_AMAP_PPREF
/*
 * what is ppref?   ppref is an _optional_ amap feature which is used
 * to keep track of reference counts on a per-page basis.  it is enabled
 * when VM_AMAP_PPREF is defined.
 *
 * when enabled, an array of ints is allocated for the pprefs.  this
 * array is allocated only when a partial reference is added to the
 * map (either by unmapping part of the amap, or gaining a reference
 * to only a part of an amap).  if the malloc of the array fails
 * (M_NOWAIT), then we set the array pointer to PPREF_NONE to indicate
 * that we tried to do ppref's but couldn't alloc the array so just
 * give up (after all, this is an optional feature!).
 *
 * the array is divided into page sized "chunks."   for chunks of length 1,
 * the chunk reference count plus one is stored in that chunk's slot.
 * for chunks of length > 1 the first slot contains (the reference count
 * plus one) * -1.    [the negative value indicates that the length is
 * greater than one.]   the second slot of the chunk contains the length
 * of the chunk.   here is an example:
 *
 * actual REFS:  2  2  2  2  3  1  1  0  0  0  4  4  0  1  1  1
 *       ppref: -3  4  x  x  4 -2  2 -1  3  x -5  2  1 -2  3  x
 *              <----------><-><----><-------><----><-><------->
 * (x = don't care)
 *
 * this allows us to allow one int to contain the ref count for the whole
 * chunk.    note that the "plus one" part is needed because a reference
 * count of zero is neither positive or negative (need a way to tell
 * if we've got one zero or a bunch of them).
 * 
 * here are some in-line functions to help us.
 */

static __inline void pp_getreflen __P((int *, int, int *, int *));
static __inline void pp_setreflen __P((int *, int, int, int));

/*
 * pp_getreflen: get the reference and length for a specific offset
 */
static __inline void
pp_getreflen(ppref, offset, refp, lenp)
	int *ppref, offset, *refp, *lenp;
{

	if (ppref[offset] > 0) {		/* chunk size must be 1 */
		*refp = ppref[offset] - 1;	/* don't forget to adjust */
		*lenp = 1;
	} else {
		*refp = (ppref[offset] * -1) - 1;
		*lenp = ppref[offset+1];
	}
}

/*
 * pp_setreflen: set the reference and length for a specific offset
 */
static __inline void
pp_setreflen(ppref, offset, ref, len)
	int *ppref, offset, ref, len;
{
	if (len == 1) {
		ppref[offset] = ref + 1;
	} else {
		ppref[offset] = (ref + 1) * -1;
		ppref[offset+1] = len;
	}
}
#endif

/*
 * amap_alloc1: internal function that allocates an amap, but does not
 *	init the overlay.
 *
 * => lock on returned amap is init'd
 */
static inline struct vm_amap *
amap_alloc1(slots, padslots, waitf)
	int slots, padslots, waitf;
{
	struct vm_amap *amap;
	int totalslots = slots + padslots;

	amap = pool_get(&uvm_amap_pool, (waitf == M_WAITOK) ? PR_WAITOK : 0);
	if (amap == NULL)
		return(NULL);

	simple_lock_init(&amap->am_l);
	amap->am_ref = 1;
	amap->am_flags = 0;
#ifdef VM_AMAP_PPREF
	amap->am_ppref = NULL;
#endif
	amap->am_maxslot = totalslots;
	amap->am_nslot = slots;
	amap->am_nused = 0;
	MALLOC(amap->am_slots,  int *, totalslots * sizeof(int), M_UVMAMAP, waitf);
	if (amap->am_slots) {
		MALLOC(amap->am_bckptr, int *, totalslots * sizeof(int), M_UVMAMAP, waitf);
		if (amap->am_bckptr) {
			MALLOC(amap->am_anon, struct vm_anon **, 
			    totalslots * sizeof(struct vm_anon *), M_UVMAMAP, waitf);
		}
	}

	if (amap->am_anon)
		return(amap);

	if (amap->am_slots) {
		FREE(amap->am_slots, M_UVMAMAP);
		if (amap->am_bckptr)
			FREE(amap->am_bckptr, M_UVMAMAP);
	}
	pool_put(&uvm_amap_pool, amap);
	return (NULL);
}

/*
 * amap_alloc: allocate an amap to manage "sz" bytes of anonymous VM
 *
 * => caller should ensure sz is a multiple of PAGE_SIZE
 * => reference count to new amap is set to one
 * => new amap is returned unlocked
 */

struct vm_amap *
amap_alloc(sz, padsz, waitf)
	vaddr_t sz, padsz;
	int waitf;
{
	struct vm_amap *amap;
	int slots, padslots;
	UVMHIST_FUNC("amap_alloc"); UVMHIST_CALLED(maphist);

	AMAP_B2SLOT(slots, sz);		/* load slots */
	AMAP_B2SLOT(padslots, padsz);

	amap = amap_alloc1(slots, padslots, waitf);
	if (amap)
		memset(amap->am_anon, 0, (slots + padslots) * sizeof(struct vm_anon *));

	UVMHIST_LOG(maphist,"<- done, amap = 0x%x, sz=%d", amap, sz, 0, 0);
	return(amap);
}


/*
 * amap_free: free an amap
 *
 * => amap must be locked.
 * => the amap is "gone" after we are done with it.
 */
void
amap_free(amap)
	struct vm_amap *amap;
{
	UVMHIST_FUNC("amap_free"); UVMHIST_CALLED(maphist);

#ifdef DIAGNOSTIC
	if (amap->am_ref || amap->am_nused)
		panic("amap_free");
#endif

	FREE(amap->am_slots, M_UVMAMAP);
	FREE(amap->am_bckptr, M_UVMAMAP);
	FREE(amap->am_anon, M_UVMAMAP);
#ifdef VM_AMAP_PPREF
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE)
		FREE(amap->am_ppref, M_UVMAMAP);
#endif
	simple_unlock(&amap->am_l);
	pool_put(&uvm_amap_pool, amap);

	UVMHIST_LOG(maphist,"<- done, freed amap = 0x%x", amap, 0, 0, 0);
}

/*
 * amap_extend: extend the size of an amap (if needed)
 *
 * => amap being extended should be passed in unlocked (we will lock
 *	it as needed).
 * => amap has a reference count of one (our map entry)
 * => XXXCDC: should it have a waitflag???
 */
void
amap_extend(entry, addsize)
	vm_map_entry_t entry;
	vsize_t addsize;
{
	struct vm_amap *amap = entry->aref.ar_amap;
	int slotoff = entry->aref.ar_slotoff;
	int slotmapped, slotadd, slotneed;
#ifdef VM_AMAP_PPREF
	int *newppref, *oldppref;
#endif
	u_int *newsl, *newbck, *oldsl, *oldbck;
	struct vm_anon **newover, **oldover;
	int slotadded;
	UVMHIST_FUNC("amap_extend"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist, "  (entry=0x%x, addsize=0x%x)", entry,addsize,0,0);

	/*
	 * first, determine how many slots we need in the amap.   don't forget
	 * that ar_slotoff could be non-zero: this means that there are some
	 * unused slots before us in the amap.
	 */

	simple_lock(&amap->am_l);				/* lock! */

	AMAP_B2SLOT(slotmapped, entry->end - entry->start); /* slots mapped */
	AMAP_B2SLOT(slotadd, addsize);			/* slots to add */
	slotneed = slotoff + slotmapped + slotadd;

	/*
	 * case 1: we already have enough slots in the map and thus only need
	 * to bump the reference counts on the slots we are adding.
	 */

	if (amap->am_nslot >= slotneed) {
#ifdef VM_AMAP_PPREF
		if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
			amap_pp_adjref(amap, slotoff + slotmapped, slotadd, 1);
		}
#endif
		simple_unlock(&amap->am_l);
		UVMHIST_LOG(maphist,"<- done (case 1), amap = 0x%x, sltneed=%d", 
		    amap, slotneed, 0, 0);
		return;				/* done! */
	}

	/*
	 * case 2: we pre-allocated slots for use and we just need to bump
	 * nslot up to take account for these slots.
	 */
	if (amap->am_maxslot >= slotneed) {
#ifdef VM_AMAP_PPREF
		if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
			if ((slotoff + slotmapped) < amap->am_nslot)
				amap_pp_adjref(amap, slotoff + slotmapped, 
				    (amap->am_nslot - (slotoff + slotmapped)),
				    1);
			pp_setreflen(amap->am_ppref, amap->am_nslot, 1, 
			   slotneed - amap->am_nslot);
		}
#endif
		amap->am_nslot = slotneed;
		simple_unlock(&amap->am_l);
		/*
		 * no need to zero am_anon since that was done at alloc time and we
		 * never shrink an allocation.
		 */
		UVMHIST_LOG(maphist,"<- done (case 2), amap = 0x%x, slotneed=%d", 
		    amap, slotneed, 0, 0);
		return;
	}

	/*
	 * case 3: we need to malloc a new amap and copy all the amap data over
	 *
	 * XXX: should we pad out this allocation in hopes of avoid future case3
	 * extends?
	 * XXX: how about using kernel realloc?
	 *
	 * NOTE: we have the only map that has a reference to this amap locked.
	 * thus, no one else is going to try and change the amap while it is 
	 * unlocked (but we unlock just to be safe).
	 */

	simple_unlock(&amap->am_l);		/* unlock in case we sleep in malloc */
#ifdef VM_AMAP_PPREF
	newppref = NULL;
	if (amap->am_ppref && amap->am_ppref != PPREF_NONE) {
		MALLOC(newppref, int *, slotneed * sizeof(int), M_UVMAMAP,
		    M_NOWAIT);
		if (newppref == NULL) {
			/* give up if malloc fails */
			FREE(amap->am_ppref, M_UVMAMAP);
			    amap->am_ppref = PPREF_NONE;
		}
	}
#endif
	MALLOC(newsl, int *, slotneed * sizeof(int), M_UVMAMAP, M_WAITOK);
	MALLOC(newbck, int *, slotneed * sizeof(int), M_UVMAMAP, M_WAITOK);
	MALLOC(newover, struct vm_anon **, slotneed * sizeof(struct vm_anon *),
						   M_UVMAMAP, M_WAITOK);
	simple_lock(&amap->am_l);		/* re-lock! */

#ifdef DIAGNOSTIC
	if (amap->am_maxslot >= slotneed)
		panic("amap_extend: amap changed during malloc");
#endif

	/*
	 * now copy everything over to new malloc'd areas...
	 */

	slotadded = slotneed - amap->am_nslot;

	/* do am_slots */
	oldsl = amap->am_slots;
	memcpy(newsl, oldsl, sizeof(int) * amap->am_nused);
	amap->am_slots = newsl;

	/* do am_anon */
	oldover = amap->am_anon;
	memcpy(newover, oldover, sizeof(struct vm_anon *) * amap->am_nslot);
	memset(newover + amap->am_nslot, 0, sizeof(struct vm_anon *) * slotadded);
	amap->am_anon = newover;

	/* do am_bckptr */
	oldbck = amap->am_bckptr;
	memcpy(newbck, oldbck, sizeof(int) * amap->am_nslot);
	memset(newbck + amap->am_nslot, 0, sizeof(int) * slotadded); /* XXX: needed? */
	amap->am_bckptr = newbck;

#ifdef VM_AMAP_PPREF
	/* do ppref */
	oldppref = amap->am_ppref;
	if (newppref) {
		memcpy(newppref, oldppref, sizeof(int) * amap->am_nslot);
		memset(newppref + amap->am_nslot, 0, sizeof(int) * slotadded);
		amap->am_ppref = newppref;
		if ((slotoff + slotmapped) < amap->am_nslot)
			amap_pp_adjref(amap, slotoff + slotmapped, 
			    (amap->am_nslot - (slotoff + slotmapped)), 1);
		pp_setreflen(newppref, amap->am_nslot, 1, slotadded);
	}
#endif

	/* update master values */
	amap->am_nslot = slotneed;
	amap->am_maxslot = slotneed;

	/* unlock */
	simple_unlock(&amap->am_l);

	/* and free */
	FREE(oldsl, M_UVMAMAP);
	FREE(oldbck, M_UVMAMAP);
	FREE(oldover, M_UVMAMAP);
#ifdef VM_AMAP_PPREF
	if (oldppref && oldppref != PPREF_NONE)
		FREE(oldppref, M_UVMAMAP);
#endif
	UVMHIST_LOG(maphist,"<- done (case 3), amap = 0x%x, slotneed=%d", 
	    amap, slotneed, 0, 0);
}

/*
 * amap_share_protect: change protection of an amap in a sharemap
 *
 * for sharemaps it is not possible to find all of the maps which
 * reference the sharemap (e.g. to remove or change a mapping).
 * in order to get around this (and support sharemaps) we use 
 * pmap_page_protect to change the protection on all mappings of the
 * page.   we traverse am_anon or am_slots depending on the current
 * state of the amap.
 *
 * => the map that entry belongs to must be locked by the caller.
 * => the amap pointed to by entry->aref.ar_amap must be locked by caller.
 * => the map should be locked before the amap (by the caller).
 */
void
amap_share_protect(entry, prot)
	vm_map_entry_t entry;
	vm_prot_t prot;
{
	struct vm_amap *amap = entry->aref.ar_amap;
	int slots, lcv, slot, stop;

	AMAP_B2SLOT(slots, (entry->end - entry->start));
	stop = entry->aref.ar_slotoff + slots;

	if (slots < amap->am_nused) {
		/* cheaper to traverse am_anon */
		for (lcv = entry->aref.ar_slotoff ; lcv < stop ; lcv++) {
			if (amap->am_anon[lcv] == NULL)
				continue;
			if (amap->am_anon[lcv]->u.an_page != NULL)
				pmap_page_protect(
				    PMAP_PGARG(amap->am_anon[lcv]->u.an_page),
				prot);
		}
		return;
	}

	/* cheaper to traverse am_slots */
	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {
		slot = amap->am_slots[lcv];
		if (slot < entry->aref.ar_slotoff || slot >= stop)
			continue;
		if (amap->am_anon[slot]->u.an_page != NULL)
			pmap_page_protect(
			    PMAP_PGARG(amap->am_anon[slot]->u.an_page), prot);
	}
	return;
}

/*
 * amap_wipeout: wipeout all anon's in an amap; then free the amap!
 *
 * => if amap is part of an active map entry, then the map that contains
 *	the map entry must be locked.
 * => amap's reference count should be one (the final reference).
 * => the amap must be locked by the caller.
 */

void
amap_wipeout(amap)
	struct vm_amap *amap;
{
	int lcv, slot;
	struct vm_anon *anon;
	UVMHIST_FUNC("amap_wipeout"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist,"(amap=0x%x)", amap, 0,0,0);

	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {
		int refs;

		slot = amap->am_slots[lcv];
		anon = amap->am_anon[slot];

		if (anon == NULL || anon->an_ref == 0) 
			panic("amap_wipeout: corrupt amap");

		simple_lock(&anon->an_lock); /* lock anon */

		UVMHIST_LOG(maphist,"  processing anon 0x%x, ref=%d", anon, 
		    anon->an_ref, 0, 0);

		refs = --anon->an_ref;
		simple_unlock(&anon->an_lock);
		if (refs == 0) {
			/*
			 * we had the last reference to a vm_anon. free it.
			 */
			uvm_anfree(anon);
		}
	}

	/*
	 * now we free the map
	 */

	amap->am_nused = 0;
	amap->am_ref--;		/* drop final reference */
	amap_free(amap);
	UVMHIST_LOG(maphist,"<- done!", 0,0,0,0);
}

/*
 * amap_copy: ensure that a map entry's "needs_copy" flag is false
 *	by copying the amap if necessary.
 * 
 * => an entry with a null amap pointer will get a new (blank) one.
 * => the map that the map entry belongs to must be locked by caller.
 * => the amap currently attached to "entry" (if any) must be unlocked.
 * => if canchunk is true, then we may clip the entry into a chunk
 * => "startva" and "endva" are used only if canchunk is true.  they are
 *     used to limit chunking (e.g. if you have a large space that you
 *     know you are going to need to allocate amaps for, there is no point
 *     in allowing that to be chunked)
 */

void
amap_copy(map, entry, waitf, canchunk, startva, endva)
	vm_map_t map;
	vm_map_entry_t entry;
	int waitf;
	boolean_t canchunk;
	vaddr_t startva, endva;
{
	struct vm_amap *amap, *srcamap;
	int slots, lcv;
	vaddr_t chunksize;
	UVMHIST_FUNC("amap_copy"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "  (map=%p, entry=%p, waitf=%d)",
		    map, entry, waitf, 0);

	/*
	 * is there a map to copy?   if not, create one from scratch.
	 */

	if (entry->aref.ar_amap == NULL) {

		/*
		 * check to see if we have a large amap that we can chunk.
		 * we align startva/endva to chunk-sized boundaries and then
		 * clip to them.
		 */

		if (canchunk && atop(entry->end - entry->start) >=
		    UVM_AMAP_LARGE) {
			/* convert slots to bytes */
			chunksize = UVM_AMAP_CHUNK << PAGE_SHIFT;
			startva = (startva / chunksize) * chunksize;
			endva = roundup(endva, chunksize);
			UVMHIST_LOG(maphist, "  chunk amap ==> clip 0x%x->0x%x"
			    "to 0x%x->0x%x", entry->start, entry->end, startva,
			    endva);
			UVM_MAP_CLIP_START(map, entry, startva);
			/* watch out for endva wrap-around! */
			if (endva >= startva)
				UVM_MAP_CLIP_END(map, entry, endva);
		}

		UVMHIST_LOG(maphist, "<- done [creating new amap 0x%x->0x%x]", 
		entry->start, entry->end, 0, 0);
		entry->aref.ar_slotoff = 0;
		entry->aref.ar_amap = amap_alloc(entry->end - entry->start, 0,
		    waitf);
		if (entry->aref.ar_amap != NULL)
			entry->etype &= ~UVM_ET_NEEDSCOPY;
		return;
	}

	/*
	 * first check and see if we are the only map entry referencing the amap
	 * we currently have.   if so, then we can just take it over rather
	 * than copying it.
	 */

	if (entry->aref.ar_amap->am_ref == 1) {
		entry->etype &= ~UVM_ET_NEEDSCOPY;
		UVMHIST_LOG(maphist, "<- done [ref cnt = 1, took it over]",
		    0, 0, 0, 0);
		return;
	}

	/*
	 * looks like we need to copy the map.
	 */

	UVMHIST_LOG(maphist,"  amap=%p, ref=%d, must copy it", 
	    entry->aref.ar_amap, entry->aref.ar_amap->am_ref, 0, 0);
	AMAP_B2SLOT(slots, entry->end - entry->start);
	amap = amap_alloc1(slots, 0, waitf);
	if (amap == NULL) {
		UVMHIST_LOG(maphist, "  amap_alloc1 failed", 0,0,0,0);
		return;
	}
	srcamap = entry->aref.ar_amap;
	simple_lock(&srcamap->am_l);

	/*
	 * need to double check reference count now that we've got the src amap
	 * locked down.   (in which case we lost a reference while we were
	 * mallocing the new map).
	 */

	if (srcamap->am_ref == 1) {
		/*
		 * take over the old amap, get rid of the new one we just
		 * allocated.
		 */
		entry->etype &= ~UVM_ET_NEEDSCOPY;
		amap->am_ref--;		/* drop final reference to map */
		amap_free(amap);
		simple_unlock(&srcamap->am_l);
		return;
	}

	/*
	 * copy it now.
	 */

	UVMHIST_LOG(maphist, "  copying amap now",0, 0, 0, 0);
	for (lcv = 0 ; lcv < slots; lcv++) {
		amap->am_anon[lcv] =
		    srcamap->am_anon[entry->aref.ar_slotoff + lcv];
		if (amap->am_anon[lcv] == NULL)
			continue;
		simple_lock(&amap->am_anon[lcv]->an_lock);
		amap->am_anon[lcv]->an_ref++;
		simple_unlock(&amap->am_anon[lcv]->an_lock);
		amap->am_bckptr[lcv] = amap->am_nused;
		amap->am_slots[amap->am_nused] = lcv;
		amap->am_nused++;
	}

	/*
	 * drop our reference to the old amap (srcamap) and unlock.  we will
	 * not have the very last reference to srcamap so there is no need
	 * to worry about freeing it.
	 */

	srcamap->am_ref--;
	if (srcamap->am_ref == 1 && (srcamap->am_flags & AMAP_SHARED) != 0)
		srcamap->am_flags &= ~AMAP_SHARED;   /* clear shared flag */
#ifdef VM_AMAP_PPREF
	if (srcamap->am_ppref && srcamap->am_ppref != PPREF_NONE) {
		amap_pp_adjref(srcamap, entry->aref.ar_slotoff, 
		    (entry->end - entry->start) >> PAGE_SHIFT, -1);
	}
#endif

	simple_unlock(&srcamap->am_l);

	/*
	 * install new amap.
	 */

	entry->aref.ar_slotoff = 0;
	entry->aref.ar_amap = amap;
	entry->etype &= ~UVM_ET_NEEDSCOPY;

	/*
	 * done!
	 */
	UVMHIST_LOG(maphist, "<- done",0, 0, 0, 0);
}

/*
 * amap_cow_now: resolve all copy-on-write faults in an amap now for fork(2)
 *
 *	called during fork(2) when the parent process has a wired map
 *	entry.   in that case we want to avoid write-protecting pages
 *	in the parent's map (e.g. like what you'd do for a COW page)
 *	so we resolve the COW here.
 *
 * => assume parent's entry was wired, thus all pages are resident.
 * => assume pages that are loaned out (loan_count) are already mapped
 *	read-only in all maps, and thus no need for us to worry about them
 * => assume both parent and child vm_map's are locked
 * => caller passes child's map/entry in to us
 * => if we run out of memory we will unlock the amap and sleep _with_ the
 *	parent and child vm_map's locked(!).    we have to do this since
 *	we are in the middle of a fork(2) and we can't let the parent
 *	map change until we are done copying all the map entrys.
 * => XXXCDC: out of memory should cause fork to fail, but there is
 *	currently no easy way to do this (needs fix)
 * => page queues must be unlocked (we may lock them)
 */

void
amap_cow_now(map, entry)
	struct vm_map *map;
	struct vm_map_entry *entry;
{
	struct vm_amap *amap = entry->aref.ar_amap;
	int lcv, slot;
	struct vm_anon *anon, *nanon;
	struct vm_page *pg, *npg;

	/*
	 * note that if we unlock the amap then we must ReStart the "lcv" for
	 * loop because some other process could reorder the anon's in the
	 * am_anon[] array on us while the lock is dropped.
	 */
ReStart:
	simple_lock(&amap->am_l);

	for (lcv = 0 ; lcv < amap->am_nused ; lcv++) {

		/*
		 * get the page
		 */

		slot = amap->am_slots[lcv];
		anon = amap->am_anon[slot];
		simple_lock(&anon->an_lock);
		pg = anon->u.an_page;

		/*
		 * page must be resident since parent is wired
		 */

		if (pg == NULL)
		    panic("amap_cow_now: non-resident wired page in anon %p",
			anon);

		/*
		 * if the anon ref count is one and the page is not loaned,
		 * then we are safe (the child has exclusive access to the
		 * page).  if the page is loaned, then it must already be
		 * mapped read-only.
		 *
		 * we only need to get involved when these are not true.
		 * [note: if loan_count == 0, then the anon must own the page]
		 */

		if (anon->an_ref > 1 && pg->loan_count == 0) {

			/*
			 * if the page is busy then we have to unlock, wait for
			 * it and then restart.
			 */
			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				simple_unlock(&amap->am_l);
				UVM_UNLOCK_AND_WAIT(pg, &anon->an_lock, FALSE,
				    "cownow", 0);
				goto ReStart;
			}

			/*
			 * ok, time to do a copy-on-write to a new anon
			 */
			nanon = uvm_analloc();
			if (nanon)
				npg = uvm_pagealloc(NULL, 0, nanon);
			else
				npg = NULL;	/* XXX: quiet gcc warning */

			if (nanon == NULL || npg == NULL) {
				/* out of memory */
				/*
				 * XXXCDC: we should cause fork to fail, but
				 * we can't ...
				 */
				if (nanon) {
					simple_lock(&nanon->an_lock);
					uvm_anfree(nanon);
				}
				simple_unlock(&anon->an_lock);
				simple_unlock(&amap->am_l);
				uvm_wait("cownowpage");
				goto ReStart;
			}
	
			/*
			 * got it... now we can copy the data and replace anon
			 * with our new one...
			 */
			uvm_pagecopy(pg, npg);		/* old -> new */
			anon->an_ref--;			/* can't drop to zero */
			amap->am_anon[slot] = nanon;	/* replace */

			/*
			 * drop PG_BUSY on new page ... since we have had it's
			 * owner locked the whole time it can't be
			 * PG_RELEASED | PG_WANTED.
			 */
			npg->flags &= ~(PG_BUSY|PG_FAKE);
			UVM_PAGE_OWN(npg, NULL);
			uvm_lock_pageq();
			uvm_pageactivate(npg);
			uvm_unlock_pageq();
		}

		simple_unlock(&anon->an_lock);
		/*
		 * done with this anon, next ...!
		 */

	}	/* end of 'for' loop */

	return;
}

/*
 * amap_splitref: split a single reference into two seperate references
 *
 * => caller must lock map which is referencing the amap
 * => caller must not lock amap referenced (we will do it)
 */
void
amap_splitref(origref, splitref, offset)
	struct vm_aref *origref, *splitref;
	vaddr_t offset;
{
	int leftslots;
	UVMHIST_FUNC("amap_splitref"); UVMHIST_CALLED(maphist);

	AMAP_B2SLOT(leftslots, offset);
	if (leftslots == 0)
		panic("amap_splitref: split at zero offset");

	/*
	 * lock the amap
	 */
	simple_lock(&origref->ar_amap->am_l);

	/*
	 * now: amap is locked and we have a valid am_mapped array.
	 */

	if (origref->ar_amap->am_nslot - origref->ar_slotoff - leftslots <= 0)
		panic("amap_splitref: map size check failed");

#ifdef VM_AMAP_PPREF
        /*
	 * establish ppref before we add a duplicate reference to the amap
	 */
	if (origref->ar_amap->am_ppref == NULL)
		amap_pp_establish(origref->ar_amap);
#endif

	splitref->ar_amap = origref->ar_amap;
	splitref->ar_amap->am_ref++;		/* not a share reference */
	splitref->ar_slotoff = origref->ar_slotoff + leftslots;

	simple_unlock(&origref->ar_amap->am_l);
}

#ifdef VM_AMAP_PPREF

/*
 * amap_pp_establish: add a ppref array to an amap, if possible
 *
 * => amap locked by caller
 */
void
amap_pp_establish(amap)
	struct vm_amap *amap;
{

	MALLOC(amap->am_ppref, int *, sizeof(int) * amap->am_maxslot,
	    M_UVMAMAP, M_NOWAIT);

	/*
	 * if we fail then we just won't use ppref for this amap
	 */
	if (amap->am_ppref == NULL) {
		amap->am_ppref = PPREF_NONE;	/* not using it */
		return;
	}

	/*
	 * init ppref
	 */
	memset(amap->am_ppref, 0, sizeof(int) * amap->am_maxslot);
	pp_setreflen(amap->am_ppref, 0, amap->am_ref, amap->am_nslot);
	return;
}

/*
 * amap_pp_adjref: adjust reference count to a part of an amap using the
 * per-page reference count array.
 *
 * => map and amap locked by caller
 * => caller must check that ppref != PPREF_NONE before calling
 */
void
amap_pp_adjref(amap, curslot, slotlen, adjval)
	struct vm_amap *amap;
	int curslot;
	int slotlen;
	int adjval;
{
	int stopslot, *ppref, lcv;
	int ref, len;

	/*
	 * get init values
	 */

	stopslot = curslot + slotlen;
	ppref = amap->am_ppref;

	/*
	 * first advance to the correct place in the ppref array, fragment
	 * if needed.
	 */

	for (lcv = 0 ; lcv < curslot ; lcv += len) {
		pp_getreflen(ppref, lcv, &ref, &len);
		if (lcv + len > curslot) {     /* goes past start? */
			pp_setreflen(ppref, lcv, ref, curslot - lcv);
			pp_setreflen(ppref, curslot, ref, len - (curslot -lcv));
			len = curslot - lcv;   /* new length of entry @ lcv */
		}
	}

	/*
	 * now adjust reference counts in range (make sure we dont overshoot)
	 */

	if (lcv != curslot)
		panic("ADJREF");

	for (/* lcv already set */; lcv < stopslot ; lcv += len) {
		pp_getreflen(ppref, lcv, &ref, &len);
		if (lcv + len > stopslot) {     /* goes past end? */
			pp_setreflen(ppref, lcv, ref, stopslot - lcv);
			pp_setreflen(ppref, stopslot, ref,
			    len - (stopslot - lcv));
			len = stopslot - lcv;
		}
		ref = ref + adjval;    /* ADJUST! */
		if (ref < 0)
			panic("amap_pp_adjref: negative reference count");
		pp_setreflen(ppref, lcv, ref, len);
		if (ref == 0)
			amap_wiperange(amap, lcv, len);
	}

}

/*
 * amap_wiperange: wipe out a range of an amap
 * [different from amap_wipeout because the amap is kept intact]
 *
 * => both map and amap must be locked by caller.
 */
void
amap_wiperange(amap, slotoff, slots)
	struct vm_amap *amap;
	int slotoff, slots;
{
	int byanon, lcv, stop, curslot, ptr;
	struct vm_anon *anon;
	UVMHIST_FUNC("amap_wiperange"); UVMHIST_CALLED(maphist);

	/*
	 * we can either traverse the amap by am_anon or by am_slots depending
	 * on which is cheaper.    decide now.
	 */

	if (slots < amap->am_nused) {
		byanon = TRUE;
		lcv = slotoff;
		stop = slotoff + slots;
	} else {
		byanon = FALSE;
		lcv = 0;
		stop = amap->am_nused;
	}

	/*
	 * ok, now do it!
	 */

	for (; lcv < stop; lcv++) {
		int refs;

		/*
		 * verify the anon is ok.
		 */
		if (byanon) {
			if (amap->am_anon[lcv] == NULL)
				continue;
			curslot = lcv;
		} else {
			curslot = amap->am_slots[lcv];
			if (curslot < slotoff || curslot >= stop)
				continue;
		}
		anon = amap->am_anon[curslot];

		/*
		 * remove it from the amap
		 */
		amap->am_anon[curslot] = NULL;
		ptr = amap->am_bckptr[curslot];
		if (ptr != (amap->am_nused - 1)) {
			amap->am_slots[ptr] =
			    amap->am_slots[amap->am_nused - 1];
			amap->am_bckptr[amap->am_slots[ptr]] =
			    ptr;    /* back ptr. */
		}
		amap->am_nused--;

		/*
		 * drop anon reference count
		 */
		simple_lock(&anon->an_lock);
		refs = --anon->an_ref;
		simple_unlock(&anon->an_lock);
		if (refs == 0) {
			/*
			 * we just eliminated the last reference to an anon.
			 * free it.
			 */
			uvm_anfree(anon);
		}
	}
}

#endif

/*
 * allocate anons
 */
void
uvm_anon_init()
{
	simple_lock_init(&uvm.afreelock);
	LIST_INIT(&anonblock_list);

	/*
	 * Initialize the vm_amap pool.
	 */
	pool_init(&uvm_amap_pool, sizeof(struct vm_amap), 0, 0, 0,
	    "amappl", 0,
	    pool_page_alloc_nointr, pool_page_free_nointr, M_UVMAMAP);

	/* XXXCDC: how many anons do we need to start with? */
	uvm_anon_add(uvmexp.free, FALSE);
}

/*
 * add some more anons to the free pool.  called when we add
 * more swap space.
 */
void
uvm_anon_add(pages, canwait)
	int pages;
	boolean_t canwait;
{
	struct anonblock *anonblock;
	struct vm_anon *anon;
	int lcv, flags;

	uvmexp.nanonneeded += pages;
	pages = uvmexp.nanonneeded - uvmexp.nanon;

	if (pages <= 0)
	    return;

	flags = canwait ? M_WAITOK : M_NOWAIT;

	MALLOC(anonblock, struct anonblock *, sizeof(*anonblock), M_UVMAMAP,
	       flags);
	anon = (void *)uvm_km_alloc(kernel_map, sizeof(*anon) * pages);

	/* XXX Should wait for VM to free up. */
	if (anonblock == NULL || anon == NULL) {
		printf("uvm_anon_add: can not allocate %d anons\n", pages);
		panic("uvm_anon_add");
	}

	anonblock->count = pages;
	anonblock->anons = anon;
	LIST_INSERT_HEAD(&anonblock_list, anonblock, list);

	memset(anon, 0, sizeof(*anon) * pages);

	simple_lock(&uvm.afreelock);
	uvmexp.nanon += pages;
	uvmexp.nfreeanon += pages;
	for (lcv = 0; lcv < pages; lcv++) {
		simple_lock_init(&anon[lcv].an_lock);
		anon[lcv].u.an_nxt = uvm.afree;
		uvm.afree = &anon[lcv];
	}
	simple_unlock(&uvm.afreelock);
}

/*
 * allocate an anon
 */
struct vm_anon *
uvm_analloc()
{
	struct vm_anon *a;

	simple_lock(&uvm.afreelock);
	a = uvm.afree;
	if (a) {
		uvm.afree = a->u.an_nxt;
		uvmexp.nfreeanon--;
		a->an_ref = 1;
#ifdef DEBUG
		if (a->an_swslot != 0)
		    panic("anon modified on free list: anon %p swslot %d\n",
			  a, a->an_swslot);
#endif
		a->u.an_page = NULL;		/* so we can free quickly */
	}
	simple_unlock(&uvm.afreelock);
	return(a);
}

/*
 * uvm_anfree: free a single anon structure
 *
 * => caller must remove anon from its amap before calling (if it was in
 *	an amap).
 * => anon must be unlocked and have a zero reference count.
 * => we may lock the pageq's.
 */
void
uvm_anfree(anon)
	struct vm_anon *anon;
{
	struct vm_page *pg;
	UVMHIST_FUNC("uvm_anfree"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist,"(anon=0x%x)", anon, 0,0,0);

#ifdef	DIAGNOSTIC
	if (anon->an_ref)
		panic("uvm_anfree: anon still has refs");
#endif

	/*
	 * get page
	 */

	pg = anon->u.an_page;

	/*
	 * if there is a resident page and it is loaned, then anon may not
	 * own it.   call out to uvm_anon_lockpage() to ensure the real owner
 	 * of the page has been identified and locked.
	 */

	if (pg && pg->loan_count)
		pg = uvm_anon_lockloanpg(anon);

	/*
	 * if we have a resident page, we must dispose of it before freeing
	 * the anon.
	 */

	if (pg) {

		/*
		 * if the page is owned by a uobject (now locked), then we must 
		 * kill the loan on the page rather than free it.
		 */

		if (pg->uobject) {

			/* kill loan */
			uvm_lock_pageq();
#ifdef DIAGNOSTIC
			if (pg->loan_count < 1)
				panic("uvm_anfree: obj owned page "
				      "with no loan count");
#endif
			pg->loan_count--;
			pg->uanon = NULL;
			uvm_unlock_pageq();
			simple_unlock(&pg->uobject->vmobjlock);

		} else {

			/*
			 * page has no uobject, so we must be the owner of it.
			 *
			 * if page is busy then we just mark it as released
			 * (who ever has it busy must check for this when they
			 * wake up).    if the page is not busy then we can
			 * free it now.
			 */

			if ((pg->flags & PG_BUSY) != 0) {
				/* tell them to dump it when done */
				pg->flags |= PG_RELEASED;
				simple_unlock(&anon->an_lock);
				UVMHIST_LOG(maphist,
				    "  anon 0x%x, page 0x%x: BUSY (released!)", 
				    anon, pg, 0, 0);
				return;
			} 

			pmap_page_protect(PMAP_PGARG(pg), VM_PROT_NONE);
			uvm_lock_pageq();	/* lock out pagedaemon */
			uvm_pagefree(pg);	/* bye bye */
			uvm_unlock_pageq();	/* free the daemon */

			UVMHIST_LOG(maphist,"  anon 0x%x, page 0x%x: freed now!", 
			    anon, pg, 0, 0);
		}
	}

	/*
	 * are we using any backing store resources?   if so, free them.
	 */
	if (anon->an_swslot) {
		/*
		 * on backing store: no I/O in progress.  sole amap reference
		 * is ours and we've got it locked down.   thus we can free,
		 * and be done.
		 */
		UVMHIST_LOG(maphist,"  freeing anon 0x%x, paged to swslot 0x%x",
		    anon, anon->an_swslot, 0, 0);
		uvm_swap_free(anon->an_swslot, 1);
		anon->an_swslot = 0;

		if (pg == NULL) {
			/* this page is no longer only in swap. */
			uvmexp.swpguniq--;
		}
	} 

	/*
	 * now that we've stripped the data areas from the anon, free the anon
	 * itself!
	 */

	(void) simple_lock_try(&anon->an_lock);
	simple_unlock(&anon->an_lock);

	simple_lock(&uvm.afreelock);
	anon->u.an_nxt = uvm.afree;
	uvm.afree = anon;
	uvmexp.nfreeanon++;
	simple_unlock(&uvm.afreelock);
	UVMHIST_LOG(maphist,"<- done!",0,0,0,0);
}

/*
 * uvm_anon_lockloanpg: given a locked anon, lock its resident page
 *
 * => anon is locked by caller
 * => on return: anon is locked
 *		 if there is a resident page:
 *			if it has a uobject, it is locked by us
 *			if it is ownerless, we take over as owner
 *		 we return the resident page (it can change during
 *		 this function)
 * => note that the only time an anon has an ownerless resident page
 *	is if the page was loaned from a uvm_object and the uvm_object
 *	disowned it
 * => this only needs to be called when you want to do an operation
 *	on an anon's resident page and that page has a non-zero loan
 *	count.
 */
struct vm_page *
uvm_anon_lockloanpg(anon)
	struct vm_anon *anon;
{
	struct vm_page *pg;
	boolean_t locked = FALSE;

	/*
	 * loop while we have a resident page that has a non-zero loan count.
	 * if we successfully get our lock, we will "break" the loop.
	 * note that the test for pg->loan_count is not protected -- this
	 * may produce false positive results.   note that a false positive
	 * result may cause us to do more work than we need to, but it will
	 * not produce an incorrect result.
	 */

	while (((pg = anon->u.an_page) != NULL) && pg->loan_count != 0) {

		/*
		 * quickly check to see if the page has an object before
		 * bothering to lock the page queues.   this may also produce
		 * a false positive result, but that's ok because we do a real
		 * check after that.
		 *
		 * XXX: quick check -- worth it?   need volatile?
		 */

		if (pg->uobject) {

			uvm_lock_pageq();
			if (pg->uobject) {	/* the "real" check */
				locked =
				    simple_lock_try(&pg->uobject->vmobjlock);
			} else {
				/* object disowned before we got PQ lock */
				locked = TRUE;
			}
			uvm_unlock_pageq();

			/*
			 * if we didn't get a lock (try lock failed), then we
			 * toggle our anon lock and try again
			 */

			if (!locked) {
				simple_unlock(&anon->an_lock);
				/*
				 * someone locking the object has a chance to
				 * lock us right now
				 */
				simple_lock(&anon->an_lock);
				continue;		/* start over */
			}
		}

		/*
		 * if page is un-owned [i.e. the object dropped its ownership],
		 * then we can take over as owner!
		 */

		if (pg->uobject == NULL && (pg->pqflags & PQ_ANON) == 0) {
			uvm_lock_pageq();
			pg->pqflags |= PQ_ANON;		/* take ownership... */
			pg->loan_count--;	/* ... and drop our loan */
			uvm_unlock_pageq();
		}

		/*
		 * we did it!   break the loop
		 */
		break;
	}

	/*
	 * done!
	 */

	return(pg);
}



/*
 * page in every page in every amap that is paged-out to a range of swslots.
 * 
 * nothing should be locked.
 */
boolean_t
anon_swap_off(startslot, endslot)
int startslot, endslot;
{
	struct anonblock *anonblock;

	for (anonblock = anonblock_list.lh_first;
	     anonblock != NULL;
	     anonblock = anonblock->list.le_next)
	{
		int i;

		/*
		 * loop thru all the anons in the anonblock,
		 * paging in where needed.
		 */
		for (i = 0; i < anonblock->count; i++) {
			struct vm_anon *anon = &anonblock->anons[i];
			int slot;

			/*
			 * lock anon to work on it.
			 */
			simple_lock(&anon->an_lock);

			/*
			 * is this anon's swap slot in range?
			 */
			slot = anon->an_swslot;
			if (slot >= startslot && slot < endslot) {
				int rv;

				/*
				 * yup, page it in.
				 */
				rv = anon_pagein(anon);
				if (rv) {
					return rv;
				}
			}
			else
			{
				/*
				 * nope, unlock and proceed.
				 */
				simple_unlock(&anon->an_lock);
			}
		}
	}

	/*
	 * done with traversal, unlock the list
	 */
	return FALSE;
}


/*
 * fetch an anon's page.
 *
 * anon must be locked, and is unlocked upon return.
 */
static boolean_t
anon_pagein(anon)
struct vm_anon *anon;
{
	struct vm_page *pg;
	int rv;
	UVMHIST_FUNC("anon_pagein"); UVMHIST_CALLED(pdhist);

	/*
	 * check if page is resident.
	 */

restart:
	while ((pg = anon->u.an_page) != NULL) {

		/*
		 * page is already resident.
		 * if the page is busy or released,
		 * wait for it and try again.
		 */

		if ((pg->flags & (PG_BUSY|PG_RELEASED)) != 0) {
			pg->flags |= PG_WANTED;
			UVM_UNLOCK_AND_WAIT(pg, &anon->an_lock, FALSE,
					    "anon_pagein", 0);
			simple_lock(&anon->an_lock);
			continue;
		}

		/*
		 * page is ours.
		 * mark it as dirty,
		 * mark the anon as no longer being in swap,
		 * and free the swap slot.
		 */

		pg->flags &= ~(PG_CLEAN);
		uvm_swap_free(anon->an_swslot, 1);
		anon->an_swslot = 0;

		simple_unlock(&anon->an_lock);
		return FALSE;
	}

	/*
	 * page is not resident.
	 * try to allocate a page, start over if we fail.
	 */

	while ((pg = uvm_pagealloc(NULL, 0, anon)) == NULL) {
		boolean_t nomem;

		/*
		 * if we are out of swap here, then we have to fail.
		 */
		/* XXX what locks these? */
		nomem = uvmexp.swpages <= uvmexp.swpginuse;

		if (nomem) {
			simple_unlock(&anon->an_lock);
			return TRUE;
		}

		/*
		 * otherwise wait for free pages.
		 */
		simple_unlock(&anon->an_lock);
		uvm_wait("anon_pagein");
		simple_lock(&anon->an_lock);
		goto restart;
	}

	/*
	 * fetch the page from swap.
	 * everything is unlocked.
	 */
	simple_unlock(&anon->an_lock);
	rv = uvm_swap_get(pg, anon->an_swslot, PGO_SYNCIO);

	switch (rv) {
	case VM_PAGER_OK:
		break;

	case VM_PAGER_AGAIN:
		/*
		 * sleep a bit, then try again.
		 */
		tsleep(&lbolt, PVM, "anon_pagein2", 0);
		simple_lock(&anon->an_lock);
		goto restart;

	default:
		panic("anon_pagein: uvm_swap_get -> %d", rv);
	}

	/*
	 * relock to finish up.
	 */
	simple_lock(&anon->an_lock);

	/*
	 * handle wanted pages
	 */
	if (pg->flags & PG_WANTED) {
		wakeup(pg);
	}

	/*
	 * handle released pages
	 */
	if (pg->flags & PG_RELEASED) {
		uvm_anfree(anon);
		return FALSE;
	}

	/*
	 * ok, we've got the page now.
	 * mark it as dirty, clear its swslot and un-busy it.
	 */
	uvm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
	pg->flags &= ~(PG_BUSY|PG_CLEAN);
	UVM_PAGE_OWN(pg, NULL);

	/*
	 * deactivate the page (to put it on a page queue)
	 */
	pmap_clear_reference(PMAP_PGARG(pg));
	pmap_page_protect(PMAP_PGARG(pg), VM_PROT_NONE);
	uvm_lock_pageq();
	uvm_pagedeactivate(pg);
	uvm_unlock_pageq();

	/*
	 * unlock and we're done.
	 */
	simple_unlock(&anon->an_lock);
	return FALSE;
}
