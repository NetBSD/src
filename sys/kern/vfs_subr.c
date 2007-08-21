/*	$NetBSD: vfs_subr.c,v 1.283.2.15 2007/08/21 18:05:41 ad Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * External virtual filesystem routines.
 *
 * This file contains vfs subroutines which are heavily dependant on
 * the kernel and are not suitable for standalone use.  Examples include
 * routines involved vnode and mountpoint management.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_subr.c,v 1.283.2.15 2007/08/21 18:05:41 ad Exp $");

#include "opt_inet.h"
#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/syscallargs.h>
#include <sys/device.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/syncfs/syncfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_ddb.h>

#include <sys/sysctl.h>

extern int dovfsusermount;	/* 1 => permit any user to mount filesystems */
extern int vfs_magiclinks;	/* 1 => expand "magic" symlinks */

/* TAILQ_HEAD(freelst, vnode) vnode_free_list =	vnode free list (in vnode.h) */
struct freelst vnode_free_list = TAILQ_HEAD_INITIALIZER(vnode_free_list);
struct freelst vnode_hold_list = TAILQ_HEAD_INITIALIZER(vnode_hold_list);

POOL_INIT(vnode_pool, sizeof(struct vnode), 0, 0, 0, "vnodepl",
    &pool_allocator_nointr, IPL_NONE);

MALLOC_DEFINE(M_VNODE, "vnodes", "Dynamically allocated vnodes");

/*
 * Local declarations.
 */

static void insmntque(struct vnode *, struct mount *);
static int getdevvp(dev_t, struct vnode **, enum vtype);
static void vclean(struct vnode *, int, struct lwp *);
static struct vnode *getcleanvnode(struct lwp *);

int
vfs_drainvnodes(long target, struct lwp *l)
{

	mutex_enter(&vnode_free_list_lock);
	while (numvnodes > target) {
		struct vnode *vp;

		vp = getcleanvnode(l);
		if (vp == NULL)
			return EBUSY; /* give up */
		UVM_OBJ_DESTROY(&vp->v_uobj);
		cv_destroy(&vp->v_cv);
		lockdestroy(&vp->v_lock);
		pool_put(&vnode_pool, vp);
		mutex_enter(&vnode_free_list_lock);
		numvnodes--;
	}
	mutex_exit(&vnode_free_list_lock);

	return 0;
}

/*
 * grab a vnode from freelist and clean it.
 */
struct vnode *
getcleanvnode(struct lwp *l)
{
	struct vnode *vp;
	struct freelst *listhd;

	KASSERT(mutex_owned(&vnode_free_list_lock));

	listhd = &vnode_free_list;
try_nextlist:
	TAILQ_FOREACH(vp, listhd, v_freelist) {
		if (!mutex_tryenter(&vp->v_interlock))
			continue;
		/*
		 * as our lwp might hold the underlying vnode locked,
		 * don't try to reclaim the VLAYER vnode if it's locked.
		 */
		if ((vp->v_iflag & VI_XLOCK) == 0 &&
		    ((vp->v_iflag & VI_LAYER) == 0 || VOP_ISLOCKED(vp) == 0)) {
				break;
		}
		mutex_exit(&vp->v_interlock);
	}

	if (vp == NULLVP) {
		if (listhd == &vnode_free_list) {
			listhd = &vnode_hold_list;
			goto try_nextlist;
		}
		mutex_exit(&vnode_free_list_lock);
		return NULLVP;
	}

	if (vp->v_usecount)
		panic("free vnode isn't, vp %p", vp);
	TAILQ_REMOVE(listhd, vp, v_freelist);
	/* see comment on why 0xdeadb is set at end of vgone (below) */
	vp->v_freelist.tqe_prev = (struct vnode **)0xdeadb;
	mutex_exit(&vnode_free_list_lock);

	if (vp->v_type != VBAD)
		vgonel(vp, l);
	else
		mutex_exit(&vp->v_interlock);
#ifdef DIAGNOSTIC
	if (vp->v_data || vp->v_uobj.uo_npages ||
	    TAILQ_FIRST(&vp->v_uobj.memq))
		panic("cleaned vnode isn't, vp %p", vp);
	if (vp->v_numoutput)
		panic("clean vnode has pending I/O's, vp %p", vp);
#endif
	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);

	return vp;
}

/*
 * Mark a mount point as busy. Used to synchronize access and to delay
 * unmounting. Interlock is not released on failure.
 */
int
vfs_busy(struct mount *mp, int flags, kmutex_t *interlkp)
{
	int lkflags;

	while (mp->mnt_iflag & IMNT_UNMOUNT) {
		int gone, n;

		if (flags & LK_NOWAIT)
			return (ENOENT);
		if ((flags & LK_RECURSEFAIL) && mp->mnt_unmounter != NULL
		    && mp->mnt_unmounter == curlwp)
			return (EDEADLK);
		if (interlkp)
			mutex_exit(interlkp);
		/*
		 * Since all busy locks are shared except the exclusive
		 * lock granted when unmounting, the only place that a
		 * wakeup needs to be done is at the release of the
		 * exclusive lock at the end of dounmount.
		 */
		mutex_enter(&mp->mnt_mutex);
		mp->mnt_wcnt++;
		mtsleep((void *)mp, PVFS, "vfs_busy", 0, &mp->mnt_mutex);
		n = --mp->mnt_wcnt;
		mutex_exit(&mp->mnt_mutex);
		gone = mp->mnt_iflag & IMNT_GONE;

		if (n == 0)
			wakeup(&mp->mnt_wcnt);
		if (interlkp)
			mutex_enter(interlkp);
		if (gone)
			return (ENOENT);
	}
	lkflags = LK_SHARED;
	if (interlkp)
		lkflags |= LK_INTERLOCK;
	if (lockmgr(&mp->mnt_lock, lkflags, interlkp))
		panic("vfs_busy: unexpected lock failure");
	return (0);
}

/*
 * Free a busy filesystem.
 */
void
vfs_unbusy(struct mount *mp)
{

	lockmgr(&mp->mnt_lock, LK_RELEASE, NULL);
}

/*
 * Lookup a filesystem type, and if found allocate and initialize
 * a mount structure for it.
 *
 * Devname is usually updated by mount(8) after booting.
 */
int
vfs_rootmountalloc(const char *fstypename, const char *devname,
    struct mount **mpp)
{
	struct vfsops *vfsp = NULL;
	struct mount *mp;

	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(vfsp, &vfs_list, vfs_list)
		if (!strncmp(vfsp->vfs_name, fstypename, 
		    sizeof(mp->mnt_stat.f_fstypename)))
			break;
	if (vfsp == NULL)
		return (ENODEV);
	vfsp->vfs_refcount++;
	mutex_exit(&vfs_list_lock);

	mp = malloc((u_long)sizeof(struct mount), M_MOUNT, M_WAITOK);
	memset((char *)mp, 0, (u_long)sizeof(struct mount));
	lockinit(&mp->mnt_lock, PVFS, "vfslock", 0, 0);
	mutex_init(&mp->mnt_mutex, MUTEX_DEFAULT, IPL_NONE);
	(void)vfs_busy(mp, LK_NOWAIT, 0);
	TAILQ_INIT(&mp->mnt_vnodelist);
	mp->mnt_op = vfsp;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_vnodecovered = NULLVP;
	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsp->vfs_name,
	    sizeof(mp->mnt_stat.f_fstypename));
	mp->mnt_stat.f_mntonname[0] = '/';
	mp->mnt_stat.f_mntonname[0] = '\0';
	mp->mnt_stat.f_mntfromname[sizeof(mp->mnt_stat.f_mntfromname) - 1] =
	    '\0';
	(void)copystr(devname, mp->mnt_stat.f_mntfromname,
	    sizeof(mp->mnt_stat.f_mntfromname) - 1, 0);
	mount_initspecific(mp);
	*mpp = mp;
	return (0);
}

/*
 * Routines having to do with the management of the vnode table.
 */
extern int (**dead_vnodeop_p)(void *);

/*
 * Return the next vnode from the free list.
 */
int
getnewvnode(enum vtagtype tag, struct mount *mp, int (**vops)(void *),
    struct vnode **vpp)
{
	extern struct uvm_pagerops uvm_vnodeops;
	struct uvm_object *uobj;
	struct lwp *l = curlwp;		/* XXX */
	static int toggle;
	struct vnode *vp;
	int error = 0, tryalloc;

 try_again:
	if (mp) {
		/*
		 * Mark filesystem busy while we're creating a vnode.
		 * If unmount is in progress, this will wait; if the
		 * unmount succeeds (only if umount -f), this will
		 * return an error.  If the unmount fails, we'll keep
		 * going afterwards.
		 * (This puts the per-mount vnode list logically under
		 * the protection of the vfs_busy lock).
		 */
		error = vfs_busy(mp, LK_RECURSEFAIL, 0);
		if (error && error != EDEADLK)
			return error;
	}

	/*
	 * We must choose whether to allocate a new vnode or recycle an
	 * existing one. The criterion for allocating a new one is that
	 * the total number of vnodes is less than the number desired or
	 * there are no vnodes on either free list. Generally we only
	 * want to recycle vnodes that have no buffers associated with
	 * them, so we look first on the vnode_free_list. If it is empty,
	 * we next consider vnodes with referencing buffers on the
	 * vnode_hold_list. The toggle ensures that half the time we
	 * will use a buffer from the vnode_hold_list, and half the time
	 * we will allocate a new one unless the list has grown to twice
	 * the desired size. We are reticent to recycle vnodes from the
	 * vnode_hold_list because we will lose the identity of all its
	 * referencing buffers.
	 */

	vp = NULL;

	mutex_enter(&vnode_free_list_lock);

	toggle ^= 1;
	if (numvnodes > 2 * desiredvnodes)
		toggle = 0;

	tryalloc = numvnodes < desiredvnodes ||
	    (TAILQ_FIRST(&vnode_free_list) == NULL &&
	     (TAILQ_FIRST(&vnode_hold_list) == NULL || toggle));

	if (tryalloc &&
	    (vp = pool_get(&vnode_pool, PR_NOWAIT)) != NULL) {
		numvnodes++;
		mutex_exit(&vnode_free_list_lock);
		memset(vp, 0, sizeof(*vp));
		UVM_OBJ_INIT(&vp->v_uobj, &uvm_vnodeops, 1);
		/*
		 * done by memset() above.
		 *	LIST_INIT(&vp->v_nclist);
		 *	LIST_INIT(&vp->v_dnclist);
		 */
	} else {
		vp = getcleanvnode(l);
		/*
		 * Unless this is a bad time of the month, at most
		 * the first NCPUS items on the free list are
		 * locked, so this is close enough to being empty.
		 */
		if (vp == NULLVP) {
			if (mp && error != EDEADLK)
				vfs_unbusy(mp);
			if (tryalloc) {
				printf("WARNING: unable to allocate new "
				    "vnode, retrying...\n");
				(void) tsleep(&lbolt, PRIBIO, "newvn", hz);
				goto try_again;
			}
			tablefull("vnode", "increase kern.maxvnodes or NVNODE");
			*vpp = 0;
			return (ENFILE);
		}
		vp->v_usecount = 1;
		vp->v_iflag = 0;
		vp->v_vflag = 0;
		vp->v_uflag = 0;
		vp->v_socket = NULL;
	}
	vp->v_type = VNON;
	vp->v_vnlock = &vp->v_lock;
	lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	KASSERT(LIST_EMPTY(&vp->v_nclist));
	KASSERT(LIST_EMPTY(&vp->v_dnclist));
	vp->v_tag = tag;
	vp->v_op = vops;
	insmntque(vp, mp);
	*vpp = vp;
	vp->v_data = 0;
	cv_init(&vp->v_cv, "vnode");

	/*
	 * initialize uvm_object within vnode.
	 */

	uobj = &vp->v_uobj;
	KASSERT(uobj->pgops == &uvm_vnodeops);
	KASSERT(uobj->uo_npages == 0);
	KASSERT(TAILQ_FIRST(&uobj->memq) == NULL);
	vp->v_size = vp->v_writesize = VSIZENOTSET;

	if (mp != NULL) {
		if ((mp->mnt_iflag & IMNT_MPSAFE) != 0)
			vp->v_vflag |= VV_MPSAFE;
		if (error != EDEADLK)
			vfs_unbusy(mp);
	}

	return (0);
}

/*
 * This is really just the reverse of getnewvnode(). Needed for
 * VFS_VGET functions who may need to push back a vnode in case
 * of a locking race.
 */
void
ungetnewvnode(struct vnode *vp)
{
#ifdef DIAGNOSTIC
	if (vp->v_usecount != 1)
		panic("ungetnewvnode: busy vnode");
#endif
	vp->v_usecount--;
	insmntque(vp, NULL);
	vp->v_type = VBAD;

	mutex_enter(&vp->v_interlock);
	/*
	 * Insert at head of LRU list
	 */
	mutex_enter(&vnode_free_list_lock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_HEAD(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_HEAD(&vnode_free_list, vp, v_freelist);
	mutex_exit(&vnode_free_list_lock);
	mutex_exit(&vp->v_interlock);
}

/*
 * Move a vnode from one mount queue to another.
 */
static void
insmntque(struct vnode *vp, struct mount *mp)
{

#ifdef DIAGNOSTIC
	if ((mp != NULL) &&
	    (mp->mnt_iflag & IMNT_UNMOUNT) &&
	    !(mp->mnt_flag & MNT_SOFTDEP) &&
	    vp->v_tag != VT_VFS) {
		panic("insmntque into dying filesystem");
	}
#endif

	mutex_enter(&mntvnode_lock);
	/*
	 * Delete from old mount point vnode list, if on one.
	 */
	if (vp->v_mount != NULL)
		TAILQ_REMOVE(&vp->v_mount->mnt_vnodelist, vp, v_mntvnodes);
	/*
	 * Insert into list of vnodes for the new mount point, if available.
	 */
	if ((vp->v_mount = mp) != NULL)
		TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vp, v_mntvnodes);
	mutex_exit(&mntvnode_lock);
}

/*
 * Create a vnode for a block device.
 * Used for root filesystem and swap areas.
 * Also used for memory file system special devices.
 */
int
bdevvp(dev_t dev, struct vnode **vpp)
{

	return (getdevvp(dev, vpp, VBLK));
}

/*
 * Create a vnode for a character device.
 * Used for kernfs and some console handling.
 */
int
cdevvp(dev_t dev, struct vnode **vpp)
{

	return (getdevvp(dev, vpp, VCHR));
}

/*
 * Create a vnode for a device.
 * Used by bdevvp (block device) for root file system etc.,
 * and by cdevvp (character device) for console and kernfs.
 */
static int
getdevvp(dev_t dev, struct vnode **vpp, enum vtype type)
{
	struct vnode *vp;
	struct vnode *nvp;
	int error;

	if (dev == NODEV) {
		*vpp = NULLVP;
		return (0);
	}
	error = getnewvnode(VT_NON, NULL, spec_vnodeop_p, &nvp);
	if (error) {
		*vpp = NULLVP;
		return (error);
	}
	vp = nvp;
	vp->v_type = type;
	uvm_vnp_setsize(vp, 0);
	if ((nvp = checkalias(vp, dev, NULL)) != 0) {
		vput(vp);
		vp = nvp;
	}
	*vpp = vp;
	return (0);
}

/*
 * Check to see if the new vnode represents a special device
 * for which we already have a vnode (either because of
 * bdevvp() or because of a different vnode representing
 * the same block device). If such an alias exists, deallocate
 * the existing contents and return the aliased vnode. The
 * caller is responsible for filling it with its new contents.
 */
struct vnode *
checkalias(struct vnode *nvp, dev_t nvp_rdev, struct mount *mp)
{
	struct lwp *l = curlwp;		/* XXX */
	struct vnode *vp;
	struct vnode **vpp;

	if (nvp->v_type != VBLK && nvp->v_type != VCHR)
		return (NULLVP);

	vpp = &speclisth[SPECHASH(nvp_rdev)];
loop:
	mutex_enter(&spechash_lock);
	for (vp = *vpp; vp; vp = vp->v_specnext) {
		if (nvp_rdev != vp->v_rdev || nvp->v_type != vp->v_type)
			continue;
		/*
		 * Alias, but not in use, so flush it out.
		 */
		mutex_enter(&vp->v_interlock);
		mutex_exit(&spechash_lock);
		if (vp->v_usecount == 0) {
			vgonel(vp, l);
			goto loop;
		}
		/*
		 * What we're interested to know here is if someone else has
		 * removed this vnode from the device hash list while we were
		 * waiting.  This can only happen if vclean() did it, and
		 * this requires the vnode to be locked.
		 */
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
			goto loop;
		if (vp->v_specinfo == NULL) {
			vput(vp);
			goto loop;
		}
		mutex_enter(&spechash_lock);
		break;
	}
	if (vp == NULL || vp->v_tag != VT_NON || vp->v_type != VBLK) {
		MALLOC(nvp->v_specinfo, struct specinfo *,
			sizeof(struct specinfo), M_VNODE, M_NOWAIT);
		/* XXX Erg. */
		if (nvp->v_specinfo == NULL) {
			mutex_exit(&spechash_lock);
			uvm_wait("checkalias");
			goto loop;
		}

		nvp->v_rdev = nvp_rdev;
		nvp->v_hashchain = vpp;
		nvp->v_specnext = *vpp;
		nvp->v_specmountpoint = NULL;
		mutex_exit(&spechash_lock);
		nvp->v_speclockf = NULL;
		mutex_init(&nvp->v_spec_cow_lock, MUTEX_DEFAULT, IPL_NONE);
		SLIST_INIT(&nvp->v_spec_cow_head);
		nvp->v_spec_cow_req = 0;
		nvp->v_spec_cow_count = 0;

		*vpp = nvp;
		if (vp != NULLVP) {
			nvp->v_iflag |= VI_ALIASED;
			vp->v_iflag |= VI_ALIASED;
			vput(vp);
		}
		return (NULLVP);
	}
	mutex_exit(&spechash_lock);
	VOP_UNLOCK(vp, 0);
	mutex_enter(&vp->v_interlock);
	vclean(vp, 0, l);
	vp->v_op = nvp->v_op;
	vp->v_tag = nvp->v_tag;
	vp->v_vnlock = &vp->v_lock;
	lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	nvp->v_type = VNON;
	insmntque(vp, mp);
	return (vp);
}

/*
 * Wait for a vnode (typically with VI_XLOCK set) to be cleaned or
 * recycled.
 */
void
vwait(struct vnode *vp, int flags)
{

	KASSERT(mutex_owned(&vp->v_interlock));

	vp->v_waitcnt++;
	while ((vp->v_iflag & flags) != 0)
		cv_wait(&vp->v_cv, &vp->v_interlock);
	vp->v_waitcnt--;

	/*
	 * Notify the locker (in vunwait()) that we are about
	 * to exit the interlock.
	 */
	cv_broadcast(&vp->v_cv);
}

/*
 * Wake all waiters in vwait, and wait for them to continue.
 */
void
vunwait(struct vnode *vp, int flags)
{

	KASSERT(mutex_owned(&vp->v_interlock));

	/*
	 * Clear the condition (typically VI_XLOCK) and notify
	 * any waiters in vwait().
	 */
	vp->v_iflag &= ~flags;
	while (vp->v_waitcnt != 0) {
		cv_broadcast(&vp->v_cv);
		cv_wait(&vp->v_cv, &vp->v_interlock);
	}
}

/*
 * Grab a particular vnode from the free list, increment its
 * reference count and lock it. If the vnode lock bit is set the
 * vnode is being eliminated in vgone. In that case, we can not
 * grab the vnode, so the process is awakened when the transition is
 * completed, and an error returned to indicate that the vnode is no
 * longer usable (possibly having been changed to a new file system type).
 */
int
vget(struct vnode *vp, int flags)
{
	int error;

	/*
	 * If the vnode is in the process of being cleaned out for
	 * another use, we wait for the cleaning to finish and then
	 * return failure. Cleaning is determined by checking that
	 * the VI_XLOCK flag is set.
	 */

	if ((flags & LK_INTERLOCK) == 0)
		mutex_enter(&vp->v_interlock);
	if ((vp->v_iflag & (VI_XLOCK | VI_FREEING)) != 0) {
		if (flags & LK_NOWAIT) {
			mutex_exit(&vp->v_interlock);
			return EBUSY;
		}
		vwait(vp, VI_XLOCK | VI_FREEING);
		mutex_exit(&vp->v_interlock);
		return (ENOENT);
	}
	if (vp->v_usecount == 0) {
		mutex_enter(&vnode_free_list_lock);
		if (vp->v_holdcnt > 0)
			TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
		else
			TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
	}
	vp->v_usecount++;
#ifdef DIAGNOSTIC
	if (vp->v_usecount == 0) {
		vprint("vget", vp);
		panic("vget: usecount overflow, vp %p", vp);
	}
#endif
	if (flags & LK_TYPE_MASK) {
		if ((error = vn_lock(vp, flags | LK_INTERLOCK))) {
			vrele(vp);
		}
		return (error);
	}
	mutex_exit(&vp->v_interlock);
	return (0);
}

/*
 * vput(), just unlock and vrele()
 */
void
vput(struct vnode *vp)
{
	struct lwp *l = curlwp;

#ifdef DIAGNOSTIC
	if (vp == NULL)
		panic("vput: null vp");
#endif
	mutex_enter(&vp->v_interlock);
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		mutex_exit(&vp->v_interlock);
		VOP_UNLOCK(vp, 0);
		return;
	}
#ifdef DIAGNOSTIC
	if (vp->v_usecount < 0 || vp->v_writecount != 0) {
		vprint("vput: bad ref count", vp);
		panic("vput: ref cnt");
	}
#endif
	/*
	 * Insert at tail of LRU list.
	 */
	mutex_enter(&vnode_free_list_lock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	mutex_exit(&vnode_free_list_lock);
	if (vp->v_iflag & VI_EXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP|VI_WRMAP|VI_MAPPED);
	vp->v_vflag &= ~VV_MAPPED;
	mutex_exit(&vp->v_interlock);
	VOP_INACTIVE(vp, l);
}

/*
 * Vnode release.
 * If count drops to zero, call inactive routine and return to freelist.
 */
static void
do_vrele(struct vnode *vp, int doinactive, int onhead)
{
	struct lwp *l = curlwp;

#ifdef DIAGNOSTIC
	if (vp == NULL)
		panic("vrele: null vp");
#endif
	mutex_enter(&vp->v_interlock);
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		mutex_exit(&vp->v_interlock);
		return;
	}
#ifdef DIAGNOSTIC
	if (vp->v_usecount < 0 || vp->v_writecount != 0) {
		vprint("vrele: bad ref count", vp);
		panic("vrele: ref cnt vp %p", vp);
	}
#endif
	/*
	 * Insert at tail of LRU list.
	 */
	mutex_enter(&vnode_free_list_lock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	mutex_exit(&vnode_free_list_lock);
	if (vp->v_iflag & VI_EXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP|VI_WRMAP|VI_MAPPED);
	if (vn_lock(vp, LK_EXCLUSIVE | LK_INTERLOCK) == 0) {
		vp->v_vflag &= ~VV_MAPPED; /* XXX if locking fails? */
		VOP_INACTIVE(vp, l);
	}
}

void
vrele(struct vnode *vp)
{

	do_vrele(vp, 1, 0);
}

void
vrele2(struct vnode *vp, int onhead)
{

	do_vrele(vp, 0, onhead);
}

/*
 * Page or buffer structure gets a reference.
 * Called with v_interlock held.
 */
void
vholdl(struct vnode *vp)
{

	/*
	 * If it is on the freelist and the hold count is currently
	 * zero, move it to the hold list. The test of the back
	 * pointer and the use reference count of zero is because
	 * it will be removed from a free list by getnewvnode,
	 * but will not have its reference count incremented until
	 * after calling vgone. If the reference count were
	 * incremented first, vgone would (incorrectly) try to
	 * close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from a freelist to ensure
	 * that we do not try to move it here.
	 */
	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		mutex_enter(&vnode_free_list_lock);
		TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
	}
	vp->v_holdcnt++;
}

/*
 * Page or buffer structure frees a reference.
 * Called with v_interlock held.
 */
void
holdrelel(struct vnode *vp)
{

	if (vp->v_holdcnt <= 0)
		panic("holdrelel: holdcnt vp %p", vp);
	vp->v_holdcnt--;

	/*
	 * If it is on the holdlist and the hold count drops to
	 * zero, move it to the free list. The test of the back
	 * pointer and the use reference count of zero is because
	 * it will be removed from a free list by getnewvnode,
	 * but will not have its reference count incremented until
	 * after calling vgone. If the reference count were
	 * incremented first, vgone would (incorrectly) try to
	 * close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from a freelist to ensure
	 * that we do not try to move it here.
	 */

	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		mutex_enter(&vnode_free_list_lock);
		TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
		mutex_exit(&vnode_free_list_lock);
	}
}

/*
 * Vnode reference.
 */
void
vref(struct vnode *vp)
{

	mutex_enter(&vp->v_interlock);
	if (vp->v_usecount <= 0)
		panic("vref used where vget required, vp %p", vp);
	vp->v_usecount++;
#ifdef DIAGNOSTIC
	if (vp->v_usecount == 0) {
		vprint("vref", vp);
		panic("vref: usecount overflow, vp %p", vp);
	}
#endif
	mutex_exit(&vp->v_interlock);
}

/*
 * Remove any vnodes in the vnode table belonging to mount point mp.
 *
 * If FORCECLOSE is not specified, there should not be any active ones,
 * return error if any are found (nb: this is a user error, not a
 * system error). If FORCECLOSE is specified, detach any active vnodes
 * that are found.
 *
 * If WRITECLOSE is set, only flush out regular file vnodes open for
 * writing.
 *
 * SKIPSYSTEM causes any vnodes marked V_SYSTEM to be skipped.
 */
#ifdef DEBUG
int busyprt = 0;	/* print out busy vnodes */
struct ctldebug debug1 = { "busyprt", &busyprt };
#endif

int
vflush(struct mount *mp, struct vnode *skipvp, int flags)
{
	struct lwp *l = curlwp;		/* XXX */
	struct vnode *vp, *nvp;
	int busy = 0;

	mutex_enter(&mntvnode_lock);
loop:
	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() are called
	 */
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		if (vp->v_mount != mp)
			goto loop;
		nvp = TAILQ_NEXT(vp, v_mntvnodes);
		/*
		 * Skip over a selected vnode.
		 */
		if (vp == skipvp)
			continue;
		mutex_enter(&vp->v_interlock);
		/*
		 * Skip over a vnodes marked VSYSTEM.
		 */
		if ((flags & SKIPSYSTEM) && (vp->v_vflag & VV_SYSTEM)) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		/*
		 * If WRITECLOSE is set, only flush out regular file
		 * vnodes open for writing.
		 */
		if ((flags & WRITECLOSE) &&
		    (vp->v_writecount == 0 || vp->v_type != VREG)) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		/*
		 * With v_usecount == 0, all we need to do is clear
		 * out the vnode data structures and we are done.
		 */
		if (vp->v_usecount == 0) {
			mutex_exit(&mntvnode_lock);
			vgonel(vp, l);
			mutex_enter(&mntvnode_lock);
			continue;
		}
		/*
		 * If FORCECLOSE is set, forcibly close the vnode.
		 * For block or character devices, revert to an
		 * anonymous device. For all other files, just kill them.
		 */
		if (flags & FORCECLOSE) {
			mutex_exit(&mntvnode_lock);
			if (vp->v_type != VBLK && vp->v_type != VCHR) {
				vgonel(vp, l);
			} else {
				vclean(vp, 0, l);
				vp->v_op = spec_vnodeop_p;
				insmntque(vp, (struct mount *)0);
			}
			mutex_enter(&mntvnode_lock);
			continue;
		}
#ifdef DEBUG
		if (busyprt)
			vprint("vflush: busy vnode", vp);
#endif
		mutex_exit(&vp->v_interlock);
		busy++;
	}
	mutex_exit(&mntvnode_lock);
	if (busy)
		return (EBUSY);
	return (0);
}

/*
 * Disassociate the underlying file system from a vnode.
 */
static void
vclean(struct vnode *vp, int flags, struct lwp *l)
{
	int active;

	KASSERT(mutex_owned(&vp->v_interlock));

	/*
	 * Check to see if the vnode is in use.
	 * If so we have to reference it before we clean it out
	 * so that its count cannot fall to zero and generate a
	 * race against ourselves to recycle it.
	 */

	if ((active = vp->v_usecount) != 0) {
		vp->v_usecount++;
#ifdef DIAGNOSTIC
		if (vp->v_usecount == 0) {
			vprint("vclean", vp);
			panic("vclean: usecount overflow");
		}
#endif
	}

	/*
	 * Prevent the vnode from being recycled or
	 * brought into use while we clean it out.
	 */
	if (vp->v_iflag & VI_XLOCK)
		panic("vclean: deadlock, vp %p", vp);
	vp->v_iflag |= VI_XLOCK;
	if (vp->v_iflag & VI_EXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP);

	/*
	 * Even if the count is zero, the VOP_INACTIVE routine may still
	 * have the object locked while it cleans it out.  For
	 * active vnodes, it ensures that no other activity can
	 * occur while the underlying object is being cleaned out.
	 *
	 * We drain the lock to make sure we are the last one trying to
	 * get it and immediately resurrect the lock.  Future accesses
	 * for locking this _vnode_ will be protected by VXLOCK.  However,
	 * upper layers might be using the _lock_ in case the file system
	 * exported it and might access it while the vnode lingers in
	 * deadfs.
	 */
	VOP_LOCK(vp, LK_DRAIN | LK_RESURRECT | LK_INTERLOCK);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If special device, remove it from special device alias list.
	 * if it is on one.
	 */
	if (flags & DOCLOSE) {
		int error;
		struct vnode *vq, *vx;

		error = vinvalbuf(vp, V_SAVE, NOCRED, l, 0, 0);
		if (error)
			error = vinvalbuf(vp, 0, NOCRED, l, 0, 0);
		KASSERT(error == 0);
		KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);

		if (active)
			VOP_CLOSE(vp, FNONBLOCK, NOCRED, NULL);

		if ((vp->v_type == VBLK || vp->v_type == VCHR) &&
		    vp->v_specinfo != 0) {
			mutex_enter(&spechash_lock);
			if (vp->v_hashchain != NULL) {
				if (*vp->v_hashchain == vp) {
					*vp->v_hashchain = vp->v_specnext;
				} else {
					for (vq = *vp->v_hashchain; vq;
					     vq = vq->v_specnext) {
						if (vq->v_specnext != vp)
							continue;
						vq->v_specnext = vp->v_specnext;
						break;
					}
					if (vq == NULL)
						panic("missing bdev");
				}
				if (vp->v_iflag & VI_ALIASED) {
					vx = NULL;
						for (vq = *vp->v_hashchain; vq;
						     vq = vq->v_specnext) {
						if (vq->v_rdev != vp->v_rdev ||
						    vq->v_type != vp->v_type)
							continue;
						if (vx)
							break;
						vx = vq;
					}
					if (vx == NULL)
						panic("missing alias");
					if (vq == NULL)
						vx->v_iflag &= ~VI_ALIASED;
					vp->v_iflag &= ~VI_ALIASED;
				}
			}
			mutex_exit(&spechash_lock);
			mutex_destroy(&vp->v_spec_cow_lock);
			FREE(vp->v_specinfo, M_VNODE);
			vp->v_specinfo = NULL;
		}
	}

	/*
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
	 */
	if (active) {
		VOP_INACTIVE(vp, l);
	} else {
		/*
		 * Any other processes trying to obtain this lock must first
		 * wait for VI_XLOCK to clear, then call the new lock operation.
		 */
		VOP_UNLOCK(vp, 0);
	}
	/*
	 * Reclaim the vnode.
	 */
	if (VOP_RECLAIM(vp, l))
		panic("vclean: cannot reclaim, vp %p", vp);
	if (active) {
		/*
		 * Inline copy of vrele() since VOP_INACTIVE
		 * has already been called.
		 */
		mutex_enter(&vp->v_interlock);
		if (--vp->v_usecount <= 0) {
#ifdef DIAGNOSTIC
			if (vp->v_usecount < 0 || vp->v_writecount != 0) {
				vprint("vclean: bad ref count", vp);
				panic("vclean: ref cnt");
			}
#endif
			/*
			 * Insert at tail of LRU list.
			 */

			mutex_exit(&vp->v_interlock);
			mutex_enter(&vnode_free_list_lock);
#ifdef DIAGNOSTIC
			if (vp->v_holdcnt > 0)
				panic("vclean: not clean, vp %p", vp);
#endif
			TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
			mutex_exit(&vnode_free_list_lock);
		} else
			mutex_exit(&vp->v_interlock);
	}

	KASSERT(vp->v_uobj.uo_npages == 0);
	if (vp->v_type == VREG && vp->v_ractx != NULL) {
		uvm_ra_freectx(vp->v_ractx);
		vp->v_ractx = NULL;
	}
	cache_purge(vp);

	/*
	 * Done with purge, notify sleepers of the grim news, and wait
	 * for them to drain.
	 */
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	mutex_enter(&vp->v_interlock);
	vp->v_vnlock = NULL;
	VN_KNOTE(vp, NOTE_REVOKE);	/* FreeBSD has this in vn_pollgone() */
	vunwait(vp, VI_XLOCK|VI_LOCKSWORK|VI_FREEING);
	mutex_exit(&vp->v_interlock);
}

/*
 * Recycle an unused vnode to the front of the free list.
 * Release the passed interlock if the vnode will be recycled.
 */
int
vrecycle(struct vnode *vp, kmutex_t *inter_lkp, struct lwp *l)
{

	mutex_enter(&vp->v_interlock);
	if (vp->v_usecount == 0) {
		if (inter_lkp)
			mutex_exit(inter_lkp);
		vgonel(vp, l);
		return (1);
	}
	mutex_exit(&vp->v_interlock);
	return (0);
}

/*
 * Eliminate all activity associated with a vnode
 * in preparation for reuse.
 */
void
vgone(struct vnode *vp)
{
	struct lwp *l = curlwp;		/* XXX */

	mutex_enter(&vp->v_interlock);
	vgonel(vp, l);
}

/*
 * vgone, with the vp interlock held.
 */
void
vgonel(struct vnode *vp, struct lwp *l)
{

	KASSERT(mutex_owned(&vp->v_interlock));

	/*
	 * If a vgone (or vclean) is already in progress,
	 * wait until it is done and return.
	 */

	if (vp->v_iflag & VI_XLOCK) {
		vwait(vp, VI_XLOCK);
		mutex_exit(&vp->v_interlock);
		return;
	}

	/*
	 * Clean out the filesystem specific data.
	 */

	vclean(vp, DOCLOSE, l);
	KASSERT((vp->v_iflag & VI_ONWORKLST) == 0);

	/*
	 * Delete from old mount point vnode list, if on one.
	 */

	if (vp->v_mount != NULL)
		insmntque(vp, (struct mount *)0);

	/*
	 * The test of the back pointer and the reference count of
	 * zero is because it will be removed from the free list by
	 * getcleanvnode, but will not have its reference count
	 * incremented until after calling vgone. If the reference
	 * count were incremented first, vgone would (incorrectly)
	 * try to close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from the freelist to ensure
	 * that we do not try to move it here.
	 */

	vp->v_type = VBAD;
	if (vp->v_usecount == 0) {
		bool dofree;

		mutex_enter(&vnode_free_list_lock);
		if (vp->v_holdcnt > 0)
			panic("vgonel: not clean, vp %p", vp);
		/*
		 * if it isn't on the freelist, we're called by getcleanvnode
		 * and vnode is being re-used.  otherwise, we'll free it.
		 */
		dofree = vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb;
		if (dofree) {
			TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
			numvnodes--;
		}
		mutex_exit(&vnode_free_list_lock);
		if (dofree) {
			UVM_OBJ_DESTROY(&vp->v_uobj);
			cv_destroy(&vp->v_cv);
			lockdestroy(&vp->v_lock);
			pool_put(&vnode_pool, vp);
		}
	}
}

/*
 * Lookup a vnode by device number.
 */
int
vfinddev(dev_t dev, enum vtype type, struct vnode **vpp)
{
	struct vnode *vp;
	int rc = 0;

	mutex_enter(&spechash_lock);
	for (vp = speclisth[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (dev != vp->v_rdev || type != vp->v_type)
			continue;
		*vpp = vp;
		rc = 1;
		break;
	}
	mutex_exit(&spechash_lock);
	return (rc);
}

/*
 * Revoke all the vnodes corresponding to the specified minor number
 * range (endpoints inclusive) of the specified major.
 */
void
vdevgone(int maj, int minl, int minh, enum vtype type)
{
	struct vnode *vp;
	int mn;

	vp = NULL;	/* XXX gcc */

	for (mn = minl; mn <= minh; mn++)
		if (vfinddev(makedev(maj, mn), type, &vp))
			VOP_REVOKE(vp, REVOKEALL);
}

/*
 * Calculate the total number of references to a special device.
 */
int
vcount(struct vnode *vp)
{
	struct vnode *vq, *vnext;
	int count;

loop:
	mutex_enter(&spechash_lock);
	if ((vp->v_iflag & VI_ALIASED) == 0) {
		mutex_exit(&spechash_lock);
		return (vp->v_usecount);
	}
	for (count = 0, vq = *vp->v_hashchain; vq; vq = vnext) {
		vnext = vq->v_specnext;
		if (vq->v_rdev != vp->v_rdev || vq->v_type != vp->v_type)
			continue;
		/*
		 * Alias, but not in use, so flush it out.
		 */
		if (vq->v_usecount == 0 && vq != vp &&
		    (vq->v_iflag & VI_XLOCK) == 0) {
			mutex_exit(&spechash_lock);
			vgone(vq);
			goto loop;
		}
		count += vq->v_usecount;
	}
	mutex_exit(&spechash_lock);
	return (count);
}

/*
 * sysctl helper routine to return list of supported fstypes
 */
static int
sysctl_vfs_generic_fstypes(SYSCTLFN_ARGS)
{
	char bf[sizeof(((struct statvfs *)NULL)->f_fstypename)];
	char *where = oldp;
	struct vfsops *v;
	size_t needed, left, slen;
	int error, first;

	if (newp != NULL)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	first = 1;
	error = 0;
	needed = 0;
	left = *oldlenp;

	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (where == NULL)
			needed += strlen(v->vfs_name) + 1;
		else {
			memset(bf, 0, sizeof(bf));
			if (first) {
				strncpy(bf, v->vfs_name, sizeof(bf));
				first = 0;
			} else {
				bf[0] = ' ';
				strncpy(bf + 1, v->vfs_name, sizeof(bf) - 1);
			}
			bf[sizeof(bf)-1] = '\0';
			slen = strlen(bf);
			if (left < slen + 1)
				break;
			/* +1 to copy out the trailing NUL byte */
			v->vfs_refcount++;
			mutex_exit(&vfs_list_lock);
			error = copyout(bf, where, slen + 1);
			mutex_enter(&vfs_list_lock);
			v->vfs_refcount--;
			if (error)
				break;
			where += slen;
			needed += slen;
			left -= slen;
		}
	}
	mutex_exit(&vfs_list_lock);
	*oldlenp = needed;
	return (error);
}

/*
 * Top level filesystem related information gathering.
 */
SYSCTL_SETUP(sysctl_vfs_setup, "sysctl vfs subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "generic",
		       SYSCTL_DESCR("Non-specific vfs related information"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, VFS_GENERIC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usermount",
		       SYSCTL_DESCR("Whether unprivileged users may mount "
				    "filesystems"),
		       NULL, 0, &dovfsusermount, 0,
		       CTL_VFS, VFS_GENERIC, VFS_USERMOUNT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "fstypes",
		       SYSCTL_DESCR("List of file systems present"),
		       sysctl_vfs_generic_fstypes, 0, NULL, 0,
		       CTL_VFS, VFS_GENERIC, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "magiclinks",
		       SYSCTL_DESCR("Whether \"magic\" symlinks are expanded"),
		       NULL, 0, &vfs_magiclinks, 0,
		       CTL_VFS, VFS_GENERIC, VFS_MAGICLINKS, CTL_EOL);
}


int kinfo_vdebug = 1;
int kinfo_vgetfailed;
#define KINFO_VNODESLOP	10
/*
 * Dump vnode list (via sysctl).
 * Copyout address of vnode followed by vnode.
 */
/* ARGSUSED */
int
sysctl_kern_vnode(SYSCTLFN_ARGS)
{
	char *where = oldp;
	size_t *sizep = oldlenp;
	struct mount *mp, *nmp;
	struct vnode *vp;
	char *bp = where, *savebp;
	char *ewhere;
	int error;

	if (namelen != 0)
		return (EOPNOTSUPP);
	if (newp != NULL)
		return (EPERM);

#define VPTRSZ	sizeof(struct vnode *)
#define VNODESZ	sizeof(struct vnode)
	if (where == NULL) {
		*sizep = (numvnodes + KINFO_VNODESLOP) * (VPTRSZ + VNODESZ);
		return (0);
	}
	ewhere = where + *sizep;

	mutex_enter(&mountlist_lock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_lock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		savebp = bp;
again:
		mutex_enter(&mntvnode_lock);
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			/*
			 * Check that the vp is still associated with
			 * this filesystem.  RACE: could have been
			 * recycled onto the same filesystem.
			 */
			if (vp->v_mount != mp) {
				mutex_exit(&mntvnode_lock);
				if (kinfo_vdebug)
					printf("kinfo: vp changed\n");
				bp = savebp;
				goto again;
			}
			if (bp + VPTRSZ + VNODESZ > ewhere) {
				mutex_exit(&mntvnode_lock);
				*sizep = bp - where;
				return (ENOMEM);
			}
			mutex_exit(&mntvnode_lock);
			if ((error = copyout((void *)&vp, bp, VPTRSZ)) ||
			   (error = copyout((void *)vp, bp + VPTRSZ, VNODESZ)))
				return (error);
			bp += VPTRSZ + VNODESZ;
			mutex_enter(&mntvnode_lock);
		}
		mutex_exit(&mntvnode_lock);
		mutex_enter(&mountlist_lock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	mutex_exit(&mountlist_lock);

	*sizep = bp - where;
	return (0);
}

/*
 * Check to see if a filesystem is mounted on a block device.
 */
int
vfs_mountedon(struct vnode *vp)
{
	struct vnode *vq;
	int error = 0;

	if (vp->v_type != VBLK)
		return ENOTBLK;
	if (vp->v_specmountpoint != NULL)
		return (EBUSY);
	if (vp->v_iflag & VI_ALIASED) {
		mutex_enter(&spechash_lock);
		for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
			if (vq->v_rdev != vp->v_rdev ||
			    vq->v_type != vp->v_type)
				continue;
			if (vq->v_specmountpoint != NULL) {
				error = EBUSY;
				break;
			}
		}
		mutex_exit(&spechash_lock);
	}
	return (error);
}

/*
 * Unmount all file systems.
 * We traverse the list in reverse order under the assumption that doing so
 * will avoid needing to worry about dependencies.
 */
void
vfs_unmountall(struct lwp *l)
{
	struct mount *mp, *nmp;
	int allerror, error;

	printf("unmounting file systems...");
	for (allerror = 0,
	     mp = mountlist.cqh_last; mp != (void *)&mountlist; mp = nmp) {
		nmp = mp->mnt_list.cqe_prev;
#ifdef DEBUG
		printf("\nunmounting %s (%s)...",
		    mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
#endif
		/*
		 * XXX Freeze syncer.  Must do this before locking the
		 * mount point.  See dounmount() for details.
		 */
		mutex_enter(&syncer_mutex);
		if (vfs_busy(mp, 0, 0)) {
			mutex_exit(&syncer_mutex);
			continue;
		}
		if ((error = dounmount(mp, MNT_FORCE, l)) != 0) {
			printf("unmount of %s failed with error %d\n",
			    mp->mnt_stat.f_mntonname, error);
			allerror = 1;
		}
	}
	printf(" done\n");
	if (allerror)
		printf("WARNING: some file systems would not unmount\n");
}

/*
 * Sync and unmount file systems before shutting down.
 */
void
vfs_shutdown(void)
{
	struct lwp *l;

	/* XXX we're certainly not running in lwp0's context! */
	l = curlwp;
	if (l == NULL)
		l = &lwp0;

	printf("syncing disks... ");

	/* remove user process from run queue */
	suspendsched();
	(void) spl0();

	/* avoid coming back this way again if we panic. */
	doing_shutdown = 1;

	sys_sync(l, NULL, NULL);

	/* Wait for sync to finish. */
	if (buf_syncwait() != 0) {
#if defined(DDB) && defined(DEBUG_HALT_BUSY)
		Debugger();
#endif
		printf("giving up\n");
		return;
	} else
		printf("done\n");

	/*
	 * If we've panic'd, don't make the situation potentially
	 * worse by unmounting the file systems.
	 */
	if (panicstr != NULL)
		return;

	/* Release inodes held by texts before update. */
#ifdef notdef
	vnshutdown();
#endif
	/* Unmount file systems. */
	vfs_unmountall(l);
}

/*
 * Mount the root file system.  If the operator didn't specify a
 * file system to use, try all possible file systems until one
 * succeeds.
 */
int
vfs_mountroot(void)
{
	struct vfsops *v;
	int error = ENODEV;

	if (root_device == NULL)
		panic("vfs_mountroot: root device unknown");

	switch (device_class(root_device)) {
	case DV_IFNET:
		if (rootdev != NODEV)
			panic("vfs_mountroot: rootdev set for DV_IFNET "
			    "(0x%08x -> %d,%d)", rootdev,
			    major(rootdev), minor(rootdev));
		break;

	case DV_DISK:
		if (rootdev == NODEV)
			panic("vfs_mountroot: rootdev not set for DV_DISK");
	        if (bdevvp(rootdev, &rootvp))
	                panic("vfs_mountroot: can't get vnode for rootdev");
		error = VOP_OPEN(rootvp, FREAD, FSCRED, curlwp);
		if (error) {
			printf("vfs_mountroot: can't open root device\n");
			return (error);
		}
		break;

	default:
		printf("%s: inappropriate for root file system\n",
		    root_device->dv_xname);
		return (ENODEV);
	}

	/*
	 * If user specified a file system, use it.
	 */
	if (mountroot != NULL) {
		error = (*mountroot)();
		goto done;
	}

	/*
	 * Try each file system currently configured into the kernel.
	 */
	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (v->vfs_mountroot == NULL)
			continue;
#ifdef DEBUG
		aprint_normal("mountroot: trying %s...\n", v->vfs_name);
#endif
		v->vfs_refcount++;
		mutex_exit(&vfs_list_lock);
		error = (*v->vfs_mountroot)();
		mutex_enter(&vfs_list_lock);
		v->vfs_refcount--;
		if (!error) {
			aprint_normal("root file system type: %s\n",
			    v->vfs_name);
			break;
		}
	}
	mutex_exit(&vfs_list_lock);

	if (v == NULL) {
		printf("no file system for %s", root_device->dv_xname);
		if (device_class(root_device) == DV_DISK)
			printf(" (dev 0x%x)", rootdev);
		printf("\n");
		error = EFTYPE;
	}

done:
	if (error && device_class(root_device) == DV_DISK) {
		VOP_CLOSE(rootvp, FREAD, FSCRED, curlwp);
		vrele(rootvp);
	}
	return (error);
}
