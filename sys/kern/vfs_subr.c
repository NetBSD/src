/*	$NetBSD: vfs_subr.c,v 1.387 2009/11/17 22:20:14 bouyer Exp $	*/

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
 * Note on v_usecount and locking:
 *
 * At nearly all points it is known that v_usecount could be zero, the
 * vnode interlock will be held.
 *
 * To change v_usecount away from zero, the interlock must be held.  To
 * change from a non-zero value to zero, again the interlock must be
 * held.
 *
 * There's a flag bit, VC_XLOCK, embedded in v_usecount.
 * To raise v_usecount, if the VC_XLOCK bit is set in it, the interlock
 * must be held.
 * To modify the VC_XLOCK bit, the interlock must be held.
 * We always keep the usecount (v_usecount & VC_MASK) non-zero while the
 * VC_XLOCK bit is set.
 *
 * Unless the VC_XLOCK bit is set, changing the usecount from a non-zero
 * value to a non-zero value can safely be done using atomic operations,
 * without the interlock held.
 * Even if the VC_XLOCK bit is set, decreasing the usecount to a non-zero
 * value can be done using atomic operations, without the interlock held.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_subr.c,v 1.387 2009/11/17 22:20:14 bouyer Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
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
#include <sys/kmem.h>
#include <sys/syscallargs.h>
#include <sys/device.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
#include <sys/kthread.h>
#include <sys/wapbl.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/syncfs/syncfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_ddb.h>

#include <sys/sysctl.h>

const enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
const int	vttoif_tab[9] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT,
};

/*
 * Insq/Remq for the vnode usage lists.
 */
#define	bufinsvn(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_vnbufs)
#define	bufremvn(bp) {							\
	LIST_REMOVE(bp, b_vnbufs);					\
	(bp)->b_vnbufs.le_next = NOLIST;				\
}

int doforce = 1;		/* 1 => permit forcible unmounting */
int prtactive = 0;		/* 1 => print out reclaim of active vnodes */

static vnodelst_t vnode_free_list = TAILQ_HEAD_INITIALIZER(vnode_free_list);
static vnodelst_t vnode_hold_list = TAILQ_HEAD_INITIALIZER(vnode_hold_list);
static vnodelst_t vrele_list = TAILQ_HEAD_INITIALIZER(vrele_list);

struct mntlist mountlist =			/* mounted filesystem list */
    CIRCLEQ_HEAD_INITIALIZER(mountlist);

u_int numvnodes;
static specificdata_domain_t mount_specificdata_domain;

static int vrele_pending;
static int vrele_gen;
static kmutex_t	vrele_lock;
static kcondvar_t vrele_cv;
static lwp_t *vrele_lwp;

static uint64_t mountgen = 0;
static kmutex_t mountgen_lock;

kmutex_t mountlist_lock;
kmutex_t mntid_lock;
kmutex_t mntvnode_lock;
kmutex_t vnode_free_list_lock;
kmutex_t vfs_list_lock;

static pool_cache_t vnode_cache;

/*
 * These define the root filesystem and device.
 */
struct vnode *rootvnode;
struct device *root_device;			/* root device */

/*
 * Local declarations.
 */

static void vrele_thread(void *);
static void insmntque(vnode_t *, struct mount *);
static int getdevvp(dev_t, vnode_t **, enum vtype);
static vnode_t *getcleanvnode(void);
void vpanic(vnode_t *, const char *);
static void vfs_shutdown1(struct lwp *);

#ifdef DEBUG 
void printlockedvnodes(void);
#endif

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

/*
 * Initialize the vnode management data structures.
 */
void
vntblinit(void)
{

	mutex_init(&mountgen_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mountlist_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mntid_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mntvnode_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&vnode_free_list_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&vfs_list_lock, MUTEX_DEFAULT, IPL_NONE);

	mount_specificdata_domain = specificdata_domain_create();

	/* Initialize the filesystem syncer. */
	vn_initialize_syncerd();
	vn_init1();
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
 * Lookup a mount point by filesystem identifier.
 *
 * XXX Needs to add a reference to the mount point.
 */
struct mount *
vfs_getvfs(fsid_t *fsid)
{
	struct mount *mp;

	mutex_enter(&mountlist_lock);
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			mutex_exit(&mountlist_lock);
			return (mp);
		}
	}
	mutex_exit(&mountlist_lock);
	return ((struct mount *)0);
}

/*
 * Drop a reference to a mount structure, freeing if the last reference.
 */
void
vfs_destroy(struct mount *mp)
{

	if (__predict_true((int)atomic_dec_uint_nv(&mp->mnt_refcnt) > 0)) {
		return;
	}

	/*
	 * Nothing else has visibility of the mount: we can now
	 * free the data structures.
	 */
	KASSERT(mp->mnt_refcnt == 0);
	specificdata_fini(mount_specificdata_domain, &mp->mnt_specdataref);
	rw_destroy(&mp->mnt_unmounting);
	mutex_destroy(&mp->mnt_updating);
	mutex_destroy(&mp->mnt_renamelock);
	if (mp->mnt_op != NULL) {
		vfs_delref(mp->mnt_op);
	}
	kmem_free(mp, sizeof(*mp));
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

	if (vp->v_usecount != 0) {
		/*
		 * was referenced again before we got the interlock
		 * Don't return to freelist - the holder of the last
		 * reference will destroy it.
		 */
		mutex_exit(&vp->v_interlock);
		mutex_enter(&vnode_free_list_lock);
		goto retry;
	}

	/*
	 * The vnode is still associated with a file system, so we must
	 * clean it out before reusing it.  We need to add a reference
	 * before doing this.  If the vnode gains another reference while
	 * being cleaned out then we lose - retry.
	 */
	atomic_add_int(&vp->v_usecount, 1 + VC_XLOCK);
	vclean(vp, DOCLOSE);
	KASSERT(vp->v_usecount >= 1 + VC_XLOCK);
	atomic_add_int(&vp->v_usecount, -VC_XLOCK);
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
		vrelel(vp, 0); /* releases vp->v_interlock */
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
 * prevent the file system from being unmounted during critical sections.
 *
 * => The caller must hold a pre-existing reference to the mount.
 * => Will fail if the file system is being unmounted, or is unmounted.
 */
int
vfs_busy(struct mount *mp, struct mount **nextp)
{

	KASSERT(mp->mnt_refcnt > 0);

	if (__predict_false(!rw_tryenter(&mp->mnt_unmounting, RW_READER))) {
		if (nextp != NULL) {
			KASSERT(mutex_owned(&mountlist_lock));
			*nextp = CIRCLEQ_NEXT(mp, mnt_list);
		}
		return EBUSY;
	}
	if (__predict_false((mp->mnt_iflag & IMNT_GONE) != 0)) {
		rw_exit(&mp->mnt_unmounting);
		if (nextp != NULL) {
			KASSERT(mutex_owned(&mountlist_lock));
			*nextp = CIRCLEQ_NEXT(mp, mnt_list);
		}
		return ENOENT;
	}
	if (nextp != NULL) {
		mutex_exit(&mountlist_lock);
	}
	atomic_inc_uint(&mp->mnt_refcnt);
	return 0;
}

/*
 * Unbusy a busy filesystem.
 *
 * => If keepref is true, preserve reference added by vfs_busy().
 * => If nextp != NULL, acquire mountlist_lock.
 */
void
vfs_unbusy(struct mount *mp, bool keepref, struct mount **nextp)
{

	KASSERT(mp->mnt_refcnt > 0);

	if (nextp != NULL) {
		mutex_enter(&mountlist_lock);
	}
	rw_exit(&mp->mnt_unmounting);
	if (!keepref) {
		vfs_destroy(mp);
	}
	if (nextp != NULL) {
		KASSERT(mutex_owned(&mountlist_lock));
		*nextp = CIRCLEQ_NEXT(mp, mnt_list);
	}
}

struct mount *
vfs_mountalloc(struct vfsops *vfsops, struct vnode *vp)
{
	int error;
	struct mount *mp;

	mp = kmem_zalloc(sizeof(*mp), KM_SLEEP);
	if (mp == NULL)
		return NULL;

	mp->mnt_op = vfsops;
	mp->mnt_refcnt = 1;
	TAILQ_INIT(&mp->mnt_vnodelist);
	rw_init(&mp->mnt_unmounting);
	mutex_init(&mp->mnt_renamelock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mp->mnt_updating, MUTEX_DEFAULT, IPL_NONE);
	error = vfs_busy(mp, NULL);
	KASSERT(error == 0);
	mp->mnt_vnodecovered = vp;
	mount_initspecific(mp);

	mutex_enter(&mountgen_lock);
	mp->mnt_gen = mountgen++;
	mutex_exit(&mountgen_lock);

	return mp;
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

	if ((mp = vfs_mountalloc(vfsp, NULL)) == NULL)
		return ENOMEM;
	mp->mnt_flag = MNT_RDONLY;
	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsp->vfs_name,
	    sizeof(mp->mnt_stat.f_fstypename));
	mp->mnt_stat.f_mntonname[0] = '/';
	mp->mnt_stat.f_mntonname[1] = '\0';
	mp->mnt_stat.f_mntfromname[sizeof(mp->mnt_stat.f_mntfromname) - 1] =
	    '\0';
	(void)copystr(devname, mp->mnt_stat.f_mntfromname,
	    sizeof(mp->mnt_stat.f_mntfromname) - 1, 0);
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
		 * fail.
		 */
		error = vfs_busy(mp, NULL);
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
				vfs_unbusy(mp, false, NULL);
			}
			if (tryalloc) {
				printf("WARNING: unable to allocate new "
				    "vnode, retrying...\n");
				kpause("newvn", false, hz, NULL);
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
		vfs_unbusy(mp, true, NULL);
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
 * Wait for a vnode (typically with VI_XLOCK set) to be cleaned or
 * recycled.
 */
void
vwait(vnode_t *vp, int flags)
{

	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(vp->v_usecount != 0);

	while ((vp->v_iflag & flags) != 0)
		cv_wait(&vp->v_cv, &vp->v_interlock);
}

/*
 * Insert a marker vnode into a mount's vnode list, after the
 * specified vnode.  mntvnode_lock must be held.
 */
void
vmark(vnode_t *mvp, vnode_t *vp)
{
	struct mount *mp;

	mp = mvp->v_mount;

	KASSERT(mutex_owned(&mntvnode_lock));
	KASSERT((mvp->v_iflag & VI_MARKER) != 0);
	KASSERT(vp->v_mount == mp);

	TAILQ_INSERT_AFTER(&mp->mnt_vnodelist, vp, mvp, v_mntvnodes);
}

/*
 * Remove a marker vnode from a mount's vnode list, and return
 * a pointer to the next vnode in the list.  mntvnode_lock must
 * be held.
 */
vnode_t *
vunmark(vnode_t *mvp)
{
	vnode_t *vp;
	struct mount *mp;

	mp = mvp->v_mount;

	KASSERT(mutex_owned(&mntvnode_lock));
	KASSERT((mvp->v_iflag & VI_MARKER) != 0);

	vp = TAILQ_NEXT(mvp, v_mntvnodes);
	TAILQ_REMOVE(&mp->mnt_vnodelist, mvp, v_mntvnodes); 

	KASSERT(vp == NULL || vp->v_mount == mp);

	return vp;
}

/*
 * Update outstanding I/O count and do wakeup if requested.
 */
void
vwakeup(struct buf *bp)
{
	struct vnode *vp;

	if ((vp = bp->b_vp) == NULL)
		return;

	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(bp->b_objlock));

	if (--vp->v_numoutput < 0)
		panic("vwakeup: neg numoutput, vp %p", vp);
	if (vp->v_numoutput == 0)
		cv_broadcast(&vp->v_cv);
}

/*
 * Flush out and invalidate all buffers associated with a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vinvalbuf(struct vnode *vp, int flags, kauth_cred_t cred, struct lwp *l,
	  bool catch, int slptimeo)
{
	struct buf *bp, *nbp;
	int error;
	int flushflags = PGO_ALLPAGES | PGO_FREE | PGO_SYNCIO |
	    (flags & V_SAVE ? PGO_CLEANIT | PGO_RECLAIM : 0);

	/* XXXUBC this doesn't look at flags or slp* */
	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, 0, 0, flushflags);
	if (error) {
		return error;
	}

	if (flags & V_SAVE) {
		error = VOP_FSYNC(vp, cred, FSYNC_WAIT|FSYNC_RECLAIM, 0, 0);
		if (error)
		        return (error);
		KASSERT(LIST_EMPTY(&vp->v_dirtyblkhd));
	}

	mutex_enter(&bufcache_lock);
restart:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		error = bbusy(bp, catch, slptimeo, NULL);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		error = bbusy(bp, catch, slptimeo, NULL);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		/*
		 * XXX Since there are no node locks for NFS, I believe
		 * there is a slight chance that a delayed write will
		 * occur while sleeping just above, so check for it.
		 */
		if ((bp->b_oflags & BO_DELWRI) && (flags & V_SAVE)) {
#ifdef DEBUG
			printf("buffer still DELWRI\n");
#endif
			bp->b_cflags |= BC_BUSY | BC_VFLUSH;
			mutex_exit(&bufcache_lock);
			VOP_BWRITE(bp);
			mutex_enter(&bufcache_lock);
			goto restart;
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

#ifdef DIAGNOSTIC
	if (!LIST_EMPTY(&vp->v_cleanblkhd) || !LIST_EMPTY(&vp->v_dirtyblkhd))
		panic("vinvalbuf: flush failed, vp %p", vp);
#endif

	mutex_exit(&bufcache_lock);

	return (0);
}

/*
 * Destroy any in core blocks past the truncation length.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vtruncbuf(struct vnode *vp, daddr_t lbn, bool catch, int slptimeo)
{
	struct buf *bp, *nbp;
	int error;
	voff_t off;

	off = round_page((voff_t)lbn << vp->v_mount->mnt_fs_bshift);
	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, off, 0, PGO_FREE | PGO_SYNCIO);
	if (error) {
		return error;
	}

	mutex_enter(&bufcache_lock);
restart:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		error = bbusy(bp, catch, slptimeo, NULL);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		error = bbusy(bp, catch, slptimeo, NULL);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}
	mutex_exit(&bufcache_lock);

	return (0);
}

/*
 * Flush all dirty buffers from a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
void
vflushbuf(struct vnode *vp, int sync)
{
	struct buf *bp, *nbp;
	int flags = PGO_CLEANIT | PGO_ALLPAGES | (sync ? PGO_SYNCIO : 0);
	bool dirty;

	mutex_enter(&vp->v_interlock);
	(void) VOP_PUTPAGES(vp, 0, 0, flags);

loop:
	mutex_enter(&bufcache_lock);
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if ((bp->b_cflags & BC_BUSY))
			continue;
		if ((bp->b_oflags & BO_DELWRI) == 0)
			panic("vflushbuf: not dirty, bp %p", bp);
		bp->b_cflags |= BC_BUSY | BC_VFLUSH;
		mutex_exit(&bufcache_lock);
		/*
		 * Wait for I/O associated with indirect blocks to complete,
		 * since there is no way to quickly wait for them below.
		 */
		if (bp->b_vp == vp || sync == 0)
			(void) bawrite(bp);
		else
			(void) bwrite(bp);
		goto loop;
	}
	mutex_exit(&bufcache_lock);

	if (sync == 0)
		return;

	mutex_enter(&vp->v_interlock);
	while (vp->v_numoutput != 0)
		cv_wait(&vp->v_cv, &vp->v_interlock);
	dirty = !LIST_EMPTY(&vp->v_dirtyblkhd);
	mutex_exit(&vp->v_interlock);

	if (dirty) {
		vprint("vflushbuf: dirty", vp);
		goto loop;
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
 * Associate a buffer with a vnode.  There must already be a hold on
 * the vnode.
 */
void
bgetvp(struct vnode *vp, struct buf *bp)
{

	KASSERT(bp->b_vp == NULL);
	KASSERT(bp->b_objlock == &buffer_lock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(mutex_owned(&bufcache_lock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);
	KASSERT(!cv_has_waiters(&bp->b_done));

	vholdl(vp);
	bp->b_vp = vp;
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		bp->b_dev = vp->v_rdev;
	else
		bp->b_dev = NODEV;

	/*
	 * Insert onto list for new vnode.
	 */
	bufinsvn(bp, &vp->v_cleanblkhd);
	bp->b_objlock = &vp->v_interlock;
}

/*
 * Disassociate a buffer from a vnode.
 */
void
brelvp(struct buf *bp)
{
	struct vnode *vp = bp->b_vp;

	KASSERT(vp != NULL);
	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(mutex_owned(&bufcache_lock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);
	KASSERT(!cv_has_waiters(&bp->b_done));

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);

	if (TAILQ_EMPTY(&vp->v_uobj.memq) && (vp->v_iflag & VI_ONWORKLST) &&
	    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
		vp->v_iflag &= ~VI_WRMAPDIRTY;
		vn_syncer_remove_from_worklist(vp);
	}

	bp->b_objlock = &buffer_lock;
	bp->b_vp = NULL;
	holdrelel(vp);
}

/*
 * Reassign a buffer from one vnode list to another.
 * The list reassignment must be within the same vnode.
 * Used to assign file specific control information
 * (indirect blocks) to the list to which they belong.
 */
void
reassignbuf(struct buf *bp, struct vnode *vp)
{
	struct buflists *listheadp;
	int delayx;

	KASSERT(mutex_owned(&bufcache_lock));
	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);

	/*
	 * If dirty, put on list of dirty buffers;
	 * otherwise insert onto list of clean buffers.
	 */
	if ((bp->b_oflags & BO_DELWRI) == 0) {
		listheadp = &vp->v_cleanblkhd;
		if (TAILQ_EMPTY(&vp->v_uobj.memq) &&
		    (vp->v_iflag & VI_ONWORKLST) &&
		    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
			vp->v_iflag &= ~VI_WRMAPDIRTY;
			vn_syncer_remove_from_worklist(vp);
		}
	} else {
		listheadp = &vp->v_dirtyblkhd;
		if ((vp->v_iflag & VI_ONWORKLST) == 0) {
			switch (vp->v_type) {
			case VDIR:
				delayx = dirdelay;
				break;
			case VBLK:
				if (vp->v_specmountpoint != NULL) {
					delayx = metadelay;
					break;
				}
				/* fall through */
			default:
				delayx = filedelay;
				break;
			}
			if (!vp->v_mount ||
			    (vp->v_mount->mnt_flag & MNT_ASYNC) == 0)
				vn_syncer_add_to_worklist(vp, delayx);
		}
	}
	bufinsvn(bp, listheadp);
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
	if (__predict_false((vp->v_iflag & (VI_XLOCK | VI_FREEING)) != 0)) {
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
		vp->v_usecount = 1;
	} else {
		atomic_inc_uint(&vp->v_usecount);
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

	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT((vp->v_iflag & VI_MARKER) == 0);
	KASSERT(vp->v_freelisthd == NULL);

	if (__predict_false(vp->v_op == dead_vnodeop_p &&
	    (vp->v_iflag & (VI_CLEAN|VI_XLOCK)) == 0)) {
		vpanic(vp, "dead but not clean");
	}

	/*
	 * If not the last reference, just drop the reference count
	 * and unlock.
	 */
	if (vtryrele(vp)) {
		vp->v_iflag |= VI_INACTREDO;
		mutex_exit(&vp->v_interlock);
		return;
	}
	if (vp->v_usecount <= 0 || vp->v_writecount != 0) {
		vpanic(vp, "vrelel: bad ref count");
	}

	KASSERT((vp->v_iflag & VI_XLOCK) == 0);

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
			vp->v_iflag &= ~VI_INACTNOW;
			vp->v_iflag |= VI_INACTPEND;
			mutex_enter(&vrele_lock);
			TAILQ_INSERT_TAIL(&vrele_list, vp, v_freelist);
			if (++vrele_pending > (desiredvnodes >> 8))
				cv_signal(&vrele_cv); 
			mutex_exit(&vrele_lock);
			cv_broadcast(&vp->v_cv);
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
		vp->v_iflag &= ~VI_INACTNOW;
		cv_broadcast(&vp->v_cv);
		if (!recycle) {
			if (vtryrele(vp)) {
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

	if ((vp->v_iflag & VI_INACTNOW) == 0 && vtryrele(vp)) {
		return;
	}
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
		mutex_enter(&vp->v_interlock);
		KASSERT((vp->v_iflag & VI_INACTPEND) != 0);
		vp->v_iflag &= ~VI_INACTPEND;
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
	KASSERT(vp->v_usecount != 0);

	atomic_inc_uint(&vp->v_usecount);
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

static vnode_t *
vflushnext(vnode_t *mvp, int *when)
{

	if (hardclock_ticks > *when) {
		mutex_exit(&mntvnode_lock);
		yield();
		mutex_enter(&mntvnode_lock);
		*when = hardclock_ticks + hz / 10;
	}

	return vunmark(mvp);
}

int
vflush(struct mount *mp, vnode_t *skipvp, int flags)
{
	vnode_t *vp, *mvp;
	int busy = 0, when = 0, gen;

	/*
	 * First, flush out any vnode references from vrele_list.
	 */
	mutex_enter(&vrele_lock);
	gen = vrele_gen;
	while (vrele_pending && gen == vrele_gen) {
		cv_broadcast(&vrele_cv);
		cv_wait(&vrele_cv, &vrele_lock);
	}
	mutex_exit(&vrele_lock);

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return (ENOMEM);

	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() are called
	 */
	mutex_enter(&mntvnode_lock);
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp != NULL;
	    vp = vflushnext(mvp, &when)) {
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
			vp->v_usecount = 1;
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
			atomic_inc_uint(&vp->v_usecount);
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
	mutex_enter(&vp->v_interlock);
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	vp->v_vnlock = &vp->v_lock;
	KNOTE(&vp->v_klist, NOTE_REVOKE);
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
	vp->v_usecount = 1;
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

	mutex_enter(&device_lock);
	for (vp = specfs_hash[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (dev != vp->v_rdev || type != vp->v_type)
			continue;
		*vpp = vp;
		rc = 1;
		break;
	}
	mutex_exit(&device_lock);
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

	mutex_enter(&device_lock);
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
			mutex_exit(&device_lock);
			if (vget(vp, LK_INTERLOCK) == 0) {
				VOP_REVOKE(vp, REVOKEALL);
				vrele(vp);
			}
			mutex_enter(&device_lock);
			vp = *vpp;
		}
	}
	mutex_exit(&device_lock);
}

/*
 * Calculate the total number of references to a special device.
 */
int
vcount(vnode_t *vp)
{
	int count;

	mutex_enter(&device_lock);
	mutex_enter(&vp->v_interlock);
	if (vp->v_specnode == NULL) {
		count = vp->v_usecount - ((vp->v_iflag & VI_INACTPEND) != 0);
		mutex_exit(&vp->v_interlock);
		mutex_exit(&device_lock);
		return (count);
	}
	mutex_exit(&vp->v_interlock);
	count = vp->v_specnode->sn_dev->sd_opencnt;
	mutex_exit(&device_lock);
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
	} else if (vp->v_type != VBLK && vp->v_type != VCHR) {
		atomic_inc_uint(&vp->v_usecount);
		vclean(vp, DOCLOSE);
		vrelel(vp, 0);
		return;
	} else {
		dev = vp->v_rdev;
		type = vp->v_type;
		mutex_exit(&vp->v_interlock);
	}

	vpp = &specfs_hash[SPECHASH(dev)];
	mutex_enter(&device_lock);
	for (vq = *vpp; vq != NULL;) {
		/* If clean or being cleaned, then ignore it. */
		mutex_enter(&vq->v_interlock);
		if ((vq->v_iflag & (VI_CLEAN | VI_XLOCK)) != 0 ||
		    vq->v_rdev != dev || vq->v_type != type) {
			mutex_exit(&vq->v_interlock);
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
 * sysctl helper routine to return list of supported fstypes
 */
int
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
			v->vfs_refcount++;
			mutex_exit(&vfs_list_lock);
			/* +1 to copy out the trailing NUL byte */
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
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		savebp = bp;
		/* Allocate a marker vnode. */
		mvp = vnalloc(mp);
		/* Should never fail for mp != NULL */
		KASSERT(mvp != NULL);
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
			if ((error = copyout(&vp, bp, VPTRSZ)) ||
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
		vnfree(mvp);
		vfs_unbusy(mp, false, &nmp);
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
	mutex_enter(&device_lock);
	for (vq = specfs_hash[SPECHASH(vp->v_rdev)]; vq != NULL;
	    vq = vq->v_specnext) {
		if (vq->v_rdev != vp->v_rdev || vq->v_type != vp->v_type)
			continue;
		if (vq->v_specmountpoint != NULL) {
			error = EBUSY;
			break;
		}
	}
	mutex_exit(&device_lock);
	return (error);
}

/*
 * Unmount all file systems.
 * We traverse the list in reverse order under the assumption that doing so
 * will avoid needing to worry about dependencies.
 */
bool
vfs_unmountall(struct lwp *l)
{
	printf("unmounting file systems...");
	return vfs_unmountall1(l, true, true);
}

static void
vfs_unmount_print(struct mount *mp, const char *pfx)
{
	printf("%sunmounted %s on %s type %s\n", pfx,
	    mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname,
	    mp->mnt_stat.f_fstypename);
}

bool
vfs_unmount_forceone(struct lwp *l)
{
	struct mount *mp, *nmp = NULL;
	int error;

	CIRCLEQ_FOREACH_REVERSE(mp, &mountlist, mnt_list) {
		if (nmp == NULL || mp->mnt_gen > nmp->mnt_gen)
			nmp = mp;
	}

	if (nmp == NULL)
		return false;

#ifdef DEBUG
	printf("\nforcefully unmounting %s (%s)...",
	    nmp->mnt_stat.f_mntonname, nmp->mnt_stat.f_mntfromname);
#endif
	atomic_inc_uint(&nmp->mnt_refcnt);
	if ((error = dounmount(nmp, MNT_FORCE, l)) == 0) {
		vfs_unmount_print(nmp, "forcefully ");
		return true;
	} else
		atomic_dec_uint(&nmp->mnt_refcnt);

#ifdef DEBUG
	printf("forceful unmount of %s failed with error %d\n",
	    nmp->mnt_stat.f_mntonname, error);
#endif

	return false;
}

bool
vfs_unmountall1(struct lwp *l, bool force, bool verbose)
{
	struct mount *mp, *nmp;
	bool any_error = false, progress = false;
	int error;

	for (mp = CIRCLEQ_LAST(&mountlist);
	     mp != (void *)&mountlist;
	     mp = nmp) {
		nmp = CIRCLEQ_PREV(mp, mnt_list);
#ifdef DEBUG
		printf("\nunmounting %p %s (%s)...",
		    (void *)mp, mp->mnt_stat.f_mntonname,
		    mp->mnt_stat.f_mntfromname);
#endif
		atomic_inc_uint(&mp->mnt_refcnt);
		if ((error = dounmount(mp, force ? MNT_FORCE : 0, l)) == 0) {
			vfs_unmount_print(mp, "");
			progress = true;
		} else {
			atomic_dec_uint(&mp->mnt_refcnt);
			if (verbose) {
				printf("unmount of %s failed with error %d\n",
				    mp->mnt_stat.f_mntonname, error);
			}
			any_error = true;
		}
	}
	if (verbose)
		printf(" done\n");
	if (any_error && verbose)
		printf("WARNING: some file systems would not unmount\n");
	return progress;
}

/*
 * Sync and unmount file systems before shutting down.
 */
void
vfs_shutdown(void)
{
	struct lwp *l;

	/* XXX we're certainly not running in lwp0's context! */
	l = (curlwp == NULL) ? &lwp0 : curlwp;

	vfs_shutdown1(l);
}

void
vfs_sync_all(struct lwp *l)
{
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
}

static void
vfs_shutdown1(struct lwp *l)
{

	vfs_sync_all(l);

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
 * Print a list of supported file system types (used by vfs_mountroot)
 */
static void
vfs_print_fstypes(void)
{
	struct vfsops *v;
	int cnt = 0;

	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list)
		++cnt;
	mutex_exit(&vfs_list_lock);

	if (cnt == 0) {
		printf("WARNING: No file system modules have been loaded.\n");
		return;
	}

	printf("Supported file systems:");
	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		printf(" %s", v->vfs_name);
	}
	mutex_exit(&vfs_list_lock);
	printf("\n");
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
			    "(0x%llx -> %llu,%llu)",
			    (unsigned long long)rootdev,
			    (unsigned long long)major(rootdev),
			    (unsigned long long)minor(rootdev));
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
		    device_xname(root_device));
		return (ENODEV);
	}

	/*
	 * If user specified a root fs type, use it.  Make sure the
	 * specified type exists and has a mount_root()
	 */
	if (strcmp(rootfstype, ROOT_FSTYPE_ANY) != 0) {
		v = vfs_getopsbyname(rootfstype);
		error = EFTYPE;
		if (v != NULL) {
			if (v->vfs_mountroot != NULL) {
				error = (v->vfs_mountroot)();
			}
			v->vfs_refcount--;
		}
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
		vfs_print_fstypes();
		printf("no file system for %s", device_xname(root_device));
		if (device_class(root_device) == DV_DISK)
			printf(" (dev 0x%llx)", (unsigned long long)rootdev);
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
 * Get a new unique fsid
 */
void
vfs_getnewfsid(struct mount *mp)
{
	static u_short xxxfs_mntid;
	fsid_t tfsid;
	int mtype;

	mutex_enter(&mntid_lock);
	mtype = makefstype(mp->mnt_op->vfs_name);
	mp->mnt_stat.f_fsidx.__fsid_val[0] = makedev(mtype, 0);
	mp->mnt_stat.f_fsidx.__fsid_val[1] = mtype;
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	if (xxxfs_mntid == 0)
		++xxxfs_mntid;
	tfsid.__fsid_val[0] = makedev(mtype & 0xff, xxxfs_mntid);
	tfsid.__fsid_val[1] = mtype;
	if (!CIRCLEQ_EMPTY(&mountlist)) {
		while (vfs_getvfs(&tfsid)) {
			tfsid.__fsid_val[0]++;
			xxxfs_mntid++;
		}
	}
	mp->mnt_stat.f_fsidx.__fsid_val[0] = tfsid.__fsid_val[0];
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mutex_exit(&mntid_lock);
}

/*
 * Make a 'unique' number from a mount type name.
 */
long
makefstype(const char *type)
{
	long rv;

	for (rv = 0; *type; type++) {
		rv <<= 2;
		rv ^= *type;
	}
	return rv;
}

/*
 * Set vnode attributes to VNOVAL
 */
void
vattr_null(struct vattr *vap)
{

	vap->va_type = VNON;

	/*
	 * Assign individually so that it is safe even if size and
	 * sign of each member are varied.
	 */
	vap->va_mode = VNOVAL;
	vap->va_nlink = VNOVAL;
	vap->va_uid = VNOVAL;
	vap->va_gid = VNOVAL;
	vap->va_fsid = VNOVAL;
	vap->va_fileid = VNOVAL;
	vap->va_size = VNOVAL;
	vap->va_blocksize = VNOVAL;
	vap->va_atime.tv_sec =
	    vap->va_mtime.tv_sec =
	    vap->va_ctime.tv_sec =
	    vap->va_birthtime.tv_sec = VNOVAL;
	vap->va_atime.tv_nsec =
	    vap->va_mtime.tv_nsec =
	    vap->va_ctime.tv_nsec =
	    vap->va_birthtime.tv_nsec = VNOVAL;
	vap->va_gen = VNOVAL;
	vap->va_flags = VNOVAL;
	vap->va_rdev = VNOVAL;
	vap->va_bytes = VNOVAL;
	vap->va_vaflags = 0;
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define ARRAY_PRINT(idx, arr) \
    ((unsigned int)(idx) < ARRAY_SIZE(arr) ? (arr)[(idx)] : "UNKNOWN")

const char * const vnode_tags[] = { VNODE_TAGS };
const char * const vnode_types[] = { VNODE_TYPES };
const char vnode_flagbits[] = VNODE_FLAGBITS;

/*
 * Print out a description of a vnode.
 */
void
vprint(const char *label, struct vnode *vp)
{
	struct vnlock *vl;
	char bf[96];
	int flag;

	vl = (vp->v_vnlock != NULL ? vp->v_vnlock : &vp->v_lock);
	flag = vp->v_iflag | vp->v_vflag | vp->v_uflag;
	snprintb(bf, sizeof(bf), vnode_flagbits, flag);

	if (label != NULL)
		printf("%s: ", label);
	printf("vnode @ %p, flags (%s)\n\ttag %s(%d), type %s(%d), "
	    "usecount %d, writecount %d, holdcount %d\n"
	    "\tfreelisthd %p, mount %p, data %p lock %p recursecnt %d\n",
	    vp, bf, ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	    ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	    vp->v_usecount, vp->v_writecount, vp->v_holdcnt,
	    vp->v_freelisthd, vp->v_mount, vp->v_data, vl, vl->vl_recursecnt);
	if (vp->v_data != NULL) {
		printf("\t");
		VOP_PRINT(vp);
	}
}

#ifdef DEBUG
/*
 * List all of the locked vnodes in the system.
 * Called when debugging the kernel.
 */
void
printlockedvnodes(void)
{
	struct mount *mp, *nmp;
	struct vnode *vp;

	printf("Locked vnodes\n");
	mutex_enter(&mountlist_lock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp))
				vprint(NULL, vp);
		}
		mutex_enter(&mountlist_lock);
		vfs_unbusy(mp, false, &nmp);
	}
	mutex_exit(&mountlist_lock);
}
#endif

/* Deprecated. Kept for KPI compatibility. */
int
vaccess(enum vtype type, mode_t file_mode, uid_t uid, gid_t gid,
    mode_t acc_mode, kauth_cred_t cred)
{

#ifdef DIAGNOSTIC
	printf("vaccess: deprecated interface used.\n");
#endif /* DIAGNOSTIC */

	return genfs_can_access(type, file_mode, uid, gid, acc_mode, cred);
}

/*
 * Given a file system name, look up the vfsops for that
 * file system, or return NULL if file system isn't present
 * in the kernel.
 */
struct vfsops *
vfs_getopsbyname(const char *name)
{
	struct vfsops *v;

	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (strcmp(v->vfs_name, name) == 0)
			break;
	}
	if (v != NULL)
		v->vfs_refcount++;
	mutex_exit(&vfs_list_lock);

	return (v);
}

void
copy_statvfs_info(struct statvfs *sbp, const struct mount *mp)
{
	const struct statvfs *mbp;

	if (sbp == (mbp = &mp->mnt_stat))
		return;

	(void)memcpy(&sbp->f_fsidx, &mbp->f_fsidx, sizeof(sbp->f_fsidx));
	sbp->f_fsid = mbp->f_fsid;
	sbp->f_owner = mbp->f_owner;
	sbp->f_flag = mbp->f_flag;
	sbp->f_syncwrites = mbp->f_syncwrites;
	sbp->f_asyncwrites = mbp->f_asyncwrites;
	sbp->f_syncreads = mbp->f_syncreads;
	sbp->f_asyncreads = mbp->f_asyncreads;
	(void)memcpy(sbp->f_spare, mbp->f_spare, sizeof(mbp->f_spare));
	(void)memcpy(sbp->f_fstypename, mbp->f_fstypename,
	    sizeof(sbp->f_fstypename));
	(void)memcpy(sbp->f_mntonname, mbp->f_mntonname,
	    sizeof(sbp->f_mntonname));
	(void)memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname,
	    sizeof(sbp->f_mntfromname));
	sbp->f_namemax = mbp->f_namemax;
}

int
set_statvfs_info(const char *onp, int ukon, const char *fromp, int ukfrom,
    const char *vfsname, struct mount *mp, struct lwp *l)
{
	int error;
	size_t size;
	struct statvfs *sfs = &mp->mnt_stat;
	int (*fun)(const void *, void *, size_t, size_t *);

	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsname,
	    sizeof(mp->mnt_stat.f_fstypename));

	if (onp) {
		struct cwdinfo *cwdi = l->l_proc->p_cwdi;
		fun = (ukon == UIO_SYSSPACE) ? copystr : copyinstr;
		if (cwdi->cwdi_rdir != NULL) {
			size_t len;
			char *bp;
			char *path = PNBUF_GET();

			bp = path + MAXPATHLEN;
			*--bp = '\0';
			rw_enter(&cwdi->cwdi_lock, RW_READER);
			error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp,
			    path, MAXPATHLEN / 2, 0, l);
			rw_exit(&cwdi->cwdi_lock);
			if (error) {
				PNBUF_PUT(path);
				return error;
			}

			len = strlen(bp);
			if (len > sizeof(sfs->f_mntonname) - 1)
				len = sizeof(sfs->f_mntonname) - 1;
			(void)strncpy(sfs->f_mntonname, bp, len);
			PNBUF_PUT(path);

			if (len < sizeof(sfs->f_mntonname) - 1) {
				error = (*fun)(onp, &sfs->f_mntonname[len],
				    sizeof(sfs->f_mntonname) - len - 1, &size);
				if (error)
					return error;
				size += len;
			} else {
				size = len;
			}
		} else {
			error = (*fun)(onp, &sfs->f_mntonname,
			    sizeof(sfs->f_mntonname) - 1, &size);
			if (error)
				return error;
		}
		(void)memset(sfs->f_mntonname + size, 0,
		    sizeof(sfs->f_mntonname) - size);
	}

	if (fromp) {
		fun = (ukfrom == UIO_SYSSPACE) ? copystr : copyinstr;
		error = (*fun)(fromp, sfs->f_mntfromname,
		    sizeof(sfs->f_mntfromname) - 1, &size);
		if (error)
			return error;
		(void)memset(sfs->f_mntfromname + size, 0,
		    sizeof(sfs->f_mntfromname) - size);
	}
	return 0;
}

void
vfs_timestamp(struct timespec *ts)
{

	nanotime(ts);
}

time_t	rootfstime;			/* recorded root fs time, if known */
void
setrootfstime(time_t t)
{
	rootfstime = t;
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

/*
 * mount_specific_key_create --
 *	Create a key for subsystem mount-specific data.
 */
int
mount_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(mount_specificdata_domain, keyp, dtor));
}

/*
 * mount_specific_key_delete --
 *	Delete a key for subsystem mount-specific data.
 */
void
mount_specific_key_delete(specificdata_key_t key)
{

	specificdata_key_delete(mount_specificdata_domain, key);
}

/*
 * mount_initspecific --
 *	Initialize a mount's specificdata container.
 */
void
mount_initspecific(struct mount *mp)
{
	int error;

	error = specificdata_init(mount_specificdata_domain,
				  &mp->mnt_specdataref);
	KASSERT(error == 0);
}

/*
 * mount_finispecific --
 *	Finalize a mount's specificdata container.
 */
void
mount_finispecific(struct mount *mp)
{

	specificdata_fini(mount_specificdata_domain, &mp->mnt_specdataref);
}

/*
 * mount_getspecific --
 *	Return mount-specific data corresponding to the specified key.
 */
void *
mount_getspecific(struct mount *mp, specificdata_key_t key)
{

	return (specificdata_getspecific(mount_specificdata_domain,
					 &mp->mnt_specdataref, key));
}

/*
 * mount_setspecific --
 *	Set mount-specific data corresponding to the specified key.
 */
void
mount_setspecific(struct mount *mp, specificdata_key_t key, void *data)
{

	specificdata_setspecific(mount_specificdata_domain,
				 &mp->mnt_specdataref, key, data);
}

int
VFS_MOUNT(struct mount *mp, const char *a, void *b, size_t *c)
{
	int error;

	KERNEL_LOCK(1, NULL);
	error = (*(mp->mnt_op->vfs_mount))(mp, a, b, c);
	KERNEL_UNLOCK_ONE(NULL);

	return error;
}
	
int
VFS_START(struct mount *mp, int a)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_start))(mp, a);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}
	
int
VFS_UNMOUNT(struct mount *mp, int a)
{
	int error;

	KERNEL_LOCK(1, NULL);
	error = (*(mp->mnt_op->vfs_unmount))(mp, a);
	KERNEL_UNLOCK_ONE(NULL);

	return error;
}

int
VFS_ROOT(struct mount *mp, struct vnode **a)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_root))(mp, a);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_QUOTACTL(struct mount *mp, int a, uid_t b, void *c)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_quotactl))(mp, a, b, c);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_STATVFS(struct mount *mp, struct statvfs *a)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_statvfs))(mp, a);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_SYNC(struct mount *mp, int a, struct kauth_cred *b)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_sync))(mp, a, b);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_FHTOVP(struct mount *mp, struct fid *a, struct vnode **b)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_fhtovp))(mp, a, b);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_VPTOFH(struct vnode *vp, struct fid *a, size_t *b)
{
	int error;

	if ((vp->v_vflag & VV_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(vp->v_mount->mnt_op->vfs_vptofh))(vp, a, b);
	if ((vp->v_vflag & VV_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_SNAPSHOT(struct mount *mp, struct vnode *a, struct timespec *b)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_snapshot))(mp, a, b);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

int
VFS_EXTATTRCTL(struct mount *mp, int a, struct vnode *b, int c, const char *d)
{
	int error;

	KERNEL_LOCK(1, NULL);		/* XXXSMP check ffs */
	error = (*(mp->mnt_op->vfs_extattrctl))(mp, a, b, c, d);
	KERNEL_UNLOCK_ONE(NULL);	/* XXX */

	return error;
}

int
VFS_SUSPENDCTL(struct mount *mp, int a)
{
	int error;

	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_LOCK(1, NULL);
	}
	error = (*(mp->mnt_op->vfs_suspendctl))(mp, a);
	if ((mp->mnt_iflag & IMNT_MPSAFE) == 0) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return error;
}

#if defined(DDB) || defined(DEBUGPRINT)
static const char buf_flagbits[] = BUF_FLAGBITS;

void
vfs_buf_print(struct buf *bp, int full, void (*pr)(const char *, ...))
{
	char bf[1024];

	(*pr)("  vp %p lblkno 0x%"PRIx64" blkno 0x%"PRIx64" rawblkno 0x%"
	    PRIx64 " dev 0x%x\n",
	    bp->b_vp, bp->b_lblkno, bp->b_blkno, bp->b_rawblkno, bp->b_dev);

	snprintb(bf, sizeof(bf),
	    buf_flagbits, bp->b_flags | bp->b_oflags | bp->b_cflags);
	(*pr)("  error %d flags 0x%s\n", bp->b_error, bf);

	(*pr)("  bufsize 0x%lx bcount 0x%lx resid 0x%lx\n",
		  bp->b_bufsize, bp->b_bcount, bp->b_resid);
	(*pr)("  data %p saveaddr %p\n",
		  bp->b_data, bp->b_saveaddr);
	(*pr)("  iodone %p objlock %p\n", bp->b_iodone, bp->b_objlock);
}


void
vfs_vnode_print(struct vnode *vp, int full, void (*pr)(const char *, ...))
{
	char bf[256];

	uvm_object_printit(&vp->v_uobj, full, pr);
	snprintb(bf, sizeof(bf),
	    vnode_flagbits, vp->v_iflag | vp->v_vflag | vp->v_uflag);
	(*pr)("\nVNODE flags %s\n", bf);
	(*pr)("mp %p numoutput %d size 0x%llx writesize 0x%llx\n",
	      vp->v_mount, vp->v_numoutput, vp->v_size, vp->v_writesize);

	(*pr)("data %p writecount %ld holdcnt %ld\n",
	      vp->v_data, vp->v_writecount, vp->v_holdcnt);

	(*pr)("tag %s(%d) type %s(%d) mount %p typedata %p\n",
	      ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	      ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	      vp->v_mount, vp->v_mountedhere);

	(*pr)("v_lock %p v_vnlock %p\n", &vp->v_lock, vp->v_vnlock);

	if (full) {
		struct buf *bp;

		(*pr)("clean bufs:\n");
		LIST_FOREACH(bp, &vp->v_cleanblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}

		(*pr)("dirty bufs:\n");
		LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}
	}
}

void
vfs_mount_print(struct mount *mp, int full, void (*pr)(const char *, ...))
{
	char sbuf[256];

	(*pr)("vnodecovered = %p syncer = %p data = %p\n",
			mp->mnt_vnodecovered,mp->mnt_syncer,mp->mnt_data);

	(*pr)("fs_bshift %d dev_bshift = %d\n",
			mp->mnt_fs_bshift,mp->mnt_dev_bshift);

	snprintb(sbuf, sizeof(sbuf), __MNT_FLAG_BITS, mp->mnt_flag);
	(*pr)("flag = %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), __IMNT_FLAG_BITS, mp->mnt_iflag);
	(*pr)("iflag = %s\n", sbuf);

	(*pr)("refcnt = %d unmounting @ %p updating @ %p\n", mp->mnt_refcnt,
	    &mp->mnt_unmounting, &mp->mnt_updating);

	(*pr)("statvfs cache:\n");
	(*pr)("\tbsize = %lu\n",mp->mnt_stat.f_bsize);
	(*pr)("\tfrsize = %lu\n",mp->mnt_stat.f_frsize);
	(*pr)("\tiosize = %lu\n",mp->mnt_stat.f_iosize);

	(*pr)("\tblocks = %"PRIu64"\n",mp->mnt_stat.f_blocks);
	(*pr)("\tbfree = %"PRIu64"\n",mp->mnt_stat.f_bfree);
	(*pr)("\tbavail = %"PRIu64"\n",mp->mnt_stat.f_bavail);
	(*pr)("\tbresvd = %"PRIu64"\n",mp->mnt_stat.f_bresvd);

	(*pr)("\tfiles = %"PRIu64"\n",mp->mnt_stat.f_files);
	(*pr)("\tffree = %"PRIu64"\n",mp->mnt_stat.f_ffree);
	(*pr)("\tfavail = %"PRIu64"\n",mp->mnt_stat.f_favail);
	(*pr)("\tfresvd = %"PRIu64"\n",mp->mnt_stat.f_fresvd);

	(*pr)("\tf_fsidx = { 0x%"PRIx32", 0x%"PRIx32" }\n",
			mp->mnt_stat.f_fsidx.__fsid_val[0],
			mp->mnt_stat.f_fsidx.__fsid_val[1]);

	(*pr)("\towner = %"PRIu32"\n",mp->mnt_stat.f_owner);
	(*pr)("\tnamemax = %lu\n",mp->mnt_stat.f_namemax);

	snprintb(sbuf, sizeof(sbuf), __MNT_FLAG_BITS, mp->mnt_stat.f_flag);

	(*pr)("\tflag = %s\n",sbuf);
	(*pr)("\tsyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_syncwrites);
	(*pr)("\tasyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_asyncwrites);
	(*pr)("\tsyncreads = %" PRIu64 "\n",mp->mnt_stat.f_syncreads);
	(*pr)("\tasyncreads = %" PRIu64 "\n",mp->mnt_stat.f_asyncreads);
	(*pr)("\tfstypename = %s\n",mp->mnt_stat.f_fstypename);
	(*pr)("\tmntonname = %s\n",mp->mnt_stat.f_mntonname);
	(*pr)("\tmntfromname = %s\n",mp->mnt_stat.f_mntfromname);

	{
		int cnt = 0;
		struct vnode *vp;
		(*pr)("locked vnodes =");
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp)) {
				if ((++cnt % 6) == 0) {
					(*pr)(" %p,\n\t", vp);
				} else {
					(*pr)(" %p,", vp);
				}
			}
		}
		(*pr)("\n");
	}

	if (full) {
		int cnt = 0;
		struct vnode *vp;
		(*pr)("all vnodes =");
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (!TAILQ_NEXT(vp, v_mntvnodes)) {
				(*pr)(" %p", vp);
			} else if ((++cnt % 6) == 0) {
				(*pr)(" %p,\n\t", vp);
			} else {
				(*pr)(" %p,", vp);
			}
		}
		(*pr)("\n", vp);
	}
}
#endif /* DDB || DEBUGPRINT */

/*
 * Check if a device pointed to by vp is mounted.
 *
 * Returns:
 *   EINVAL	if it's not a disk
 *   EBUSY	if it's a disk and mounted
 *   0		if it's a disk and not mounted
 */
int
rawdev_mounted(struct vnode *vp, struct vnode **bvpp)
{
	struct vnode *bvp;
	dev_t dev;
	int d_type;

	bvp = NULL;
	dev = vp->v_rdev;
	d_type = D_OTHER;

	if (iskmemvp(vp))
		return EINVAL;

	switch (vp->v_type) {
	case VCHR: {
		const struct cdevsw *cdev;

		cdev = cdevsw_lookup(dev);
		if (cdev != NULL) {
			dev_t blkdev;

			blkdev = devsw_chr2blk(dev);
			if (blkdev != NODEV) {
				vfinddev(blkdev, VBLK, &bvp);
				if (bvp != NULL)
					d_type = (cdev->d_flag & D_TYPEMASK);
			}
		}

		break;
		}

	case VBLK: {
		const struct bdevsw *bdev;

		bdev = bdevsw_lookup(dev);
		if (bdev != NULL)
			d_type = (bdev->d_flag & D_TYPEMASK);

		bvp = vp;

		break;
		}

	default:
		break;
	}

	if (d_type != D_DISK)
		return EINVAL;

	if (bvpp != NULL)
		*bvpp = bvp;

	/*
	 * XXX: This is bogus. We should be failing the request
	 * XXX: not only if this specific slice is mounted, but
	 * XXX: if it's on a disk with any other mounted slice.
	 */
	if (vfs_mountedon(bvp))
		return EBUSY;

	return 0;
}
