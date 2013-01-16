/*	$NetBSD: vfs_vnode.c,v 1.14.2.4 2013/01/16 05:33:45 yamt Exp $	*/

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
 *	- Allocation, via getnewvnode(9) and/or vnalloc(9).
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
 *	or cleaned via vclean(9), which calls VOP_RECLAIM(9) to disassociate
 *	underlying file system from the vnode, and finally destroyed.
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
 *	There is a flag bit, VC_XLOCK, embedded in v_usecount.  To raise
 *	v_usecount, if the VC_XLOCK bit is set in it, the interlock must
 *	be held.  To modify the VC_XLOCK bit, the interlock must be held.
 *	We always keep the usecount (v_usecount & VC_MASK) non-zero while
 *	the VC_XLOCK bit is set.
 *
 *	Unless the VC_XLOCK bit is set, changing the usecount from a non-zero
 *	value to a non-zero value can safely be done using atomic operations,
 *	without the interlock held.
 *
 *	Even if the VC_XLOCK bit is set, decreasing the usecount to a non-zero
 *	value can be done using atomic operations, without the interlock held.
 *
 *	Note: if VI_CLEAN is set, vnode_t::v_interlock will be released while
 *	mntvnode_lock is still held.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_vnode.c,v 1.14.2.4 2013/01/16 05:33:45 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
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

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

u_int			numvnodes		__cacheline_aligned;

static pool_cache_t	vnode_cache		__read_mostly;

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

static int		cleanvnode(void);
static void		vdrain_thread(void *);
static void		vrele_thread(void *);
static void		vnpanic(vnode_t *, const char *, ...)
    __attribute__((__format__(__printf__, 2, 3)));

/* Routines having to do with the management of the vnode table. */
extern int		(**dead_vnodeop_p)(void *);

void
vfs_vnode_sysinit(void)
{
	int error;

	vnode_cache = pool_cache_init(sizeof(vnode_t), 0, 0, 0, "vnodepl",
	    NULL, IPL_NONE, NULL, NULL, NULL);
	KASSERT(vnode_cache != NULL);

	mutex_init(&vnode_free_list_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&vnode_free_list);
	TAILQ_INIT(&vnode_hold_list);
	TAILQ_INIT(&vrele_list);

	mutex_init(&vrele_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vdrain_cv, "vdrain");
	cv_init(&vrele_cv, "vrele");
	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vdrain_thread,
	    NULL, NULL, "vdrain");
	KASSERT(error == 0);
	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vrele_thread,
	    NULL, &vrele_lwp, "vrele");
	KASSERT(error == 0);
}

/*
 * Allocate a new, uninitialized vnode.  If 'mp' is non-NULL, this is a
 * marker vnode.
 */
vnode_t *
vnalloc(struct mount *mp)
{
	vnode_t *vp;

	vp = pool_cache_get(vnode_cache, PR_WAITOK);
	KASSERT(vp != NULL);

	memset(vp, 0, sizeof(*vp));
	uvm_obj_init(&vp->v_uobj, &uvm_vnodeops, true, 0);
	cv_init(&vp->v_cv, "vnode");
	/*
	 * Done by memset() above.
	 *	LIST_INIT(&vp->v_nclist);
	 *	LIST_INIT(&vp->v_dnclist);
	 */

	if (mp != NULL) {
		vp->v_mount = mp;
		vp->v_type = VBAD;
		vp->v_iflag = VI_MARKER;
	} else {
		rw_init(&vp->v_lock);
	}

	return vp;
}

/*
 * Free an unused, unreferenced vnode.
 */
void
vnfree(vnode_t *vp)
{

	KASSERT(vp->v_usecount == 0);

	if ((vp->v_iflag & VI_MARKER) == 0) {
		rw_destroy(&vp->v_lock);
		mutex_enter(&vnode_free_list_lock);
		numvnodes--;
		mutex_exit(&vnode_free_list_lock);
	}

	/*
	 * Note: the vnode interlock will either be freed, of reference
	 * dropped (if VI_LOCKSHARE was in use).
	 */
	uvm_obj_destroy(&vp->v_uobj, true);
	cv_destroy(&vp->v_cv);
	pool_cache_put(vnode_cache, vp);
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

	KASSERT(mutex_owned(&vnode_free_list_lock));
retry:
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
		KASSERT((vp->v_iflag & VI_CLEAN) == 0);
		KASSERT(vp->v_freelisthd == listhd);

		if (!mutex_tryenter(vp->v_interlock))
			continue;
		if ((vp->v_iflag & VI_XLOCK) == 0)
			break;
		mutex_exit(vp->v_interlock);
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
	 * before doing this.  If the vnode gains another reference while
	 * being cleaned out then we lose - retry.
	 */
	atomic_add_int(&vp->v_usecount, 1 + VC_XLOCK);
	vclean(vp, DOCLOSE);
	KASSERT(vp->v_usecount >= 1 + VC_XLOCK);
	atomic_add_int(&vp->v_usecount, -VC_XLOCK);
	if (vp->v_usecount > 1) {
		/*
		 * Don't return to freelist - the holder of the last
		 * reference will destroy it.
		 */
		vrelel(vp, 0); /* releases vp->v_interlock */
		mutex_enter(&vnode_free_list_lock);
		goto retry;
	}

	KASSERT((vp->v_iflag & VI_CLEAN) == VI_CLEAN);
	mutex_exit(vp->v_interlock);
	if (vp->v_type == VBLK || vp->v_type == VCHR) {
		spec_node_destroy(vp);
	}
	vp->v_type = VNON;

	KASSERT(vp->v_data == NULL);
	KASSERT(vp->v_uobj.uo_npages == 0);
	KASSERT(radix_tree_empty_tree_p(&vp->v_uobj.uo_pages));
	KASSERT(vp->v_numoutput == 0);
	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);

	vrele(vp);

	return 0;
}

/*
 * getnewvnode: return a fresh vnode.
 *
 * => Returns referenced vnode, moved into the mount queue.
 * => Shares the interlock specified by 'slock', if it is not NULL.
 */
int
getnewvnode(enum vtagtype tag, struct mount *mp, int (**vops)(void *),
    kmutex_t *slock, vnode_t **vpp)
{
	struct uvm_object *uobj;
	vnode_t *vp;
	int error = 0;

	if (mp != NULL) {
		/*
		 * Mark filesystem busy while we are creating a vnode.
		 * If unmount is in progress, this will fail.
		 */
		error = vfs_busy(mp, NULL);
		if (error)
			return error;
	}

	vp = NULL;

	/* Allocate a new vnode. */
	mutex_enter(&vnode_free_list_lock);
	numvnodes++;
	if (numvnodes > desiredvnodes + desiredvnodes / 10)
		cv_signal(&vdrain_cv);
	mutex_exit(&vnode_free_list_lock);
	vp = vnalloc(NULL);

	KASSERT(vp->v_freelisthd == NULL);
	KASSERT(LIST_EMPTY(&vp->v_nclist));
	KASSERT(LIST_EMPTY(&vp->v_dnclist));

	/* Initialize vnode. */
	vp->v_usecount = 1;
	vp->v_type = VNON;
	vp->v_tag = tag;
	vp->v_op = vops;
	vp->v_data = NULL;

	uobj = &vp->v_uobj;
	KASSERT(uobj->pgops == &uvm_vnodeops);
	KASSERT(uobj->uo_npages == 0);
	KASSERT(radix_tree_empty_tree_p(&vp->v_uobj.uo_pages));
	vp->v_size = vp->v_writesize = VSIZENOTSET;

	/* Share the vnode_t::v_interlock, if requested. */
	if (slock) {
		/* Set the interlock and mark that it is shared. */
		KASSERT(vp->v_mount == NULL);
		mutex_obj_hold(slock);
		uvm_obj_setlock(&vp->v_uobj, slock);
		KASSERT(vp->v_interlock == slock);
		vp->v_iflag |= VI_LOCKSHARE;
	}

	/* Finally, move vnode into the mount queue. */
	vfs_insmntque(vp, mp);

	if (mp != NULL) {
		if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
			vp->v_vflag |= VV_MPSAFE;
		vfs_unbusy(mp, true, NULL);
	}

	*vpp = vp;
	return 0;
}

/*
 * This is really just the reverse of getnewvnode(). Needed for
 * VFS_VGET functions who may need to push back a vnode in case
 * of a locking race.
 */
void
ungetnewvnode(vnode_t *vp)
{

	KASSERT(vp->v_usecount == 1);
	KASSERT(vp->v_data == NULL);
	KASSERT(vp->v_freelisthd == NULL);

	mutex_enter(vp->v_interlock);
	vp->v_iflag |= VI_CLEAN;
	vrelel(vp, 0);
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
 * Try to gain a reference to a vnode, without acquiring its interlock.
 * The caller must hold a lock that will prevent the vnode from being
 * recycled or freed.
 */
bool
vtryget(vnode_t *vp)
{
	u_int use, next;

	/*
	 * If the vnode is being freed, don't make life any harder
	 * for vclean() by adding another reference without waiting.
	 * This is not strictly necessary, but we'll do it anyway.
	 */
	if (__predict_false((vp->v_iflag & VI_XLOCK) != 0)) {
		return false;
	}
	for (use = vp->v_usecount;; use = next) {
		if (use == 0 || __predict_false((use & VC_XLOCK) != 0)) {
			/* Need interlock held if first reference. */
			return false;
		}
		next = atomic_cas_uint(&vp->v_usecount, use, use + 1);
		if (__predict_true(next == use)) {
			return true;
		}
	}
}

/*
 * vget: get a particular vnode from the free list, increment its reference
 * count and lock it.
 *
 * => Should be called with v_interlock held.
 *
 * If VI_XLOCK is set, the vnode is being eliminated in vgone()/vclean().
 * In that case, we cannot grab the vnode, so the process is awakened when
 * the transition is completed, and an error returned to indicate that the
 * vnode is no longer usable (e.g. changed to a new file system type).
 */
int
vget(vnode_t *vp, int flags)
{
	int error = 0;

	KASSERT((vp->v_iflag & VI_MARKER) == 0);
	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT((flags & ~(LK_SHARED|LK_EXCLUSIVE|LK_NOWAIT)) == 0);

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
	 * If the vnode is in the process of being cleaned out for
	 * another use, we wait for the cleaning to finish and then
	 * return failure.  Cleaning is determined by checking if
	 * the VI_XLOCK flag is set.
	 */
	if ((vp->v_iflag & VI_XLOCK) != 0) {
		if ((flags & LK_NOWAIT) != 0) {
			vrelel(vp, 0);
			return EBUSY;
		}
		vwait(vp, VI_XLOCK);
		vrelel(vp, 0);
		return ENOENT;
	}

	if ((vp->v_iflag & VI_INACTNOW) != 0) {
		/*
		 * if it's being desactived, wait for it to complete.
		 * Make sure to not return a clean vnode.
		 */
		 if ((flags & LK_NOWAIT) != 0) {
			vrelel(vp, 0);
			return EBUSY;
		}
		vwait(vp, VI_INACTNOW);
		if ((vp->v_iflag & VI_CLEAN) != 0) {
			vrelel(vp, 0);
			return ENOENT;
		}
	}

	/*
	 * Ok, we got it in good shape.  Just locking left.
	 */
	KASSERT((vp->v_iflag & VI_CLEAN) == 0);
	mutex_exit(vp->v_interlock);
	if (flags & (LK_EXCLUSIVE | LK_SHARED)) {
		error = vn_lock(vp, flags);
		if (error != 0) {
			vrele(vp);
		}
	}
	return error;
}

/*
 * vput: unlock and release the reference.
 */
void
vput(vnode_t *vp)
{

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

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
		KASSERT((use & VC_MASK) > 1);
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
void
vrelel(vnode_t *vp, int flags)
{
	bool recycle, defer;
	int error;

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT((vp->v_iflag & VI_MARKER) == 0);
	KASSERT(vp->v_freelisthd == NULL);

	if (__predict_false(vp->v_op == dead_vnodeop_p &&
	    (vp->v_iflag & (VI_CLEAN|VI_XLOCK)) == 0)) {
		vnpanic(vp, "dead but not clean");
	}

	/*
	 * If not the last reference, just drop the reference count
	 * and unlock.
	 */
	if (vtryrele(vp)) {
		vp->v_iflag |= VI_INACTREDO;
		mutex_exit(vp->v_interlock);
		return;
	}
	if (vp->v_usecount <= 0 || vp->v_writecount != 0) {
		vnpanic(vp, "%s: bad ref count", __func__);
	}

	KASSERT((vp->v_iflag & VI_XLOCK) == 0);

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
retry:
	if ((vp->v_iflag & VI_CLEAN) == 0) {
		recycle = false;
		vp->v_iflag |= VI_INACTNOW;

		/*
		 * XXX This ugly block can be largely eliminated if
		 * locking is pushed down into the file systems.
		 *
		 * Defer vnode release to vrele_thread if caller
		 * requests it explicitly.
		 */
		if ((curlwp == uvm.pagedaemon_lwp) ||
		    (flags & VRELEL_ASYNC_RELE) != 0) {
			/* The pagedaemon can't wait around; defer. */
			defer = true;
		} else if (curlwp == vrele_lwp) {
			/*
			 * We have to try harder. But we can't sleep
			 * with VI_INACTNOW as vget() may be waiting on it.
			 */
			vp->v_iflag &= ~(VI_INACTREDO|VI_INACTNOW);
			cv_broadcast(&vp->v_cv);
			mutex_exit(vp->v_interlock);
			error = vn_lock(vp, LK_EXCLUSIVE);
			if (error != 0) {
				/* XXX */
				vnpanic(vp, "%s: unable to lock %p",
				    __func__, vp);
			}
			mutex_enter(vp->v_interlock);
			/*
			 * if we did get another reference while
			 * sleeping, don't try to inactivate it yet.
			 */
			if (__predict_false(vtryrele(vp))) {
				VOP_UNLOCK(vp);
				mutex_exit(vp->v_interlock);
				return;
			}
			vp->v_iflag |= VI_INACTNOW;
			mutex_exit(vp->v_interlock);
			defer = false;
		} else if ((vp->v_iflag & VI_LAYER) != 0) {
			/* 
			 * Acquiring the stack's lock in vclean() even
			 * for an honest vput/vrele is dangerous because
			 * our caller may hold other vnode locks; defer.
			 */
			defer = true;
		} else {
			/* If we can't acquire the lock, then defer. */
			vp->v_iflag &= ~VI_INACTREDO;
			mutex_exit(vp->v_interlock);
			error = vn_lock(vp, LK_EXCLUSIVE | LK_NOWAIT);
			if (error != 0) {
				defer = true;
				mutex_enter(vp->v_interlock);
			} else {
				defer = false;
			}
		}

		if (defer) {
			/*
			 * Defer reclaim to the kthread; it's not safe to
			 * clean it here.  We donate it our last reference.
			 */
			KASSERT(mutex_owned(vp->v_interlock));
			KASSERT((vp->v_iflag & VI_INACTPEND) == 0);
			vp->v_iflag &= ~VI_INACTNOW;
			vp->v_iflag |= VI_INACTPEND;
			mutex_enter(&vrele_lock);
			TAILQ_INSERT_TAIL(&vrele_list, vp, v_freelist);
			if (++vrele_pending > (desiredvnodes >> 8))
				cv_signal(&vrele_cv); 
			mutex_exit(&vrele_lock);
			cv_broadcast(&vp->v_cv);
			mutex_exit(vp->v_interlock);
			return;
		}

		/*
		 * The vnode can gain another reference while being
		 * deactivated.  If VOP_INACTIVE() indicates that
		 * the described file has been deleted, then recycle
		 * the vnode irrespective of additional references.
		 * Another thread may be waiting to re-use the on-disk
		 * inode.
		 *
		 * Note that VOP_INACTIVE() will drop the vnode lock.
		 */
		VOP_INACTIVE(vp, &recycle);
		mutex_enter(vp->v_interlock);
		vp->v_iflag &= ~VI_INACTNOW;
		cv_broadcast(&vp->v_cv);
		if (!recycle) {
			if (vtryrele(vp)) {
				mutex_exit(vp->v_interlock);
				return;
			}

			/*
			 * If we grew another reference while
			 * VOP_INACTIVE() was underway, retry.
			 */
			if ((vp->v_iflag & VI_INACTREDO) != 0) {
				goto retry;
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
			vclean(vp, DOCLOSE);
		}
		KASSERT(vp->v_usecount > 0);
	}

	if (atomic_dec_uint_nv(&vp->v_usecount) != 0) {
		/* Gained another reference while being reclaimed. */
		mutex_exit(vp->v_interlock);
		return;
	}

	if ((vp->v_iflag & VI_CLEAN) != 0) {
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
		vnfree(vp);
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

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	if ((vp->v_iflag & VI_INACTNOW) == 0 && vtryrele(vp)) {
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

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	if ((vp->v_iflag & VI_INACTNOW) == 0 && vtryrele(vp)) {
		return;
	}
	mutex_enter(vp->v_interlock);
	vrelel(vp, VRELEL_ASYNC_RELE);
}

static void
vrele_thread(void *cookie)
{
	vnode_t *vp;

	for (;;) {
		mutex_enter(&vrele_lock);
		while (TAILQ_EMPTY(&vrele_list)) {
			vrele_gen++;
			cv_broadcast(&vrele_cv);
			cv_timedwait(&vrele_cv, &vrele_lock, hz);
		}
		vp = TAILQ_FIRST(&vrele_list);
		TAILQ_REMOVE(&vrele_list, vp, v_freelist);
		vrele_pending--;
		mutex_exit(&vrele_lock);

		/*
		 * If not the last reference, then ignore the vnode
		 * and look for more work.
		 */
		mutex_enter(vp->v_interlock);
		KASSERT((vp->v_iflag & VI_INACTPEND) != 0);
		vp->v_iflag &= ~VI_INACTPEND;
		vrelel(vp, 0);
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

	KASSERT((vp->v_iflag & VI_MARKER) == 0);
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
	KASSERT((vp->v_iflag & VI_MARKER) == 0);

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
	KASSERT((vp->v_iflag & VI_MARKER) == 0);

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
 * Disassociate the underlying file system from a vnode.
 *
 * Must be called with the interlock held, and will return with it held.
 */
void
vclean(vnode_t *vp, int flags)
{
	lwp_t *l = curlwp;
	bool recycle, active;
	int error;

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT((vp->v_iflag & VI_MARKER) == 0);
	KASSERT(vp->v_usecount != 0);

	/* If cleaning is already in progress wait until done and return. */
	if (vp->v_iflag & VI_XLOCK) {
		vwait(vp, VI_XLOCK);
		return;
	}

	/* If already clean, nothing to do. */
	if ((vp->v_iflag & VI_CLEAN) != 0) {
		return;
	}

	/*
	 * Prevent the vnode from being recycled or brought into use
	 * while we clean it out.
	 */
	vp->v_iflag |= VI_XLOCK;
	if (vp->v_iflag & VI_EXECMAP) {
		atomic_add_int(&uvmexp.execpages, -vp->v_uobj.uo_npages);
		atomic_add_int(&uvmexp.filepages, vp->v_uobj.uo_npages);
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP);
	active = (vp->v_usecount & VC_MASK) > 1;

	/* XXXAD should not lock vnode under layer */
	mutex_exit(vp->v_interlock);
	VOP_LOCK(vp, LK_EXCLUSIVE);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
	 */
	if (flags & DOCLOSE) {
		error = vinvalbuf(vp, V_SAVE, NOCRED, l, 0, 0);
		if (error != 0) {
			/* XXX, fix vn_start_write's grab of mp and use that. */

			if (wapbl_vphaswapbl(vp))
				WAPBL_DISCARD(wapbl_vptomp(vp));
			error = vinvalbuf(vp, 0, NOCRED, l, 0, 0);
		}
		KASSERT(error == 0);
		KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
		if (active && (vp->v_type == VBLK || vp->v_type == VCHR)) {
			 spec_node_revoke(vp);
		}
	}
	if (active) {
		VOP_INACTIVE(vp, &recycle);
	} else {
		/*
		 * Any other processes trying to obtain this lock must first
		 * wait for VI_XLOCK to clear, then call the new lock operation.
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

	/* Done with purge, notify sleepers of the grim news. */
	mutex_enter(vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	KNOTE(&vp->v_klist, NOTE_REVOKE);
	vp->v_iflag &= ~VI_XLOCK;
	vp->v_vflag &= ~VV_LOCKSWORK;
	if ((flags & DOCLOSE) != 0) {
		vp->v_iflag |= VI_CLEAN;
	}
	cv_broadcast(&vp->v_cv);

	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);
}

/*
 * Recycle an unused vnode to the front of the free list.
 * Release the passed interlock if the vnode will be recycled.
 */
int
vrecycle(vnode_t *vp, kmutex_t *inter_lkp, struct lwp *l)
{

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	mutex_enter(vp->v_interlock);
	if (vp->v_usecount != 0) {
		mutex_exit(vp->v_interlock);
		return 0;
	}
	if (inter_lkp) {
		mutex_exit(inter_lkp);
	}
	vremfree(vp);
	vp->v_usecount = 1;
	vclean(vp, DOCLOSE);
	vrelel(vp, 0);
	return 1;
}

/*
 * Eliminate all activity associated with the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
void
vrevoke(vnode_t *vp)
{
	vnode_t *vq, **vpp;
	enum vtype type;
	dev_t dev;

	KASSERT(vp->v_usecount > 0);

	mutex_enter(vp->v_interlock);
	if ((vp->v_iflag & VI_CLEAN) != 0) {
		mutex_exit(vp->v_interlock);
		return;
	} else if (vp->v_type != VBLK && vp->v_type != VCHR) {
		atomic_inc_uint(&vp->v_usecount);
		vclean(vp, DOCLOSE);
		vrelel(vp, 0);
		return;
	} else {
		dev = vp->v_rdev;
		type = vp->v_type;
		mutex_exit(vp->v_interlock);
	}

	vpp = &specfs_hash[SPECHASH(dev)];
	mutex_enter(&device_lock);
	for (vq = *vpp; vq != NULL;) {
		/* If clean or being cleaned, then ignore it. */
		mutex_enter(vq->v_interlock);
		if ((vq->v_iflag & (VI_CLEAN | VI_XLOCK)) != 0 ||
		    vq->v_type != type || vq->v_rdev != dev) {
			mutex_exit(vq->v_interlock);
			vq = vq->v_specnext;
			continue;
		}
		mutex_exit(&device_lock);
		if (vq->v_usecount == 0) {
			vremfree(vq);
			vq->v_usecount = 1;
		} else {
			atomic_inc_uint(&vq->v_usecount);
		}
		vclean(vq, DOCLOSE);
		vrelel(vq, 0);
		mutex_enter(&device_lock);
		vq = *vpp;
	}
	mutex_exit(&device_lock);
}

/*
 * Eliminate all activity associated with a vnode in preparation for
 * reuse.  Drops a reference from the vnode.
 */
void
vgone(vnode_t *vp)
{

	mutex_enter(vp->v_interlock);
	vclean(vp, DOCLOSE);
	vrelel(vp, 0);
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
 * Wait for a vnode (typically with VI_XLOCK set) to be cleaned or
 * recycled.
 */
void
vwait(vnode_t *vp, int flags)
{

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT(vp->v_usecount != 0);

	while ((vp->v_iflag & flags) != 0)
		cv_wait(&vp->v_cv, vp->v_interlock);
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
