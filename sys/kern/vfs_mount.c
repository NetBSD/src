/*	$NetBSD: vfs_mount.c,v 1.93 2022/04/09 23:38:33 riastradh Exp $	*/

/*-
 * Copyright (c) 1997-2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_mount.c,v 1.93 2022/04/09 23:38:33 riastradh Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/device.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/fstrans.h>
#include <sys/namei.h>
#include <sys/extattr.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/vfs_syscalls.h>
#include <sys/vnode_impl.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_swap.h>

enum mountlist_type {
	ME_MOUNT,
	ME_MARKER
};
struct mountlist_entry {
	TAILQ_ENTRY(mountlist_entry) me_list;	/* Mount list. */
	struct mount *me_mount;			/* Actual mount if ME_MOUNT,
						   current mount else. */
	enum mountlist_type me_type;		/* Mount or marker. */
};
struct mount_iterator {
	struct mountlist_entry mi_entry;
};

static struct vnode *vfs_vnode_iterator_next1(struct vnode_iterator *,
    bool (*)(void *, struct vnode *), void *, bool);

/* Root filesystem. */
vnode_t *			rootvnode;

/* Mounted filesystem list. */
static TAILQ_HEAD(mountlist, mountlist_entry) mountlist;
static kmutex_t			mountlist_lock __cacheline_aligned;
int vnode_offset_next_by_lru	/* XXX: ugly hack for pstat.c */
    = offsetof(vnode_impl_t, vi_lrulist.tqe_next);

kmutex_t			vfs_list_lock __cacheline_aligned;

static specificdata_domain_t	mount_specificdata_domain;
static kmutex_t			mntid_lock;

static kmutex_t			mountgen_lock __cacheline_aligned;
static uint64_t			mountgen;

void
vfs_mount_sysinit(void)
{

	TAILQ_INIT(&mountlist);
	mutex_init(&mountlist_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&vfs_list_lock, MUTEX_DEFAULT, IPL_NONE);

	mount_specificdata_domain = specificdata_domain_create();
	mutex_init(&mntid_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mountgen_lock, MUTEX_DEFAULT, IPL_NONE);
	mountgen = 0;
}

struct mount *
vfs_mountalloc(struct vfsops *vfsops, vnode_t *vp)
{
	struct mount *mp;
	int error __diagused;

	mp = kmem_zalloc(sizeof(*mp), KM_SLEEP);
	mp->mnt_op = vfsops;
	mp->mnt_refcnt = 1;
	TAILQ_INIT(&mp->mnt_vnodelist);
	mp->mnt_renamelock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	mp->mnt_vnodelock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	mp->mnt_updating = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	mp->mnt_vnodecovered = vp;
	mount_initspecific(mp);

	error = fstrans_mount(mp);
	KASSERT(error == 0);

	mutex_enter(&mountgen_lock);
	mp->mnt_gen = mountgen++;
	mutex_exit(&mountgen_lock);

	return mp;
}

/*
 * vfs_rootmountalloc: lookup a filesystem type, and if found allocate and
 * initialize a mount structure for it.
 *
 * Devname is usually updated by mount(8) after booting.
 */
int
vfs_rootmountalloc(const char *fstypename, const char *devname,
    struct mount **mpp)
{
	struct vfsops *vfsp = NULL;
	struct mount *mp;
	int error __diagused;

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
	error = vfs_busy(mp);
	KASSERT(error == 0);
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
	return 0;
}

/*
 * vfs_getnewfsid: get a new unique fsid.
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
	while (vfs_getvfs(&tfsid)) {
		tfsid.__fsid_val[0]++;
		xxxfs_mntid++;
	}
	mp->mnt_stat.f_fsidx.__fsid_val[0] = tfsid.__fsid_val[0];
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mutex_exit(&mntid_lock);
}

/*
 * Lookup a mount point by filesystem identifier.
 *
 * XXX Needs to add a reference to the mount point.
 */
struct mount *
vfs_getvfs(fsid_t *fsid)
{
	mount_iterator_t *iter;
	struct mount *mp;

	mountlist_iterator_init(&iter);
	while ((mp = mountlist_iterator_next(iter)) != NULL) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			mountlist_iterator_destroy(iter);
			return mp;
		}
	}
	mountlist_iterator_destroy(iter);
	return NULL;
}

/*
 * Take a reference to a mount structure.
 */
void
vfs_ref(struct mount *mp)
{

	KASSERT(mp->mnt_refcnt > 0 || mutex_owned(&mountlist_lock));

	atomic_inc_uint(&mp->mnt_refcnt);
}

/*
 * Drop a reference to a mount structure, freeing if the last reference.
 */
void
vfs_rele(struct mount *mp)
{

#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif
	if (__predict_true((int)atomic_dec_uint_nv(&mp->mnt_refcnt) > 0)) {
		return;
	}
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif

	/*
	 * Nothing else has visibility of the mount: we can now
	 * free the data structures.
	 */
	KASSERT(mp->mnt_refcnt == 0);
	specificdata_fini(mount_specificdata_domain, &mp->mnt_specdataref);
	mutex_obj_free(mp->mnt_updating);
	mutex_obj_free(mp->mnt_renamelock);
	mutex_obj_free(mp->mnt_vnodelock);
	if (mp->mnt_op != NULL) {
		vfs_delref(mp->mnt_op);
	}
	fstrans_unmount(mp);
	/*
	 * Final free of mp gets done from fstrans_mount_dtor().
	 *
	 * Prevents this memory to be reused as a mount before
	 * fstrans releases all references to it.
	 */
}

/*
 * Mark a mount point as busy, and gain a new reference to it.  Used to
 * prevent the file system from being unmounted during critical sections.
 *
 * vfs_busy can be called multiple times and by multiple threads
 * and must be accompanied by the same number of vfs_unbusy calls.
 *
 * => The caller must hold a pre-existing reference to the mount.
 * => Will fail if the file system is being unmounted, or is unmounted.
 */
static inline int
_vfs_busy(struct mount *mp, bool wait)
{

	KASSERT(mp->mnt_refcnt > 0);

	if (wait) {
		fstrans_start(mp);
	} else {
		if (fstrans_start_nowait(mp))
			return EBUSY;
	}
	if (__predict_false((mp->mnt_iflag & IMNT_GONE) != 0)) {
		fstrans_done(mp);
		return ENOENT;
	}
	vfs_ref(mp);
	return 0;
}

int
vfs_busy(struct mount *mp)
{

	return _vfs_busy(mp, true);
}

int
vfs_trybusy(struct mount *mp)
{

	return _vfs_busy(mp, false);
}

/*
 * Unbusy a busy filesystem.
 *
 * Every successful vfs_busy() call must be undone by a vfs_unbusy() call.
 */
void
vfs_unbusy(struct mount *mp)
{

	KASSERT(mp->mnt_refcnt > 0);

	fstrans_done(mp);
	vfs_rele(mp);
}

struct vnode_iterator {
	vnode_impl_t vi_vnode;
};

void
vfs_vnode_iterator_init(struct mount *mp, struct vnode_iterator **vnip)
{
	vnode_t *vp;
	vnode_impl_t *vip;

	vp = vnalloc_marker(mp);
	vip = VNODE_TO_VIMPL(vp);

	mutex_enter(mp->mnt_vnodelock);
	TAILQ_INSERT_HEAD(&mp->mnt_vnodelist, vip, vi_mntvnodes);
	vp->v_usecount = 1;
	mutex_exit(mp->mnt_vnodelock);

	*vnip = (struct vnode_iterator *)vip;
}

void
vfs_vnode_iterator_destroy(struct vnode_iterator *vni)
{
	vnode_impl_t *mvip = &vni->vi_vnode;
	vnode_t *mvp = VIMPL_TO_VNODE(mvip);
	kmutex_t *lock;

	KASSERT(vnis_marker(mvp));
	if (vrefcnt(mvp) != 0) {
		lock = mvp->v_mount->mnt_vnodelock;
		mutex_enter(lock);
		TAILQ_REMOVE(&mvp->v_mount->mnt_vnodelist, mvip, vi_mntvnodes);
		mvp->v_usecount = 0;
		mutex_exit(lock);
	}
	vnfree_marker(mvp);
}

static struct vnode *
vfs_vnode_iterator_next1(struct vnode_iterator *vni,
    bool (*f)(void *, struct vnode *), void *cl, bool do_wait)
{
	vnode_impl_t *mvip = &vni->vi_vnode;
	struct mount *mp = VIMPL_TO_VNODE(mvip)->v_mount;
	vnode_t *vp;
	vnode_impl_t *vip;
	kmutex_t *lock;
	int error;

	KASSERT(vnis_marker(VIMPL_TO_VNODE(mvip)));

	lock = mp->mnt_vnodelock;
	do {
		mutex_enter(lock);
		vip = TAILQ_NEXT(mvip, vi_mntvnodes);
		TAILQ_REMOVE(&mp->mnt_vnodelist, mvip, vi_mntvnodes);
		VIMPL_TO_VNODE(mvip)->v_usecount = 0;
again:
		if (vip == NULL) {
			mutex_exit(lock);
	       		return NULL;
		}
		vp = VIMPL_TO_VNODE(vip);
		KASSERT(vp != NULL);
		mutex_enter(vp->v_interlock);
		if (vnis_marker(vp) ||
		    vdead_check(vp, (do_wait ? 0 : VDEAD_NOWAIT)) ||
		    (f && !(*f)(cl, vp))) {
			mutex_exit(vp->v_interlock);
			vip = TAILQ_NEXT(vip, vi_mntvnodes);
			goto again;
		}

		TAILQ_INSERT_AFTER(&mp->mnt_vnodelist, vip, mvip, vi_mntvnodes);
		VIMPL_TO_VNODE(mvip)->v_usecount = 1;
		mutex_exit(lock);
		error = vcache_vget(vp);
		KASSERT(error == 0 || error == ENOENT);
	} while (error != 0);

	return vp;
}

struct vnode *
vfs_vnode_iterator_next(struct vnode_iterator *vni,
    bool (*f)(void *, struct vnode *), void *cl)
{

	return vfs_vnode_iterator_next1(vni, f, cl, false);
}

/*
 * Move a vnode from one mount queue to another.
 */
void
vfs_insmntque(vnode_t *vp, struct mount *mp)
{
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	struct mount *omp;
	kmutex_t *lock;

	KASSERT(mp == NULL || (mp->mnt_iflag & IMNT_UNMOUNT) == 0 ||
	    vp->v_tag == VT_VFS);

	/*
	 * Delete from old mount point vnode list, if on one.
	 */
	if ((omp = vp->v_mount) != NULL) {
		lock = omp->mnt_vnodelock;
		mutex_enter(lock);
		TAILQ_REMOVE(&vp->v_mount->mnt_vnodelist, vip, vi_mntvnodes);
		mutex_exit(lock);
	}

	/*
	 * Insert into list of vnodes for the new mount point, if
	 * available.  The caller must take a reference on the mount
	 * structure and donate to the vnode.
	 */
	if ((vp->v_mount = mp) != NULL) {
		lock = mp->mnt_vnodelock;
		mutex_enter(lock);
		TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vip, vi_mntvnodes);
		mutex_exit(lock);
	}

	if (omp != NULL) {
		/* Release reference to old mount. */
		vfs_rele(omp);
	}
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
 * SKIPSYSTEM causes any vnodes marked VV_SYSTEM to be skipped.
 */
#ifdef DEBUG
int busyprt = 0;	/* print out busy vnodes */
struct ctldebug debug1 = { "busyprt", &busyprt };
#endif

static vnode_t *
vflushnext(struct vnode_iterator *marker, int *when)
{
	if (getticks() > *when) {
		yield();
		*when = getticks() + hz / 10;
	}
	return vfs_vnode_iterator_next1(marker, NULL, NULL, true);
}

/*
 * Flush one vnode.  Referenced on entry, unreferenced on return.
 */
static int
vflush_one(vnode_t *vp, vnode_t *skipvp, int flags)
{
	int error;
	struct vattr vattr;

	if (vp == skipvp ||
	    ((flags & SKIPSYSTEM) && (vp->v_vflag & VV_SYSTEM))) {
		vrele(vp);
		return 0;
	}
	/*
	 * If WRITECLOSE is set, only flush out regular file
	 * vnodes open for writing or open and unlinked.
	 */
	if ((flags & WRITECLOSE)) {
		if (vp->v_type != VREG) {
			vrele(vp);
			return 0;
		}
		error = vn_lock(vp, LK_EXCLUSIVE);
		if (error) {
			KASSERT(error == ENOENT);
			vrele(vp);
			return 0;
		}
		error = VOP_FSYNC(vp, curlwp->l_cred, FSYNC_WAIT, 0, 0);
		if (error == 0)
			error = VOP_GETATTR(vp, &vattr, curlwp->l_cred);
		VOP_UNLOCK(vp);
		if (error) {
			vrele(vp);
			return error;
		}
		if (vp->v_writecount == 0 && vattr.va_nlink > 0) {
			vrele(vp);
			return 0;
		}
	}
	/*
	 * First try to recycle the vnode.
	 */
	if (vrecycle(vp))
		return 0;
	/*
	 * If FORCECLOSE is set, forcibly close the vnode.
	 * For block or character devices, revert to an
	 * anonymous device.  For all other files, just
	 * kill them.
	 */
	if (flags & FORCECLOSE) {
		if (vrefcnt(vp) > 1 &&
		    (vp->v_type == VBLK || vp->v_type == VCHR))
			vcache_make_anon(vp);
		else
			vgone(vp);
		return 0;
	}
	vrele(vp);
	return EBUSY;
}

int
vflush(struct mount *mp, vnode_t *skipvp, int flags)
{
	vnode_t *vp;
	struct vnode_iterator *marker;
	int busy, error, when, retries = 2;

	do {
		busy = error = when = 0;

		/*
		 * First, flush out any vnode references from the
		 * deferred vrele list.
		 */
		vrele_flush(mp);

		vfs_vnode_iterator_init(mp, &marker);

		while ((vp = vflushnext(marker, &when)) != NULL) {
			error = vflush_one(vp, skipvp, flags);
			if (error == EBUSY) {
				error = 0;
				busy++;
#ifdef DEBUG
				if (busyprt && retries == 0)
					vprint("vflush: busy vnode", vp);
#endif
			} else if (error != 0) {
				break;
			}
		}

		vfs_vnode_iterator_destroy(marker);
	} while (error == 0 && busy > 0 && retries-- > 0);

	if (error)
		return error;
	if (busy)
		return EBUSY;
	return 0;
}

/*
 * Mount a file system.
 */

/*
 * Scan all active processes to see if any of them have a current or root
 * directory onto which the new filesystem has just been  mounted. If so,
 * replace them with the new mount point.
 */
static void
mount_checkdirs(vnode_t *olddp)
{
	vnode_t *newdp, *rele1, *rele2;
	struct cwdinfo *cwdi;
	struct proc *p;
	bool retry;

	if (vrefcnt(olddp) == 1) {
		return;
	}
	if (VFS_ROOT(olddp->v_mountedhere, LK_EXCLUSIVE, &newdp))
		panic("mount: lost mount");

	do {
		retry = false;
		mutex_enter(&proc_lock);
		PROCLIST_FOREACH(p, &allproc) {
			if ((cwdi = p->p_cwdi) == NULL)
				continue;
			/*
			 * Cannot change to the old directory any more,
			 * so even if we see a stale value it is not a
			 * problem.
			 */
			if (cwdi->cwdi_cdir != olddp &&
			    cwdi->cwdi_rdir != olddp)
				continue;
			retry = true;
			rele1 = NULL;
			rele2 = NULL;
			atomic_inc_uint(&cwdi->cwdi_refcnt);
			mutex_exit(&proc_lock);
			rw_enter(&cwdi->cwdi_lock, RW_WRITER);
			if (cwdi->cwdi_cdir == olddp) {
				rele1 = cwdi->cwdi_cdir;
				vref(newdp);
				cwdi->cwdi_cdir = newdp;
			}
			if (cwdi->cwdi_rdir == olddp) {
				rele2 = cwdi->cwdi_rdir;
				vref(newdp);
				cwdi->cwdi_rdir = newdp;
			}
			rw_exit(&cwdi->cwdi_lock);
			cwdfree(cwdi);
			if (rele1 != NULL)
				vrele(rele1);
			if (rele2 != NULL)
				vrele(rele2);
			mutex_enter(&proc_lock);
			break;
		}
		mutex_exit(&proc_lock);
	} while (retry);

	if (rootvnode == olddp) {
		vrele(rootvnode);
		vref(newdp);
		rootvnode = newdp;
	}
	vput(newdp);
}

/*
 * Start extended attributes
 */
static int
start_extattr(struct mount *mp)
{
	int error;

	error = VFS_EXTATTRCTL(mp, EXTATTR_CMD_START, NULL, 0, NULL);
	if (error) 
		printf("%s: failed to start extattr: error = %d\n",
		       mp->mnt_stat.f_mntonname, error);

	return error;
}

int
mount_domount(struct lwp *l, vnode_t **vpp, struct vfsops *vfsops,
    const char *path, int flags, void *data, size_t *data_len)
{
	vnode_t *vp = *vpp;
	struct mount *mp;
	struct pathbuf *pb;
	struct nameidata nd;
	int error, error2;

	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_NEW, vp, KAUTH_ARG(flags), data);
	if (error) {
		vfs_delref(vfsops);
		return error;
	}

	/* Cannot make a non-dir a mount-point (from here anyway). */
	if (vp->v_type != VDIR) {
		vfs_delref(vfsops);
		return ENOTDIR;
	}

	if (flags & MNT_EXPORTED) {
		vfs_delref(vfsops);
		return EINVAL;
	}

	if ((mp = vfs_mountalloc(vfsops, vp)) == NULL) {
		vfs_delref(vfsops);
		return ENOMEM;
	}

	mp->mnt_stat.f_owner = kauth_cred_geteuid(l->l_cred);

	/*
	 * The underlying file system may refuse the mount for
	 * various reasons.  Allow the user to force it to happen.
	 *
	 * Set the mount level flags.
	 */
	mp->mnt_flag = flags & (MNT_BASIC_FLAGS | MNT_FORCE | MNT_IGNORE);

	mutex_enter(mp->mnt_updating);
	error = VFS_MOUNT(mp, path, data, data_len);
	mp->mnt_flag &= ~MNT_OP_FLAGS;

	if (error != 0)
		goto err_unmounted;

	/*
	 * Validate and prepare the mount point.
	 */
	error = pathbuf_copyin(path, &pb);
	if (error != 0) {
		goto err_mounted;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	pathbuf_destroy(pb);
	if (error != 0) {
		goto err_mounted;
	}
	if (nd.ni_vp != vp) {
		vput(nd.ni_vp);
		error = EINVAL;
		goto err_mounted;
	}
	if (vp->v_mountedhere != NULL) {
		vput(nd.ni_vp);
		error = EBUSY;
		goto err_mounted;
	}
	error = vinvalbuf(vp, V_SAVE, l->l_cred, l, 0, 0);
	if (error != 0) {
		vput(nd.ni_vp);
		goto err_mounted;
	}

	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(vp);
	mp->mnt_iflag &= ~IMNT_WANTRDWR;

	mountlist_append(mp);
	if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
		vfs_syncer_add_to_worklist(mp);
	vp->v_mountedhere = mp;
	vput(nd.ni_vp);

	mount_checkdirs(vp);
	mutex_exit(mp->mnt_updating);

	/* Hold an additional reference to the mount across VFS_START(). */
	vfs_ref(mp);
	(void) VFS_STATVFS(mp, &mp->mnt_stat);
	error = VFS_START(mp, 0);
	if (error) {
		vrele(vp);
	} else if (flags & MNT_EXTATTR) {
		if (start_extattr(mp) != 0)
			mp->mnt_flag &= ~MNT_EXTATTR;
	}
	/* Drop reference held for VFS_START(). */
	vfs_rele(mp);
	*vpp = NULL;
	return error;

err_mounted:
	do {
		error2 = vfs_suspend(mp, 0);
	} while (error2 == EINTR || error2 == ERESTART);
	KASSERT(error2 == 0 || error2 == EOPNOTSUPP);

	if (VFS_UNMOUNT(mp, MNT_FORCE) != 0)
		panic("Unmounting fresh file system failed");

	if (error2 == 0)
		vfs_resume(mp);

err_unmounted:
	mutex_exit(mp->mnt_updating);
	vfs_rele(mp);

	return error;
}

/*
 * Do the actual file system unmount.  File system is assumed to have
 * been locked by the caller.
 *
 * => Caller hold reference to the mount, explicitly for dounmount().
 */
int
dounmount(struct mount *mp, int flags, struct lwp *l)
{
	vnode_t *coveredvp;
	int error, async, used_syncer, used_extattr;
	const bool was_suspended = fstrans_is_owner(mp);

#if NVERIEXEC > 0
	error = veriexec_unmountchk(mp);
	if (error)
		return (error);
#endif /* NVERIEXEC > 0 */

	if (!was_suspended) {
		error = vfs_suspend(mp, 0);
		if (error) {
			return error;
		}
	}

	KASSERT((mp->mnt_iflag & IMNT_GONE) == 0);

	used_syncer = (mp->mnt_iflag & IMNT_ONWORKLIST) != 0;
	used_extattr = mp->mnt_flag & MNT_EXTATTR;

	mp->mnt_iflag |= IMNT_UNMOUNT;
	mutex_enter(mp->mnt_updating);
	async = mp->mnt_flag & MNT_ASYNC;
	mp->mnt_flag &= ~MNT_ASYNC;
	cache_purgevfs(mp);	/* remove cache entries for this file sys */
	if (used_syncer)
		vfs_syncer_remove_from_worklist(mp);
	error = 0;
	if (((mp->mnt_flag & MNT_RDONLY) == 0) && ((flags & MNT_FORCE) == 0)) {
		error = VFS_SYNC(mp, MNT_WAIT, l->l_cred);
	}
	if (error == 0 || (flags & MNT_FORCE)) {
		error = VFS_UNMOUNT(mp, flags);
	}
	if (error) {
		mp->mnt_iflag &= ~IMNT_UNMOUNT;
		if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
			vfs_syncer_add_to_worklist(mp);
		mp->mnt_flag |= async;
		mutex_exit(mp->mnt_updating);
		if (!was_suspended)
			vfs_resume(mp);
		if (used_extattr) {
			if (start_extattr(mp) != 0)
				mp->mnt_flag &= ~MNT_EXTATTR;
			else
				mp->mnt_flag |= MNT_EXTATTR;
		}
		return (error);
	}
	mutex_exit(mp->mnt_updating);

	/*
	 * mark filesystem as gone to prevent further umounts
	 * after mnt_umounting lock is gone, this also prevents
	 * vfs_busy() from succeeding.
	 */
	mp->mnt_iflag |= IMNT_GONE;
	if (!was_suspended)
		vfs_resume(mp);

	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP) {
		vn_lock(coveredvp, LK_EXCLUSIVE | LK_RETRY);
		coveredvp->v_mountedhere = NULL;
		VOP_UNLOCK(coveredvp);
	}
	mountlist_remove(mp);
	if (TAILQ_FIRST(&mp->mnt_vnodelist) != NULL)
		panic("unmount: dangling vnode");
	vfs_hooks_unmount(mp);

	vfs_rele(mp);	/* reference from mount() */
	if (coveredvp != NULLVP) {
		vrele(coveredvp);
	}
	return (0);
}

/*
 * Unmount all file systems.
 * We traverse the list in reverse order under the assumption that doing so
 * will avoid needing to worry about dependencies.
 */
bool
vfs_unmountall(struct lwp *l)
{

	printf("unmounting file systems...\n");
	return vfs_unmountall1(l, true, true);
}

static void
vfs_unmount_print(struct mount *mp, const char *pfx)
{

	aprint_verbose("%sunmounted %s on %s type %s\n", pfx,
	    mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname,
	    mp->mnt_stat.f_fstypename);
}

/*
 * Return the mount with the highest generation less than "gen".
 */
static struct mount *
vfs_unmount_next(uint64_t gen)
{
	mount_iterator_t *iter;
	struct mount *mp, *nmp;

	nmp = NULL;

	mountlist_iterator_init(&iter);
	while ((mp = mountlist_iterator_next(iter)) != NULL) {
		if ((nmp == NULL || mp->mnt_gen > nmp->mnt_gen) && 
		    mp->mnt_gen < gen) {
			if (nmp != NULL)
				vfs_rele(nmp);
			nmp = mp;
			vfs_ref(nmp);
		}
	}
	mountlist_iterator_destroy(iter);

	return nmp;
}

bool
vfs_unmount_forceone(struct lwp *l)
{
	struct mount *mp;
	int error;

	mp = vfs_unmount_next(mountgen);
	if (mp == NULL) {
		return false;
	}

#ifdef DEBUG
	printf("forcefully unmounting %s (%s)...\n",
	    mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
#endif
	if ((error = dounmount(mp, MNT_FORCE, l)) == 0) {
		vfs_unmount_print(mp, "forcefully ");
		return true;
	} else {
		vfs_rele(mp);
	}

#ifdef DEBUG
	printf("forceful unmount of %s failed with error %d\n",
	    mp->mnt_stat.f_mntonname, error);
#endif

	return false;
}

bool
vfs_unmountall1(struct lwp *l, bool force, bool verbose)
{
	struct mount *mp;
	mount_iterator_t *iter;
	bool any_error = false, progress = false;
	uint64_t gen;
	int error;

	gen = mountgen;
	for (;;) {
		mp = vfs_unmount_next(gen);
		if (mp == NULL)
			break;
		gen = mp->mnt_gen;

#ifdef DEBUG
		printf("unmounting %p %s (%s)...\n",
		    (void *)mp, mp->mnt_stat.f_mntonname,
		    mp->mnt_stat.f_mntfromname);
#endif
		if ((error = dounmount(mp, force ? MNT_FORCE : 0, l)) == 0) {
			vfs_unmount_print(mp, "");
			progress = true;
		} else {
			vfs_rele(mp);
			if (verbose) {
				printf("unmount of %s failed with error %d\n",
				    mp->mnt_stat.f_mntonname, error);
			}
			any_error = true;
		}
	}
	if (verbose) {
		printf("unmounting done\n");
	}
	if (any_error && verbose) {
		printf("WARNING: some file systems would not unmount\n");
	}
	/* If the mountlist is empty it is time to remove swap. */
	mountlist_iterator_init(&iter);
	if (mountlist_iterator_next(iter) == NULL) {
		uvm_swap_shutdown(l);
	}
	mountlist_iterator_destroy(iter);

	return progress;
}

void
vfs_sync_all(struct lwp *l)
{
	printf("syncing disks... ");

	/* remove user processes from run queue */
	suspendsched();
	(void)spl0();

	/* avoid coming back this way again if we panic. */
	doing_shutdown = 1;

	do_sys_sync(l);

	/* Wait for sync to finish. */
	if (vfs_syncwait() != 0) {
#if defined(DDB) && defined(DEBUG_HALT_BUSY)
		Debugger();
#endif
		printf("giving up\n");
		return;
	} else
		printf("done\n");
}

/*
 * Sync and unmount file systems before shutting down.
 */
void
vfs_shutdown(void)
{
	lwp_t *l = curlwp;

	vfs_sync_all(l);

	/*
	 * If we have panicked - do not make the situation potentially
	 * worse by unmounting the file systems.
	 */
	if (panicstr != NULL) {
		return;
	}

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
		vn_lock(rootvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_OPEN(rootvp, FREAD, FSCRED);
		VOP_UNLOCK(rootvp);
		if (error) {
			printf("vfs_mountroot: can't open root device\n");
			return (error);
		}
		break;

	case DV_VIRTUAL:
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
		vn_lock(rootvp, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(rootvp, FREAD, FSCRED);
		VOP_UNLOCK(rootvp);
		vrele(rootvp);
	}
	if (error == 0) {
		mount_iterator_t *iter;
		struct mount *mp;
		extern struct cwdinfo cwdi0;

		mountlist_iterator_init(&iter);
		mp = mountlist_iterator_next(iter);
		KASSERT(mp != NULL);
		mountlist_iterator_destroy(iter);

		mp->mnt_flag |= MNT_ROOTFS;
		mp->mnt_op->vfs_refcount++;

		/*
		 * Get the vnode for '/'.  Set cwdi0.cwdi_cdir to
		 * reference it, and donate it the reference grabbed
		 * with VFS_ROOT().
		 */
		error = VFS_ROOT(mp, LK_NONE, &rootvnode);
		if (error)
			panic("cannot find root vnode, error=%d", error);
		cwdi0.cwdi_cdir = rootvnode;
		cwdi0.cwdi_rdir = NULL;

		/*
		 * Now that root is mounted, we can fixup initproc's CWD
		 * info.  All other processes are kthreads, which merely
		 * share proc0's CWD info.
		 */
		initproc->p_cwdi->cwdi_cdir = rootvnode;
		vref(initproc->p_cwdi->cwdi_cdir);
		initproc->p_cwdi->cwdi_rdir = NULL;
		/*
		 * Enable loading of modules from the filesystem
		 */
		module_load_vfs_init();

	}
	return (error);
}

/*
 * mount_specific_key_create --
 *	Create a key for subsystem mount-specific data.
 */
int
mount_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return specificdata_key_create(mount_specificdata_domain, keyp, dtor);
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
	int error __diagused;

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

	return specificdata_getspecific(mount_specificdata_domain,
					 &mp->mnt_specdataref, key);
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
	if (spec_node_getmountedfs(vp) != NULL)
		return EBUSY;
	if (spec_node_lookup_by_dev(vp->v_type, vp->v_rdev, VDEAD_NOWAIT, &vq)
	    == 0) {
		if (spec_node_getmountedfs(vq) != NULL)
			error = EBUSY;
		vrele(vq);
	}

	return error;
}

/*
 * Check if a device pointed to by vp is mounted.
 *
 * Returns:
 *   EINVAL	if it's not a disk
 *   EBUSY	if it's a disk and mounted
 *   0		if it's a disk and not mounted
 */
int
rawdev_mounted(vnode_t *vp, vnode_t **bvpp)
{
	vnode_t *bvp;
	dev_t dev;
	int d_type;

	bvp = NULL;
	d_type = D_OTHER;

	if (iskmemvp(vp))
		return EINVAL;

	switch (vp->v_type) {
	case VCHR: {
		const struct cdevsw *cdev;

		dev = vp->v_rdev;
		cdev = cdevsw_lookup(dev);
		if (cdev != NULL) {
			dev_t blkdev;

			blkdev = devsw_chr2blk(dev);
			if (blkdev != NODEV) {
				if (vfinddev(blkdev, VBLK, &bvp) != 0) {
					d_type = (cdev->d_flag & D_TYPEMASK);
					/* XXX: what if bvp disappears? */
					vrele(bvp);
				}
			}
		}

		break;
		}

	case VBLK: {
		const struct bdevsw *bdev;

		dev = vp->v_rdev;
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

static struct mountlist_entry *
mountlist_alloc(enum mountlist_type type, struct mount *mp)
{
	struct mountlist_entry *me;

	me = kmem_zalloc(sizeof(*me), KM_SLEEP);
	me->me_mount = mp;
	me->me_type = type;

	return me;
}

static void
mountlist_free(struct mountlist_entry *me)
{

	kmem_free(me, sizeof(*me));
}

void
mountlist_iterator_init(mount_iterator_t **mip)
{
	struct mountlist_entry *me;

	me = mountlist_alloc(ME_MARKER, NULL);
	mutex_enter(&mountlist_lock);
	TAILQ_INSERT_HEAD(&mountlist, me, me_list);
	mutex_exit(&mountlist_lock);
	*mip = (mount_iterator_t *)me;
}

void
mountlist_iterator_destroy(mount_iterator_t *mi)
{
	struct mountlist_entry *marker = &mi->mi_entry;

	if (marker->me_mount != NULL)
		vfs_unbusy(marker->me_mount);

	mutex_enter(&mountlist_lock);
	TAILQ_REMOVE(&mountlist, marker, me_list);
	mutex_exit(&mountlist_lock);

	mountlist_free(marker);

}

/*
 * Return the next mount or NULL for this iterator.
 * Mark it busy on success.
 */
static inline struct mount *
_mountlist_iterator_next(mount_iterator_t *mi, bool wait)
{
	struct mountlist_entry *me, *marker = &mi->mi_entry;
	struct mount *mp;
	int error;

	if (marker->me_mount != NULL) {
		vfs_unbusy(marker->me_mount);
		marker->me_mount = NULL;
	}

	mutex_enter(&mountlist_lock);
	for (;;) {
		KASSERT(marker->me_type == ME_MARKER);

		me = TAILQ_NEXT(marker, me_list);
		if (me == NULL) {
			/* End of list: keep marker and return. */
			mutex_exit(&mountlist_lock);
			return NULL;
		}
		TAILQ_REMOVE(&mountlist, marker, me_list);
		TAILQ_INSERT_AFTER(&mountlist, me, marker, me_list);

		/* Skip other markers. */
		if (me->me_type != ME_MOUNT)
			continue;

		/* Take an initial reference for vfs_busy() below. */
		mp = me->me_mount;
		KASSERT(mp != NULL);
		vfs_ref(mp);
		mutex_exit(&mountlist_lock);

		/* Try to mark this mount busy and return on success. */
		if (wait)
			error = vfs_busy(mp);
		else
			error = vfs_trybusy(mp);
		if (error == 0) {
			vfs_rele(mp);
			marker->me_mount = mp;
			return mp;
		}
		vfs_rele(mp);
		mutex_enter(&mountlist_lock);
	}
}

struct mount *
mountlist_iterator_next(mount_iterator_t *mi)
{

	return _mountlist_iterator_next(mi, true);
}

struct mount *
mountlist_iterator_trynext(mount_iterator_t *mi)
{

	return _mountlist_iterator_next(mi, false);
}

/*
 * Attach new mount to the end of the mount list.
 */
void
mountlist_append(struct mount *mp)
{
	struct mountlist_entry *me;

	me = mountlist_alloc(ME_MOUNT, mp);
	mutex_enter(&mountlist_lock);
	TAILQ_INSERT_TAIL(&mountlist, me, me_list);
	mutex_exit(&mountlist_lock);
}

/*
 * Remove mount from mount list.
 */void
mountlist_remove(struct mount *mp)
{
	struct mountlist_entry *me;

	mutex_enter(&mountlist_lock);
	TAILQ_FOREACH(me, &mountlist, me_list)
		if (me->me_type == ME_MOUNT && me->me_mount == mp)
			break;
	KASSERT(me != NULL);
	TAILQ_REMOVE(&mountlist, me, me_list);
	mutex_exit(&mountlist_lock);
	mountlist_free(me);
}

/*
 * Unlocked variant to traverse the mountlist.
 * To be used from DDB only.
 */
struct mount *
_mountlist_next(struct mount *mp)
{
	struct mountlist_entry *me;

	if (mp == NULL) {
		me = TAILQ_FIRST(&mountlist);
	} else {
		TAILQ_FOREACH(me, &mountlist, me_list)
			if (me->me_type == ME_MOUNT && me->me_mount == mp)
				break;
		if (me != NULL)
			me = TAILQ_NEXT(me, me_list);
	}

	while (me != NULL && me->me_type != ME_MOUNT)
		me = TAILQ_NEXT(me, me_list);

	return (me ? me->me_mount : NULL);
}
