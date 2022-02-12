/*	$NetBSD: vfs_vnode.c,v 1.130 2022/02/12 15:51:29 thorpej Exp $	*/

/*-
 * Copyright (c) 1997-2011, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, and by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_subr.c	8.13 (Berkeley) 4/18/94
 */

/*
 * The vnode cache subsystem.
 *
 * Life-cycle
 *
 *	Normally, there are two points where new vnodes are created:
 *	VOP_CREATE(9) and VOP_LOOKUP(9).  The life-cycle of a vnode
 *	starts in one of the following ways:
 *
 *	- Allocation, via vcache_get(9) or vcache_new(9).
 *	- Reclamation of inactive vnode, via vcache_vget(9).
 *
 *	Recycle from a free list, via getnewvnode(9) -> getcleanvnode(9)
 *	was another, traditional way.  Currently, only the draining thread
 *	recycles the vnodes.  This behaviour might be revisited.
 *
 *	The life-cycle ends when the last reference is dropped, usually
 *	in VOP_REMOVE(9).  In such case, VOP_INACTIVE(9) is called to inform
 *	the file system that vnode is inactive.  Via this call, file system
 *	indicates whether vnode can be recycled (usually, it checks its own
 *	references, e.g. count of links, whether the file was removed).
 *
 *	Depending on indication, vnode can be put into a free list (cache),
 *	or cleaned via vcache_reclaim, which calls VOP_RECLAIM(9) to
 *	disassociate underlying file system from the vnode, and finally
 *	destroyed.
 *
 * Vnode state
 *
 *	Vnode is always in one of six states:
 *	- MARKER	This is a marker vnode to help list traversal.  It
 *			will never change its state.
 *	- LOADING	Vnode is associating underlying file system and not
 *			yet ready to use.
 *	- LOADED	Vnode has associated underlying file system and is
 *			ready to use.
 *	- BLOCKED	Vnode is active but cannot get new references.
 *	- RECLAIMING	Vnode is disassociating from the underlying file
 *			system.
 *	- RECLAIMED	Vnode has disassociated from underlying file system
 *			and is dead.
 *
 *	Valid state changes are:
 *	LOADING -> LOADED
 *			Vnode has been initialised in vcache_get() or
 *			vcache_new() and is ready to use.
 *	BLOCKED -> RECLAIMING
 *			Vnode starts disassociation from underlying file
 *			system in vcache_reclaim().
 *	RECLAIMING -> RECLAIMED
 *			Vnode finished disassociation from underlying file
 *			system in vcache_reclaim().
 *	LOADED -> BLOCKED
 *			Either vcache_rekey*() is changing the vnode key or
 *			vrelel() is about to call VOP_INACTIVE().
 *	BLOCKED -> LOADED
 *			The block condition is over.
 *	LOADING -> RECLAIMED
 *			Either vcache_get() or vcache_new() failed to
 *			associate the underlying file system or vcache_rekey*()
 *			drops a vnode used as placeholder.
 *
 *	Of these states LOADING, BLOCKED and RECLAIMING are intermediate
 *	and it is possible to wait for state change.
 *
 *	State is protected with v_interlock with one exception:
 *	to change from LOADING both v_interlock and vcache_lock must be held
 *	so it is possible to check "state == LOADING" without holding
 *	v_interlock.  See vcache_get() for details.
 *
 * Reference counting
 *
 *	Vnode is considered active, if reference count (vnode_t::v_usecount)
 *	is non-zero.  It is maintained using: vref(9) and vrele(9), as well
 *	as vput(9), routines.  Common points holding references are e.g.
 *	file openings, current working directory, mount points, etc.  
 *
 *	v_usecount is adjusted with atomic operations, however to change
 *	from a non-zero value to zero the interlock must also be held.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_vnode.c,v 1.130 2022/02/12 15:51:29 thorpej Exp $");

#ifdef _KERNEL_OPT
#include "opt_pax.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/hash.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/pax.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/vnode_impl.h>
#include <sys/wapbl.h>
#include <sys/fstrans.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_stat.h>

/* Flags to vrelel. */
#define	VRELEL_ASYNC	0x0001	/* Always defer to vrele thread. */

#define	LRU_VRELE	0
#define	LRU_FREE	1
#define	LRU_HOLD	2
#define	LRU_COUNT	3

/*
 * There are three lru lists: one holds vnodes waiting for async release,
 * one is for vnodes which have no buffer/page references and one for those
 * which do (i.e.  v_holdcnt is non-zero).  We put the lists into a single,
 * private cache line as vnodes migrate between them while under the same
 * lock (vdrain_lock).
 */
u_int			numvnodes		__cacheline_aligned;
static vnodelst_t	lru_list[LRU_COUNT]	__cacheline_aligned;
static kmutex_t		vdrain_lock		__cacheline_aligned;
static kcondvar_t	vdrain_cv;
static int		vdrain_gen;
static kcondvar_t	vdrain_gen_cv;
static bool		vdrain_retry;
static lwp_t *		vdrain_lwp;
SLIST_HEAD(hashhead, vnode_impl);
static kmutex_t		vcache_lock		__cacheline_aligned;
static kcondvar_t	vcache_cv;
static u_int		vcache_hashsize;
static u_long		vcache_hashmask;
static struct hashhead	*vcache_hashtab;
static pool_cache_t	vcache_pool;
static void		lru_requeue(vnode_t *, vnodelst_t *);
static vnodelst_t *	lru_which(vnode_t *);
static vnode_impl_t *	vcache_alloc(void);
static void		vcache_dealloc(vnode_impl_t *);
static void		vcache_free(vnode_impl_t *);
static void		vcache_init(void);
static void		vcache_reinit(void);
static void		vcache_reclaim(vnode_t *);
static void		vrelel(vnode_t *, int, int);
static void		vdrain_thread(void *);
static void		vnpanic(vnode_t *, const char *, ...)
    __printflike(2, 3);

/* Routines having to do with the management of the vnode table. */
extern struct mount	*dead_rootmount;
extern int		(**dead_vnodeop_p)(void *);
extern int		(**spec_vnodeop_p)(void *);
extern struct vfsops	dead_vfsops;

/*
 * The high bit of v_usecount is a gate for vcache_tryvget().  It's set
 * only when the vnode state is LOADED.
 */
#define	VUSECOUNT_MASK	0x7fffffff
#define	VUSECOUNT_GATE	0x80000000

/*
 * Return the current usecount of a vnode.
 */
inline int
vrefcnt(struct vnode *vp)
{

	return atomic_load_relaxed(&vp->v_usecount) & VUSECOUNT_MASK;
}

/* Vnode state operations and diagnostics. */

#if defined(DIAGNOSTIC)

#define VSTATE_VALID(state) \
	((state) != VS_ACTIVE && (state) != VS_MARKER)
#define VSTATE_GET(vp) \
	vstate_assert_get((vp), __func__, __LINE__)
#define VSTATE_CHANGE(vp, from, to) \
	vstate_assert_change((vp), (from), (to), __func__, __LINE__)
#define VSTATE_WAIT_STABLE(vp) \
	vstate_assert_wait_stable((vp), __func__, __LINE__)

void
_vstate_assert(vnode_t *vp, enum vnode_state state, const char *func, int line,
    bool has_lock)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	int refcnt = vrefcnt(vp);

	if (!has_lock) {
		/*
		 * Prevent predictive loads from the CPU, but check the state
		 * without loooking first.
		 */
		membar_enter();
		if (state == VS_ACTIVE && refcnt > 0 &&
		    (vip->vi_state == VS_LOADED || vip->vi_state == VS_BLOCKED))
			return;
		if (vip->vi_state == state)
			return;
		mutex_enter((vp)->v_interlock);
	}

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);

	if ((state == VS_ACTIVE && refcnt > 0 &&
	    (vip->vi_state == VS_LOADED || vip->vi_state == VS_BLOCKED)) ||
	    vip->vi_state == state) {
		if (!has_lock)
			mutex_exit((vp)->v_interlock);
		return;
	}
	vnpanic(vp, "state is %s, usecount %d, expected %s at %s:%d",
	    vstate_name(vip->vi_state), refcnt,
	    vstate_name(state), func, line);
}

static enum vnode_state
vstate_assert_get(vnode_t *vp, const char *func, int line)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (! VSTATE_VALID(vip->vi_state))
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(vip->vi_state), func, line);

	return vip->vi_state;
}

static void
vstate_assert_wait_stable(vnode_t *vp, const char *func, int line)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (! VSTATE_VALID(vip->vi_state))
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(vip->vi_state), func, line);

	while (vip->vi_state != VS_LOADED && vip->vi_state != VS_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);

	if (! VSTATE_VALID(vip->vi_state))
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(vip->vi_state), func, line);
}

static void
vstate_assert_change(vnode_t *vp, enum vnode_state from, enum vnode_state to,
    const char *func, int line)
{
	bool gated = (atomic_load_relaxed(&vp->v_usecount) & VUSECOUNT_GATE);
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (from == VS_LOADING)
		KASSERTMSG(mutex_owned(&vcache_lock), "at %s:%d", func, line);

	if (! VSTATE_VALID(from))
		vnpanic(vp, "from is %s at %s:%d",
		    vstate_name(from), func, line);
	if (! VSTATE_VALID(to))
		vnpanic(vp, "to is %s at %s:%d",
		    vstate_name(to), func, line);
	if (vip->vi_state != from)
		vnpanic(vp, "from is %s, expected %s at %s:%d\n",
		    vstate_name(vip->vi_state), vstate_name(from), func, line);
	if ((from == VS_LOADED) != gated)
		vnpanic(vp, "state is %s, gate %d does not match at %s:%d\n",
		    vstate_name(vip->vi_state), gated, func, line);

	/* Open/close the gate for vcache_tryvget(). */	
	if (to == VS_LOADED)
		atomic_or_uint(&vp->v_usecount, VUSECOUNT_GATE);
	else
		atomic_and_uint(&vp->v_usecount, ~VUSECOUNT_GATE);

	vip->vi_state = to;
	if (from == VS_LOADING)
		cv_broadcast(&vcache_cv);
	if (to == VS_LOADED || to == VS_RECLAIMED)
		cv_broadcast(&vp->v_cv);
}

#else /* defined(DIAGNOSTIC) */

#define VSTATE_GET(vp) \
	(VNODE_TO_VIMPL((vp))->vi_state)
#define VSTATE_CHANGE(vp, from, to) \
	vstate_change((vp), (from), (to))
#define VSTATE_WAIT_STABLE(vp) \
	vstate_wait_stable((vp))
void
_vstate_assert(vnode_t *vp, enum vnode_state state, const char *func, int line,
    bool has_lock)
{

}

static void
vstate_wait_stable(vnode_t *vp)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	while (vip->vi_state != VS_LOADED && vip->vi_state != VS_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);
}

static void
vstate_change(vnode_t *vp, enum vnode_state from, enum vnode_state to)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	/* Open/close the gate for vcache_tryvget(). */	
	if (to == VS_LOADED)
		atomic_or_uint(&vp->v_usecount, VUSECOUNT_GATE);
	else
		atomic_and_uint(&vp->v_usecount, ~VUSECOUNT_GATE);

	vip->vi_state = to;
	if (from == VS_LOADING)
		cv_broadcast(&vcache_cv);
	if (to == VS_LOADED || to == VS_RECLAIMED)
		cv_broadcast(&vp->v_cv);
}

#endif /* defined(DIAGNOSTIC) */

void
vfs_vnode_sysinit(void)
{
	int error __diagused, i;

	dead_rootmount = vfs_mountalloc(&dead_vfsops, NULL);
	KASSERT(dead_rootmount != NULL);
	dead_rootmount->mnt_iflag |= IMNT_MPSAFE;

	mutex_init(&vdrain_lock, MUTEX_DEFAULT, IPL_NONE);
	for (i = 0; i < LRU_COUNT; i++) {
		TAILQ_INIT(&lru_list[i]);
	}
	vcache_init();

	cv_init(&vdrain_cv, "vdrain");
	cv_init(&vdrain_gen_cv, "vdrainwt");
	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vdrain_thread,
	    NULL, &vdrain_lwp, "vdrain");
	KASSERTMSG((error == 0), "kthread_create(vdrain) failed: %d", error);
}

/*
 * Allocate a new marker vnode.
 */
vnode_t *
vnalloc_marker(struct mount *mp)
{
	vnode_impl_t *vip;
	vnode_t *vp;

	vip = pool_cache_get(vcache_pool, PR_WAITOK);
	memset(vip, 0, sizeof(*vip));
	vp = VIMPL_TO_VNODE(vip);
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 1);
	vp->v_mount = mp;
	vp->v_type = VBAD;
	vp->v_interlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	klist_init(&vp->v_klist);
	vip->vi_state = VS_MARKER;

	return vp;
}

/*
 * Free a marker vnode.
 */
void
vnfree_marker(vnode_t *vp)
{
	vnode_impl_t *vip;

	vip = VNODE_TO_VIMPL(vp);
	KASSERT(vip->vi_state == VS_MARKER);
	mutex_obj_free(vp->v_interlock);
	uvm_obj_destroy(&vp->v_uobj, true);
	klist_fini(&vp->v_klist);
	pool_cache_put(vcache_pool, vip);
}

/*
 * Test a vnode for being a marker vnode.
 */
bool
vnis_marker(vnode_t *vp)
{

	return (VNODE_TO_VIMPL(vp)->vi_state == VS_MARKER);
}

/*
 * Return the lru list this node should be on.
 */
static vnodelst_t *
lru_which(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));

	if (vp->v_holdcnt > 0)
		return &lru_list[LRU_HOLD];
	else
		return &lru_list[LRU_FREE];
}

/*
 * Put vnode to end of given list.
 * Both the current and the new list may be NULL, used on vnode alloc/free.
 * Adjust numvnodes and signal vdrain thread if there is work.
 */
static void
lru_requeue(vnode_t *vp, vnodelst_t *listhd)
{
	vnode_impl_t *vip;
	int d;

	/*
	 * If the vnode is on the correct list, and was put there recently,
	 * then leave it be, thus avoiding huge cache and lock contention.
	 */
	vip = VNODE_TO_VIMPL(vp);
	if (listhd == vip->vi_lrulisthd &&
	    (getticks() - vip->vi_lrulisttm) < hz) {
	    	return;
	}

	mutex_enter(&vdrain_lock);
	d = 0;
	if (vip->vi_lrulisthd != NULL)
		TAILQ_REMOVE(vip->vi_lrulisthd, vip, vi_lrulist);
	else
		d++;
	vip->vi_lrulisthd = listhd;
	vip->vi_lrulisttm = getticks();
	if (vip->vi_lrulisthd != NULL)
		TAILQ_INSERT_TAIL(vip->vi_lrulisthd, vip, vi_lrulist);
	else
		d--;
	if (d != 0) {
		/*
		 * Looks strange?  This is not a bug.  Don't store
		 * numvnodes unless there is a change - avoid false
		 * sharing on MP.
		 */
		numvnodes += d;
	}
	if ((d > 0 && numvnodes > desiredvnodes) ||
	    listhd == &lru_list[LRU_VRELE])
		cv_signal(&vdrain_cv);
	mutex_exit(&vdrain_lock);
}

/*
 * Release deferred vrele vnodes for this mount.
 * Called with file system suspended.
 */
void
vrele_flush(struct mount *mp)
{
	vnode_impl_t *vip, *marker;
	vnode_t *vp;
	int when = 0;

	KASSERT(fstrans_is_owner(mp));

	marker = VNODE_TO_VIMPL(vnalloc_marker(NULL));

	mutex_enter(&vdrain_lock);
	TAILQ_INSERT_HEAD(&lru_list[LRU_VRELE], marker, vi_lrulist);

	while ((vip = TAILQ_NEXT(marker, vi_lrulist))) {
		TAILQ_REMOVE(&lru_list[LRU_VRELE], marker, vi_lrulist);
		TAILQ_INSERT_AFTER(&lru_list[LRU_VRELE], vip, marker,
		    vi_lrulist);
		vp = VIMPL_TO_VNODE(vip);
		if (vnis_marker(vp))
			continue;

		KASSERT(vip->vi_lrulisthd == &lru_list[LRU_VRELE]);
		TAILQ_REMOVE(vip->vi_lrulisthd, vip, vi_lrulist);
		vip->vi_lrulisthd = &lru_list[LRU_HOLD];
		vip->vi_lrulisttm = getticks();
		TAILQ_INSERT_TAIL(vip->vi_lrulisthd, vip, vi_lrulist);
		mutex_exit(&vdrain_lock);

		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		mutex_enter(vp->v_interlock);
		vrelel(vp, 0, LK_EXCLUSIVE);

		if (getticks() > when) {
			yield();
			when = getticks() + hz / 10;
		}

		mutex_enter(&vdrain_lock);
	}

	TAILQ_REMOVE(&lru_list[LRU_VRELE], marker, vi_lrulist);
	mutex_exit(&vdrain_lock);

	vnfree_marker(VIMPL_TO_VNODE(marker));
}

/*
 * Reclaim a cached vnode.  Used from vdrain_thread only.
 */
static __inline void
vdrain_remove(vnode_t *vp)
{
	struct mount *mp;

	KASSERT(mutex_owned(&vdrain_lock));

	/* Probe usecount (unlocked). */
	if (vrefcnt(vp) > 0)
		return;
	/* Try v_interlock -- we lock the wrong direction! */
	if (!mutex_tryenter(vp->v_interlock))
		return;
	/* Probe usecount and state. */
	if (vrefcnt(vp) > 0 || VSTATE_GET(vp) != VS_LOADED) {
		mutex_exit(vp->v_interlock);
		return;
	}
	mp = vp->v_mount;
	if (fstrans_start_nowait(mp) != 0) {
		mutex_exit(vp->v_interlock);
		return;
	}
	vdrain_retry = true;
	mutex_exit(&vdrain_lock);

	if (vcache_vget(vp) == 0) {
		if (!vrecycle(vp)) {
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			mutex_enter(vp->v_interlock);
			vrelel(vp, 0, LK_EXCLUSIVE);
		}
	}
	fstrans_done(mp);

	mutex_enter(&vdrain_lock);
}

/*
 * Release a cached vnode.  Used from vdrain_thread only.
 */
static __inline void
vdrain_vrele(vnode_t *vp)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	struct mount *mp;

	KASSERT(mutex_owned(&vdrain_lock));

	mp = vp->v_mount;
	if (fstrans_start_nowait(mp) != 0)
		return;

	/*
	 * First remove the vnode from the vrele list.
	 * Put it on the last lru list, the last vrele()
	 * will put it back onto the right list before
	 * its usecount reaches zero.
	 */
	KASSERT(vip->vi_lrulisthd == &lru_list[LRU_VRELE]);
	TAILQ_REMOVE(vip->vi_lrulisthd, vip, vi_lrulist);
	vip->vi_lrulisthd = &lru_list[LRU_HOLD];
	vip->vi_lrulisttm = getticks();
	TAILQ_INSERT_TAIL(vip->vi_lrulisthd, vip, vi_lrulist);

	vdrain_retry = true;
	mutex_exit(&vdrain_lock);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	mutex_enter(vp->v_interlock);
	vrelel(vp, 0, LK_EXCLUSIVE);
	fstrans_done(mp);

	mutex_enter(&vdrain_lock);
}

/*
 * Helper thread to keep the number of vnodes below desiredvnodes
 * and release vnodes from asynchronous vrele.
 */
static void
vdrain_thread(void *cookie)
{
	int i;
	u_int target;
	vnode_impl_t *vip, *marker;

	marker = VNODE_TO_VIMPL(vnalloc_marker(NULL));

	mutex_enter(&vdrain_lock);

	for (;;) {
		vdrain_retry = false;
		target = desiredvnodes - desiredvnodes/10;

		for (i = 0; i < LRU_COUNT; i++) {
			TAILQ_INSERT_HEAD(&lru_list[i], marker, vi_lrulist);
			while ((vip = TAILQ_NEXT(marker, vi_lrulist))) {
				TAILQ_REMOVE(&lru_list[i], marker, vi_lrulist);
				TAILQ_INSERT_AFTER(&lru_list[i], vip, marker,
				    vi_lrulist);
				if (vnis_marker(VIMPL_TO_VNODE(vip)))
					continue;
				if (i == LRU_VRELE)
					vdrain_vrele(VIMPL_TO_VNODE(vip));
				else if (numvnodes < target)
					break;
				else
					vdrain_remove(VIMPL_TO_VNODE(vip));
			}
			TAILQ_REMOVE(&lru_list[i], marker, vi_lrulist);
		}

		if (vdrain_retry) {
			kpause("vdrainrt", false, 1, &vdrain_lock);
		} else {
			vdrain_gen++;
			cv_broadcast(&vdrain_gen_cv);
			cv_wait(&vdrain_cv, &vdrain_lock);
		}
	}
}

/*
 * Try to drop reference on a vnode.  Abort if we are releasing the
 * last reference.  Note: this _must_ succeed if not the last reference.
 */
static bool
vtryrele(vnode_t *vp)
{
	u_int use, next;

	for (use = atomic_load_relaxed(&vp->v_usecount);; use = next) {
		if (__predict_false((use & VUSECOUNT_MASK) == 1)) {
			return false;
		}
		KASSERT((use & VUSECOUNT_MASK) > 1);
		next = atomic_cas_uint(&vp->v_usecount, use, use - 1);
		if (__predict_true(next == use)) {
			return true;
		}
	}
}

/*
 * vput: unlock and release the reference.
 */
void
vput(vnode_t *vp)
{
	int lktype;

	/*
	 * Do an unlocked check of the usecount.  If it looks like we're not
	 * about to drop the last reference, then unlock the vnode and try
	 * to drop the reference.  If it ends up being the last reference
	 * after all, vrelel() can fix it all up.  Most of the time this
	 * will all go to plan.
	 */
	if (vrefcnt(vp) > 1) {
		VOP_UNLOCK(vp);
		if (vtryrele(vp)) {
			return;
		}
		lktype = LK_NONE;
	} else if ((vp->v_vflag & VV_LOCKSWORK) == 0) {
		VOP_UNLOCK(vp);
		lktype = LK_NONE;
	} else {
		lktype = VOP_ISLOCKED(vp);
		KASSERT(lktype != LK_NONE);
	}
	mutex_enter(vp->v_interlock);
	vrelel(vp, 0, lktype);
}

/*
 * Vnode release.  If reference count drops to zero, call inactive
 * routine and either return to freelist or free to the pool.
 */
static void
vrelel(vnode_t *vp, int flags, int lktype)
{
	const bool async = ((flags & VRELEL_ASYNC) != 0);
	bool recycle, defer;
	int error;

	KASSERT(mutex_owned(vp->v_interlock));

	if (__predict_false(vp->v_op == dead_vnodeop_p &&
	    VSTATE_GET(vp) != VS_RECLAIMED)) {
		vnpanic(vp, "dead but not clean");
	}

	/*
	 * If not the last reference, just drop the reference count and
	 * unlock.  VOP_UNLOCK() is called here without a vnode reference
	 * held, but is ok as the hold of v_interlock will stop the vnode
	 * from disappearing.
	 */
	if (vtryrele(vp)) {
		if (lktype != LK_NONE) {
			VOP_UNLOCK(vp);
		}
		mutex_exit(vp->v_interlock);
		return;
	}
	if (vrefcnt(vp) <= 0 || vp->v_writecount != 0) {
		vnpanic(vp, "%s: bad ref count", __func__);
	}

#ifdef DIAGNOSTIC
	if ((vp->v_type == VBLK || vp->v_type == VCHR) &&
	    vp->v_specnode != NULL && vp->v_specnode->sn_opencnt != 0) {
		vprint("vrelel: missing VOP_CLOSE()", vp);
	}
#endif

	/*
	 * First try to get the vnode locked for VOP_INACTIVE().
	 * Defer vnode release to vdrain_thread if caller requests
	 * it explicitly, is the pagedaemon or the lock failed.
	 */
	defer = false;
	if ((curlwp == uvm.pagedaemon_lwp) || async) {
		defer = true;
	} else if (lktype == LK_SHARED) {
		/* Excellent chance of getting, if the last ref. */
		error = vn_lock(vp, LK_UPGRADE | LK_RETRY |
		    LK_NOWAIT);
		if (error != 0) {
			defer = true;
		} else {
			lktype = LK_EXCLUSIVE;
		}
	} else if (lktype == LK_NONE) {
		/* Excellent chance of getting, if the last ref. */
		error = vn_lock(vp, LK_EXCLUSIVE | LK_RETRY |
		    LK_NOWAIT);
		if (error != 0) {
			defer = true;
		} else {
			lktype = LK_EXCLUSIVE;
		}
	}
	KASSERT(mutex_owned(vp->v_interlock));
	if (defer) {
		/*
		 * Defer reclaim to the kthread; it's not safe to
		 * clean it here.  We donate it our last reference.
		 */
		if (lktype != LK_NONE) {
			VOP_UNLOCK(vp);
		}
		lru_requeue(vp, &lru_list[LRU_VRELE]);
		mutex_exit(vp->v_interlock);
		return;
	}
	KASSERT(lktype == LK_EXCLUSIVE);

	/*
	 * If not clean, deactivate the vnode, but preserve
	 * our reference across the call to VOP_INACTIVE().
	 */
	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		VOP_UNLOCK(vp);
	} else {
		/*
		 * If VOP_INACTIVE() indicates that the file has been
		 * deleted, then recycle the vnode.
		 *
		 * Note that VOP_INACTIVE() will not drop the vnode lock.
		 */
		mutex_exit(vp->v_interlock);
		recycle = false;
		VOP_INACTIVE(vp, &recycle);
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		mutex_enter(vp->v_interlock);

		for (;;) {
			/*
			 * If no longer the last reference, try to shed it. 
			 * On success, drop the interlock last thereby
			 * preventing the vnode being freed behind us.
			 */
			if (vtryrele(vp)) {
				VOP_UNLOCK(vp);
				rw_exit(vp->v_uobj.vmobjlock);
				mutex_exit(vp->v_interlock);
				return;
			}
			/*
			 * Block new references then check again to see if a
			 * new reference was acquired in the meantime.  If
			 * it was, restore the vnode state and try again.
			 */
			if (recycle) {
				VSTATE_CHANGE(vp, VS_LOADED, VS_BLOCKED);
				if (vrefcnt(vp) != 1) {
					VSTATE_CHANGE(vp, VS_BLOCKED,
					    VS_LOADED);
					continue;
				}
			}
			break;
 		}

		/* Take care of space accounting. */
		if ((vp->v_iflag & VI_EXECMAP) != 0) {
			cpu_count(CPU_COUNT_EXECPAGES, -vp->v_uobj.uo_npages);
		}
		vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP|VI_WRMAP);
		vp->v_vflag &= ~VV_MAPPED;
		rw_exit(vp->v_uobj.vmobjlock);

		/*
		 * Recycle the vnode if the file is now unused (unlinked),
		 * otherwise just free it.
		 */
		if (recycle) {
			VSTATE_ASSERT(vp, VS_BLOCKED);
			/* vcache_reclaim drops the lock. */
			vcache_reclaim(vp);
		} else {
			VOP_UNLOCK(vp);
		}
		KASSERT(vrefcnt(vp) > 0);
	}

	if ((atomic_dec_uint_nv(&vp->v_usecount) & VUSECOUNT_MASK) != 0) {
		/* Gained another reference while being reclaimed. */
		mutex_exit(vp->v_interlock);
		return;
	}

	if (VSTATE_GET(vp) == VS_RECLAIMED && vp->v_holdcnt == 0) {
		/*
		 * It's clean so destroy it.  It isn't referenced
		 * anywhere since it has been reclaimed.
		 */
		vcache_free(VNODE_TO_VIMPL(vp));
	} else {
		/*
		 * Otherwise, put it back onto the freelist.  It
		 * can't be destroyed while still associated with
		 * a file system.
		 */
		lru_requeue(vp, lru_which(vp));
		mutex_exit(vp->v_interlock);
	}
}

void
vrele(vnode_t *vp)
{

	if (vtryrele(vp)) {
		return;
	}
	mutex_enter(vp->v_interlock);
	vrelel(vp, 0, LK_NONE);
}

/*
 * Asynchronous vnode release, vnode is released in different context.
 */
void
vrele_async(vnode_t *vp)
{

	if (vtryrele(vp)) {
		return;
	}
	mutex_enter(vp->v_interlock);
	vrelel(vp, VRELEL_ASYNC, LK_NONE);
}

/*
 * Vnode reference, where a reference is already held by some other
 * object (for example, a file structure).
 *
 * NB: lockless code sequences may rely on this not blocking.
 */
void
vref(vnode_t *vp)
{

	KASSERT(vrefcnt(vp) > 0);

	atomic_inc_uint(&vp->v_usecount);
}

/*
 * Page or buffer structure gets a reference.
 * Called with v_interlock held.
 */
void
vholdl(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));

	if (vp->v_holdcnt++ == 0 && vrefcnt(vp) == 0)
		lru_requeue(vp, lru_which(vp));
}

/*
 * Page or buffer structure gets a reference.
 */
void
vhold(vnode_t *vp)
{

	mutex_enter(vp->v_interlock);
	vholdl(vp);
	mutex_exit(vp->v_interlock);
}

/*
 * Page or buffer structure frees a reference.
 * Called with v_interlock held.
 */
void
holdrelel(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));

	if (vp->v_holdcnt <= 0) {
		vnpanic(vp, "%s: holdcnt vp %p", __func__, vp);
	}

	vp->v_holdcnt--;
	if (vp->v_holdcnt == 0 && vrefcnt(vp) == 0)
		lru_requeue(vp, lru_which(vp));
}

/*
 * Page or buffer structure frees a reference.
 */
void
holdrele(vnode_t *vp)
{

	mutex_enter(vp->v_interlock);
	holdrelel(vp);
	mutex_exit(vp->v_interlock);
}

/*
 * Recycle an unused vnode if caller holds the last reference.
 */
bool
vrecycle(vnode_t *vp)
{
	int error __diagused;

	mutex_enter(vp->v_interlock);

	/* If the vnode is already clean we're done. */
	VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) != VS_LOADED) {
		VSTATE_ASSERT(vp, VS_RECLAIMED);
		vrelel(vp, 0, LK_NONE);
		return true;
	}

	/* Prevent further references until the vnode is locked. */
	VSTATE_CHANGE(vp, VS_LOADED, VS_BLOCKED);

	/* Make sure we hold the last reference. */
	if (vrefcnt(vp) != 1) {
		VSTATE_CHANGE(vp, VS_BLOCKED, VS_LOADED);
		mutex_exit(vp->v_interlock);
		return false;
	}

	mutex_exit(vp->v_interlock);

	/*
	 * On a leaf file system this lock will always succeed as we hold
	 * the last reference and prevent further references.
	 * On layered file systems waiting for the lock would open a can of
	 * deadlocks as the lower vnodes may have other active references.
	 */
	error = vn_lock(vp, LK_EXCLUSIVE | LK_RETRY | LK_NOWAIT);

	mutex_enter(vp->v_interlock);
	if (error) {
		VSTATE_CHANGE(vp, VS_BLOCKED, VS_LOADED);
		mutex_exit(vp->v_interlock);
		return false;
	}

	KASSERT(vrefcnt(vp) == 1);
	vcache_reclaim(vp);
	vrelel(vp, 0, LK_NONE);

	return true;
}

/*
 * Helper for vrevoke() to propagate suspension from lastmp
 * to thismp.  Both args may be NULL.
 * Returns the currently suspended file system or NULL.
 */
static struct mount *
vrevoke_suspend_next(struct mount *lastmp, struct mount *thismp)
{
	int error;

	if (lastmp == thismp)
		return thismp;

	if (lastmp != NULL)
		vfs_resume(lastmp);

	if (thismp == NULL)
		return NULL;

	do {
		error = vfs_suspend(thismp, 0);
	} while (error == EINTR || error == ERESTART);

	if (error == 0)
		return thismp;

	KASSERT(error == EOPNOTSUPP || error == ENOENT);
	return NULL;
}

/*
 * Eliminate all activity associated with the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
void
vrevoke(vnode_t *vp)
{
	struct mount *mp;
	vnode_t *vq;
	enum vtype type;
	dev_t dev;

	KASSERT(vrefcnt(vp) > 0);

	mp = vrevoke_suspend_next(NULL, vp->v_mount);

	mutex_enter(vp->v_interlock);
	VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		mutex_exit(vp->v_interlock);
	} else if (vp->v_type != VBLK && vp->v_type != VCHR) {
		atomic_inc_uint(&vp->v_usecount);
		mutex_exit(vp->v_interlock);
		vgone(vp);
	} else {
		dev = vp->v_rdev;
		type = vp->v_type;
		mutex_exit(vp->v_interlock);

		while (spec_node_lookup_by_dev(type, dev, &vq) == 0) {
			mp = vrevoke_suspend_next(mp, vq->v_mount);
			vgone(vq);
		}
	}
	vrevoke_suspend_next(mp, NULL);
}

/*
 * Eliminate all activity associated with a vnode in preparation for
 * reuse.  Drops a reference from the vnode.
 */
void
vgone(vnode_t *vp)
{
	int lktype;

	KASSERT(vp->v_mount == dead_rootmount || fstrans_is_owner(vp->v_mount));

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	lktype = LK_EXCLUSIVE;
	mutex_enter(vp->v_interlock);
	VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) == VS_LOADED) { 
		VSTATE_CHANGE(vp, VS_LOADED, VS_BLOCKED);
		vcache_reclaim(vp);
		lktype = LK_NONE;
	}
	VSTATE_ASSERT(vp, VS_RECLAIMED);
	vrelel(vp, 0, lktype);
}

static inline uint32_t
vcache_hash(const struct vcache_key *key)
{
	uint32_t hash = HASH32_BUF_INIT;

	KASSERT(key->vk_key_len > 0);

	hash = hash32_buf(&key->vk_mount, sizeof(struct mount *), hash);
	hash = hash32_buf(key->vk_key, key->vk_key_len, hash);
	return hash;
}

static int
vcache_stats(struct hashstat_sysctl *hs, bool fill)
{
	vnode_impl_t *vip;
	uint64_t chain;

	strlcpy(hs->hash_name, "vcache", sizeof(hs->hash_name));
	strlcpy(hs->hash_desc, "vnode cache hash", sizeof(hs->hash_desc));
	if (!fill)
		return 0;

	hs->hash_size = vcache_hashmask + 1;

	for (size_t i = 0; i < hs->hash_size; i++) {
		chain = 0;
		mutex_enter(&vcache_lock);
		SLIST_FOREACH(vip, &vcache_hashtab[i], vi_hash) {
			chain++;
		}
		mutex_exit(&vcache_lock);
		if (chain > 0) {
			hs->hash_used++;
			hs->hash_items += chain;
			if (chain > hs->hash_maxchain)
				hs->hash_maxchain = chain;
		}
		preempt_point();
	}

	return 0;
}

static void
vcache_init(void)
{

	vcache_pool = pool_cache_init(sizeof(vnode_impl_t), coherency_unit,
	    0, 0, "vcachepl", NULL, IPL_NONE, NULL, NULL, NULL);
	KASSERT(vcache_pool != NULL);
	mutex_init(&vcache_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vcache_cv, "vcache");
	vcache_hashsize = desiredvnodes;
	vcache_hashtab = hashinit(desiredvnodes, HASH_SLIST, true,
	    &vcache_hashmask);
	hashstat_register("vcache", vcache_stats);
}

static void
vcache_reinit(void)
{
	int i;
	uint32_t hash;
	u_long oldmask, newmask;
	struct hashhead *oldtab, *newtab;
	vnode_impl_t *vip;

	newtab = hashinit(desiredvnodes, HASH_SLIST, true, &newmask);
	mutex_enter(&vcache_lock);
	oldtab = vcache_hashtab;
	oldmask = vcache_hashmask;
	vcache_hashsize = desiredvnodes;
	vcache_hashtab = newtab;
	vcache_hashmask = newmask;
	for (i = 0; i <= oldmask; i++) {
		while ((vip = SLIST_FIRST(&oldtab[i])) != NULL) {
			SLIST_REMOVE(&oldtab[i], vip, vnode_impl, vi_hash);
			hash = vcache_hash(&vip->vi_key);
			SLIST_INSERT_HEAD(&newtab[hash & vcache_hashmask],
			    vip, vi_hash);
		}
	}
	mutex_exit(&vcache_lock);
	hashdone(oldtab, HASH_SLIST, oldmask);
}

static inline vnode_impl_t *
vcache_hash_lookup(const struct vcache_key *key, uint32_t hash)
{
	struct hashhead *hashp;
	vnode_impl_t *vip;

	KASSERT(mutex_owned(&vcache_lock));

	hashp = &vcache_hashtab[hash & vcache_hashmask];
	SLIST_FOREACH(vip, hashp, vi_hash) {
		if (key->vk_mount != vip->vi_key.vk_mount)
			continue;
		if (key->vk_key_len != vip->vi_key.vk_key_len)
			continue;
		if (memcmp(key->vk_key, vip->vi_key.vk_key, key->vk_key_len))
			continue;
		return vip;
	}
	return NULL;
}

/*
 * Allocate a new, uninitialized vcache node.
 */
static vnode_impl_t *
vcache_alloc(void)
{
	vnode_impl_t *vip;
	vnode_t *vp;

	vip = pool_cache_get(vcache_pool, PR_WAITOK);
	vp = VIMPL_TO_VNODE(vip);
	memset(vip, 0, sizeof(*vip));

	rw_init(&vip->vi_lock);
	vp->v_interlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 1);
	klist_init(&vp->v_klist);
	cv_init(&vp->v_cv, "vnode");
	cache_vnode_init(vp);

	vp->v_usecount = 1;
	vp->v_type = VNON;
	vp->v_size = vp->v_writesize = VSIZENOTSET;

	vip->vi_state = VS_LOADING;

	lru_requeue(vp, &lru_list[LRU_FREE]);

	return vip;
}

/*
 * Deallocate a vcache node in state VS_LOADING.
 *
 * vcache_lock held on entry and released on return.
 */
static void
vcache_dealloc(vnode_impl_t *vip)
{
	vnode_t *vp;

	KASSERT(mutex_owned(&vcache_lock));

	vp = VIMPL_TO_VNODE(vip);
	vfs_ref(dead_rootmount);
	vfs_insmntque(vp, dead_rootmount);
	mutex_enter(vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	VSTATE_CHANGE(vp, VS_LOADING, VS_RECLAIMED);
	mutex_exit(&vcache_lock);
	vrelel(vp, 0, LK_NONE);
}

/*
 * Free an unused, unreferenced vcache node.
 * v_interlock locked on entry.
 */
static void
vcache_free(vnode_impl_t *vip)
{
	vnode_t *vp;

	vp = VIMPL_TO_VNODE(vip);
	KASSERT(mutex_owned(vp->v_interlock));

	KASSERT(vrefcnt(vp) == 0);
	KASSERT(vp->v_holdcnt == 0);
	KASSERT(vp->v_writecount == 0);
	lru_requeue(vp, NULL);
	mutex_exit(vp->v_interlock);

	vfs_insmntque(vp, NULL);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		spec_node_destroy(vp);

	mutex_obj_free(vp->v_interlock);
	rw_destroy(&vip->vi_lock);
	uvm_obj_destroy(&vp->v_uobj, true);
	klist_fini(&vp->v_klist);
	cv_destroy(&vp->v_cv);
	cache_vnode_fini(vp);
	pool_cache_put(vcache_pool, vip);
}

/*
 * Try to get an initial reference on this cached vnode.
 * Returns zero on success or EBUSY if the vnode state is not LOADED.
 *
 * NB: lockless code sequences may rely on this not blocking.
 */
int
vcache_tryvget(vnode_t *vp)
{
	u_int use, next;

	for (use = atomic_load_relaxed(&vp->v_usecount);; use = next) {
		if (__predict_false((use & VUSECOUNT_GATE) == 0)) {
			return EBUSY;
		}
		next = atomic_cas_uint(&vp->v_usecount, use, use + 1);
		if (__predict_true(next == use)) {
			return 0;
		}
	}
}

/*
 * Try to get an initial reference on this cached vnode.
 * Returns zero on success and  ENOENT if the vnode has been reclaimed.
 * Will wait for the vnode state to be stable.
 *
 * v_interlock locked on entry and unlocked on exit.
 */
int
vcache_vget(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));

	/* Increment hold count to prevent vnode from disappearing. */
	vp->v_holdcnt++;
	VSTATE_WAIT_STABLE(vp);
	vp->v_holdcnt--;

	/* If this was the last reference to a reclaimed vnode free it now. */
	if (__predict_false(VSTATE_GET(vp) == VS_RECLAIMED)) {
		if (vp->v_holdcnt == 0 && vrefcnt(vp) == 0)
			vcache_free(VNODE_TO_VIMPL(vp));
		else
			mutex_exit(vp->v_interlock);
		return ENOENT;
	}
	VSTATE_ASSERT(vp, VS_LOADED);
	atomic_inc_uint(&vp->v_usecount);
	mutex_exit(vp->v_interlock);

	return 0;
}

/*
 * Get a vnode / fs node pair by key and return it referenced through vpp.
 */
int
vcache_get(struct mount *mp, const void *key, size_t key_len,
    struct vnode **vpp)
{
	int error;
	uint32_t hash;
	const void *new_key;
	struct vnode *vp;
	struct vcache_key vcache_key;
	vnode_impl_t *vip, *new_vip;

	new_key = NULL;
	*vpp = NULL;

	vcache_key.vk_mount = mp;
	vcache_key.vk_key = key;
	vcache_key.vk_key_len = key_len;
	hash = vcache_hash(&vcache_key);

again:
	mutex_enter(&vcache_lock);
	vip = vcache_hash_lookup(&vcache_key, hash);

	/* If found, take a reference or retry. */
	if (__predict_true(vip != NULL)) {
		/*
		 * If the vnode is loading we cannot take the v_interlock
		 * here as it might change during load (see uvm_obj_setlock()).
		 * As changing state from VS_LOADING requires both vcache_lock
		 * and v_interlock it is safe to test with vcache_lock held.
		 *
		 * Wait for vnodes changing state from VS_LOADING and retry.
		 */
		if (__predict_false(vip->vi_state == VS_LOADING)) {
			cv_wait(&vcache_cv, &vcache_lock);
			mutex_exit(&vcache_lock);
			goto again;
		}
		vp = VIMPL_TO_VNODE(vip);
		mutex_enter(vp->v_interlock);
		mutex_exit(&vcache_lock);
		error = vcache_vget(vp);
		if (error == ENOENT)
			goto again;
		if (error == 0)
			*vpp = vp;
		KASSERT((error != 0) == (*vpp == NULL));
		return error;
	}
	mutex_exit(&vcache_lock);

	/* Allocate and initialize a new vcache / vnode pair. */
	error = vfs_busy(mp);
	if (error)
		return error;
	new_vip = vcache_alloc();
	new_vip->vi_key = vcache_key;
	vp = VIMPL_TO_VNODE(new_vip);
	mutex_enter(&vcache_lock);
	vip = vcache_hash_lookup(&vcache_key, hash);
	if (vip == NULL) {
		SLIST_INSERT_HEAD(&vcache_hashtab[hash & vcache_hashmask],
		    new_vip, vi_hash);
		vip = new_vip;
	}

	/* If another thread beat us inserting this node, retry. */
	if (vip != new_vip) {
		vcache_dealloc(new_vip);
		vfs_unbusy(mp);
		goto again;
	}
	mutex_exit(&vcache_lock);

	/* Load the fs node.  Exclusive as new_node is VS_LOADING. */
	error = VFS_LOADVNODE(mp, vp, key, key_len, &new_key);
	if (error) {
		mutex_enter(&vcache_lock);
		SLIST_REMOVE(&vcache_hashtab[hash & vcache_hashmask],
		    new_vip, vnode_impl, vi_hash);
		vcache_dealloc(new_vip);
		vfs_unbusy(mp);
		KASSERT(*vpp == NULL);
		return error;
	}
	KASSERT(new_key != NULL);
	KASSERT(memcmp(key, new_key, key_len) == 0);
	KASSERT(vp->v_op != NULL);
	vfs_insmntque(vp, mp);
	if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
		vp->v_vflag |= VV_MPSAFE;
	vfs_ref(mp);
	vfs_unbusy(mp);

	/* Finished loading, finalize node. */
	mutex_enter(&vcache_lock);
	new_vip->vi_key.vk_key = new_key;
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VS_LOADING, VS_LOADED);
	mutex_exit(vp->v_interlock);
	mutex_exit(&vcache_lock);
	*vpp = vp;
	return 0;
}

/*
 * Create a new vnode / fs node pair and return it referenced through vpp.
 */
int
vcache_new(struct mount *mp, struct vnode *dvp, struct vattr *vap,
    kauth_cred_t cred, void *extra, struct vnode **vpp)
{
	int error;
	uint32_t hash;
	struct vnode *vp, *ovp;
	vnode_impl_t *vip, *ovip;

	*vpp = NULL;

	/* Allocate and initialize a new vcache / vnode pair. */
	error = vfs_busy(mp);
	if (error)
		return error;
	vip = vcache_alloc();
	vip->vi_key.vk_mount = mp;
	vp = VIMPL_TO_VNODE(vip);

	/* Create and load the fs node. */
	error = VFS_NEWVNODE(mp, dvp, vp, vap, cred, extra,
	    &vip->vi_key.vk_key_len, &vip->vi_key.vk_key);
	if (error) {
		mutex_enter(&vcache_lock);
		vcache_dealloc(vip);
		vfs_unbusy(mp);
		KASSERT(*vpp == NULL);
		return error;
	}
	KASSERT(vp->v_op != NULL);
	KASSERT((vip->vi_key.vk_key_len == 0) == (mp == dead_rootmount));
	if (vip->vi_key.vk_key_len > 0) {
		KASSERT(vip->vi_key.vk_key != NULL);
		hash = vcache_hash(&vip->vi_key);

		/*
		 * Wait for previous instance to be reclaimed,
		 * then insert new node.
		 */
		mutex_enter(&vcache_lock);
		while ((ovip = vcache_hash_lookup(&vip->vi_key, hash))) {
			ovp = VIMPL_TO_VNODE(ovip);
			mutex_enter(ovp->v_interlock);
			mutex_exit(&vcache_lock);
			error = vcache_vget(ovp);
			KASSERT(error == ENOENT);
			mutex_enter(&vcache_lock);
		}
		SLIST_INSERT_HEAD(&vcache_hashtab[hash & vcache_hashmask],
		    vip, vi_hash);
		mutex_exit(&vcache_lock);
	}
	vfs_insmntque(vp, mp);
	if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
		vp->v_vflag |= VV_MPSAFE;
	vfs_ref(mp);
	vfs_unbusy(mp);

	/* Finished loading, finalize node. */
	mutex_enter(&vcache_lock);
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VS_LOADING, VS_LOADED);
	mutex_exit(&vcache_lock);
	mutex_exit(vp->v_interlock);
	*vpp = vp;
	return 0;
}

/*
 * Prepare key change: update old cache nodes key and lock new cache node.
 * Return an error if the new node already exists.
 */
int
vcache_rekey_enter(struct mount *mp, struct vnode *vp,
    const void *old_key, size_t old_key_len,
    const void *new_key, size_t new_key_len)
{
	uint32_t old_hash, new_hash;
	struct vcache_key old_vcache_key, new_vcache_key;
	vnode_impl_t *vip, *new_vip;

	old_vcache_key.vk_mount = mp;
	old_vcache_key.vk_key = old_key;
	old_vcache_key.vk_key_len = old_key_len;
	old_hash = vcache_hash(&old_vcache_key);

	new_vcache_key.vk_mount = mp;
	new_vcache_key.vk_key = new_key;
	new_vcache_key.vk_key_len = new_key_len;
	new_hash = vcache_hash(&new_vcache_key);

	new_vip = vcache_alloc();
	new_vip->vi_key = new_vcache_key;

	/* Insert locked new node used as placeholder. */
	mutex_enter(&vcache_lock);
	vip = vcache_hash_lookup(&new_vcache_key, new_hash);
	if (vip != NULL) {
		vcache_dealloc(new_vip);
		return EEXIST;
	}
	SLIST_INSERT_HEAD(&vcache_hashtab[new_hash & vcache_hashmask],
	    new_vip, vi_hash);

	/* Replace old nodes key with the temporary copy. */
	vip = vcache_hash_lookup(&old_vcache_key, old_hash);
	KASSERT(vip != NULL);
	KASSERT(VIMPL_TO_VNODE(vip) == vp);
	KASSERT(vip->vi_key.vk_key != old_vcache_key.vk_key);
	vip->vi_key = old_vcache_key;
	mutex_exit(&vcache_lock);
	return 0;
}

/*
 * Key change complete: update old node and remove placeholder.
 */
void
vcache_rekey_exit(struct mount *mp, struct vnode *vp,
    const void *old_key, size_t old_key_len,
    const void *new_key, size_t new_key_len)
{
	uint32_t old_hash, new_hash;
	struct vcache_key old_vcache_key, new_vcache_key;
	vnode_impl_t *vip, *new_vip;
	struct vnode *new_vp;

	old_vcache_key.vk_mount = mp;
	old_vcache_key.vk_key = old_key;
	old_vcache_key.vk_key_len = old_key_len;
	old_hash = vcache_hash(&old_vcache_key);

	new_vcache_key.vk_mount = mp;
	new_vcache_key.vk_key = new_key;
	new_vcache_key.vk_key_len = new_key_len;
	new_hash = vcache_hash(&new_vcache_key);

	mutex_enter(&vcache_lock);

	/* Lookup old and new node. */
	vip = vcache_hash_lookup(&old_vcache_key, old_hash);
	KASSERT(vip != NULL);
	KASSERT(VIMPL_TO_VNODE(vip) == vp);

	new_vip = vcache_hash_lookup(&new_vcache_key, new_hash);
	KASSERT(new_vip != NULL);
	KASSERT(new_vip->vi_key.vk_key_len == new_key_len);
	new_vp = VIMPL_TO_VNODE(new_vip);
	mutex_enter(new_vp->v_interlock);
	VSTATE_ASSERT(VIMPL_TO_VNODE(new_vip), VS_LOADING);
	mutex_exit(new_vp->v_interlock);

	/* Rekey old node and put it onto its new hashlist. */
	vip->vi_key = new_vcache_key;
	if (old_hash != new_hash) {
		SLIST_REMOVE(&vcache_hashtab[old_hash & vcache_hashmask],
		    vip, vnode_impl, vi_hash);
		SLIST_INSERT_HEAD(&vcache_hashtab[new_hash & vcache_hashmask],
		    vip, vi_hash);
	}

	/* Remove new node used as placeholder. */
	SLIST_REMOVE(&vcache_hashtab[new_hash & vcache_hashmask],
	    new_vip, vnode_impl, vi_hash);
	vcache_dealloc(new_vip);
}

/*
 * Disassociate the underlying file system from a vnode.
 *
 * Must be called with vnode locked and will return unlocked.
 * Must be called with the interlock held, and will return with it held.
 */
static void
vcache_reclaim(vnode_t *vp)
{
	lwp_t *l = curlwp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	struct mount *mp = vp->v_mount;
	uint32_t hash;
	uint8_t temp_buf[64], *temp_key;
	size_t temp_key_len;
	bool recycle, active;
	int error;

	KASSERT((vp->v_vflag & VV_LOCKSWORK) == 0 ||
	    VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT(vrefcnt(vp) != 0);

	active = (vrefcnt(vp) > 1);
	temp_key_len = vip->vi_key.vk_key_len;
	/*
	 * Prevent the vnode from being recycled or brought into use
	 * while we clean it out.
	 */
	VSTATE_CHANGE(vp, VS_BLOCKED, VS_RECLAIMING);
	mutex_exit(vp->v_interlock);

	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	mutex_enter(vp->v_interlock);
	if ((vp->v_iflag & VI_EXECMAP) != 0) {
		cpu_count(CPU_COUNT_EXECPAGES, -vp->v_uobj.uo_npages);
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP);
	vp->v_iflag |= VI_DEADCHECK; /* for genfs_getpages() */
	mutex_exit(vp->v_interlock);
	rw_exit(vp->v_uobj.vmobjlock);

	/*
	 * With vnode state set to reclaiming, purge name cache immediately
	 * to prevent new handles on vnode, and wait for existing threads
	 * trying to get a handle to notice VS_RECLAIMED status and abort.
	 */
	cache_purge(vp);

	/* Replace the vnode key with a temporary copy. */
	if (vip->vi_key.vk_key_len > sizeof(temp_buf)) {
		temp_key = kmem_alloc(temp_key_len, KM_SLEEP);
	} else {
		temp_key = temp_buf;
	}
	if (vip->vi_key.vk_key_len > 0) {
		mutex_enter(&vcache_lock);
		memcpy(temp_key, vip->vi_key.vk_key, temp_key_len);
		vip->vi_key.vk_key = temp_key;
		mutex_exit(&vcache_lock);
	}

	fstrans_start(mp);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed.
	 */
	error = vinvalbuf(vp, V_SAVE, NOCRED, l, 0, 0);
	if (error != 0) {
		if (wapbl_vphaswapbl(vp))
			WAPBL_DISCARD(wapbl_vptomp(vp));
		error = vinvalbuf(vp, 0, NOCRED, l, 0, 0);
	}
	KASSERTMSG((error == 0), "vinvalbuf failed: %d", error);
	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
	if (active && (vp->v_type == VBLK || vp->v_type == VCHR)) {
		 spec_node_revoke(vp);
	}

	/*
	 * Disassociate the underlying file system from the vnode.
	 * VOP_INACTIVE leaves the vnode locked; VOP_RECLAIM unlocks
	 * the vnode, and may destroy the vnode so that VOP_UNLOCK
	 * would no longer function.
	 */
	VOP_INACTIVE(vp, &recycle);
	KASSERT((vp->v_vflag & VV_LOCKSWORK) == 0 ||
	    VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	if (VOP_RECLAIM(vp)) {
		vnpanic(vp, "%s: cannot reclaim", __func__);
	}

	KASSERT(vp->v_data == NULL);
	KASSERT((vp->v_iflag & VI_PAGES) == 0);

	if (vp->v_type == VREG && vp->v_ractx != NULL) {
		uvm_ra_freectx(vp->v_ractx);
		vp->v_ractx = NULL;
	}

	if (vip->vi_key.vk_key_len > 0) {
	/* Remove from vnode cache. */
		hash = vcache_hash(&vip->vi_key);
		mutex_enter(&vcache_lock);
		KASSERT(vip == vcache_hash_lookup(&vip->vi_key, hash));
		SLIST_REMOVE(&vcache_hashtab[hash & vcache_hashmask],
		    vip, vnode_impl, vi_hash);
		mutex_exit(&vcache_lock);
	}
	if (temp_key != temp_buf)
		kmem_free(temp_key, temp_key_len);

	/* Done with purge, notify sleepers of the grim news. */
	mutex_enter(vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	vp->v_vflag |= VV_LOCKSWORK;
	VSTATE_CHANGE(vp, VS_RECLAIMING, VS_RECLAIMED);
	vp->v_tag = VT_NON;
	/*
	 * Don't check for interest in NOTE_REVOKE; it's always posted
	 * because it sets EV_EOF.
	 */
	KNOTE(&vp->v_klist, NOTE_REVOKE);
	mutex_exit(vp->v_interlock);

	/*
	 * Move to dead mount.  Must be after changing the operations
	 * vector as vnode operations enter the mount before using the
	 * operations vector.  See sys/kern/vnode_if.c.
	 */
	vp->v_vflag &= ~VV_ROOT;
	vfs_ref(dead_rootmount);
	vfs_insmntque(vp, dead_rootmount);

#ifdef PAX_SEGVGUARD
	pax_segvguard_cleanup(vp);
#endif /* PAX_SEGVGUARD */

	mutex_enter(vp->v_interlock);
	fstrans_done(mp);
	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
}

/*
 * Disassociate the underlying file system from an open device vnode
 * and make it anonymous.
 *
 * Vnode unlocked on entry, drops a reference to the vnode.
 */
void
vcache_make_anon(vnode_t *vp)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	uint32_t hash;
	bool recycle;

	KASSERT(vp->v_type == VBLK || vp->v_type == VCHR);
	KASSERT(vp->v_mount == dead_rootmount || fstrans_is_owner(vp->v_mount));
	VSTATE_ASSERT_UNLOCKED(vp, VS_ACTIVE);

	/* Remove from vnode cache. */
	hash = vcache_hash(&vip->vi_key);
	mutex_enter(&vcache_lock);
	KASSERT(vip == vcache_hash_lookup(&vip->vi_key, hash));
	SLIST_REMOVE(&vcache_hashtab[hash & vcache_hashmask],
	    vip, vnode_impl, vi_hash);
	vip->vi_key.vk_mount = dead_rootmount;
	vip->vi_key.vk_key_len = 0;
	vip->vi_key.vk_key = NULL;
	mutex_exit(&vcache_lock);

	/*
	 * Disassociate the underlying file system from the vnode.
	 * VOP_INACTIVE leaves the vnode locked; VOP_RECLAIM unlocks
	 * the vnode, and may destroy the vnode so that VOP_UNLOCK
	 * would no longer function.
	 */
	if (vn_lock(vp, LK_EXCLUSIVE)) {
		vnpanic(vp, "%s: cannot lock", __func__);
	}
	VOP_INACTIVE(vp, &recycle);
	KASSERT((vp->v_vflag & VV_LOCKSWORK) == 0 ||
	    VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	if (VOP_RECLAIM(vp)) {
		vnpanic(vp, "%s: cannot reclaim", __func__);
	}

	/* Purge name cache. */
	cache_purge(vp);

	/* Done with purge, change operations vector. */
	mutex_enter(vp->v_interlock);
	vp->v_op = spec_vnodeop_p;
	vp->v_vflag |= VV_MPSAFE;
	vp->v_vflag &= ~VV_LOCKSWORK;
	mutex_exit(vp->v_interlock);

	/*
	 * Move to dead mount.  Must be after changing the operations
	 * vector as vnode operations enter the mount before using the
	 * operations vector.  See sys/kern/vnode_if.c.
	 */
	vfs_ref(dead_rootmount);
	vfs_insmntque(vp, dead_rootmount);

	vrele(vp);
}

/*
 * Update outstanding I/O count and do wakeup if requested.
 */
void
vwakeup(struct buf *bp)
{
	vnode_t *vp;

	if ((vp = bp->b_vp) == NULL)
		return;

	KASSERT(bp->b_objlock == vp->v_interlock);
	KASSERT(mutex_owned(bp->b_objlock));

	if (--vp->v_numoutput < 0)
		vnpanic(vp, "%s: neg numoutput, vp %p", __func__, vp);
	if (vp->v_numoutput == 0)
		cv_broadcast(&vp->v_cv);
}

/*
 * Test a vnode for being or becoming dead.  Returns one of:
 * EBUSY:  vnode is becoming dead, with "flags == VDEAD_NOWAIT" only.
 * ENOENT: vnode is dead.
 * 0:      otherwise.
 *
 * Whenever this function returns a non-zero value all future
 * calls will also return a non-zero value.
 */
int
vdead_check(struct vnode *vp, int flags)
{

	KASSERT(mutex_owned(vp->v_interlock));

	if (! ISSET(flags, VDEAD_NOWAIT))
		VSTATE_WAIT_STABLE(vp);

	if (VSTATE_GET(vp) == VS_RECLAIMING) {
		KASSERT(ISSET(flags, VDEAD_NOWAIT));
		return EBUSY;
	} else if (VSTATE_GET(vp) == VS_RECLAIMED) {
		return ENOENT;
	}

	return 0;
}

int
vfs_drainvnodes(void)
{
	int i, gen;

	mutex_enter(&vdrain_lock);
	for (i = 0; i < 2; i++) {
		gen = vdrain_gen;
		while (gen == vdrain_gen) {
			cv_broadcast(&vdrain_cv);
			cv_wait(&vdrain_gen_cv, &vdrain_lock);
		}
	}
	mutex_exit(&vdrain_lock);

	if (numvnodes >= desiredvnodes)
		return EBUSY;

	if (vcache_hashsize != desiredvnodes)
		vcache_reinit();

	return 0;
}

void
vnpanic(vnode_t *vp, const char *fmt, ...)
{
	va_list ap;

#ifdef DIAGNOSTIC
	vprint(NULL, vp);
#endif
	va_start(ap, fmt);
	vpanic(fmt, ap);
	va_end(ap);
}

void
vshareilock(vnode_t *tvp, vnode_t *fvp)
{
	kmutex_t *oldlock;

	oldlock = tvp->v_interlock;
	mutex_obj_hold(fvp->v_interlock);
	tvp->v_interlock = fvp->v_interlock;
	mutex_obj_free(oldlock);
}
