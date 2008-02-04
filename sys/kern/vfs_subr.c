/*	$NetBSD: vfs_subr.c,v 1.250.2.9 2008/02/04 09:24:23 yamt Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005, 2007, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vfs_subr.c,v 1.250.2.9 2008/02/04 09:24:23 yamt Exp $");

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
#include <sys/atomic.h>
#include <sys/kthread.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/syncfs/syncfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_ddb.h>

#include <sys/sysctl.h>

extern int dovfsusermount;	/* 1 => permit any user to mount filesystems */
extern int vfs_magiclinks;	/* 1 => expand "magic" symlinks */

static vnodelst_t vnode_free_list = TAILQ_HEAD_INITIALIZER(vnode_free_list);
static vnodelst_t vnode_hold_list = TAILQ_HEAD_INITIALIZER(vnode_hold_list);
static vnodelst_t vrele_list = TAILQ_HEAD_INITIALIZER(vrele_list);

static int vrele_pending;
static kmutex_t	vrele_lock;
static kcondvar_t vrele_cv;
static lwp_t *vrele_lwp;

static pool_cache_t vnode_cache;

MALLOC_DEFINE(M_VNODE, "vnodes", "Dynamically allocated vnodes");

/*
 * Local declarations.
 */

static void vrele_thread(void *);
static void insmntque(vnode_t *, struct mount *);
static int getdevvp(dev_t, vnode_t **, enum vtype);
static vnode_t *getcleanvnode(void);;
void vpanic(vnode_t *, const char *);

#ifdef DIAGNOSTIC
void
vpanic(vnode_t *vp, const char *msg)
{

	vprint(NULL, vp);
	panic("%s\n", msg);
}
#else
#define	vpanic(vp, msg)	/* nothing */
#endif

void
vn_init1(void)
{

	vnode_cache = pool_cache_init(sizeof(struct vnode), 0, 0, 0, "vnodepl",
	    NULL, IPL_NONE, NULL, NULL, NULL);
	KASSERT(vnode_cache != NULL);

	/* Create deferred release thread. */
	mutex_init(&vrele_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&vrele_cv, "vrele");
	if (kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, vrele_thread,
	    NULL, &vrele_lwp, "vrele"))
		panic("fork vrele");
}

int
vfs_drainvnodes(long target, struct lwp *l)
{

	while (numvnodes > target) {
		vnode_t *vp;

		mutex_enter(&vnode_free_list_lock);
		vp = getcleanvnode();
		if (vp == NULL)
			return EBUSY; /* give up */
		ungetnewvnode(vp);
	}

	return 0;
}

/*
 * grab a vnode from freelist and clean it.
 */
vnode_t *
getcleanvnode(void)
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
		if (vp->v_usecount != 0) {
			vpanic(vp, "free vnode isn't");
		}
		if ((vp->v_iflag & VI_CLEAN) != 0) {
			vpanic(vp, "clean vnode on freelist");
		}
		if (vp->v_freelisthd != listhd) {
			printf("vnode sez %p, listhd %p\n", vp->v_freelisthd, listhd);
			vpanic(vp, "list head mismatch");
		}
		if (!mutex_tryenter(&vp->v_interlock))
			continue;
		/*
		 * Our lwp might hold the underlying vnode
		 * locked, so don't try to reclaim a VI_LAYER
		 * node if it's locked.
		 */
		if ((vp->v_iflag & VI_XLOCK) == 0 &&
		    ((vp->v_iflag & VI_LAYER) == 0 || VOP_ISLOCKED(vp) == 0)) {
			break;
		}
		mutex_exit(&vp->v_interlock);
	}

	if (vp == NULL) {
		if (listhd == &vnode_free_list) {
			listhd = &vnode_hold_list;
			goto try_nextlist;
		}
		mutex_exit(&vnode_free_list_lock);
		return NULL;
	}

	/* Remove it from the freelist. */
	TAILQ_REMOVE(listhd, vp, v_freelist);
	vp->v_freelisthd = NULL;
	mutex_exit(&vnode_free_list_lock);

	/*
	 * The vnode is still associated with a file system, so we must
	 * clean it out before reusing it.  We need to add a reference
	 * before doing this.  If the vnode gains another reference while
	 * being cleaned out then we lose - retry.
	 */
	vp->v_usecount++;
	vclean(vp, DOCLOSE);
	if (vp->v_usecount == 1) {
		/* We're about to dirty it. */
		vp->v_iflag &= ~VI_CLEAN;
		mutex_exit(&vp->v_interlock);
		if (vp->v_type == VBLK || vp->v_type == VCHR) {
			spec_node_destroy(vp);
		}
		vp->v_type = VNON;
	} else {
		/*
		 * Don't return to freelist - the holder of the last
		 * reference will destroy it.
		 */
		KASSERT(vp->v_usecount > 1);
		vp->v_usecount--;
		mutex_exit(&vp->v_interlock);
		mutex_enter(&vnode_free_list_lock);
		goto retry;
	}

	if (vp->v_data != NULL || vp->v_uobj.uo_npages != 0 ||
	    !TAILQ_EMPTY(&vp->v_uobj.memq)) {
		vpanic(vp, "cleaned vnode isn't");
	}
	if (vp->v_numoutput != 0) {
		vpanic(vp, "clean vnode has pending I/O's");
	}
	if ((vp->v_iflag & VI_ONWORKLST) != 0) {
		vpanic(vp, "clean vnode on syncer list");
	}

	return vp;
}

/*
 * Mark a mount point as busy, and gain a new reference to it.  Used to
 * synchronize access and to delay unmounting.
 *
 * => Interlock is not released on failure.
 * => If no interlock, the caller is expected to already hold a reference
 *    on the mount.
 * => If interlocked, the interlock must prevent the last reference to
 *    the mount from disappearing.
 */
int
vfs_busy(struct mount *mp, const krw_t op, kmutex_t *interlock)
{

	KASSERT(mp->mnt_refcnt > 0);

	atomic_inc_uint(&mp->mnt_refcnt);
	if (interlock != NULL) {
		mutex_exit(interlock);
	}
	if (mp->mnt_writer == curlwp) {
		mp->mnt_recursecnt++;
	} else {
		rw_enter(&mp->mnt_lock, op);
		if (op == RW_WRITER) {
			KASSERT(mp->mnt_writer == NULL);
			mp->mnt_writer = curlwp;
		}
	}
	if ((mp->mnt_iflag & IMNT_GONE) != 0) {
		vfs_unbusy(mp, false);
		if (interlock != NULL) {
			mutex_enter(interlock);
		}
		return ENOENT;
	}

	return 0;
}

/*
 * As vfs_busy(), but return immediatley if the mount cannot be
 * locked without waiting.
 */
int
vfs_trybusy(struct mount *mp, krw_t op, kmutex_t *interlock)
{

	KASSERT(mp->mnt_refcnt > 0);

	if (mp->mnt_writer == curlwp) {
		mp->mnt_recursecnt++;
	} else {
		if (!rw_tryenter(&mp->mnt_lock, op)) {
			return EBUSY;
		}
		if (op == RW_WRITER) {
			KASSERT(mp->mnt_writer == NULL);
			mp->mnt_writer = curlwp;
		}
	}
	atomic_inc_uint(&mp->mnt_refcnt);
	if ((mp->mnt_iflag & IMNT_GONE) != 0) {
		vfs_unbusy(mp, false);
		return ENOENT;
	}
	if (interlock != NULL) {
		mutex_exit(interlock);
	}
	return 0;
}

/*
 * Unlock a busy filesystem and drop reference to it.  If 'keepref' is
 * true, unlock but preserve the reference.
 */
void
vfs_unbusy(struct mount *mp, bool keepref)
{

	KASSERT(mp->mnt_refcnt > 0);

	if (mp->mnt_writer == curlwp) {
		KASSERT(rw_write_held(&mp->mnt_lock));
		if (mp->mnt_recursecnt != 0) {
			mp->mnt_recursecnt--;
		} else {
			mp->mnt_writer = NULL;
			rw_exit(&mp->mnt_lock);
		}
	} else {
		rw_exit(&mp->mnt_lock);
	}
	if (!keepref) {
		vfs_destroy(mp);
	}
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
	if (vfsp == NULL) {
		mutex_exit(&vfs_list_lock);
		return (ENODEV);
	}
	vfsp->vfs_refcount++;
	mutex_exit(&vfs_list_lock);

	mp = kmem_zalloc(sizeof(*mp), KM_SLEEP);
	if (mp == NULL)
		return ENOMEM;
	mp->mnt_refcnt = 1;
	rw_init(&mp->mnt_lock);
	mutex_init(&mp->mnt_renamelock, MUTEX_DEFAULT, IPL_NONE);
	(void)vfs_busy(mp, RW_WRITER, NULL);
	TAILQ_INIT(&mp->mnt_vnodelist);
	mp->mnt_op = vfsp;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_vnodecovered = NULL;
	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsp->vfs_name,
	    sizeof(mp->mnt_stat.f_fstypename));
	mp->mnt_stat.f_mntonname[0] = '/';
	mp->mnt_stat.f_mntonname[1] = '\0';
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
	    vnode_t **vpp)
{
	struct uvm_object *uobj;
	static int toggle;
	vnode_t *vp;
	int error = 0, tryalloc;

 try_again:
	if (mp != NULL) {
		/*
		 * Mark filesystem busy while we're creating a
		 * vnode.  If unmount is in progress, this will
		 * wait; if the unmount succeeds (only if umount
		 * -f), this will return an error.  If the
		 * unmount fails, we'll keep going afterwards.
		 */
		error = vfs_busy(mp, RW_READER, NULL);
		if (error)
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

	if (tryalloc) {
		numvnodes++;
		mutex_exit(&vnode_free_list_lock);
		if ((vp = vnalloc(NULL)) == NULL) {
			mutex_enter(&vnode_free_list_lock);
			numvnodes--;
		} else
			vp->v_usecount = 1;
	}

	if (vp == NULL) {
		vp = getcleanvnode();
		if (vp == NULL) {
			if (mp != NULL) {
				vfs_unbusy(mp, false);
			}
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
		vp->v_iflag = 0;
		vp->v_vflag = 0;
		vp->v_uflag = 0;
		vp->v_socket = NULL;
	}

	KASSERT(vp->v_usecount == 1);
	KASSERT(vp->v_freelisthd == NULL);
	KASSERT(LIST_EMPTY(&vp->v_nclist));
	KASSERT(LIST_EMPTY(&vp->v_dnclist));

	vp->v_type = VNON;
	vp->v_vnlock = &vp->v_lock;
	vp->v_tag = tag;
	vp->v_op = vops;
	insmntque(vp, mp);
	*vpp = vp;
	vp->v_data = 0;

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
		vfs_unbusy(mp, true);
	}

	return (0);
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

	mutex_enter(&vp->v_interlock);
	vp->v_iflag |= VI_CLEAN;
	vrelel(vp, 0);
}

/*
 * Allocate a new, uninitialized vnode.  If 'mp' is non-NULL, this is a
 * marker vnode and we are prepared to wait for the allocation.
 */
vnode_t *
vnalloc(struct mount *mp)
{
	vnode_t *vp;

	vp = pool_cache_get(vnode_cache, (mp != NULL ? PR_WAITOK : PR_NOWAIT));
	if (vp == NULL) {
		return NULL;
	}

	memset(vp, 0, sizeof(*vp));
	UVM_OBJ_INIT(&vp->v_uobj, &uvm_vnodeops, 0);
	cv_init(&vp->v_cv, "vnode");
	/*
	 * done by memset() above.
	 *	LIST_INIT(&vp->v_nclist);
	 *	LIST_INIT(&vp->v_dnclist);
	 */

	if (mp != NULL) {
		vp->v_mount = mp;
		vp->v_type = VBAD;
		vp->v_iflag = VI_MARKER;
	} else {
		rw_init(&vp->v_lock.vl_lock);
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
		rw_destroy(&vp->v_lock.vl_lock);
		mutex_enter(&vnode_free_list_lock);
		numvnodes--;
		mutex_exit(&vnode_free_list_lock);
	}

	UVM_OBJ_DESTROY(&vp->v_uobj);
	cv_destroy(&vp->v_cv);
	pool_cache_put(vnode_cache, vp);
}

/*
 * Remove a vnode from its freelist.
 */
static inline void
vremfree(vnode_t *vp)
{

	KASSERT(mutex_owned(&vp->v_interlock));
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
 * Move a vnode from one mount queue to another.
 */
static void
insmntque(vnode_t *vp, struct mount *mp)
{
	struct mount *omp;

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
	if ((omp = vp->v_mount) != NULL)
		TAILQ_REMOVE(&vp->v_mount->mnt_vnodelist, vp, v_mntvnodes);
	/*
	 * Insert into list of vnodes for the new mount point, if
	 * available.  The caller must take a reference on the mount
	 * structure and donate to the vnode.
	 */
	if ((vp->v_mount = mp) != NULL)
		TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vp, v_mntvnodes);
	mutex_exit(&mntvnode_lock);

	if (omp != NULL) {
		/* Release reference to old mount. */
		vfs_destroy(omp);
	}
}

/*
 * Create a vnode for a block device.
 * Used for root filesystem and swap areas.
 * Also used for memory file system special devices.
 */
int
bdevvp(dev_t dev, vnode_t **vpp)
{

	return (getdevvp(dev, vpp, VBLK));
}

/*
 * Create a vnode for a character device.
 * Used for kernfs and some console handling.
 */
int
cdevvp(dev_t dev, vnode_t **vpp)
{

	return (getdevvp(dev, vpp, VCHR));
}

/*
 * Create a vnode for a device.
 * Used by bdevvp (block device) for root file system etc.,
 * and by cdevvp (character device) for console and kernfs.
 */
static int
getdevvp(dev_t dev, vnode_t **vpp, enum vtype type)
{
	vnode_t *vp;
	vnode_t *nvp;
	int error;

	if (dev == NODEV) {
		*vpp = NULL;
		return (0);
	}
	error = getnewvnode(VT_NON, NULL, spec_vnodeop_p, &nvp);
	if (error) {
		*vpp = NULL;
		return (error);
	}
	vp = nvp;
	vp->v_type = type;
	vp->v_vflag |= VV_MPSAFE;
	uvm_vnp_setsize(vp, 0);
	spec_node_init(vp, dev);
	*vpp = vp;
	return (0);
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
vget(vnode_t *vp, int flags)
{
	int error;

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	if ((flags & LK_INTERLOCK) == 0)
		mutex_enter(&vp->v_interlock);

	/*
	 * Before adding a reference, we must remove the vnode
	 * from its freelist.
	 */
	if (vp->v_usecount == 0) {
		vremfree(vp);
	}
	if (++vp->v_usecount == 0) {
		vpanic(vp, "vget: usecount overflow");
	}

	/*
	 * If the vnode is in the process of being cleaned out for
	 * another use, we wait for the cleaning to finish and then
	 * return failure.  Cleaning is determined by checking if
	 * the VI_XLOCK or VI_FREEING flags are set.
	 */
	if ((vp->v_iflag & (VI_XLOCK | VI_FREEING)) != 0) {
		if ((flags & LK_NOWAIT) != 0) {
			vrelel(vp, 0);
			return EBUSY;
		}
		vwait(vp, VI_XLOCK | VI_FREEING);
		vrelel(vp, 0);
		return ENOENT;
	}
	if (flags & LK_TYPE_MASK) {
		error = vn_lock(vp, flags | LK_INTERLOCK);
		if (error != 0) {
			vrele(vp);
		}
		return error;
	}
	mutex_exit(&vp->v_interlock);
	return 0;
}

/*
 * vput(), just unlock and vrele()
 */
void
vput(vnode_t *vp)
{

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	VOP_UNLOCK(vp, 0);
	vrele(vp);
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

	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT((vp->v_iflag & VI_MARKER) == 0);
	KASSERT(vp->v_freelisthd == NULL);

	if (vp->v_op == dead_vnodeop_p && (vp->v_iflag & VI_CLEAN) == 0) {
		vpanic(vp, "dead but not clean");
	}

	/*
	 * If not the last reference, just drop the reference count
	 * and unlock.
	 */
	if (vp->v_usecount > 1) {
		vp->v_usecount--;
		vp->v_iflag |= VI_INACTREDO;
		mutex_exit(&vp->v_interlock);
		return;
	}
	if (vp->v_usecount <= 0 || vp->v_writecount != 0) {
		vpanic(vp, "vput: bad ref count");
	}

	/*
	 * If not clean, deactivate the vnode, but preserve
	 * our reference across the call to VOP_INACTIVE().
	 */
 retry:
	if ((vp->v_iflag & VI_CLEAN) == 0) {
		recycle = false;
		/*
		 * XXX This ugly block can be largely eliminated if
		 * locking is pushed down into the file systems.
		 */
		if (curlwp == uvm.pagedaemon_lwp) {
			/* The pagedaemon can't wait around; defer. */
			defer = true;
		} else if (curlwp == vrele_lwp) {
			/* We have to try harder. */
			vp->v_iflag &= ~VI_INACTREDO;
			error = vn_lock(vp, LK_EXCLUSIVE | LK_INTERLOCK |
			    LK_RETRY);
			if (error != 0) {
				/* XXX */
				vpanic(vp, "vrele: unable to lock %p");
			}
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
			error = vn_lock(vp, LK_EXCLUSIVE | LK_INTERLOCK |
			    LK_NOWAIT);
			if (error != 0) {
				defer = true;
				mutex_enter(&vp->v_interlock);
			} else {
				defer = false;
			}
		}

		if (defer) {
			/*
			 * Defer reclaim to the kthread; it's not safe to
			 * clean it here.  We donate it our last reference.
			 */
			KASSERT(mutex_owned(&vp->v_interlock));
			KASSERT((vp->v_iflag & VI_INACTPEND) == 0);
			vp->v_iflag |= VI_INACTPEND;
			mutex_enter(&vrele_lock);
			TAILQ_INSERT_TAIL(&vrele_list, vp, v_freelist);
			if (++vrele_pending > (desiredvnodes >> 8))
				cv_signal(&vrele_cv); 
			mutex_exit(&vrele_lock);
			mutex_exit(&vp->v_interlock);
			return;
		}

#ifdef DIAGNOSTIC
		if ((vp->v_type == VBLK || vp->v_type == VCHR) &&
		    vp->v_specnode != NULL && vp->v_specnode->sn_opencnt != 0) {
			vprint("vrelel: missing VOP_CLOSE()", vp);
		}
#endif

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
		mutex_enter(&vp->v_interlock);
		if (!recycle) {
			if (vp->v_usecount > 1) {
				vp->v_usecount--;
				mutex_exit(&vp->v_interlock);
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
		vp->v_iflag &= ~(VI_TEXT|VI_EXECMAP|VI_WRMAP|VI_MAPPED);
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

	if (--vp->v_usecount != 0) {
		/* Gained another reference while being reclaimed. */
		mutex_exit(&vp->v_interlock);
		return;
	}

	if ((vp->v_iflag & VI_CLEAN) != 0) {
		/*
		 * It's clean so destroy it.  It isn't referenced
		 * anywhere since it has been reclaimed.
		 */
		KASSERT(vp->v_holdcnt == 0);
		KASSERT(vp->v_writecount == 0);
		mutex_exit(&vp->v_interlock);
		insmntque(vp, NULL);
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
		mutex_exit(&vp->v_interlock);
	}
}

void
vrele(vnode_t *vp)
{

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	mutex_enter(&vp->v_interlock);
	vrelel(vp, 0);
}

static void
vrele_thread(void *cookie)
{
	vnode_t *vp;

	for (;;) {
		mutex_enter(&vrele_lock);
		while (TAILQ_EMPTY(&vrele_list)) {
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
		mutex_enter(&vp->v_interlock);
		KASSERT((vp->v_iflag & VI_INACTPEND) != 0);
		vp->v_iflag &= ~VI_INACTPEND;
		if (vp->v_usecount > 1) {
			vp->v_usecount--;
			mutex_exit(&vp->v_interlock);
			continue;
		}
		vrelel(vp, 0);
	}
}

/*
 * Page or buffer structure gets a reference.
 * Called with v_interlock held.
 */
void
vholdl(vnode_t *vp)
{

	KASSERT(mutex_owned(&vp->v_interlock));
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

	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	if (vp->v_holdcnt <= 0) {
		vpanic(vp, "holdrelel: holdcnt vp %p");
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
 * Vnode reference, where a reference is already held by some other
 * object (for example, a file structure).
 */
void
vref(vnode_t *vp)
{

	KASSERT((vp->v_iflag & VI_MARKER) == 0);

	mutex_enter(&vp->v_interlock);
	if (vp->v_usecount <= 0) {
		vpanic(vp, "vref used where vget required");
	}
	if (++vp->v_usecount == 0) {
		vpanic(vp, "vref: usecount overflow");
	}
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
vflush(struct mount *mp, vnode_t *skipvp, int flags)
{
	vnode_t *vp, *mvp;
	int busy = 0;

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return (ENOMEM);

	mutex_enter(&mntvnode_lock);
	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() are called
	 */
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		if (vp->v_mount != mp || vismarker(vp))
			continue;
		/*
		 * Skip over a selected vnode.
		 */
		if (vp == skipvp)
			continue;
		mutex_enter(&vp->v_interlock);
		/*
		 * Ignore clean but still referenced vnodes.
		 */
		if ((vp->v_iflag & VI_CLEAN) != 0) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
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
			vremfree(vp);
			vp->v_usecount++;
			vclean(vp, DOCLOSE);
			vrelel(vp, 0);
			mutex_enter(&mntvnode_lock);
			continue;
		}
		/*
		 * If FORCECLOSE is set, forcibly close the vnode.
		 * For block or character devices, revert to an
		 * anonymous device.  For all other files, just
		 * kill them.
		 */
		if (flags & FORCECLOSE) {
			mutex_exit(&mntvnode_lock);
			vp->v_usecount++;
			if (vp->v_type != VBLK && vp->v_type != VCHR) {
				vclean(vp, DOCLOSE);
				vrelel(vp, 0);
			} else {
				vclean(vp, 0);
				vp->v_op = spec_vnodeop_p; /* XXXSMP */
				mutex_exit(&vp->v_interlock);
				/*
				 * The vnode isn't clean, but still resides
				 * on the mount list.  Remove it. XXX This
				 * is a bit dodgy.
				 */
				insmntque(vp, NULL);
				vrele(vp);
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
	vnfree(mvp);
	if (busy)
		return (EBUSY);
	return (0);
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

	KASSERT(mutex_owned(&vp->v_interlock));
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
	active = (vp->v_usecount > 1);

	/* XXXAD should not lock vnode under layer */
	VOP_LOCK(vp, LK_EXCLUSIVE | LK_INTERLOCK);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
	 */
	if (flags & DOCLOSE) {
		error = vinvalbuf(vp, V_SAVE, NOCRED, l, 0, 0);
		if (error != 0)
			error = vinvalbuf(vp, 0, NOCRED, l, 0, 0);
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
		VOP_UNLOCK(vp, 0);
	}

	/* Disassociate the underlying file system from the vnode. */
	if (VOP_RECLAIM(vp)) {
		vpanic(vp, "vclean: cannot reclaim");
	}

	KASSERT(vp->v_uobj.uo_npages == 0);
	if (vp->v_type == VREG && vp->v_ractx != NULL) {
		uvm_ra_freectx(vp->v_ractx);
		vp->v_ractx = NULL;
	}
	cache_purge(vp);

	/* Done with purge, notify sleepers of the grim news. */
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	mutex_enter(&vp->v_interlock);
	vp->v_vnlock = &vp->v_lock;
	VN_KNOTE(vp, NOTE_REVOKE);
	vp->v_iflag &= ~(VI_XLOCK | VI_FREEING);
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

	mutex_enter(&vp->v_interlock);
	if (vp->v_usecount != 0) {
		mutex_exit(&vp->v_interlock);
		return (0);
	}
	if (inter_lkp)
		mutex_exit(inter_lkp);
	vremfree(vp);
	vp->v_usecount++;
	vclean(vp, DOCLOSE);
	vrelel(vp, 0);
	return (1);
}

/*
 * Eliminate all activity associated with a vnode in preparation for
 * reuse.  Drops a reference from the vnode.
 */
void
vgone(vnode_t *vp)
{

	mutex_enter(&vp->v_interlock);
	vclean(vp, DOCLOSE);
	vrelel(vp, 0);
}

/*
 * Lookup a vnode by device number.
 */
int
vfinddev(dev_t dev, enum vtype type, vnode_t **vpp)
{
	vnode_t *vp;
	int rc = 0;

	mutex_enter(&specfs_lock);
	for (vp = specfs_hash[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (dev != vp->v_rdev || type != vp->v_type)
			continue;
		*vpp = vp;
		rc = 1;
		break;
	}
	mutex_exit(&specfs_lock);
	return (rc);
}

/*
 * Revoke all the vnodes corresponding to the specified minor number
 * range (endpoints inclusive) of the specified major.
 */
void
vdevgone(int maj, int minl, int minh, enum vtype type)
{
	vnode_t *vp, **vpp;
	dev_t dev;
	int mn;

	vp = NULL;	/* XXX gcc */

	mutex_enter(&specfs_lock);
	for (mn = minl; mn <= minh; mn++) {
		dev = makedev(maj, mn);
		vpp = &specfs_hash[SPECHASH(dev)];
		for (vp = *vpp; vp != NULL;) {
			mutex_enter(&vp->v_interlock);
			if ((vp->v_iflag & VI_CLEAN) != 0 ||
			    dev != vp->v_rdev || type != vp->v_type) {
				mutex_exit(&vp->v_interlock);
				vp = vp->v_specnext;
				continue;
			}
			mutex_exit(&specfs_lock);
			if (vget(vp, LK_INTERLOCK) == 0) {
				VOP_REVOKE(vp, REVOKEALL);
				vrele(vp);
			}
			mutex_enter(&specfs_lock);
			vp = *vpp;
		}
	}
	mutex_exit(&specfs_lock);
}

/*
 * Calculate the total number of references to a special device.
 */
int
vcount(vnode_t *vp)
{
	int count;

	mutex_enter(&specfs_lock);
	mutex_enter(&vp->v_interlock);
	if (vp->v_specnode == NULL) {
		count = vp->v_usecount - ((vp->v_iflag & VI_INACTPEND) != 0);
		mutex_exit(&vp->v_interlock);
		mutex_exit(&specfs_lock);
		return (count);
	}
	mutex_exit(&vp->v_interlock);
	count = vp->v_specnode->sn_dev->sd_opencnt;
	mutex_exit(&specfs_lock);
	return (count);
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

	mutex_enter(&vp->v_interlock);
	if ((vp->v_iflag & VI_CLEAN) != 0) {
		mutex_exit(&vp->v_interlock);
		return;
	} else {
		dev = vp->v_rdev;
		type = vp->v_type;
		mutex_exit(&vp->v_interlock);
	}

	vpp = &specfs_hash[SPECHASH(dev)];
	mutex_enter(&specfs_lock);
	for (vq = *vpp; vq != NULL;) {
		if ((vq->v_iflag & VI_CLEAN) != 0 ||
		    vq->v_rdev != dev || vq->v_type != type) {
			vq = vq->v_specnext;
			continue;
		}
		mutex_enter(&vq->v_interlock);
		mutex_exit(&specfs_lock);
		if (vq->v_usecount == 0) {
			vremfree(vq);
		}
		vq->v_usecount++;
		vclean(vq, DOCLOSE);
		vrelel(vq, 0);
		mutex_enter(&specfs_lock);
		vq = *vpp;
	}
	mutex_exit(&specfs_lock);
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

	sysctl_unlock();
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
	sysctl_relock();
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
	vnode_t *vp, *mvp, vbuf;
	char *bp = where, *savebp;
	char *ewhere;
	int error;

	if (namelen != 0)
		return (EOPNOTSUPP);
	if (newp != NULL)
		return (EPERM);

#define VPTRSZ	sizeof(vnode_t *)
#define VNODESZ	sizeof(vnode_t)
	if (where == NULL) {
		*sizep = (numvnodes + KINFO_VNODESLOP) * (VPTRSZ + VNODESZ);
		return (0);
	}
	ewhere = where + *sizep;

	sysctl_unlock();
	mutex_enter(&mountlist_lock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_trybusy(mp, RW_READER, &mountlist_lock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		savebp = bp;
		/* Allocate a marker vnode. */
		if ((mvp = vnalloc(mp)) == NULL) {
			sysctl_relock();
			return (ENOMEM);
		}
		mutex_enter(&mntvnode_lock);
		for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
			vmark(mvp, vp);
			/*
			 * Check that the vp is still associated with
			 * this filesystem.  RACE: could have been
			 * recycled onto the same filesystem.
			 */
			if (vp->v_mount != mp || vismarker(vp))
				continue;
			if (bp + VPTRSZ + VNODESZ > ewhere) {
				(void)vunmark(mvp);
				mutex_exit(&mntvnode_lock);
				vnfree(mvp);
				sysctl_relock();
				*sizep = bp - where;
				return (ENOMEM);
			}
			memcpy(&vbuf, vp, VNODESZ);
			mutex_exit(&mntvnode_lock);
			if ((error = copyout(vp, bp, VPTRSZ)) ||
			   (error = copyout(&vbuf, bp + VPTRSZ, VNODESZ))) {
			   	mutex_enter(&mntvnode_lock);
				(void)vunmark(mvp);
				mutex_exit(&mntvnode_lock);
				vnfree(mvp);
				sysctl_relock();
				return (error);
			}
			bp += VPTRSZ + VNODESZ;
			mutex_enter(&mntvnode_lock);
		}
		mutex_exit(&mntvnode_lock);
		mutex_enter(&mountlist_lock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp, false);
		vnfree(mvp);
	}
	mutex_exit(&mountlist_lock);
	sysctl_relock();

	*sizep = bp - where;
	return (0);
}

/*
 * Remove clean vnodes from a mountpoint's vnode list.
 */
void
vfs_scrubvnlist(struct mount *mp)
{
	vnode_t *vp, *nvp;

 retry:
	mutex_enter(&mntvnode_lock);
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		nvp = TAILQ_NEXT(vp, v_mntvnodes);
		mutex_enter(&vp->v_interlock);
		if ((vp->v_iflag & VI_CLEAN) != 0) {
			TAILQ_REMOVE(&mp->mnt_vnodelist, vp, v_mntvnodes);
			vp->v_mount = NULL;
			mutex_exit(&mntvnode_lock);
			mutex_exit(&vp->v_interlock);
			vfs_destroy(mp);
			goto retry;
		}
		mutex_exit(&vp->v_interlock);
	}
	mutex_exit(&mntvnode_lock);
}

/*
 * Check to see if a filesystem is mounted on a block device.
 */
int
vfs_mountedon(vnode_t *vp)
{
	vnode_t *vq;
	int error = 0;

	if (vp->v_type != VBLK)
		return ENOTBLK;
	if (vp->v_specmountpoint != NULL)
		return (EBUSY);
	mutex_enter(&specfs_lock);
	for (vq = specfs_hash[SPECHASH(vp->v_rdev)]; vq != NULL;
	    vq = vq->v_specnext) {
		if (vq->v_rdev != vp->v_rdev || vq->v_type != vp->v_type)
			continue;
		if (vq->v_specmountpoint != NULL) {
			error = EBUSY;
			break;
		}
	}
	mutex_exit(&specfs_lock);
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
	for (allerror = 0, mp = CIRCLEQ_LAST(&mountlist);
	     !CIRCLEQ_EMPTY(&mountlist);
	     mp = nmp) {
		nmp = CIRCLEQ_PREV(mp, mnt_list);
#ifdef DEBUG
		printf("\nunmounting %s (%s)...",
		    mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
#endif
		/*
		 * XXX Freeze syncer.  Must do this before locking the
		 * mount point.  See dounmount() for details.
		 */
		mutex_enter(&syncer_mutex);
		if (vfs_busy(mp, RW_WRITER, NULL)) {
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

	/* remove user processes from run queue */
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
		error = VOP_OPEN(rootvp, FREAD, FSCRED);
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
		VOP_CLOSE(rootvp, FREAD, FSCRED);
		vrele(rootvp);
	}
	return (error);
}

/*
 * Sham lock manager for vnodes.  This is a temporary measure.
 */
int
vlockmgr(struct vnlock *vl, int flags)
{

	KASSERT((flags & ~(LK_CANRECURSE | LK_NOWAIT | LK_TYPE_MASK)) == 0);

	switch (flags & LK_TYPE_MASK) {
	case LK_SHARED:
		if (rw_tryenter(&vl->vl_lock, RW_READER)) {
			return 0;
		}
		if ((flags & LK_NOWAIT) != 0) {
			return EBUSY;
		}
		rw_enter(&vl->vl_lock, RW_READER);
		return 0;

	case LK_EXCLUSIVE:
		if (rw_tryenter(&vl->vl_lock, RW_WRITER)) {
			return 0;
		}
		if ((vl->vl_canrecurse || (flags & LK_CANRECURSE) != 0) &&
		    rw_write_held(&vl->vl_lock)) {
			vl->vl_recursecnt++;
			return 0;
		}
		if ((flags & LK_NOWAIT) != 0) {
			return EBUSY;
		}
		rw_enter(&vl->vl_lock, RW_WRITER);
		return 0;

	case LK_RELEASE:
		if (vl->vl_recursecnt != 0) {
			KASSERT(rw_write_held(&vl->vl_lock));
			vl->vl_recursecnt--;
			return 0;
		}
		rw_exit(&vl->vl_lock);
		return 0;

	default:
		panic("vlockmgr: flags %x", flags);
	}
}

int
vlockstatus(struct vnlock *vl)
{

	if (rw_write_held(&vl->vl_lock)) {
		return LK_EXCLUSIVE;
	}
	if (rw_read_held(&vl->vl_lock)) {
		return LK_SHARED;
	}
	return 0;
}
