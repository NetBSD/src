/*	$NetBSD: vfs_vnode.c,v 1.39.2.6 2016/10/05 20:56:03 skrll Exp $	*/

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
 *	- Reclamation of inactive vnode, via vget(9).
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
__KERNEL_RCSID(0, "$NetBSD: vfs_vnode.c,v 1.39.2.6 2016/10/05 20:56:03 skrll Exp $");

#define _VFS_VNODE_PRIVATE

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
#include <sys/vnode.h>
#include <sys/wapbl.h>
#include <sys/fstrans.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

/* Flags to vrelel. */
#define	VRELEL_ASYNC_RELE	0x0001	/* Always defer to vrele thread. */

enum vcache_state {
	VN_MARKER,	/* Stable, used as marker. Will not change. */
	VN_LOADING,	/* Intermediate, initialising the fs node. */
	VN_ACTIVE,	/* Stable, valid fs node attached. */
	VN_BLOCKED,	/* Intermediate, active, no new references allowed. */
	VN_RECLAIMING,	/* Intermediate, detaching the fs node. */
	VN_RECLAIMED	/* Stable, no fs node attached. */
};
struct vcache_key {
	struct mount *vk_mount;
	const void *vk_key;
	size_t vk_key_len;
};
struct vcache_node {
	struct vnode vn_vnode;
	enum vcache_state vn_state;
	SLIST_ENTRY(vcache_node) vn_hash;
	struct vcache_key vn_key;
};

#define VN_TO_VP(node)	((vnode_t *)(node))
#define VP_TO_VN(vp)	((struct vcache_node *)(vp))

u_int			numvnodes		__cacheline_aligned;

/*
 * There are two free lists: one is for vnodes which have no buffer/page
 * references and one for those which do (i.e. v_holdcnt is non-zero).
 * Vnode recycling mechanism first attempts to look into the former list.
 */
static kmutex_t		vnode_free_list_lock	__cacheline_aligned;
static vnodelst_t	vnode_free_list		__cacheline_aligned;
static vnodelst_t	vnode_hold_list		__cacheline_aligned;
static kcondvar_t	vdrain_cv		__cacheline_aligned;

static vnodelst_t	vrele_list		__cacheline_aligned;
static kmutex_t		vrele_lock		__cacheline_aligned;
static kcondvar_t	vrele_cv		__cacheline_aligned;
static lwp_t *		vrele_lwp		__cacheline_aligned;
static int		vrele_pending		__cacheline_aligned;
static int		vrele_gen		__cacheline_aligned;

SLIST_HEAD(hashhead, vcache_node);
static struct {
	kmutex_t	lock;
	kcondvar_t	cv;
	u_long		hashmask;
	struct hashhead	*hashtab;
	pool_cache_t	pool;
}			vcache			__cacheline_aligned;

static int		cleanvnode(void);
static struct vcache_node *vcache_alloc(void);
static void		vcache_free(struct vcache_node *);
static void		vcache_init(void);
static void		vcache_reinit(void);
static void		vcache_reclaim(vnode_t *);
static void		vrelel(vnode_t *, int);
static void		vdrain_thread(void *);
static void		vrele_thread(void *);
static void		vnpanic(vnode_t *, const char *, ...)
    __printflike(2, 3);

/* Routines having to do with the management of the vnode table. */
extern struct mount	*dead_rootmount;
extern int		(**dead_vnodeop_p)(void *);
extern struct vfsops	dead_vfsops;

/* Vnode state operations and diagnostics. */

static const char *
vstate_name(enum vcache_state state)
{

	switch (state) {
	case VN_MARKER:
		return "MARKER";
	case VN_LOADING:
		return "LOADING";
	case VN_ACTIVE:
		return "ACTIVE";
	case VN_BLOCKED:
		return "BLOCKED";
	case VN_RECLAIMING:
		return "RECLAIMING";
	case VN_RECLAIMED:
		return "RECLAIMED";
	default:
		return "ILLEGAL";
	}
}

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
vstate_assert(vnode_t *vp, enum vcache_state state, const char *func, int line)
{
	struct vcache_node *node = VP_TO_VN(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);

	if (__predict_true(node->vn_state == state))
		return;
	vnpanic(vp, "state is %s, expected %s at %s:%d",
	    vstate_name(node->vn_state), vstate_name(state), func, line);
}

static enum vcache_state
vstate_assert_get(vnode_t *vp, const char *func, int line)
{
	struct vcache_node *node = VP_TO_VN(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (node->vn_state == VN_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vn_state), func, line);

	return node->vn_state;
}

static void
vstate_assert_wait_stable(vnode_t *vp, const char *func, int line)
{
	struct vcache_node *node = VP_TO_VN(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (node->vn_state == VN_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vn_state), func, line);

	while (node->vn_state != VN_ACTIVE && node->vn_state != VN_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);

	if (node->vn_state == VN_MARKER)
		vnpanic(vp, "state is %s at %s:%d",
		    vstate_name(node->vn_state), func, line);
}

static void
vstate_assert_change(vnode_t *vp, enum vcache_state from, enum vcache_state to,
    const char *func, int line)
{
	struct vcache_node *node = VP_TO_VN(vp);

	KASSERTMSG(mutex_owned(vp->v_interlock), "at %s:%d", func, line);
	if (from == VN_LOADING)
		KASSERTMSG(mutex_owned(&vcache.lock), "at %s:%d", func, line);

	if (from == VN_MARKER)
		vnpanic(vp, "from is %s at %s:%d",
		    vstate_name(from), func, line);
	if (to == VN_MARKER)
		vnpanic(vp, "to is %s at %s:%d",
		    vstate_name(to), func, line);
	if (node->vn_state != from)
		vnpanic(vp, "from is %s, expected %s at %s:%d\n",
		    vstate_name(node->vn_state), vstate_name(from), func, line);

	node->vn_state = to;
	if (from == VN_LOADING)
		cv_broadcast(&vcache.cv);
	if (to == VN_ACTIVE || to == VN_RECLAIMED)
		cv_broadcast(&vp->v_cv);
}

#else /* defined(DIAGNOSTIC) */

#define VSTATE_GET(vp) \
	(VP_TO_VN((vp))->vn_state)
#define VSTATE_CHANGE(vp, from, to) \
	vstate_change((vp), (from), (to))
#define VSTATE_WAIT_STABLE(vp) \
	vstate_wait_stable((vp))
#define VSTATE_ASSERT(vp, state)

static void
vstate_wait_stable(vnode_t *vp)
{
	struct vcache_node *node = VP_TO_VN(vp);

	while (node->vn_state != VN_ACTIVE && node->vn_state != VN_RECLAIMED)
		cv_wait(&vp->v_cv, vp->v_interlock);
}

static void
vstate_change(vnode_t *vp, enum vcache_state from, enum vcache_state to)
{
	struct vcache_node *node = VP_TO_VN(vp);

	node->vn_state = to;
	if (from == VN_LOADING)
		cv_broadcast(&vcache.cv);
	if (to == VN_ACTIVE || to == VN_RECLAIMED)
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

	mutex_init(&vnode_free_list_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&vnode_free_list);
	TAILQ_INIT(&vnode_hold_list);
	TAILQ_INIT(&vrele_list);

	vcache_init();

	mutex_init(&vrele_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vdrain_cv, "vdrain");
	cv_init(&vrele_cv, "vrele");
	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vdrain_thread,
	    NULL, NULL, "vdrain");
	KASSERTMSG((error == 0), "kthread_create(vdrain) failed: %d", error);
	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vrele_thread,
	    NULL, &vrele_lwp, "vrele");
	KASSERTMSG((error == 0), "kthread_create(vrele) failed: %d", error);
}

/*
 * Allocate a new marker vnode.
 */
vnode_t *
vnalloc_marker(struct mount *mp)
{
	struct vcache_node *node;
	vnode_t *vp;

	node = pool_cache_get(vcache.pool, PR_WAITOK);
	memset(node, 0, sizeof(*node));
	vp = VN_TO_VP(node);
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 0);
	vp->v_mount = mp;
	vp->v_type = VBAD;
	node->vn_state = VN_MARKER;

	return vp;
}

/*
 * Free a marker vnode.
 */
void
vnfree_marker(vnode_t *vp)
{
	struct vcache_node *node;

	node = VP_TO_VN(vp);
	KASSERT(node->vn_state == VN_MARKER);
	uvm_obj_destroy(&vp->v_uobj, true);
	pool_cache_put(vcache.pool, node);
}

/*
 * Test a vnode for being a marker vnode.
 */
bool
vnis_marker(vnode_t *vp)
{

	return (VP_TO_VN(vp)->vn_state == VN_MARKER);
}

/*
 * cleanvnode: grab a vnode from freelist, clean and free it.
 *
 * => Releases vnode_free_list_lock.
 */
static int
cleanvnode(void)
{
	vnode_t *vp;
	vnodelst_t *listhd;
	struct mount *mp;

	KASSERT(mutex_owned(&vnode_free_list_lock));

	listhd = &vnode_free_list;
try_nextlist:
	TAILQ_FOREACH(vp, listhd, v_freelist) {
		/*
		 * It's safe to test v_usecount and v_iflag
		 * without holding the interlock here, since
		 * these vnodes should never appear on the
		 * lists.
		 */
		KASSERT(vp->v_usecount == 0);
		KASSERT(vp->v_freelisthd == listhd);

		if (vn_lock(vp, LK_EXCLUSIVE | LK_NOWAIT) != 0)
			continue;
		if (!mutex_tryenter(vp->v_interlock)) {
			VOP_UNLOCK(vp);
			continue;
		}
		mp = vp->v_mount;
		if (fstrans_start_nowait(mp, FSTRANS_SHARED) != 0) {
			mutex_exit(vp->v_interlock);
			VOP_UNLOCK(vp);
			continue;
		}
		break;
	}

	if (vp == NULL) {
		if (listhd == &vnode_free_list) {
			listhd = &vnode_hold_list;
			goto try_nextlist;
		}
		mutex_exit(&vnode_free_list_lock);
		return EBUSY;
	}

	/* Remove it from the freelist. */
	TAILQ_REMOVE(listhd, vp, v_freelist);
	vp->v_freelisthd = NULL;
	mutex_exit(&vnode_free_list_lock);

	KASSERT(vp->v_usecount == 0);

	/*
	 * The vnode is still associated with a file system, so we must
	 * clean it out before freeing it.  We need to add a reference
	 * before doing this.
	 */
	vp->v_usecount = 1;
	vcache_reclaim(vp);
	vrelel(vp, 0);
	fstrans_done(mp);

	return 0;
}

/*
 * Helper thread to keep the number of vnodes below desiredvnodes.
 */
static void
vdrain_thread(void *cookie)
{
	int error;

	mutex_enter(&vnode_free_list_lock);

	for (;;) {
		cv_timedwait(&vdrain_cv, &vnode_free_list_lock, hz);
		while (numvnodes > desiredvnodes) {
			error = cleanvnode();
			if (error)
				kpause("vndsbusy", false, hz, NULL);
			mutex_enter(&vnode_free_list_lock);
			if (error)
				break;
		}
	}
}

/*
 * Remove a vnode from its freelist.
 */
void
vremfree(vnode_t *vp)
{

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT(vp->v_usecount == 0);

	/*
	 * Note that the reference count must not change until
	 * the vnode is removed.
	 */
	mutex_enter(&vnode_free_list_lock);
	if (vp->v_holdcnt > 0) {
		KASSERT(vp->v_freelisthd == &vnode_hold_list);
	} else {
		KASSERT(vp->v_freelisthd == &vnode_free_list);
	}
	TAILQ_REMOVE(vp->v_freelisthd, vp, v_freelist);
	vp->v_freelisthd = NULL;
	mutex_exit(&vnode_free_list_lock);
}

/*
 * vget: get a particular vnode from the free list, increment its reference
 * count and return it.
 *
 * => Must be called with v_interlock held.
 *
 * If state is VN_RECLAIMING, the vnode may be eliminated in vcache_reclaim().
 * In that case, we cannot grab the vnode, so the process is awakened when
 * the transition is completed, and an error returned to indicate that the
 * vnode is no longer usable.
 *
 * If state is VN_LOADING or VN_BLOCKED, wait until the vnode enters a
 * stable state (VN_ACTIVE or VN_RECLAIMED).
 */
int
vget(vnode_t *vp, int flags, bool waitok)
{

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT((flags & ~LK_NOWAIT) == 0);
	KASSERT(waitok == ((flags & LK_NOWAIT) == 0));

	/*
	 * Before adding a reference, we must remove the vnode
	 * from its freelist.
	 */
	if (vp->v_usecount == 0) {
		vremfree(vp);
		vp->v_usecount = 1;
	} else {
		atomic_inc_uint(&vp->v_usecount);
	}

	/*
	 * If the vnode is in the process of changing state we wait
	 * for the change to complete and take care not to return
	 * a clean vnode.
	 */
	if (! ISSET(flags, LK_NOWAIT))
		VSTATE_WAIT_STABLE(vp);
	if (VSTATE_GET(vp) == VN_RECLAIMED) {
		vrelel(vp, 0);
		return ENOENT;
	} else if (VSTATE_GET(vp) != VN_ACTIVE) {
		KASSERT(ISSET(flags, LK_NOWAIT));
		vrelel(vp, 0);
		return EBUSY;
	}

	/*
	 * Ok, we got it in good shape.
	 */
	VSTATE_ASSERT(vp, VN_ACTIVE);
	mutex_exit(vp->v_interlock);

	return 0;
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
	KASSERT(vp->v_freelisthd == NULL);

	if (__predict_false(vp->v_op == dead_vnodeop_p &&
	    VSTATE_GET(vp) != VN_RECLAIMED)) {
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
	if (VSTATE_GET(vp) != VN_RECLAIMED) {
		recycle = false;

		/*
		 * XXX This ugly block can be largely eliminated if
		 * locking is pushed down into the file systems.
		 *
		 * Defer vnode release to vrele_thread if caller
		 * requests it explicitly or is the pagedaemon.
		 */
		if ((curlwp == uvm.pagedaemon_lwp) ||
		    (flags & VRELEL_ASYNC_RELE) != 0) {
			defer = true;
		} else if (curlwp == vrele_lwp) {
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
		KASSERT(! (curlwp == vrele_lwp && defer));

		if (defer) {
			/*
			 * Defer reclaim to the kthread; it's not safe to
			 * clean it here.  We donate it our last reference.
			 */
			mutex_enter(&vrele_lock);
			TAILQ_INSERT_TAIL(&vrele_list, vp, v_freelist);
			if (++vrele_pending > (desiredvnodes >> 8))
				cv_signal(&vrele_cv);
			mutex_exit(&vrele_lock);
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
		VSTATE_CHANGE(vp, VN_ACTIVE, VN_BLOCKED);
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
		VSTATE_CHANGE(vp, VN_BLOCKED, VN_ACTIVE);
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
			VSTATE_ASSERT(vp, VN_ACTIVE);
			vcache_reclaim(vp);
		}
		KASSERT(vp->v_usecount > 0);
	}

	if (atomic_dec_uint_nv(&vp->v_usecount) != 0) {
		/* Gained another reference while being reclaimed. */
		mutex_exit(vp->v_interlock);
		return;
	}

	if (VSTATE_GET(vp) == VN_RECLAIMED) {
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
		vcache_free(VP_TO_VN(vp));
	} else {
		/*
		 * Otherwise, put it back onto the freelist.  It
		 * can't be destroyed while still associated with
		 * a file system.
		 */
		mutex_enter(&vnode_free_list_lock);
		if (vp->v_holdcnt > 0) {
			vp->v_freelisthd = &vnode_hold_list;
		} else {
			vp->v_freelisthd = &vnode_free_list;
		}
		TAILQ_INSERT_TAIL(vp->v_freelisthd, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
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

static void
vrele_thread(void *cookie)
{
	vnodelst_t skip_list;
	vnode_t *vp;
	struct mount *mp;

	TAILQ_INIT(&skip_list);

	mutex_enter(&vrele_lock);
	for (;;) {
		while (TAILQ_EMPTY(&vrele_list)) {
			vrele_gen++;
			cv_broadcast(&vrele_cv);
			cv_timedwait(&vrele_cv, &vrele_lock, hz);
			TAILQ_CONCAT(&vrele_list, &skip_list, v_freelist);
		}
		vp = TAILQ_FIRST(&vrele_list);
		mp = vp->v_mount;
		TAILQ_REMOVE(&vrele_list, vp, v_freelist);
		if (fstrans_start_nowait(mp, FSTRANS_LAZY) != 0) {
			TAILQ_INSERT_TAIL(&skip_list, vp, v_freelist);
			continue;
		}
		vrele_pending--;
		mutex_exit(&vrele_lock);

		/*
		 * If not the last reference, then ignore the vnode
		 * and look for more work.
		 */
		mutex_enter(vp->v_interlock);
		vrelel(vp, 0);
		fstrans_done(mp);
		mutex_enter(&vrele_lock);
	}
}

void
vrele_flush(void)
{
	int gen;

	mutex_enter(&vrele_lock);
	gen = vrele_gen;
	while (vrele_pending && gen == vrele_gen) {
		cv_broadcast(&vrele_cv);
		cv_wait(&vrele_cv, &vrele_lock);
	}
	mutex_exit(&vrele_lock);
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

	if (vp->v_holdcnt++ == 0 && vp->v_usecount == 0) {
		mutex_enter(&vnode_free_list_lock);
		KASSERT(vp->v_freelisthd == &vnode_free_list);
		TAILQ_REMOVE(vp->v_freelisthd, vp, v_freelist);
		vp->v_freelisthd = &vnode_hold_list;
		TAILQ_INSERT_TAIL(vp->v_freelisthd, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
	}
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
	if (vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		mutex_enter(&vnode_free_list_lock);
		KASSERT(vp->v_freelisthd == &vnode_hold_list);
		TAILQ_REMOVE(vp->v_freelisthd, vp, v_freelist);
		vp->v_freelisthd = &vnode_free_list;
		TAILQ_INSERT_TAIL(vp->v_freelisthd, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
	}
}

/*
 * Recycle an unused vnode if caller holds the last reference.
 */
bool
vrecycle(vnode_t *vp)
{

	if (vn_lock(vp, LK_EXCLUSIVE) != 0)
		return false;

	mutex_enter(vp->v_interlock);

	if (vp->v_usecount != 1) {
		mutex_exit(vp->v_interlock);
		VOP_UNLOCK(vp);
		return false;
	}
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
	if (VSTATE_GET(vp) == VN_RECLAIMED) {
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
		VSTATE_ASSERT(vp, VN_RECLAIMED);
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

	vcache.pool = pool_cache_init(sizeof(struct vcache_node), 0, 0, 0,
	    "vcachepl", NULL, IPL_NONE, NULL, NULL, NULL);
	KASSERT(vcache.pool != NULL);
	mutex_init(&vcache.lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vcache.cv, "vcache");
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
	struct vcache_node *node;

	newtab = hashinit(desiredvnodes, HASH_SLIST, true, &newmask);
	mutex_enter(&vcache.lock);
	oldtab = vcache.hashtab;
	oldmask = vcache.hashmask;
	vcache.hashtab = newtab;
	vcache.hashmask = newmask;
	for (i = 0; i <= oldmask; i++) {
		while ((node = SLIST_FIRST(&oldtab[i])) != NULL) {
			SLIST_REMOVE(&oldtab[i], node, vcache_node, vn_hash);
			hash = vcache_hash(&node->vn_key);
			SLIST_INSERT_HEAD(&newtab[hash & vcache.hashmask],
			    node, vn_hash);
		}
	}
	mutex_exit(&vcache.lock);
	hashdone(oldtab, HASH_SLIST, oldmask);
}

static inline struct vcache_node *
vcache_hash_lookup(const struct vcache_key *key, uint32_t hash)
{
	struct hashhead *hashp;
	struct vcache_node *node;

	KASSERT(mutex_owned(&vcache.lock));

	hashp = &vcache.hashtab[hash & vcache.hashmask];
	SLIST_FOREACH(node, hashp, vn_hash) {
		if (key->vk_mount != node->vn_key.vk_mount)
			continue;
		if (key->vk_key_len != node->vn_key.vk_key_len)
			continue;
		if (memcmp(key->vk_key, node->vn_key.vk_key, key->vk_key_len))
			continue;
		return node;
	}
	return NULL;
}

/*
 * Allocate a new, uninitialized vcache node.
 */
static struct vcache_node *
vcache_alloc(void)
{
	struct vcache_node *node;
	vnode_t *vp;

	node = pool_cache_get(vcache.pool, PR_WAITOK);
	memset(node, 0, sizeof(*node));

	/* SLIST_INIT(&node->vn_hash); */

	vp = VN_TO_VP(node);
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 0);
	cv_init(&vp->v_cv, "vnode");
	/* LIST_INIT(&vp->v_nclist); */
	/* LIST_INIT(&vp->v_dnclist); */

	mutex_enter(&vnode_free_list_lock);
	numvnodes++;
	if (numvnodes > desiredvnodes + desiredvnodes / 10)
		cv_signal(&vdrain_cv);
	mutex_exit(&vnode_free_list_lock);

	rw_init(&vp->v_lock);
	vp->v_usecount = 1;
	vp->v_type = VNON;
	vp->v_size = vp->v_writesize = VSIZENOTSET;

	node->vn_state = VN_LOADING;

	return node;
}

/*
 * Free an unused, unreferenced vcache node.
 */
static void
vcache_free(struct vcache_node *node)
{
	vnode_t *vp;

	vp = VN_TO_VP(node);

	KASSERT(vp->v_usecount == 0);

	rw_destroy(&vp->v_lock);
	mutex_enter(&vnode_free_list_lock);
	numvnodes--;
	mutex_exit(&vnode_free_list_lock);

	uvm_obj_destroy(&vp->v_uobj, true);
	cv_destroy(&vp->v_cv);
	pool_cache_put(vcache.pool, node);
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
	struct vcache_node *node, *new_node;

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
		 * As changing state from VN_LOADING requires both vcache.lock
		 * and v_interlock it is safe to test with vcache.lock held.
		 *
		 * Wait for vnodes changing state from VN_LOADING and retry.
		 */
		if (__predict_false(node->vn_state == VN_LOADING)) {
			cv_wait(&vcache.cv, &vcache.lock);
			mutex_exit(&vcache.lock);
			goto again;
		}
		vp = VN_TO_VP(node);
		mutex_enter(vp->v_interlock);
		mutex_exit(&vcache.lock);
		error = vget(vp, 0, true /* wait */);
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
	new_node->vn_key = vcache_key;
	vp = VN_TO_VP(new_node);
	mutex_enter(&vcache.lock);
	node = vcache_hash_lookup(&vcache_key, hash);
	if (node == NULL) {
		SLIST_INSERT_HEAD(&vcache.hashtab[hash & vcache.hashmask],
		    new_node, vn_hash);
		node = new_node;
	}

	/* If another thread beat us inserting this node, retry. */
	if (node != new_node) {
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VN_LOADING, VN_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(vp, 0);
		vfs_unbusy(mp, false, NULL);
		goto again;
	}
	mutex_exit(&vcache.lock);

	/* Load the fs node.  Exclusive as new_node is VN_LOADING. */
	error = VFS_LOADVNODE(mp, vp, key, key_len, &new_key);
	if (error) {
		mutex_enter(&vcache.lock);
		SLIST_REMOVE(&vcache.hashtab[hash & vcache.hashmask],
		    new_node, vcache_node, vn_hash);
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VN_LOADING, VN_RECLAIMED);
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
	new_node->vn_key.vk_key = new_key;
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VN_LOADING, VN_ACTIVE);
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
	struct vcache_node *new_node;
	struct vcache_node *old_node __diagused;

	*vpp = NULL;

	/* Allocate and initialize a new vcache / vnode pair. */
	error = vfs_busy(mp, NULL);
	if (error)
		return error;
	new_node = vcache_alloc();
	new_node->vn_key.vk_mount = mp;
	vp = VN_TO_VP(new_node);

	/* Create and load the fs node. */
	error = VFS_NEWVNODE(mp, dvp, vp, vap, cred,
	    &new_node->vn_key.vk_key_len, &new_node->vn_key.vk_key);
	if (error) {
		mutex_enter(&vcache.lock);
		mutex_enter(vp->v_interlock);
		VSTATE_CHANGE(vp, VN_LOADING, VN_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(vp, 0);
		vfs_unbusy(mp, false, NULL);
		KASSERT(*vpp == NULL);
		return error;
	}
	KASSERT(new_node->vn_key.vk_key != NULL);
	KASSERT(vp->v_op != NULL);
	hash = vcache_hash(&new_node->vn_key);

	/* Wait for previous instance to be reclaimed, then insert new node. */
	mutex_enter(&vcache.lock);
	while ((old_node = vcache_hash_lookup(&new_node->vn_key, hash))) {
		ovp = VN_TO_VP(old_node);
		mutex_enter(ovp->v_interlock);
		mutex_exit(&vcache.lock);
		error = vget(ovp, 0, true /* wait */);
		KASSERT(error == ENOENT);
		mutex_enter(&vcache.lock);
	}
	SLIST_INSERT_HEAD(&vcache.hashtab[hash & vcache.hashmask],
	    new_node, vn_hash);
	mutex_exit(&vcache.lock);
	vfs_insmntque(vp, mp);
	if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
		vp->v_vflag |= VV_MPSAFE;
	vfs_unbusy(mp, true, NULL);

	/* Finished loading, finalize node. */
	mutex_enter(&vcache.lock);
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VN_LOADING, VN_ACTIVE);
	mutex_exit(&vcache.lock);
	mutex_exit(vp->v_interlock);
	*vpp = vp;
	return 0;
}

/*
 * Prepare key change: lock old and new cache node.
 * Return an error if the new node already exists.
 */
int
vcache_rekey_enter(struct mount *mp, struct vnode *vp,
    const void *old_key, size_t old_key_len,
    const void *new_key, size_t new_key_len)
{
	uint32_t old_hash, new_hash;
	struct vcache_key old_vcache_key, new_vcache_key;
	struct vcache_node *node, *new_node;
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
	new_node->vn_key = new_vcache_key;
	tvp = VN_TO_VP(new_node);

	/* Insert locked new node used as placeholder. */
	mutex_enter(&vcache.lock);
	node = vcache_hash_lookup(&new_vcache_key, new_hash);
	if (node != NULL) {
		mutex_enter(tvp->v_interlock);
		VSTATE_CHANGE(tvp, VN_LOADING, VN_RECLAIMED);
		mutex_exit(&vcache.lock);
		vrelel(tvp, 0);
		return EEXIST;
	}
	SLIST_INSERT_HEAD(&vcache.hashtab[new_hash & vcache.hashmask],
	    new_node, vn_hash);

	/* Lock old node. */
	node = vcache_hash_lookup(&old_vcache_key, old_hash);
	KASSERT(node != NULL);
	KASSERT(VN_TO_VP(node) == vp);
	mutex_enter(vp->v_interlock);
	VSTATE_CHANGE(vp, VN_ACTIVE, VN_BLOCKED);
	node->vn_key = old_vcache_key;
	mutex_exit(vp->v_interlock);
	mutex_exit(&vcache.lock);
	return 0;
}

/*
 * Key change complete: remove old node and unlock new node.
 */
void
vcache_rekey_exit(struct mount *mp, struct vnode *vp,
    const void *old_key, size_t old_key_len,
    const void *new_key, size_t new_key_len)
{
	uint32_t old_hash, new_hash;
	struct vcache_key old_vcache_key, new_vcache_key;
	struct vcache_node *old_node, *new_node;
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
	KASSERT(VN_TO_VP(old_node) == vp);
	mutex_enter(vp->v_interlock);
	VSTATE_ASSERT(vp, VN_BLOCKED);

	new_node = vcache_hash_lookup(&new_vcache_key, new_hash);
	KASSERT(new_node != NULL);
	KASSERT(new_node->vn_key.vk_key_len == new_key_len);
	tvp = VN_TO_VP(new_node);
	mutex_enter(tvp->v_interlock);
	VSTATE_ASSERT(VN_TO_VP(new_node), VN_LOADING);

	/* Rekey old node and put it onto its new hashlist. */
	old_node->vn_key = new_vcache_key;
	if (old_hash != new_hash) {
		SLIST_REMOVE(&vcache.hashtab[old_hash & vcache.hashmask],
		    old_node, vcache_node, vn_hash);
		SLIST_INSERT_HEAD(&vcache.hashtab[new_hash & vcache.hashmask],
		    old_node, vn_hash);
	}
	VSTATE_CHANGE(vp, VN_BLOCKED, VN_ACTIVE);
	mutex_exit(vp->v_interlock);

	/* Remove new node used as placeholder. */
	SLIST_REMOVE(&vcache.hashtab[new_hash & vcache.hashmask],
	    new_node, vcache_node, vn_hash);
	VSTATE_CHANGE(tvp, VN_LOADING, VN_RECLAIMED);
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
	struct vcache_node *node = VP_TO_VN(vp);
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
	temp_key_len = node->vn_key.vk_key_len;
	/*
	 * Prevent the vnode from being recycled or brought into use
	 * while we clean it out.
	 */
	VSTATE_CHANGE(vp, VN_ACTIVE, VN_RECLAIMING);
	if (vp->v_iflag & VI_EXECMAP) {
		atomic_add_int(&uvmexp.execpages, -vp->v_uobj.uo_npages);
		atomic_add_int(&uvmexp.filepages, vp->v_uobj.uo_npages);
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP);
	mutex_exit(vp->v_interlock);

	/* Replace the vnode key with a temporary copy. */
	if (node->vn_key.vk_key_len > sizeof(temp_buf)) {
		temp_key = kmem_alloc(temp_key_len, KM_SLEEP);
	} else {
		temp_key = temp_buf;
	}
	mutex_enter(&vcache.lock);
	memcpy(temp_key, node->vn_key.vk_key, temp_key_len);
	node->vn_key.vk_key = temp_key;
	mutex_exit(&vcache.lock);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
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
	if (active) {
		VOP_INACTIVE(vp, &recycle);
	} else {
		/*
		 * Any other processes trying to obtain this lock must first
		 * wait for VN_RECLAIMED, then call the new lock operation.
		 */
		VOP_UNLOCK(vp);
	}

	/* Disassociate the underlying file system from the vnode. */
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
	hash = vcache_hash(&node->vn_key);
	mutex_enter(&vcache.lock);
	KASSERT(node == vcache_hash_lookup(&node->vn_key, hash));
	SLIST_REMOVE(&vcache.hashtab[hash & vcache.hashmask],
	    node, vcache_node, vn_hash);
	mutex_exit(&vcache.lock);
	if (temp_key != temp_buf)
		kmem_free(temp_key, temp_key_len);

	/* Done with purge, notify sleepers of the grim news. */
	mutex_enter(vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	vp->v_vflag |= VV_LOCKSWORK;
	VSTATE_CHANGE(vp, VN_RECLAIMING, VN_RECLAIMED);
	vp->v_tag = VT_NON;
	KNOTE(&vp->v_klist, NOTE_REVOKE);

	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
}

/*
 * Print a vcache node.
 */
void
vcache_print(vnode_t *vp, const char *prefix, void (*pr)(const char *, ...))
{
	int n;
	const uint8_t *cp;
	struct vcache_node *node;

	node = VP_TO_VN(vp);
	n = node->vn_key.vk_key_len;
	cp = node->vn_key.vk_key;

	(*pr)("%sstate %s, key(%d)", prefix, vstate_name(node->vn_state), n);

	while (n-- > 0)
		(*pr)(" %02x", *cp++);
	(*pr)("\n");
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

	if (VSTATE_GET(vp) == VN_RECLAIMING) {
		KASSERT(ISSET(flags, VDEAD_NOWAIT));
		return EBUSY;
	} else if (VSTATE_GET(vp) == VN_RECLAIMED) {
		return ENOENT;
	}

	return 0;
}

int
vfs_drainvnodes(long target)
{
	int error;

	mutex_enter(&vnode_free_list_lock);

	while (numvnodes > target) {
		error = cleanvnode();
		if (error != 0)
			return error;
		mutex_enter(&vnode_free_list_lock);
	}

	mutex_exit(&vnode_free_list_lock);

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
