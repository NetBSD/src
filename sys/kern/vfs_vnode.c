/*	$NetBSD: vfs_vnode.c,v 1.66 2017/01/02 10:33:28 hannken Exp $	*/

/*-
 * Copyright (c) 1997-2011 The NetBSD Foundation, Inc.
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
 *	- ACTIVE	Vnode has associated underlying file system and is
 *			ready to use.
 *	- BLOCKED	Vnode is active but cannot get new references.
 *	- RECLAIMING	Vnode is disassociating from the underlying file
 *			system.
 *	- RECLAIMED	Vnode has disassociated from underlying file system
 *			and is dead.
 *
 *	Valid state changes are:
 *	LOADING -> ACTIVE
 *			Vnode has been initialised in vcache_get() or
 *			vcache_new() and is ready to use.
 *	ACTIVE -> RECLAIMING
 *			Vnode starts disassociation from underlying file
 *			system in vcache_reclaim().
 *	RECLAIMING -> RECLAIMED
 *			Vnode finished disassociation from underlying file
 *			system in vcache_reclaim().
 *	ACTIVE -> BLOCKED
 *			Either vcache_rekey*() is changing the vnode key or
 *			vrelel() is about to call VOP_INACTIVE().
 *	BLOCKED -> ACTIVE
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
 *	to change from LOADING both v_interlock and vcache.lock must be held
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
 * Note on v_usecount and its locking
 *
 *	At nearly all points it is known that v_usecount could be zero,
 *	the vnode_t::v_interlock will be held.  To change v_usecount away
 *	from zero, the interlock must be held.  To change from a non-zero
 *	value to zero, again the interlock must be held.
 *
 *	Changing the usecount from a non-zero value to a non-zero value can
 *	safely be done using atomic operations, without the interlock held.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_vnode.c,v 1.66 2017/01/02 10:33:28 hannken Exp $");

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
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/vnode_impl.h>
#include <sys/wapbl.h>
#include <sys/fstrans.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

/* Flags to vrelel. */
#define	VRELEL_ASYNC_RELE	0x0001	/* Always defer to vrele thread. */

u_int			numvnodes		__cacheline_aligned;

/*
 * There are three lru lists: one holds vnodes waiting for async release,
 * one is for vnodes which have no buffer/page references and
 * one for those which do (i.e. v_holdcnt is non-zero).
 */
static vnodelst_t	lru_vrele_list		__cacheline_aligned;
static vnodelst_t	lru_free_list		__cacheline_aligned;
static vnodelst_t	lru_hold_list		__cacheline_aligned;
static kmutex_t		vdrain_lock		__cacheline_aligned;
static kcondvar_t	vdrain_cv		__cacheline_aligned;
static int		vdrain_gen;
static kcondvar_t	vdrain_gen_cv;
static bool		vdrain_retry;
static lwp_t *		vdrain_lwp;
SLIST_HEAD(hashhead, vnode_impl);
static struct {
	kmutex_t	lock;
	kcondvar_t	cv;
	u_int		hashsize;
	u_long		hashmask;
	struct hashhead	*hashtab;
	pool_cache_t	pool;
}			vcache			__cacheline_aligned;

static void		lru_requeue(vnode_t *, vnodelst_t *);
static vnodelst_t *	lru_which(vnode_t *);
static vnode_impl_t *	vcache_alloc(void);
static void		vcache_free(vnode_impl_t *);
static void		vcache_init(void);
static void		vcache_reinit(void);
static void		vcache_reclaim(vnode_t *);
static void		vrelel(vnode_t *, int);
static void		vdrain_thread(void *);
static void		vnpanic(vnode_t *, const char *, ...)
    __printflike(2, 3);

/* Routines having to do with the management of the vnode table. */
extern struct mount	*dead_rootmount;
extern int		(**dead_vnodeop_p)(void *);
extern struct vfsops	dead_vfsops;

/* Vnode state operations and diagnostics. */

#if defined(DIAGNOSTIC)

#define VSTATE_GET(vp) \
	vstate_assert_get((vp), __func__, __LINE__)
#define VSTATE_CHANGE(vp, from, to) \
	vstate_assert_change((vp), (from), (to), __func__, __LINE__)
#define VSTATE_WAIT_STABLE(vp) \
	vstate_assert_wait_stable((vp), __func__, __LINE__)
#define VSTATE_ASSERT(vp, state) \
	vstate_assert((vp), (state), __func__, __LINE__)

static void
vstate_assert(vnode_t *vp, enum vnode_state state, const char *func, int line)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);

	if (__predict_true(node->vi_state == state))
		return;
	vnpanic(vp, "state is %s, expected %s at %s:%d",
	    vstate_name(node->vi_state), vstate_name(state), func, line);
}

static enum vnode_state
vstate_assert_get(vnode_t *vp, const char *func, int line)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (node->vi_state == VS_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vi_state), func, line);

	return node->vi_state;
}

static void
vstate_assert_wait_stable(vnode_t *vp, const char *func, int line)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (node->vi_state == VS_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vi_state), func, line);

	while (node->vi_state != VS_ACTIVE && node->vi_state != VS_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);

	if (node->vi_state == VS_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vi_state), func, line);
}

static void
vstate_assert_change(vnode_t *vp, enum vnode_state from, enum vnode_state to,
    const char *func, int line)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (from == VS_LOADING)
		KASSERTMSG(mutex_owned(&vcache.lock), "at %s:%d", func, line);

	if (from == VS_MARKER)
		vnpanic(vp, "from is %s at %s:%d",
		    vstate_name(from), func, line);
	if (to == VS_MARKER)
		vnpanic(vp, "to is %s at %s:%d",
		    vstate_name(to), func, line);
	if (node->vi_state != from)
		vnpanic(vp, "from is %s, expected %s at %s:%d\n",
		    vstate_name(node->vi_state), vstate_name(from), func, line);

	node->vi_state = to;
	if (from == VS_LOADING)
		cv_broadcast(&vcache.cv);
	if (to == VS_ACTIVE || to == VS_RECLAIMED)
		cv_broadcast(&vp->v_cv);
}

#else /* defined(DIAGNOSTIC) */

#define VSTATE_GET(vp) \
	(VNODE_TO_VIMPL((vp))->vi_state)
#define VSTATE_CHANGE(vp, from, to) \
	vstate_change((vp), (from), (to))
#define VSTATE_WAIT_STABLE(vp) \
	vstate_wait_stable((vp))
#define VSTATE_ASSERT(vp, state)

static void
vstate_wait_stable(vnode_t *vp)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	while (node->vi_state != VS_ACTIVE && node->vi_state != VS_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);
}

static void
vstate_change(vnode_t *vp, enum vnode_state from, enum vnode_state to)
{
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);

	node->vi_state = to;
	if (from == VS_LOADING)
		cv_broadcast(&vcache.cv);
	if (to == VS_ACTIVE || to == VS_RECLAIMED)
		cv_broadcast(&vp->v_cv);
}

#endif /* defined(DIAGNOSTIC) */

void
vfs_vnode_sysinit(void)
{
	int error __diagused;

	dead_rootmount = vfs_mountalloc(&dead_vfsops, NULL);
	KASSERT(dead_rootmount != NULL);
	dead_rootmount->mnt_iflag = IMNT_MPSAFE;

	mutex_init(&vdrain_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&lru_free_list);
	TAILQ_INIT(&lru_hold_list);
	TAILQ_INIT(&lru_vrele_list);

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
	vnode_impl_t *node;
	vnode_t *vp;

	node = pool_cache_get(vcache.pool, PR_WAITOK);
	memset(node, 0, sizeof(*node));
	vp = VIMPL_TO_VNODE(node);
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 0);
	vp->v_mount = mp;
	vp->v_type = VBAD;
	node->vi_state = VS_MARKER;

	return vp;
}

/*
 * Free a marker vnode.
 */
void
vnfree_marker(vnode_t *vp)
{
	vnode_impl_t *node;

	node = VNODE_TO_VIMPL(vp);
	KASSERT(node->vi_state == VS_MARKER);
	uvm_obj_destroy(&vp->v_uobj, true);
	pool_cache_put(vcache.pool, node);
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
		return &lru_hold_list;
	else
		return &lru_free_list;
}

/*
 * Put vnode to end of given list.
 * Both the current and the new list may be NULL, used on vnode alloc/free.
 * Adjust numvnodes and signal vdrain thread if there is work.
 */
static void
lru_requeue(vnode_t *vp, vnodelst_t *listhd)
{
	vnode_impl_t *node;

	mutex_enter(&vdrain_lock);
	node = VNODE_TO_VIMPL(vp);
	if (node->vi_lrulisthd != NULL)
		TAILQ_REMOVE(node->vi_lrulisthd, node, vi_lrulist);
	else
		numvnodes++;
	node->vi_lrulisthd = listhd;
	if (node->vi_lrulisthd != NULL)
		TAILQ_INSERT_TAIL(node->vi_lrulisthd, node, vi_lrulist);
	else
		numvnodes--;
	if (numvnodes > desiredvnodes || listhd == &lru_vrele_list)
		cv_broadcast(&vdrain_cv);
	mutex_exit(&vdrain_lock);
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
	if (vp->v_usecount > 0)
		return;
	/* Try v_interlock -- we lock the wrong direction! */
	if (!mutex_tryenter(vp->v_interlock))
		return;
	/* Probe usecount and state. */
	if (vp->v_usecount > 0 || VSTATE_GET(vp) != VS_ACTIVE) {
		mutex_exit(vp->v_interlock);
		return;
	}
	mp = vp->v_mount;
	if (fstrans_start_nowait(mp, FSTRANS_SHARED) != 0) {
		mutex_exit(vp->v_interlock);
		return;
	}
	vdrain_retry = true;
	mutex_exit(&vdrain_lock);

	if (vcache_vget(vp) == 0) {
		if (!vrecycle(vp))
			vrele(vp);
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
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);
	struct mount *mp;

	KASSERT(mutex_owned(&vdrain_lock));

	mp = vp->v_mount;
	if (fstrans_start_nowait(mp, FSTRANS_LAZY) != 0)
		return;

	/*
	 * First remove the vnode from the vrele list.
	 * Put it on the last lru list, the last vrele()
	 * will put it back onto the right list before
	 * its v_usecount reaches zero.
	 */
	KASSERT(node->vi_lrulisthd == &lru_vrele_list);
	TAILQ_REMOVE(node->vi_lrulisthd, node, vi_lrulist);
	node->vi_lrulisthd = &lru_hold_list;
	TAILQ_INSERT_TAIL(node->vi_lrulisthd, node, vi_lrulist);

	vdrain_retry = true;
	mutex_exit(&vdrain_lock);

	mutex_enter(vp->v_interlock);
	vrelel(vp, 0);
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
	vnodelst_t *listhd[] = {
	    &lru_vrele_list, &lru_free_list, &lru_hold_list
	};
	int i;
	u_int target;
	vnode_impl_t *node, *marker;

	marker = VNODE_TO_VIMPL(vnalloc_marker(NULL));

	mutex_enter(&vdrain_lock);

	for (;;) {
		vdrain_retry = false;
		target = desiredvnodes - desiredvnodes/10;

		for (i = 0; i < __arraycount(listhd); i++) {
			TAILQ_INSERT_HEAD(listhd[i], marker, vi_lrulist);
			while ((node = TAILQ_NEXT(marker, vi_lrulist))) {
				TAILQ_REMOVE(listhd[i], marker, vi_lrulist);
				TAILQ_INSERT_AFTER(listhd[i], node, marker,
				    vi_lrulist);
				if (listhd[i] == &lru_vrele_list)
					vdrain_vrele(VIMPL_TO_VNODE(node));
				else if (numvnodes < target)
					break;
				else
					vdrain_remove(VIMPL_TO_VNODE(node));
			}
			TAILQ_REMOVE(listhd[i], marker, vi_lrulist);
		}

		if (vdrain_retry) {
			mutex_exit(&vdrain_lock);
			yield();
			mutex_enter(&vdrain_lock);
		} else {
			vdrain_gen++;
			cv_broadcast(&vdrain_gen_cv);
			cv_wait(&vdrain_cv, &vdrain_lock);
		}
	}
}

/*
 * vput: unlock and release the reference.
 */
void
vput(vnode_t *vp)
{

	VOP_UNLOCK(vp);
	vrele(vp);
}

/*
 * Try to drop reference on a vnode.  Abort if we are releasing the
 * last reference.  Note: this _must_ succeed if not the last reference.
 */
static inline bool
vtryrele(vnode_t *vp)
{
	u_int use, next;

	for (use = vp->v_usecount;; use = next) {
		if (use == 1) {
			return false;
		}
		KASSERT(use > 1);
		next = atomic_cas_uint(&vp->v_usecount, use, use - 1);
		if (__predict_true(next == use)) {
			return true;
		}
	}
}

/*
 * Vnode release.  If reference count drops to zero, call inactive
 * routine and either return to freelist or free to the pool.
 */
static void
vrelel(vnode_t *vp, int flags)
{
	bool recycle, defer;
	int error;

	KASSERT(mutex_owned(vp->v_interlock));

	if (__predict_false(vp->v_op == dead_vnodeop_p &&
	    VSTATE_GET(vp) != VS_RECLAIMED)) {
		vnpanic(vp, "dead but not clean");
	}

	/*
	 * If not the last reference, just drop the reference count
	 * and unlock.
	 */
	if (vtryrele(vp)) {
		mutex_exit(vp->v_interlock);
		return;
	}
	if (vp->v_usecount <= 0 || vp->v_writecount != 0) {
		vnpanic(vp, "%s: bad ref count", __func__);
	}

#ifdef DIAGNOSTIC
	if ((vp->v_type == VBLK || vp->v_type == VCHR) &&
	    vp->v_specnode != NULL && vp->v_specnode->sn_opencnt != 0) {
		vprint("vrelel: missing VOP_CLOSE()", vp);
	}
#endif

	/*
	 * If not clean, deactivate the vnode, but preserve
	 * our reference across the call to VOP_INACTIVE().
	 */
	if (VSTATE_GET(vp) != VS_RECLAIMED) {
		recycle = false;

		/*
		 * XXX This ugly block can be largely eliminated if
		 * locking is pushed down into the file systems.
		 *
		 * Defer vnode release to vdrain_thread if caller
		 * requests it explicitly or is the pagedaemon.
		 */
		if ((curlwp == uvm.pagedaemon_lwp) ||
		    (flags & VRELEL_ASYNC_RELE) != 0) {
			defer = true;
		} else if (curlwp == vdrain_lwp) {
			/*
			 * We have to try harder.
			 */
			mutex_exit(vp->v_interlock);
			error = vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			KASSERTMSG((error == 0), "vn_lock failed: %d", error);
			mutex_enter(vp->v_interlock);
			defer = false;
		} else {
			/* If we can't acquire the lock, then defer. */
			mutex_exit(vp->v_interlock);
			error = vn_lock(vp,
			    LK_EXCLUSIVE | LK_RETRY | LK_NOWAIT);
			defer = (error != 0);
			mutex_enter(vp->v_interlock);
		}

		KASSERT(mutex_owned(vp->v_interlock));
		KASSERT(! (curlwp == vdrain_lwp && defer));

		if (defer) {
			/*
			 * Defer reclaim to the kthread; it's not safe to
			 * clean it here.  We donate it our last reference.
			 */
			lru_requeue(vp, &lru_vrele_list);
			mutex_exit(vp->v_interlock);
			return;
		}

		/*
		 * If the node got another reference while we
		 * released the interlock, don't try to inactivate it yet.
		 */
		if (__predict_false(vtryrele(vp))) {
			VOP_UNLOCK(vp);
			mutex_exit(vp->v_interlock);
			return;
		}
		VSTATE_CHANGE(vp, VS_ACTIVE, VS_BLOCKED);
		mutex_exit(vp->v_interlock);

		/*
		 * The vnode must not gain another reference while being
		 * deactivated.  If VOP_INACTIVE() indicates that
		 * the described file has been deleted, then recycle
		 * the vnode.
		 *
		 * Note that VOP_INACTIVE() will drop the vnode lock.
		 */
		VOP_INACTIVE(vp, &recycle);
		if (recycle) {
			/* vcache_reclaim() below will drop the lock. */
			if (vn_lock(vp, LK_EXCLUSIVE) != 0)
				recycle = false;
		}
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VS_BLOCKED, VS_ACTIVE);
		if (!recycle) {
			if (vtryrele(vp)) {
				mutex_exit(vp->v_interlock);
				return;
			}
		}

		/* Take care of space accounting. */
		if (vp->v_iflag & VI_EXECMAP) {
			atomic_add_int(&uvmexp.execpages,
			    -vp->v_uobj.uo_npages);
			atomic_add_int(&uvmexp.filepages,
			    vp->v_uobj.uo_npages);
		}
		vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP|VI_WRMAP);
		vp->v_vflag &= ~VV_MAPPED;

		/*
		 * Recycle the vnode if the file is now unused (unlinked),
		 * otherwise just free it.
		 */
		if (recycle) {
			VSTATE_ASSERT(vp, VS_ACTIVE);
			vcache_reclaim(vp);
		}
		KASSERT(vp->v_usecount > 0);
	}

	if (atomic_dec_uint_nv(&vp->v_usecount) != 0) {
		/* Gained another reference while being reclaimed. */
		mutex_exit(vp->v_interlock);
		return;
	}

	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		/*
		 * It's clean so destroy it.  It isn't referenced
		 * anywhere since it has been reclaimed.
		 */
		KASSERT(vp->v_holdcnt == 0);
		KASSERT(vp->v_writecount == 0);
		mutex_exit(vp->v_interlock);
		vfs_insmntque(vp, NULL);
		if (vp->v_type == VBLK || vp->v_type == VCHR) {
			spec_node_destroy(vp);
		}
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
	vrelel(vp, 0);
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
	vrelel(vp, VRELEL_ASYNC_RELE);
}

/*
 * Vnode reference, where a reference is already held by some other
 * object (for example, a file structure).
 */
void
vref(vnode_t *vp)
{

	KASSERT(vp->v_usecount != 0);

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

	if (vp->v_holdcnt++ == 0 && vp->v_usecount == 0)
		lru_requeue(vp, lru_which(vp));
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
	if (vp->v_holdcnt == 0 && vp->v_usecount == 0)
		lru_requeue(vp, lru_which(vp));
}

/*
 * Recycle an unused vnode if caller holds the last reference.
 */
bool
vrecycle(vnode_t *vp)
{
	int error __diagused;

	mutex_enter(vp->v_interlock);

	/* Make sure we hold the last reference. */
	VSTATE_WAIT_STABLE(vp);
	if (vp->v_usecount != 1) {
		mutex_exit(vp->v_interlock);
		return false;
	}

	/* If the vnode is already clean we're done. */
	if (VSTATE_GET(vp) != VS_ACTIVE) {
		VSTATE_ASSERT(vp, VS_RECLAIMED);
		vrelel(vp, 0);
		return true;
	}

	/* Prevent further references until the vnode is locked. */
	VSTATE_CHANGE(vp, VS_ACTIVE, VS_BLOCKED);
	mutex_exit(vp->v_interlock);

	error = vn_lock(vp, LK_EXCLUSIVE);
	KASSERT(error == 0);

	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VS_BLOCKED, VS_ACTIVE);

	vcache_reclaim(vp);
	vrelel(vp, 0);

	return true;
}

/*
 * Eliminate all activity associated with the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
void
vrevoke(vnode_t *vp)
{
	vnode_t *vq;
	enum vtype type;
	dev_t dev;

	KASSERT(vp->v_usecount > 0);

	mutex_enter(vp->v_interlock);
	VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		mutex_exit(vp->v_interlock);
		return;
	} else if (vp->v_type != VBLK && vp->v_type != VCHR) {
		atomic_inc_uint(&vp->v_usecount);
		mutex_exit(vp->v_interlock);
		vgone(vp);
		return;
	} else {
		dev = vp->v_rdev;
		type = vp->v_type;
		mutex_exit(vp->v_interlock);
	}

	while (spec_node_lookup_by_dev(type, dev, &vq) == 0) {
		vgone(vq);
	}
}

/*
 * Eliminate all activity associated with a vnode in preparation for
 * reuse.  Drops a reference from the vnode.
 */
void
vgone(vnode_t *vp)
{

	if (vn_lock(vp, LK_EXCLUSIVE) != 0) {
		VSTATE_ASSERT(vp, VS_RECLAIMED);
		vrele(vp);
	}

	mutex_enter(vp->v_interlock);
	vcache_reclaim(vp);
	vrelel(vp, 0);
}

static inline uint32_t
vcache_hash(const struct vcache_key *key)
{
	uint32_t hash = HASH32_BUF_INIT;

	hash = hash32_buf(&key->vk_mount, sizeof(struct mount *), hash);
	hash = hash32_buf(key->vk_key, key->vk_key_len, hash);
	return hash;
}

static void
vcache_init(void)
{

	vcache.pool = pool_cache_init(sizeof(vnode_impl_t), 0, 0, 0,
	    "vcachepl", NULL, IPL_NONE, NULL, NULL, NULL);
	KASSERT(vcache.pool != NULL);
	mutex_init(&vcache.lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vcache.cv, "vcache");
	vcache.hashsize = desiredvnodes;
	vcache.hashtab = hashinit(desiredvnodes, HASH_SLIST, true,
	    &vcache.hashmask);
}

static void
vcache_reinit(void)
{
	int i;
	uint32_t hash;
	u_long oldmask, newmask;
	struct hashhead *oldtab, *newtab;
	vnode_impl_t *node;

	newtab = hashinit(desiredvnodes, HASH_SLIST, true, &newmask);
	mutex_enter(&vcache.lock);
	oldtab = vcache.hashtab;
	oldmask = vcache.hashmask;
	vcache.hashsize = desiredvnodes;
	vcache.hashtab = newtab;
	vcache.hashmask = newmask;
	for (i = 0; i <= oldmask; i++) {
		while ((node = SLIST_FIRST(&oldtab[i])) != NULL) {
			SLIST_REMOVE(&oldtab[i], node, vnode_impl, vi_hash);
			hash = vcache_hash(&node->vi_key);
			SLIST_INSERT_HEAD(&newtab[hash & vcache.hashmask],
			    node, vi_hash);
		}
	}
	mutex_exit(&vcache.lock);
	hashdone(oldtab, HASH_SLIST, oldmask);
}

static inline vnode_impl_t *
vcache_hash_lookup(const struct vcache_key *key, uint32_t hash)
{
	struct hashhead *hashp;
	vnode_impl_t *node;

	KASSERT(mutex_owned(&vcache.lock));

	hashp = &vcache.hashtab[hash & vcache.hashmask];
	SLIST_FOREACH(node, hashp, vi_hash) {
		if (key->vk_mount != node->vi_key.vk_mount)
			continue;
		if (key->vk_key_len != node->vi_key.vk_key_len)
			continue;
		if (memcmp(key->vk_key, node->vi_key.vk_key, key->vk_key_len))
			continue;
		return node;
	}
	return NULL;
}

/*
 * Allocate a new, uninitialized vcache node.
 */
static vnode_impl_t *
vcache_alloc(void)
{
	vnode_impl_t *node;
	vnode_t *vp;

	node = pool_cache_get(vcache.pool, PR_WAITOK);
	memset(node, 0, sizeof(*node));

	/* SLIST_INIT(&node->vi_hash); */

	vp = VIMPL_TO_VNODE(node);
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 0);
	cv_init(&vp->v_cv, "vnode");
	/* LIST_INIT(&vp->v_nclist); */
	/* LIST_INIT(&vp->v_dnclist); */

	rw_init(&vp->v_lock);
	vp->v_usecount = 1;
	vp->v_type = VNON;
	vp->v_size = vp->v_writesize = VSIZENOTSET;

	node->vi_state = VS_LOADING;

	lru_requeue(vp, &lru_free_list);

	return node;
}

/*
 * Free an unused, unreferenced vcache node.
 */
static void
vcache_free(vnode_impl_t *node)
{
	vnode_t *vp;

	vp = VIMPL_TO_VNODE(node);

	KASSERT(vp->v_usecount == 0);

	lru_requeue(vp, NULL);
	rw_destroy(&vp->v_lock);
	uvm_obj_destroy(&vp->v_uobj, true);
	cv_destroy(&vp->v_cv);
	pool_cache_put(vcache.pool, node);
}

/*
 * Try to get an initial reference on this cached vnode.
 * Returns zero on success,  ENOENT if the vnode has been reclaimed and
 * EBUSY if the vnode state is unstable.
 *
 * v_interlock locked on entry and unlocked on exit.
 */
int
vcache_tryvget(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));

	/*
	 * Before adding a reference, we must remove the vnode
	 * from its freelist.
	 */
	if (vp->v_usecount == 0) {
		vp->v_usecount = 1;
	} else {
		atomic_inc_uint(&vp->v_usecount);
	}

	/*
	 * If the vnode is in the process of changing state we wait
	 * for the change to complete and take care not to return
	 * a clean vnode.
	 */
	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		vrelel(vp, 0);
		return ENOENT;
	} else if (VSTATE_GET(vp) != VS_ACTIVE) {
		vrelel(vp, 0);
		return EBUSY;
	}

	/*
	 * Ok, we got it in good shape.
	 */
	VSTATE_ASSERT(vp, VS_ACTIVE);
	mutex_exit(vp->v_interlock);

	return 0;
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

	/*
	 * Before adding a reference, we must remove the vnode
	 * from its freelist.
	 */
	if (vp->v_usecount == 0) {
		vp->v_usecount = 1;
	} else {
		atomic_inc_uint(&vp->v_usecount);
	}

	/*
	 * If the vnode is in the process of changing state we wait
	 * for the change to complete and take care not to return
	 * a clean vnode.
	 */
	VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) == VS_RECLAIMED) {
		vrelel(vp, 0);
		return ENOENT;
	} else if (VSTATE_GET(vp) != VS_ACTIVE) {
		vrelel(vp, 0);
		return EBUSY;
	}

	/*
	 * Ok, we got it in good shape.
	 */
	VSTATE_ASSERT(vp, VS_ACTIVE);
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
	vnode_impl_t *node, *new_node;

	new_key = NULL;
	*vpp = NULL;

	vcache_key.vk_mount = mp;
	vcache_key.vk_key = key;
	vcache_key.vk_key_len = key_len;
	hash = vcache_hash(&vcache_key);

again:
	mutex_enter(&vcache.lock);
	node = vcache_hash_lookup(&vcache_key, hash);

	/* If found, take a reference or retry. */
	if (__predict_true(node != NULL)) {
		/*
		 * If the vnode is loading we cannot take the v_interlock
		 * here as it might change during load (see uvm_obj_setlock()).
		 * As changing state from VS_LOADING requires both vcache.lock
		 * and v_interlock it is safe to test with vcache.lock held.
		 *
		 * Wait for vnodes changing state from VS_LOADING and retry.
		 */
		if (__predict_false(node->vi_state == VS_LOADING)) {
			cv_wait(&vcache.cv, &vcache.lock);
			mutex_exit(&vcache.lock);
			goto again;
		}
		vp = VIMPL_TO_VNODE(node);
		mutex_enter(vp->v_interlock);
		mutex_exit(&vcache.lock);
		error = vcache_vget(vp);
		if (error == ENOENT)
			goto again;
		if (error == 0)
			*vpp = vp;
		KASSERT((error != 0) == (*vpp == NULL));
		return error;
	}
	mutex_exit(&vcache.lock);

	/* Allocate and initialize a new vcache / vnode pair. */
	error = vfs_busy(mp, NULL);
	if (error)
		return error;
	new_node = vcache_alloc();
	new_node->vi_key = vcache_key;
	vp = VIMPL_TO_VNODE(new_node);
	mutex_enter(&vcache.lock);
	node = vcache_hash_lookup(&vcache_key, hash);
	if (node == NULL) {
		SLIST_INSERT_HEAD(&vcache.hashtab[hash & vcache.hashmask],
		    new_node, vi_hash);
		node = new_node;
	}

	/* If another thread beat us inserting this node, retry. */
	if (node != new_node) {
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VS_LOADING, VS_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(vp, 0);
		vfs_unbusy(mp, false, NULL);
		goto again;
	}
	mutex_exit(&vcache.lock);

	/* Load the fs node.  Exclusive as new_node is VS_LOADING. */
	error = VFS_LOADVNODE(mp, vp, key, key_len, &new_key);
	if (error) {
		mutex_enter(&vcache.lock);
		SLIST_REMOVE(&vcache.hashtab[hash & vcache.hashmask],
		    new_node, vnode_impl, vi_hash);
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VS_LOADING, VS_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(vp, 0);
		vfs_unbusy(mp, false, NULL);
		KASSERT(*vpp == NULL);
		return error;
	}
	KASSERT(new_key != NULL);
	KASSERT(memcmp(key, new_key, key_len) == 0);
	KASSERT(vp->v_op != NULL);
	vfs_insmntque(vp, mp);
	if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
		vp->v_vflag |= VV_MPSAFE;
	vfs_unbusy(mp, true, NULL);

	/* Finished loading, finalize node. */
	mutex_enter(&vcache.lock);
	new_node->vi_key.vk_key = new_key;
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VS_LOADING, VS_ACTIVE);
	mutex_exit(vp->v_interlock);
	mutex_exit(&vcache.lock);
	*vpp = vp;
	return 0;
}

/*
 * Create a new vnode / fs node pair and return it referenced through vpp.
 */
int
vcache_new(struct mount *mp, struct vnode *dvp, struct vattr *vap,
    kauth_cred_t cred, struct vnode **vpp)
{
	int error;
	uint32_t hash;
	struct vnode *ovp, *vp;
	vnode_impl_t *new_node;
	vnode_impl_t *old_node __diagused;

	*vpp = NULL;

	/* Allocate and initialize a new vcache / vnode pair. */
	error = vfs_busy(mp, NULL);
	if (error)
		return error;
	new_node = vcache_alloc();
	new_node->vi_key.vk_mount = mp;
	vp = VIMPL_TO_VNODE(new_node);

	/* Create and load the fs node. */
	error = VFS_NEWVNODE(mp, dvp, vp, vap, cred,
	    &new_node->vi_key.vk_key_len, &new_node->vi_key.vk_key);
	if (error) {
		mutex_enter(&vcache.lock);
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VS_LOADING, VS_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(vp, 0);
		vfs_unbusy(mp, false, NULL);
		KASSERT(*vpp == NULL);
		return error;
	}
	KASSERT(new_node->vi_key.vk_key != NULL);
	KASSERT(vp->v_op != NULL);
	hash = vcache_hash(&new_node->vi_key);

	/* Wait for previous instance to be reclaimed, then insert new node. */
	mutex_enter(&vcache.lock);
	while ((old_node = vcache_hash_lookup(&new_node->vi_key, hash))) {
		ovp = VIMPL_TO_VNODE(old_node);
		mutex_enter(ovp->v_interlock);
		mutex_exit(&vcache.lock);
		error = vcache_vget(ovp);
		KASSERT(error == ENOENT);
		mutex_enter(&vcache.lock);
	}
	SLIST_INSERT_HEAD(&vcache.hashtab[hash & vcache.hashmask],
	    new_node, vi_hash);
	mutex_exit(&vcache.lock);
	vfs_insmntque(vp, mp);
	if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
		vp->v_vflag |= VV_MPSAFE;
	vfs_unbusy(mp, true, NULL);

	/* Finished loading, finalize node. */
	mutex_enter(&vcache.lock);
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VS_LOADING, VS_ACTIVE);
	mutex_exit(&vcache.lock);
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
	vnode_impl_t *node, *new_node;
	struct vnode *tvp;

	old_vcache_key.vk_mount = mp;
	old_vcache_key.vk_key = old_key;
	old_vcache_key.vk_key_len = old_key_len;
	old_hash = vcache_hash(&old_vcache_key);

	new_vcache_key.vk_mount = mp;
	new_vcache_key.vk_key = new_key;
	new_vcache_key.vk_key_len = new_key_len;
	new_hash = vcache_hash(&new_vcache_key);

	new_node = vcache_alloc();
	new_node->vi_key = new_vcache_key;
	tvp = VIMPL_TO_VNODE(new_node);

	/* Insert locked new node used as placeholder. */
	mutex_enter(&vcache.lock);
	node = vcache_hash_lookup(&new_vcache_key, new_hash);
	if (node != NULL) {
		mutex_enter(tvp->v_interlock);
		VSTATE_CHANGE(tvp, VS_LOADING, VS_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(tvp, 0);
		return EEXIST;
	}
	SLIST_INSERT_HEAD(&vcache.hashtab[new_hash & vcache.hashmask],
	    new_node, vi_hash);

	/* Replace old nodes key with the temporary copy. */
	node = vcache_hash_lookup(&old_vcache_key, old_hash);
	KASSERT(node != NULL);
	KASSERT(VIMPL_TO_VNODE(node) == vp);
	KASSERT(node->vi_key.vk_key != old_vcache_key.vk_key);
	node->vi_key = old_vcache_key;
	mutex_exit(&vcache.lock);
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
	vnode_impl_t *old_node, *new_node;
	struct vnode *tvp;

	old_vcache_key.vk_mount = mp;
	old_vcache_key.vk_key = old_key;
	old_vcache_key.vk_key_len = old_key_len;
	old_hash = vcache_hash(&old_vcache_key);

	new_vcache_key.vk_mount = mp;
	new_vcache_key.vk_key = new_key;
	new_vcache_key.vk_key_len = new_key_len;
	new_hash = vcache_hash(&new_vcache_key);

	mutex_enter(&vcache.lock);

	/* Lookup old and new node. */
	old_node = vcache_hash_lookup(&old_vcache_key, old_hash);
	KASSERT(old_node != NULL);
	KASSERT(VIMPL_TO_VNODE(old_node) == vp);

	new_node = vcache_hash_lookup(&new_vcache_key, new_hash);
	KASSERT(new_node != NULL);
	KASSERT(new_node->vi_key.vk_key_len == new_key_len);
	tvp = VIMPL_TO_VNODE(new_node);
	mutex_enter(tvp->v_interlock);
	VSTATE_ASSERT(VIMPL_TO_VNODE(new_node), VS_LOADING);

	/* Rekey old node and put it onto its new hashlist. */
	old_node->vi_key = new_vcache_key;
	if (old_hash != new_hash) {
		SLIST_REMOVE(&vcache.hashtab[old_hash & vcache.hashmask],
		    old_node, vnode_impl, vi_hash);
		SLIST_INSERT_HEAD(&vcache.hashtab[new_hash & vcache.hashmask],
		    old_node, vi_hash);
	}

	/* Remove new node used as placeholder. */
	SLIST_REMOVE(&vcache.hashtab[new_hash & vcache.hashmask],
	    new_node, vnode_impl, vi_hash);
	VSTATE_CHANGE(tvp, VS_LOADING, VS_RECLAIMED);
	mutex_exit(&vcache.lock);
	vrelel(tvp, 0);
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
	vnode_impl_t *node = VNODE_TO_VIMPL(vp);
	uint32_t hash;
	uint8_t temp_buf[64], *temp_key;
	size_t temp_key_len;
	bool recycle, active;
	int error;

	KASSERT((vp->v_vflag & VV_LOCKSWORK) == 0 ||
	    VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT(vp->v_usecount != 0);

	active = (vp->v_usecount > 1);
	temp_key_len = node->vi_key.vk_key_len;
	/*
	 * Prevent the vnode from being recycled or brought into use
	 * while we clean it out.
	 */
	VSTATE_CHANGE(vp, VS_ACTIVE, VS_RECLAIMING);
	if (vp->v_iflag & VI_EXECMAP) {
		atomic_add_int(&uvmexp.execpages, -vp->v_uobj.uo_npages);
		atomic_add_int(&uvmexp.filepages, vp->v_uobj.uo_npages);
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP);
	mutex_exit(vp->v_interlock);

	/* Replace the vnode key with a temporary copy. */
	if (node->vi_key.vk_key_len > sizeof(temp_buf)) {
		temp_key = kmem_alloc(temp_key_len, KM_SLEEP);
	} else {
		temp_key = temp_buf;
	}
	mutex_enter(&vcache.lock);
	memcpy(temp_key, node->vi_key.vk_key, temp_key_len);
	node->vi_key.vk_key = temp_key;
	mutex_exit(&vcache.lock);

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
	 * Note that the VOP_INACTIVE will unlock the vnode.
	 */
	VOP_INACTIVE(vp, &recycle);
	if (VOP_RECLAIM(vp)) {
		vnpanic(vp, "%s: cannot reclaim", __func__);
	}

	KASSERT(vp->v_data == NULL);
	KASSERT(vp->v_uobj.uo_npages == 0);

	if (vp->v_type == VREG && vp->v_ractx != NULL) {
		uvm_ra_freectx(vp->v_ractx);
		vp->v_ractx = NULL;
	}

	/* Purge name cache. */
	cache_purge(vp);

	/* Move to dead mount. */
	vp->v_vflag &= ~VV_ROOT;
	atomic_inc_uint(&dead_rootmount->mnt_refcnt);
	vfs_insmntque(vp, dead_rootmount);

	/* Remove from vnode cache. */
	hash = vcache_hash(&node->vi_key);
	mutex_enter(&vcache.lock);
	KASSERT(node == vcache_hash_lookup(&node->vi_key, hash));
	SLIST_REMOVE(&vcache.hashtab[hash & vcache.hashmask],
	    node, vnode_impl, vi_hash);
	mutex_exit(&vcache.lock);
	if (temp_key != temp_buf)
		kmem_free(temp_key, temp_key_len);

	/* Done with purge, notify sleepers of the grim news. */
	mutex_enter(vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	vp->v_vflag |= VV_LOCKSWORK;
	VSTATE_CHANGE(vp, VS_RECLAIMING, VS_RECLAIMED);
	vp->v_tag = VT_NON;
	KNOTE(&vp->v_klist, NOTE_REVOKE);

	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
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

	if (vcache.hashsize != desiredvnodes)
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
