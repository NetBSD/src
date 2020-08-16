/*	$NetBSD: uvm_vnode.c,v 1.117 2020/08/16 00:24:41 chs Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.
 * Copyright (c) 1990 University of Utah.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *      @(#)vnode_pager.c       8.8 (Berkeley) 2/13/94
 * from: Id: uvm_vnode.c,v 1.1.2.26 1998/02/02 20:38:07 chuck Exp
 */

/*
 * uvm_vnode.c: the vnode pager.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_vnode.c,v 1.117 2020/08/16 00:24:41 chs Exp $");

#ifdef _KERNEL_OPT
#include "opt_uvmhist.h"
#endif

#include <sys/atomic.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/pool.h>
#include <sys/mount.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_page_array.h>

#ifdef UVMHIST
UVMHIST_DEFINE(ubchist);
#endif

/*
 * functions
 */

static void	uvn_alloc_ractx(struct uvm_object *);
static void	uvn_detach(struct uvm_object *);
static int	uvn_get(struct uvm_object *, voff_t, struct vm_page **, int *,
			int, vm_prot_t, int, int);
static void	uvn_markdirty(struct uvm_object *);
static int	uvn_put(struct uvm_object *, voff_t, voff_t, int);
static void	uvn_reference(struct uvm_object *);

static int	uvn_findpage(struct uvm_object *, voff_t, struct vm_page **,
			     unsigned int, struct uvm_page_array *a,
			     unsigned int);

/*
 * master pager structure
 */

const struct uvm_pagerops uvm_vnodeops = {
	.pgo_reference = uvn_reference,
	.pgo_detach = uvn_detach,
	.pgo_get = uvn_get,
	.pgo_put = uvn_put,
	.pgo_markdirty = uvn_markdirty,
};

/*
 * the ops!
 */

/*
 * uvn_reference
 *
 * duplicate a reference to a VM object.  Note that the reference
 * count must already be at least one (the passed in reference) so
 * there is no chance of the uvn being killed or locked out here.
 *
 * => caller must call with object unlocked.
 * => caller must be using the same accessprot as was used at attach time
 */

static void
uvn_reference(struct uvm_object *uobj)
{
	vref((struct vnode *)uobj);
}


/*
 * uvn_detach
 *
 * remove a reference to a VM object.
 *
 * => caller must call with object unlocked and map locked.
 */

static void
uvn_detach(struct uvm_object *uobj)
{
	vrele((struct vnode *)uobj);
}

/*
 * uvn_put: flush page data to backing store.
 *
 * => object must be locked on entry!   VOP_PUTPAGES must unlock it.
 * => flags: PGO_SYNCIO -- use sync. I/O
 */

static int
uvn_put(struct uvm_object *uobj, voff_t offlo, voff_t offhi, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;
	int error;

	KASSERT(rw_write_held(uobj->vmobjlock));
	error = VOP_PUTPAGES(vp, offlo, offhi, flags);

	return error;
}

/*
 * uvn_get: get pages (synchronously) from backing store
 *
 * => prefer map unlocked (not required)
 * => object must be locked!  we will _unlock_ it before starting any I/O.
 * => flags: PGO_LOCKED: fault data structures are locked
 * => NOTE: offset is the offset of pps[0], _NOT_ pps[centeridx]
 * => NOTE: caller must check for released pages!!
 */

static int
uvn_get(struct uvm_object *uobj, voff_t offset,
    struct vm_page **pps /* IN/OUT */,
    int *npagesp /* IN (OUT if PGO_LOCKED)*/,
    int centeridx, vm_prot_t access_type, int advice, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;
	int error;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(ubchist, "vp %#jx off 0x%jx", (uintptr_t)vp, offset,
	    0, 0);

	if (vp->v_type == VREG && (access_type & VM_PROT_WRITE) == 0
	    && (flags & PGO_LOCKED) == 0 && vp->v_tag != VT_TMPFS) {
		uvn_alloc_ractx(uobj);
		uvm_ra_request(vp->v_ractx, advice, uobj, offset,
		    *npagesp << PAGE_SHIFT);
	}

	error = VOP_GETPAGES(vp, offset, pps, npagesp, centeridx,
			     access_type, advice, flags);

	KASSERT(((flags & PGO_LOCKED) != 0 && rw_lock_held(uobj->vmobjlock)) ||
	    (flags & PGO_LOCKED) == 0);
	return error;
}

/*
 * uvn_markdirty: called when the object gains first dirty page
 *
 * => uobj must be write locked.
 */

static void
uvn_markdirty(struct uvm_object *uobj)
{
	struct vnode *vp = (struct vnode *)uobj;

	KASSERT(rw_write_held(uobj->vmobjlock));

	mutex_enter(vp->v_interlock);
	if ((vp->v_iflag & VI_ONWORKLST) == 0) {
		vn_syncer_add_to_worklist(vp, filedelay);
	}
	mutex_exit(vp->v_interlock);
}

/*
 * uvn_findpages:
 * return the page for the uobj and offset requested, allocating if needed.
 * => uobj must be locked.
 * => returned pages will be BUSY.
 */

int
uvn_findpages(struct uvm_object *uobj, voff_t offset, unsigned int *npagesp,
    struct vm_page **pgs, struct uvm_page_array *a, unsigned int flags)
{
	unsigned int count, found, npages;
	int i, rv;
	struct uvm_page_array a_store;

	if (a == NULL) {
		/*
		 * XXX fragile API
		 * note that the array can be the one supplied by the caller of
		 * uvn_findpages.  in that case, fillflags used by the caller
		 * might not match strictly with ours.
		 * in particular, the caller might have filled the array
		 * without DENSE but passed us UFP_DIRTYONLY (thus DENSE).
		 */
		const unsigned int fillflags =
		    ((flags & UFP_BACKWARD) ? UVM_PAGE_ARRAY_FILL_BACKWARD : 0) |
		    ((flags & UFP_DIRTYONLY) ?
		    (UVM_PAGE_ARRAY_FILL_DIRTY|UVM_PAGE_ARRAY_FILL_DENSE) : 0);
		a = &a_store;
		uvm_page_array_init(a, uobj, fillflags);
	}
	count = found = 0;
	npages = *npagesp;
	if (flags & UFP_BACKWARD) {
		for (i = npages - 1; i >= 0; i--, offset -= PAGE_SIZE) {
			rv = uvn_findpage(uobj, offset, &pgs[i], flags, a,
			    i + 1);
			if (rv == 0) {
				if (flags & UFP_DIRTYONLY)
					break;
			} else
				found++;
			count++;
		}
	} else {
		for (i = 0; i < npages; i++, offset += PAGE_SIZE) {
			rv = uvn_findpage(uobj, offset, &pgs[i], flags, a,
			    npages - i);
			if (rv == 0) {
				if (flags & UFP_DIRTYONLY)
					break;
			} else
				found++;
			count++;
		}
	}
	if (a == &a_store) {
		uvm_page_array_fini(a);
	}
	*npagesp = count;
	return (found);
}

/*
 * uvn_findpage: find a single page
 *
 * if a suitable page was found, put it in *pgp and return 1.
 * otherwise return 0.
 */

static int
uvn_findpage(struct uvm_object *uobj, voff_t offset, struct vm_page **pgp,
    unsigned int flags, struct uvm_page_array *a, unsigned int nleft)
{
	struct vm_page *pg;
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(ubchist, "vp %#jx off 0x%jx", (uintptr_t)uobj, offset,
	    0, 0);

	/*
	 * NOBUSY must come with NOWAIT and NOALLOC.  if NOBUSY is
	 * specified, this may be called with a reader lock.
	 */

	KASSERT(rw_lock_held(uobj->vmobjlock));
	KASSERT((flags & UFP_NOBUSY) == 0 || (flags & UFP_NOWAIT) != 0);
	KASSERT((flags & UFP_NOBUSY) == 0 || (flags & UFP_NOALLOC) != 0);
	KASSERT((flags & UFP_NOBUSY) != 0 || rw_write_held(uobj->vmobjlock));

	if (*pgp != NULL) {
		UVMHIST_LOG(ubchist, "dontcare", 0,0,0,0);
		goto skip_offset;
	}
	for (;;) {
		/*
		 * look for an existing page.
		 */
		pg = uvm_page_array_fill_and_peek(a, offset, nleft);
		if (pg != NULL && pg->offset != offset) {
			struct vm_page __diagused *tpg;
			KASSERT(
			    ((a->ar_flags & UVM_PAGE_ARRAY_FILL_BACKWARD) != 0)
			    == (pg->offset < offset));
			KASSERT((tpg = uvm_pagelookup(uobj, offset)) == NULL ||
				((a->ar_flags & UVM_PAGE_ARRAY_FILL_DIRTY) != 0 &&
				 !uvm_obj_page_dirty_p(tpg)));
			pg = NULL;
			if ((a->ar_flags & UVM_PAGE_ARRAY_FILL_DENSE) != 0) {
				UVMHIST_LOG(ubchist, "dense", 0,0,0,0);
				return 0;
			}
		}

		/* nope?  allocate one now */
		if (pg == NULL) {
			if (flags & UFP_NOALLOC) {
				UVMHIST_LOG(ubchist, "noalloc", 0,0,0,0);
				return 0;
			}
			pg = uvm_pagealloc(uobj, offset, NULL,
			    UVM_FLAG_COLORMATCH);
			if (pg == NULL) {
				if (flags & UFP_NOWAIT) {
					UVMHIST_LOG(ubchist, "nowait",0,0,0,0);
					return 0;
				}
				rw_exit(uobj->vmobjlock);
				uvm_wait("uvnfp1");
				uvm_page_array_clear(a);
				rw_enter(uobj->vmobjlock, RW_WRITER);
				continue;
			}
			UVMHIST_LOG(ubchist, "alloced %#jx (color %ju)",
			    (uintptr_t)pg, VM_PGCOLOR(pg), 0, 0);
			KASSERTMSG(uvm_pagegetdirty(pg) ==
			    UVM_PAGE_STATUS_CLEAN, "page %p not clean", pg);
			break;
		} else if (flags & UFP_NOCACHE) {
			UVMHIST_LOG(ubchist, "nocache",0,0,0,0);
			goto skip;
		}

		/* page is there, see if we need to wait on it */
		if ((pg->flags & PG_BUSY) != 0) {
			if (flags & UFP_NOWAIT) {
				UVMHIST_LOG(ubchist, "nowait",0,0,0,0);
				goto skip;
			}
			UVMHIST_LOG(ubchist, "wait %#jx (color %ju)",
			    (uintptr_t)pg, VM_PGCOLOR(pg), 0, 0);
			uvm_pagewait(pg, uobj->vmobjlock, "uvnfp2");
			uvm_page_array_clear(a);
			rw_enter(uobj->vmobjlock, RW_WRITER);
			continue;
		}

		/* skip PG_RDONLY pages if requested */
		if ((flags & UFP_NORDONLY) && (pg->flags & PG_RDONLY)) {
			UVMHIST_LOG(ubchist, "nordonly",0,0,0,0);
			goto skip;
		}

		/* stop on clean pages if requested */
		if (flags & UFP_DIRTYONLY) {
			const bool dirty = uvm_pagecheckdirty(pg, false);
			if (!dirty) {
				UVMHIST_LOG(ubchist, "dirtonly", 0,0,0,0);
				return 0;
			}
		}

		/* mark the page BUSY and we're done. */
		if ((flags & UFP_NOBUSY) == 0) {
			pg->flags |= PG_BUSY;
			UVM_PAGE_OWN(pg, "uvn_findpage");
		}
		UVMHIST_LOG(ubchist, "found %#jx (color %ju)",
		    (uintptr_t)pg, VM_PGCOLOR(pg), 0, 0);
		uvm_page_array_advance(a);
		break;
	}
	*pgp = pg;
	return 1;

 skip_offset:
	/*
	 * skip this offset
	 */
	pg = uvm_page_array_peek(a);
	if (pg != NULL) {
		if (pg->offset == offset) {
			uvm_page_array_advance(a);
		} else {
			KASSERT((a->ar_flags & UVM_PAGE_ARRAY_FILL_DENSE) == 0);
		}
	}
	return 0;

 skip:
	/*
	 * skip this page
	 */
	KASSERT(pg != NULL);
	uvm_page_array_advance(a);
	return 0;
}

/*
 * uvm_vnp_setsize: grow or shrink a vnode uobj
 *
 * grow   => just update size value
 * shrink => toss un-needed pages
 *
 * => we assume that the caller has a reference of some sort to the
 *	vnode in question so that it will not be yanked out from under
 *	us.
 */

void
uvm_vnp_setsize(struct vnode *vp, voff_t newsize)
{
	struct uvm_object *uobj = &vp->v_uobj;
	voff_t pgend = round_page(newsize);
	voff_t oldsize;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	rw_enter(uobj->vmobjlock, RW_WRITER);
	UVMHIST_LOG(ubchist, "vp %#jx old 0x%jx new 0x%jx",
	    (uintptr_t)vp, vp->v_size, newsize, 0);

	/*
	 * now check if the size has changed: if we shrink we had better
	 * toss some pages...
	 */

	KASSERT(newsize != VSIZENOTSET && newsize >= 0);
	KASSERT(vp->v_size <= vp->v_writesize);
	KASSERT(vp->v_size == vp->v_writesize ||
	    newsize == vp->v_writesize || newsize <= vp->v_size);

	oldsize = vp->v_writesize;

	/*
	 * check whether size shrinks
	 * if old size hasn't been set, there are no pages to drop
	 * if there was an integer overflow in pgend, then this is no shrink
	 */
	if (oldsize > pgend && oldsize != VSIZENOTSET && pgend >= 0) {
		(void) uvn_put(uobj, pgend, 0, PGO_FREE | PGO_SYNCIO);
		rw_enter(uobj->vmobjlock, RW_WRITER);
	}
	mutex_enter(vp->v_interlock);
	vp->v_size = vp->v_writesize = newsize;
	mutex_exit(vp->v_interlock);
	rw_exit(uobj->vmobjlock);
}

void
uvm_vnp_setwritesize(struct vnode *vp, voff_t newsize)
{

	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	KASSERT(newsize != VSIZENOTSET && newsize >= 0);
	KASSERT(vp->v_size != VSIZENOTSET);
	KASSERT(vp->v_writesize != VSIZENOTSET);
	KASSERT(vp->v_size <= vp->v_writesize);
	KASSERT(vp->v_size <= newsize);
	mutex_enter(vp->v_interlock);
	vp->v_writesize = newsize;
	mutex_exit(vp->v_interlock);
	rw_exit(vp->v_uobj.vmobjlock);
}

bool
uvn_text_p(struct uvm_object *uobj)
{
	struct vnode *vp = (struct vnode *)uobj;
	int iflag;

	/*
	 * v_interlock is not held here, but VI_EXECMAP is only ever changed
	 * with the vmobjlock held too.
	 */
	iflag = atomic_load_relaxed(&vp->v_iflag);
	return (iflag & VI_EXECMAP) != 0;
}

static void
uvn_alloc_ractx(struct uvm_object *uobj)
{
	struct vnode *vp = (struct vnode *)uobj;
	struct uvm_ractx *ra = NULL;

	KASSERT(rw_write_held(uobj->vmobjlock));

	if (vp->v_type != VREG) {
		return;
	}
	if (vp->v_ractx != NULL) {
		return;
	}
	if (vp->v_ractx == NULL) {
		rw_exit(uobj->vmobjlock);
		ra = uvm_ra_allocctx();
		rw_enter(uobj->vmobjlock, RW_WRITER);
		if (ra != NULL && vp->v_ractx == NULL) {
			vp->v_ractx = ra;
			ra = NULL;
		}
	}
	if (ra != NULL) {
		uvm_ra_freectx(ra);
	}
}
