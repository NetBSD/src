/*	$NetBSD: uvm_map.h,v 1.53.2.1 2006/06/19 04:11:44 chap Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and
 *      its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_map.h    8.3 (Berkeley) 3/15/94
 * from: Id: uvm_map.h,v 1.1.2.3 1998/02/07 01:16:55 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _UVM_UVM_MAP_H_
#define _UVM_UVM_MAP_H_

/*
 * uvm_map.h
 */

#ifdef _KERNEL

/*
 * macros
 */

/*
 * UVM_MAP_CLIP_START: ensure that the entry begins at or after
 * the starting address, if it doesn't we split the entry.
 *
 * => map must be locked by caller
 */

#define UVM_MAP_CLIP_START(MAP,ENTRY,VA,UMR) { \
	if ((VA) > (ENTRY)->start) uvm_map_clip_start(MAP,ENTRY,VA,UMR); }

/*
 * UVM_MAP_CLIP_END: ensure that the entry ends at or before
 *      the ending address, if it does't we split the entry.
 *
 * => map must be locked by caller
 */

#define UVM_MAP_CLIP_END(MAP,ENTRY,VA,UMR) { \
	if ((VA) < (ENTRY)->end) uvm_map_clip_end(MAP,ENTRY,VA,UMR); }

/*
 * extract flags
 */
#define UVM_EXTRACT_REMOVE	0x01	/* remove mapping from old map */
#define UVM_EXTRACT_CONTIG	0x02	/* try to keep it contig */
#define UVM_EXTRACT_QREF	0x04	/* use quick refs */
#define UVM_EXTRACT_FIXPROT	0x08	/* set prot to maxprot as we go */
#define UVM_EXTRACT_RESERVED	0x10	/* caller did uvm_map_reserve() */

#endif /* _KERNEL */

#include <sys/tree.h>
#include <sys/pool.h>

#include <uvm/uvm_anon.h>

/*
 * Address map entries consist of start and end addresses,
 * a VM object (or sharing map) and offset into that object,
 * and user-exported inheritance and protection information.
 * Also included is control information for virtual copy operations.
 */
struct vm_map_entry {
	RB_ENTRY(vm_map_entry)	rb_entry;	/* tree information */
	vaddr_t			ownspace;	/* free space after */
	vaddr_t			space;		/* space in subtree */
	struct vm_map_entry	*prev;		/* previous entry */
	struct vm_map_entry	*next;		/* next entry */
	vaddr_t			start;		/* start address */
	vaddr_t			end;		/* end address */
	union {
		struct uvm_object *uvm_obj;	/* uvm object */
		struct vm_map	*sub_map;	/* belongs to another map */
	} object;				/* object I point to */
	voff_t			offset;		/* offset into object */
	int			etype;		/* entry type */
	vm_prot_t		protection;	/* protection code */
	vm_prot_t		max_protection;	/* maximum protection */
	vm_inherit_t		inheritance;	/* inheritance */
	int			wired_count;	/* can be paged if == 0 */
	struct vm_aref		aref;		/* anonymous overlay */
	int			advice;		/* madvise advice */
#define uvm_map_entry_stop_copy flags
	u_int8_t		flags;		/* flags */

#define	UVM_MAP_KERNEL		0x01		/* kernel map entry */
#define	UVM_MAP_KMAPENT		0x02		/* contains map entries */
#define	UVM_MAP_FIRST		0x04		/* the first special entry */
#define	UVM_MAP_QUANTUM		0x08		/* allocated with
						 * UVM_FLAG_QUANTUM */
#define	UVM_MAP_NOMERGE		0x10		/* this entry is not mergable */

};

#define	VM_MAPENT_ISWIRED(entry)	((entry)->wired_count != 0)

/*
 *	Maps are doubly-linked lists of map entries, kept sorted
 *	by address.  A single hint is provided to start
 *	searches again from the last successful search,
 *	insertion, or removal.
 *
 *	LOCKING PROTOCOL NOTES:
 *	-----------------------
 *
 *	VM map locking is a little complicated.  There are both shared
 *	and exclusive locks on maps.  However, it is sometimes required
 *	to downgrade an exclusive lock to a shared lock, and upgrade to
 *	an exclusive lock again (to perform error recovery).  However,
 *	another thread *must not* queue itself to receive an exclusive
 *	lock while before we upgrade back to exclusive, otherwise the
 *	error recovery becomes extremely difficult, if not impossible.
 *
 *	In order to prevent this scenario, we introduce the notion of
 *	a `busy' map.  A `busy' map is read-locked, but other threads
 *	attempting to write-lock wait for this flag to clear before
 *	entering the lock manager.  A map may only be marked busy
 *	when the map is write-locked (and then the map must be downgraded
 *	to read-locked), and may only be marked unbusy by the thread
 *	which marked it busy (holding *either* a read-lock or a
 *	write-lock, the latter being gained by an upgrade).
 *
 *	Access to the map `flags' member is controlled by the `flags_lock'
 *	simple lock.  Note that some flags are static (set once at map
 *	creation time, and never changed), and thus require no locking
 *	to check those flags.  All flags which are r/w must be set or
 *	cleared while the `flags_lock' is asserted.  Additional locking
 *	requirements are:
 *
 *		VM_MAP_PAGEABLE		r/o static flag; no locking required
 *
 *		VM_MAP_INTRSAFE		r/o static flag; no locking required
 *
 *		VM_MAP_WIREFUTURE	r/w; may only be set or cleared when
 *					map is write-locked.  may be tested
 *					without asserting `flags_lock'.
 *
 *		VM_MAP_BUSY		r/w; may only be set when map is
 *					write-locked, may only be cleared by
 *					thread which set it, map read-locked
 *					or write-locked.  must be tested
 *					while `flags_lock' is asserted.
 *
 *		VM_MAP_WANTLOCK		r/w; may only be set when the map
 *					is busy, and thread is attempting
 *					to write-lock.  must be tested
 *					while `flags_lock' is asserted.
 *
 *		VM_MAP_DYING		r/o; set when a vmspace is being
 *					destroyed to indicate that updates
 *					to the pmap can be skipped.
 *
 *		VM_MAP_TOPDOWN		r/o; set when the vmspace is
 *					created if the unspecified map
 *					allocations are to be arranged in
 *					a "top down" manner.
 */
struct vm_map {
	struct pmap *		pmap;		/* Physical map */
	struct lock		lock;		/* Lock for map data */
	RB_HEAD(uvm_tree, vm_map_entry) rbhead;	/* Tree for entries */
	struct vm_map_entry	header;		/* List of entries */
	int			nentries;	/* Number of entries */
	vsize_t			size;		/* virtual size */
	int			ref_count;	/* Reference count */
	struct simplelock	ref_lock;	/* Lock for ref_count field */
	struct vm_map_entry *	hint;		/* hint for quick lookups */
	struct simplelock	hint_lock;	/* lock for hint storage */
	struct vm_map_entry *	first_free;	/* First free space hint */
	int			flags;		/* flags */
	struct simplelock	flags_lock;	/* Lock for flags field */
	unsigned int		timestamp;	/* Version number */
};

#if defined(_KERNEL)

#include <sys/callback.h>

struct vm_map_kernel {
	struct vm_map vmk_map;
	LIST_HEAD(, uvm_kmapent_hdr) vmk_kentry_free;
			/* Freelist of map entry */
	struct vm_map_entry	*vmk_merged_entries;
			/* Merged entries, kept for later splitting */

	struct callback_head vmk_reclaim_callback;
#if !defined(PMAP_MAP_POOLPAGE)
	struct pool vmk_vacache; /* kva cache */
	struct pool_allocator vmk_vacache_allocator; /* ... and its allocator */
#endif
};
#endif /* defined(_KERNEL) */

#define	VM_MAP_IS_KERNEL(map)	(vm_map_pmap(map) == pmap_kernel())

/* vm_map flags */
#define	VM_MAP_PAGEABLE		0x01		/* ro: entries are pageable */
#define	VM_MAP_INTRSAFE		0x02		/* ro: interrupt safe map */
#define	VM_MAP_WIREFUTURE	0x04		/* rw: wire future mappings */
#define	VM_MAP_BUSY		0x08		/* rw: map is busy */
#define	VM_MAP_WANTLOCK		0x10		/* rw: want to write-lock */
#define	VM_MAP_DYING		0x20		/* rw: map is being destroyed */
#define	VM_MAP_TOPDOWN		0x40		/* ro: arrange map top-down */
#define	VM_MAP_VACACHE		0x80		/* ro: use kva cache */
#define	VM_MAP_WANTVA		0x100		/* rw: want va */

#ifdef _KERNEL
struct uvm_mapent_reservation {
	struct vm_map_entry *umr_entries[2];
	int umr_nentries;
};
#define	UMR_EMPTY(umr)		((umr) == NULL || (umr)->umr_nentries == 0)
#define	UMR_GETENTRY(umr)	((umr)->umr_entries[--(umr)->umr_nentries])
#define	UMR_PUTENTRY(umr, ent)	\
	(umr)->umr_entries[(umr)->umr_nentries++] = (ent)

struct uvm_map_args {
	struct vm_map_entry *uma_prev;

	vaddr_t uma_start;
	vsize_t uma_size;

	struct uvm_object *uma_uobj;
	voff_t uma_uoffset;

	uvm_flag_t uma_flags;
};
#endif /* _KERNEL */

#ifdef _KERNEL
#define	vm_map_modflags(map, set, clear)				\
do {									\
	simple_lock(&(map)->flags_lock);				\
	(map)->flags = ((map)->flags | (set)) & ~(clear);		\
	simple_unlock(&(map)->flags_lock);				\
} while (/*CONSTCOND*/ 0)
#endif /* _KERNEL */

/*
 * globals:
 */

#ifdef _KERNEL

#ifdef PMAP_GROWKERNEL
extern vaddr_t	uvm_maxkaddr;
#endif

/*
 * protos: the following prototypes define the interface to vm_map
 */

void		uvm_map_deallocate(struct vm_map *);

int		uvm_map_clean(struct vm_map *, vaddr_t, vaddr_t, int);
void		uvm_map_clip_start(struct vm_map *, struct vm_map_entry *,
		    vaddr_t, struct uvm_mapent_reservation *);
void		uvm_map_clip_end(struct vm_map *, struct vm_map_entry *,
		    vaddr_t, struct uvm_mapent_reservation *);
struct vm_map	*uvm_map_create(pmap_t, vaddr_t, vaddr_t, int);
int		uvm_map_extract(struct vm_map *, vaddr_t, vsize_t,
		    struct vm_map *, vaddr_t *, int);
struct vm_map_entry *
		uvm_map_findspace(struct vm_map *, vaddr_t, vsize_t,
		    vaddr_t *, struct uvm_object *, voff_t, vsize_t, int);
int		uvm_map_inherit(struct vm_map *, vaddr_t, vaddr_t,
		    vm_inherit_t);
int		uvm_map_advice(struct vm_map *, vaddr_t, vaddr_t, int);
void		uvm_map_init(void);
boolean_t	uvm_map_lookup_entry(struct vm_map *, vaddr_t,
		    struct vm_map_entry **);
void		uvm_map_reference(struct vm_map *);
int		uvm_map_replace(struct vm_map *, vaddr_t, vaddr_t,
		    struct vm_map_entry *, int);
int		uvm_map_reserve(struct vm_map *, vsize_t, vaddr_t, vsize_t,
		    vaddr_t *, uvm_flag_t);
void		uvm_map_setup(struct vm_map *, vaddr_t, vaddr_t, int);
void		uvm_map_setup_kernel(struct vm_map_kernel *,
		    vaddr_t, vaddr_t, int);
struct vm_map_kernel *
		vm_map_to_kernel(struct vm_map *);
int		uvm_map_submap(struct vm_map *, vaddr_t, vaddr_t,
		    struct vm_map *);
void		uvm_unmap1(struct vm_map *, vaddr_t, vaddr_t, int);
#define	uvm_unmap(map, s, e)	uvm_unmap1((map), (s), (e), 0)
void		uvm_unmap_detach(struct vm_map_entry *,int);
void		uvm_unmap_remove(struct vm_map *, vaddr_t, vaddr_t,
		    struct vm_map_entry **, struct uvm_mapent_reservation *,
		    int);

int		uvm_map_prepare(struct vm_map *, vaddr_t, vsize_t,
		    struct uvm_object *, voff_t, vsize_t, uvm_flag_t,
		    struct uvm_map_args *);
int		uvm_map_enter(struct vm_map *, const struct uvm_map_args *,
		    struct vm_map_entry *);

int		uvm_mapent_reserve(struct vm_map *,
		    struct uvm_mapent_reservation *, int, int);
void		uvm_mapent_unreserve(struct vm_map *,
		    struct uvm_mapent_reservation *);

vsize_t		uvm_mapent_overhead(vsize_t, int);

int		uvm_mapent_trymerge(struct vm_map *,
		    struct vm_map_entry *, int);
#define	UVM_MERGE_COPYING	1

#endif /* _KERNEL */

/*
 * VM map locking operations:
 *
 *	These operations perform locking on the data portion of the
 *	map.
 *
 *	vm_map_lock_try: try to lock a map, failing if it is already locked.
 *
 *	vm_map_lock: acquire an exclusive (write) lock on a map.
 *
 *	vm_map_lock_read: acquire a shared (read) lock on a map.
 *
 *	vm_map_unlock: release an exclusive lock on a map.
 *
 *	vm_map_unlock_read: release a shared lock on a map.
 *
 *	vm_map_downgrade: downgrade an exclusive lock to a shared lock.
 *
 *	vm_map_upgrade: upgrade a shared lock to an exclusive lock.
 *
 *	vm_map_busy: mark a map as busy.
 *
 *	vm_map_unbusy: clear busy status on a map.
 *
 * Note that "intrsafe" maps use only exclusive, spin locks.  We simply
 * use the sleep lock's interlock for this.
 */

#ifdef _KERNEL
/* XXX: clean up later */
#include <sys/time.h>
#include <sys/proc.h>	/* for tsleep(), wakeup() */
#include <sys/systm.h>	/* for panic() */

static __inline boolean_t	vm_map_lock_try(struct vm_map *);
static __inline void		vm_map_lock(struct vm_map *);
extern const char vmmapbsy[];

static __inline boolean_t
vm_map_lock_try(struct vm_map *map)
{
	boolean_t rv;

	if (map->flags & VM_MAP_INTRSAFE)
		rv = simple_lock_try(&map->lock.lk_interlock);
	else {
		simple_lock(&map->flags_lock);
		if (map->flags & VM_MAP_BUSY) {
			simple_unlock(&map->flags_lock);
			return (FALSE);
		}
		rv = (lockmgr(&map->lock, LK_EXCLUSIVE|LK_NOWAIT|LK_INTERLOCK,
		    &map->flags_lock) == 0);
	}

	if (rv)
		map->timestamp++;

	return (rv);
}

static __inline void
vm_map_lock(struct vm_map *map)
{
	int error;

	if (map->flags & VM_MAP_INTRSAFE) {
		simple_lock(&map->lock.lk_interlock);
		return;
	}

 try_again:
	simple_lock(&map->flags_lock);
	while (map->flags & VM_MAP_BUSY) {
		map->flags |= VM_MAP_WANTLOCK;
		ltsleep(&map->flags, PVM, vmmapbsy, 0, &map->flags_lock);
	}

	error = lockmgr(&map->lock, LK_EXCLUSIVE|LK_SLEEPFAIL|LK_INTERLOCK,
	    &map->flags_lock);

	if (error) {
		KASSERT(error == ENOLCK);
		goto try_again;
	}

	(map)->timestamp++;
}

#ifdef DIAGNOSTIC
#define	vm_map_lock_read(map)						\
do {									\
	if ((map)->flags & VM_MAP_INTRSAFE)				\
		panic("vm_map_lock_read: intrsafe Map");		\
	(void) lockmgr(&(map)->lock, LK_SHARED, NULL);			\
} while (/*CONSTCOND*/ 0)
#else
#define	vm_map_lock_read(map)						\
	(void) lockmgr(&(map)->lock, LK_SHARED, NULL)
#endif

#define	vm_map_unlock(map)						\
do {									\
	if ((map)->flags & VM_MAP_INTRSAFE)				\
		simple_unlock(&(map)->lock.lk_interlock);		\
	else								\
		(void) lockmgr(&(map)->lock, LK_RELEASE, NULL);		\
} while (/*CONSTCOND*/ 0)

#define	vm_map_unlock_read(map)						\
	(void) lockmgr(&(map)->lock, LK_RELEASE, NULL)

#define	vm_map_downgrade(map)						\
	(void) lockmgr(&(map)->lock, LK_DOWNGRADE, NULL)

#ifdef DIAGNOSTIC
#define	vm_map_upgrade(map)						\
do {									\
	if (lockmgr(&(map)->lock, LK_UPGRADE, NULL) != 0)		\
		panic("vm_map_upgrade: failed to upgrade lock");	\
} while (/*CONSTCOND*/ 0)
#else
#define	vm_map_upgrade(map)						\
	(void) lockmgr(&(map)->lock, LK_UPGRADE, NULL)
#endif

#define	vm_map_busy(map)						\
do {									\
	simple_lock(&(map)->flags_lock);				\
	(map)->flags |= VM_MAP_BUSY;					\
	simple_unlock(&(map)->flags_lock);				\
} while (/*CONSTCOND*/ 0)

#define	vm_map_unbusy(map)						\
do {									\
	int oflags;							\
									\
	simple_lock(&(map)->flags_lock);				\
	oflags = (map)->flags;						\
	(map)->flags &= ~(VM_MAP_BUSY|VM_MAP_WANTLOCK);			\
	simple_unlock(&(map)->flags_lock);				\
	if (oflags & VM_MAP_WANTLOCK)					\
		wakeup(&(map)->flags);					\
} while (/*CONSTCOND*/ 0)

boolean_t vm_map_starved_p(struct vm_map *);

#endif /* _KERNEL */

/*
 *	Functions implemented as macros
 */
#define		vm_map_min(map)		((map)->header.end)
#define		vm_map_max(map)		((map)->header.start)
#define		vm_map_setmin(map, v)	((map)->header.end = (v))
#define		vm_map_setmax(map, v)	((map)->header.start = (v))

#define		vm_map_pmap(map)	((map)->pmap)

#endif /* _UVM_UVM_MAP_H_ */
