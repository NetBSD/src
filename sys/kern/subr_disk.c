e -1:
			goto wraparound;
		}
		if (tmp->ownspace >= length)
			goto listsearch;
	}
	if (prev == NULL)
		goto notfound;

	if (topdown) {
		KASSERT(orig_hint >= prev->next->start - length ||
		    prev->next->start - length > prev->next->start);
		hint = prev->next->start - length;
	} else {
		KASSERT(orig_hint <= prev->end);
		hint = prev->end;
	}
	switch (uvm_map_space_avail(&hint, length, uoffset, align,
	    topdown, prev)) {
	case 1:
		entry = prev;
		goto found;
	case -1:
		goto wraparound;
	}
	if (prev->ownspace >= length)
		goto listsearch;

	if (topdown)
		tmp = RB_LEFT(prev, rb_entry);
	else
		tmp = RB_RIGHT(prev, rb_entry);
	for (;;) {
		KASSERT(tmp && tmp->space >= length);
		if (topdown)
			child = RB_RIGHT(tmp, rb_entry);
		else
			child = RB_LEFT(tmp, rb_entry);
		if (child && child->space >= length) {
			tmp = child;
			continue;
		}
		if (tmp->ownspace >= length)
			break;
		if (topdown)
			tmp = RB_LEFT(tmp, rb_entry);
		else
			tmp = RB_RIGHT(tmp, rb_entry);
	}

	if (topdown) {
		KASSERT(orig_hint >= tmp->next->start - length ||
		    tmp->next->start - length > tmp->next->start);
		hint = tmp->next->start - length;
	} else {
		KASSERT(orig_hint <= tmp->end);
		hint = tmp->end;
	}
	switch (uvm_map_space_avail(&hint, length, uoffset, align,
	    topdown, tmp)) {
	case 1:
		entry = tmp;
		goto found;
	case -1:
		goto wraparound;
	}

	/*
	 * The tree fails to find an entry because of offset or alignment
	 * restrictions.  Search the list instead.
	 */
 listsearch:
	/*
	 * Look through the rest of the map, trying to fit a new region in
	 * the gap between existing regions, or after the very last region.
	 * note: entry->end = base VA of current gap,
	 *	 entry->next->start = VA of end of current gap
	 */

	for (;;) {
		/* Update hint for current gap. */
		hint = topdown ? entry->next->start - length : entry->end;

		/* See if it fits. */
		switch (uvm_map_space_avail(&hint, length, uoffset, align,
		    topdown, entry)) {
		case 1:
			goto found;
		case -1:
			goto wraparound;
		}

		/* Advance to next/previous gap */
		if (topdown) {
			if (entry == &map->header) {
				UVMHIST_LOG(maphist, "<- failed (off start)",
				    0,0,0,0);
				goto notfound;
			}
			entry = entry->prev;
		} else {
			entry = entry->next;
			if (entry == &map->header) {
				UVMHIST_LOG(maphist, "<- failed (off end)",
				    0,0,0,0);
				goto notfound;
			}
		}
	}

 found:
	SAVE_HINT(map, map->hint, entry);
	*result = hint;
	UVMHIST_LOG(maphist,"<- got it!  (result=0x%x)", hint, 0,0,0);
	KASSERT( topdown || hint >= orig_hint);
	KASSERT(!topdown || hint <= orig_hint);
	KASSERT(entry->end <= hint);
	KASSERT(hint + length <= entry->next->start);
	return (entry);

 wraparound:
	UVMHIST_LOG(maphist, "<- failed (wrap around)", 0,0,0,0);

	return (NULL);

 notfound:
	UVMHIST_LOG(maphist, "<- failed (notfound)", 0,0,0,0);

	return (NULL);
}

/*
 *   U N M A P   -   m a i n   h e l p e r   f u n c t i o n s
 */

/*
 * uvm_unmap_remove: remove mappings from a vm_map (from "start" up to "stop")
 *
 * => caller must check alignment and size
 * => map must be locked by caller
 * => we return a list of map entries that we've remove from the map
 *    in "entry_list"
 */

void
uvm_unmap_remove(struct vm_map *map, vaddr_t start, vaddr_t end,
    struct vm_map_entry **entry_list /* OUT */,
    struct uvm_mapent_reservation *umr, int flags)
{
	struct vm_map_entry *entry, *first_entry, *next;
	vaddr_t len;
	UVMHIST_FUNC("uvm_unmap_remove"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist,"(map=0x%x, start=0x%x, end=0x%x)",
	    map, start, end, 0);
	VM_MAP_RANGE_CHECK(map, start, end);

	uvm_map_check(map, "unmap_remove entry");

	/*
	 * find first entry
	 */

	if (uvm_map_lookup_entry(map, start, &first_entry) == true) {
		/* clip and go... */
		entry = first_entry;
		UVM_MAP_CLIP_START(map, entry, start, umr);
		/* critical!  prevents stale hint */
		SAVE_HINT(map, entry, entry->prev);
	} else {
		entry = first_entry->next;
	}

	/*
	 * Save the free space hint
	 */

	if (map->first_free != &map->header && map->first_free->start >= start)
		map->first_free = entry->prev;

	/*
	 * note: we now re-use first_entry for a different task.  we remove
	 * a number of map entries from the map and save them in a linked
	 * list headed by "first_entry".  once we remove them from the map
	 * the caller should unlock the map and drop the references to the
	 * backing objects [c.f. uvm_unmap_detach].  the object is to
	 * separate unmapping from reference dropping.  why?
	 *   [1] the map has to be locked for unmapping
	 *   [2] the map need not be locked for reference dropping
	 *   [3] dropping references may trigger pager I/O, and if we hit
	 *       a pager that does synchronous I/O we may have to wait for it.
	 *   [4] we would like all waiting for I/O to occur with maps unlocked
	 *       so that we don't block other threads.
	 */

	first_entry = NULL;
	*entry_list = NULL;

	/*
	 * break up the area into map entry sized regions and unmap.  note
	 * that all mappings have to be removed before we can even consider
	 * dropping references to amaps or VM objects (otherwise we could end
	 * up with a mapping to a page on the free list which would be very bad)
	 */

	while ((entry != &map->header) && (entry->start < end)) {
		KASSERT((entry->flags & UVM_MAP_FIRST) == 0);

		UVM_MAP_CLIP_END(map, entry, end, umr);
		next = entry->next;
		len = entry->end - entry->start;

		/*
		 * unwire before removing addresses from the pmap; otherwise
		 * unwiring will put the entries back into the pmap (XXX).
		 */

		if (VM_MAPENT_ISWIRED(entry)) {
			uvm_map_entry_unwire(map, entry);
		}
		if (flags & UVM_FLAG_VAONLY) {

			/* nothing */

		} else if ((map->flags & VM_MAP_PAGEABLE) == 0) {

			/*
			 * if the map is non-pageable, any pages mapped there
			 * must be wired and entered with pmap_kenter_pa(),
			 * and we should free any such pages immediately.
			 * this is mostly used for kmem_map and mb_map.
			 */

			if ((entry->flags & UVM_MAP_KMAPENT) == 0) {
				uvm_km_pgremove_intrsafe(entry->start,
				    entry->end);
				pmap_kremove(entry->start, len);
			}
		} else if (UVM_ET_ISOBJ(entry) &&
			   UVM_OBJ_IS_KERN_OBJECT(entry->object.uvm_obj)) {
			KASSERT(vm_map_pmap(map) == pmap_kernel());

			/*
			 * note: kernel object mappings are currently used in
			 * two ways:
			 *  [1] "normal" mappings of pages in the kernel object
			 *  [2] uvm_km_valloc'd allocations in which we
			 *      pmap_enter in some non-kernel-object page
			 *      (e.g. vmapbuf).
			 *
			 * for case [1], we need to remove the mapping from
			 * the pmap and then remove the page from the kernel
			 * object (because, once pages in a kernel object are
			 * unmapped they are no longer needed, unlike, say,
			 * a vnode where you might want the data to persist
			 * until flushed out of a queue).
			 *
			 * for case [2], we need to remove the mapping from
			 * the pmap.  there shouldn't be any pages at the
			 * specified offset in the kernel object [but it
			 * doesn't hurt to call uvm_km_pgremove just to be
			 * safe?]
			 *
			 * uvm_km_pgremove currently does the following:
			 *   for pages in the kernel object in range:
			 *     - drops the swap slot
			 *     - uvm_pagefree the page
			 */

			/*
			 * remove mappings from pmap and drop the pages
			 * from the object.  offsets are always relative
			 * to vm_map_min(kernel_map).
			 */

			pmap_remove(pmap_kernel(), entry->start,
			    entry->start + len);
			uvm_km_pgremove(entry->start, entry->end);

			/*
			 * null out kernel_object reference, we've just
			 * dropped it
			 */

			entry->etype &= ~UVM_ET_OBJ;
			entry->object.uvm_obj = NULL;
		} else if (UVM_ET_ISOBJ(entry) || entry->aref.ar_amap) {

			/*
			 * remove mappings the standard way.
			 */

			pmap_remove(map->pmap, entry->start, entry->end);
		}

#if defined(DEBUG)
		if ((entry->flags & UVM_MAP_KMAPENT) == 0) {

			/*
			 * check if there's remaining mapping,
			 * which is a bug in caller.
			 */

			vaddr_t va;
			for (va = entry->start; va < entry->end;
			    va += PAGE_SIZE) {
				if (pmap_extract(vm_map_pmap(map), va, NULL)) {
					panic("uvm_unmap_remove: has mapping");
				}
			}

			if (VM_MAP_IS_KERNEL(map)) {
				uvm_km_check_empty(entry->start, entry->end,
				    (map->flags & VM_MAP_INTRSAFE) != 0);
			}
		}
#endif /* defined(DEBUG) */

		/*
		 * remove entry from map and put it on our list of entries
		 * that we've nuked.  then go to next entry.
		 */

		UVMHIST_LOG(maphist, "  removed map entry 0x%x", entry, 0, 0,0);

		/* critical!  prevents stale hint */
		SAVE_HINT(map, entry, entry->prev);

		uvm_map_entry_unlink(map, entry);
		KASSERT(map->size >= len);
		map->size -= len;
		entry->prev = NULL;
		entry->next = first_entry;
		first_entry = entry;
		entry = next;
	}
	if ((map->flags & VM_MAP_DYING) == 0) {
		pmap_update(vm_map_pmap(map));
	}

	uvm_map_check(map, "unmap_remove leave");

	/*
	 * now we've cleaned up the map and are ready for the caller to drop
	 * references to the mapped objects.
	 */

	*entry_list = first_entry;
	UVMHIST_LOG(maphist,"<- done!", 0, 0, 0, 0);

	if (map->flags & VM_MAP_WANTVA) {
		mutex_enter(&map->misc_lock);
		map->flags &= ~VM_MAP_WANTVA;
		cv_broadcast(&map->cv);
		mutex_exit(&map->misc_lock);
	}
}

/*
 * uvm_unmap_detach: drop references in a chain of map entries
 *
 * => we will free the map entries as we traverse the list.
 */

void
uvm_unmap_detach(struct vm_map_entry *first_entry, int flags)
{
	struct vm_map_entry *next_entry;
	UVMHIST_FUNC("uvm_unmap_detach"); UVMHIST_CALLED(maphist);

	while (first_entry) {
		KASSERT(!VM_MAPENT_ISWIRED(first_entry));
		UVMHIST_LOG(maphist,
		    "  detach 0x%x: amap=0x%x, obj=0x%x, submap?=%d",
		    first_entry, first_entry->aref.ar_amap,
		    first_entry->object.uvm_obj,
		    UVM_ET_ISSUBMAP(first_entry));

		/*
		 * drop reference to amap, if we've got one
		 */

		if (first_entry->aref.ar_amap)
			uvm_map_unreference_amap(first_entry, flags);

		/*
		 * drop reference to our backing object, if we've got one
		 */

		KASSERT(!UVM_ET_ISSUBMAP(first_entry));
		if (UVM_ET_ISOBJ(first_entry) &&
		    first_entry->object.uvm_obj->pgops->pgo_detach) {
			(*first_entry->object.uvm_obj->pgops->pgo_detach)
				(first_entry->object.uvm_obj);
		}
		next_entry = first_entry->next;
		uvm_mapent_free(first_entry);
		first_entry = next_entry;
	}
	UVMHIST_LOG(maphist, "<- done", 0,0,0,0);
}

/*
 *   E X T R A C T I O N   F U N C T I O N S
 */

/*
 * uvm_map_reserve: reserve space in a vm_map for future use.
 *
 * => we reserve space in a map by putting a dummy map entry in the
 *    map (dummy means obj=NULL, amap=NULL, prot=VM_PROT_NONE)
 * => map should be unlocked (we will write lock it)
 * => we return true if we were able to reserve space
 * => XXXCDC: should be inline?
 */

int
uvm_map_reserve(struct vm_map *map, vsize_t size,
    vaddr_t offset	/* hint for pmap_prefer */,
    vsize_t align	/* alignment hint */,
    vaddr_t *raddr	/* IN:hint, OUT: reserved VA */,
    uvm_flag_t flags	/* UVM_FLAG_FIXED or 0 */)
{
	UVMHIST_FUNC("uvm_map_reserve"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist, "(map=0x%x, size=0x%x, offset=0x%x,addr=0x%x)",
	    map,size,offset,raddr);

	size = round_page(size);

	/*
	 * reserve some virtual space.
	 */

	if (uvm_map(map, raddr, size, NULL, offset, 0,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
	    UVM_ADV_RANDOM, UVM_FLAG_NOMERGE|flags)) != 0) {
	    UVMHIST_LOG(maphist, "<- done (no VM)", 0,0,0,0);
		return (false);
	}

	UVMHIST_LOG(maphist, "<- done (*raddr=0x%x)", *raddr,0,0,0);
	return (true);
}

/*
 * uvm_map_replace: replace a reserved (blank) area of memory with
 * real mappings.
 *
 * => caller must WRITE-LOCK the map
 * => we return true if replacement was a success
 * => we expect the newents chain to have nnewents entrys on it and
 *    we expect newents->prev to point to the last entry on the list
 * => note newents is allowed to be NULL
 */

int
uvm_map_replace(struct vm_map *map, vaddr_t start, vaddr_t end,
    struct vm_map_entry *newents, int nnewents)
{
	struct vm_map_entry *oldent, *last;

	uvm_map_check(map, "map_replace entry");

	/*
	 * first find the blank map entry at the specified address
	 */

	if (!uvm_map_lookup_entry(map, start, &oldent)) {
		return (false);
	}

	/*
	 * check to make sure we have a proper blank entry
	 */

	if (end < oldent->end && !VM_MAP_USE_KMAPENT(map)) {
		UVM_MAP_CLIP_END(map, oldent, end, NULL);
	}
	if (oldent->start != start || oldent->end != end ||
	    oldent->object.uvm_obj != NULL || oldent->aref.ar_amap != NULL) {
		return (false);
	}

#ifdef DIAGNOSTIC

	/*
	 * sanity check the newents chain
	 */

	{
		struct vm_map_entry *tmpent = newents;
		int nent = 0;
		vaddr_t cur = start;

		while (tmpent) {
			nent++;
			if (tmpent->start < cur)
				panic("uvm_map_replace1");
			if (tmpent->start > tmpent->end || tmpent->end > end) {
		printf("tmpent->start=0x%lx, tmpent->end=0x%lx, end=0x%lx\n",
			    tmpent->start, tmpent->end, end);
				panic("uvm_map_replace2");
			}
			cur = tmpent->end;
			if (tmpent->next) {
				if (tmpent->next->prev != tmpent)
					panic("uvm_map_replace3");
			} else {
				if (newents->prev != tmpent)
					panic("uvm_map_replace4");
			}
			tmpent = tmpent->next;
		}
		if (nent != nnewents)
			panic("uvm_map_replace5");
	}
#endif

	/*
	 * map entry is a valid blank!   replace it.   (this does all the
	 * work of map entry link/unlink...).
	 */

	if (newents) {
		last = newents->prev;

		/* critical: flush stale hints out of map */
		SAVE_HINT(map, map->hint, newents);
		if (map->first_free == oldent)
			map->first_free = last;

		last->next = oldent->next;
		last->next->prev = last;

		/* Fix RB tree */
		uvm_rb_remove(map, oldent);

		newents->prev = oldent->prev;
		newents->prev->next = newents;
		map->nentries = map->nentries + (nnewents - 1);

		/* Fixup the RB tree */
		{
			int i;
			struct vm_map_entry *tmp;

			tmp = newents;
			for (i = 0; i < nnewents && tmp; i++) {
				uvm_rb_insert(map, tmp);
				tmp = tmp->next;
			}
		}
	} else {
		/* NULL list of new entries: just remove the old one */
		clear_hints(map, oldent);
		uvm_map_entry_unlink(map, oldent);
	}

	uvm_map_check(map, "map_repl