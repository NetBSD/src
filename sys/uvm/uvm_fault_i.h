/*	$NetBSD: uvm_fault_i.h,v 1.5 1998/03/09 00:58:56 mrg Exp $	*/

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
 * from: Id: uvm_fault_i.h,v 1.1.6.1 1997/12/08 16:07:12 chuck Exp
 */

#ifndef _UVM_UVM_FAULT_I_H_
#define _UVM_UVM_FAULT_I_H_

/*
 * uvm_fault_i.h: fault inline functions
 */

/*
 * uvmfault_unlockmaps: unlock the maps
 */

static __inline void
uvmfault_unlockmaps(ufi, write_locked)
	struct uvm_faultinfo *ufi;
	boolean_t write_locked;
{

	if (write_locked) {
		vm_map_unlock(ufi->map);
		if (ufi->parent_map) vm_map_unlock(ufi->parent_map);
	} else {
		vm_map_unlock_read(ufi->map);
		if (ufi->parent_map) vm_map_unlock_read(ufi->parent_map);
	}
}

/*
 * uvmfault_unlockall: unlock everything passed in.
 *
 * => maps must be read-locked (not write-locked).
 */

static __inline void
uvmfault_unlockall(ufi, amap, uobj, anon)
	struct uvm_faultinfo *ufi;
	struct vm_amap *amap;
	struct uvm_object *uobj;
	struct vm_anon *anon;
{

	if (anon)
		simple_unlock(&anon->an_lock);
	if (uobj)
		simple_unlock(&uobj->vmobjlock);
	if (amap)
		simple_unlock(&amap->am_l);
	uvmfault_unlockmaps(ufi, FALSE);
}

/*
 * uvmfault_lookup: lookup a virtual address in a map
 *
 * => caller must provide a uvm_faultinfo structure with the (IN)
 *	params properly filled in
 * => we will lookup the map entry and fill in parent_map, etc, as we go
 * => if the lookup is a success we will return with the maps locked
 * => if "write_lock" is TRUE, we write_lock the map, otherwise we only
 *	get a read lock.
 * => currently we require sharemaps to have the same virtual addresses
 *	as the main maps they are attached to.   in other words, the share
 *	map starts at zero and goes to the map user address.   the main
 *	map references it by setting its offset to be the same as the
 *	starting virtual address.    if we ever wanted to have sharemaps
 *	have different virtual addresses than main maps we would calculate
 *	it like:
 *		share_va = (rvaddr - entry->start) + entry->offset
 *	[i.e. offset from start of map entry plus offset of mapping]
 *	since (entry->start == entry->offset), share_va must equal rvaddr.
 *	if we need to change this we should store share_va in rvaddr
 *	and move rvaddr to orig_rvaddr.
 */

static __inline boolean_t
uvmfault_lookup(ufi, write_lock)
	struct uvm_faultinfo *ufi;
	boolean_t write_lock;
{
	vm_map_t tmpmap;

	/*
	 * init ufi values for lookup.
	 */

	ufi->map = ufi->orig_map;
	ufi->rvaddr = ufi->orig_rvaddr;
	ufi->parent_map = NULL;
	ufi->size = ufi->orig_size;

	/*
	 * keep going down levels until we are done.   note that there can
	 * only be two levels so we won't loop very long.
	 */

	while (1) {

		/*
		 * lock map
		 */
		if (write_lock) {
			vm_map_lock(ufi->map);
		} else {
			vm_map_lock_read(ufi->map);
		}

		/*
		 * lookup
		 */
		if (!uvm_map_lookup_entry(ufi->map, ufi->rvaddr, &ufi->entry)) {
			uvmfault_unlockmaps(ufi, write_lock);
			return(FALSE);
		}

		/*
		 * reduce size if necessary
		 */
		if (ufi->entry->end - ufi->rvaddr < ufi->size)
			ufi->size = ufi->entry->end - ufi->rvaddr;

		/*
		 * submap?    replace map with the submap and lookup again.
		 * note: VAs in submaps must match VAs in main map.
		 */
		if (UVM_ET_ISSUBMAP(ufi->entry)) {
			if (ufi->parent_map)
				panic("uvmfault_lookup: submap inside a "
				    "sharemap (illegal)");
			tmpmap = ufi->entry->object.sub_map;
			if (write_lock) {
				vm_map_unlock(ufi->map);
			} else {
				vm_map_unlock_read(ufi->map);
			}
			ufi->map = tmpmap;
			continue;
		}

		/*
		 * share map?  drop down a level.   already taken care of
		 * submap case.
		 */
		if (UVM_ET_ISMAP(ufi->entry)) {
			if (ufi->parent_map)
				panic("uvmfault_lookup: sharemap inside a "
				    "sharemap (illegal)");
			ufi->parent_map = ufi->map;
			ufi->parentv = ufi->parent_map->timestamp;
			ufi->map = ufi->entry->object.share_map;
#ifdef DIAGNOSTIC
			/* see note above */
			if (ufi->entry->offset != ufi->entry->start)
				panic("uvmfault_lookup: sharemap VA != "
				    "mainmap VA (not supported)");
#endif
			continue;
		}
		
		/*
		 * got it!
		 */

		ufi->mapv = ufi->map->timestamp;
		return(TRUE);

	}	/* while loop */

	/*NOTREACHED*/
}

/*
 * uvmfault_relock: attempt to relock the same version of the map
 *
 * => fault data structures should be unlocked before calling.
 * => if a success (TRUE) maps will be locked after call.
 */

static __inline boolean_t
uvmfault_relock(ufi)
	struct uvm_faultinfo *ufi;
{

	uvmexp.fltrelck++;
	/*
	 * simply relock parent (if any) then map in order.   fail if version
	 * mismatch (in which case nothing gets locked).
	 */

	if (ufi->parent_map) {
		vm_map_lock_read(ufi->parent_map);
		if (ufi->parentv != ufi->parent_map->timestamp) {
			vm_map_unlock_read(ufi->parent_map);
			return(FALSE);
		}
	}

	vm_map_lock_read(ufi->map);
	if (ufi->mapv != ufi->map->timestamp) {
		if (ufi->parent_map)
			vm_map_unlock_read(ufi->parent_map);
		vm_map_unlock_read(ufi->map);
		return(FALSE);
	}

	uvmexp.fltrelckok++;
	return(TRUE);		/* got it! */
}

#endif /* _UVM_UVM_FAULT_I_H_ */
