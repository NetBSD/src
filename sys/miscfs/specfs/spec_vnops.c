/*	$NetBSD: spec_vnops.c,v 1.205 2022/03/28 12:37:18 riastradh Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *	@(#)spec_vnops.c	8.15 (Berkeley) 7/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spec_vnops.c,v 1.205 2022/03/28 12:37:18 riastradh Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode_impl.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/lockf.h>
#include <sys/tty.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>
#include <sys/module.h>
#include <sys/atomic.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

/*
 * Lock order:
 *
 *	vnode lock
 *	-> device_lock
 *	-> struct vnode::v_interlock
 */

/* symbolic sleep message strings for devices */
const char	devopn[] = "devopn";
const char	devio[] = "devio";
const char	devwait[] = "devwait";
const char	devin[] = "devin";
const char	devout[] = "devout";
const char	devioc[] = "devioc";
const char	devcls[] = "devcls";

#define	SPECHSZ	64
#if	((SPECHSZ&(SPECHSZ-1)) == 0)
#define	SPECHASH(rdev)	(((rdev>>5)+(rdev))&(SPECHSZ-1))
#else
#define	SPECHASH(rdev)	(((unsigned)((rdev>>5)+(rdev)))%SPECHSZ)
#endif

static vnode_t	*specfs_hash[SPECHSZ];
extern struct mount *dead_rootmount;

/*
 * This vnode operations vector is used for special device nodes
 * created from whole cloth by the kernel.  For the ops vector for
 * vnodes built from special devices found in a filesystem, see (e.g)
 * ffs_specop_entries[] in ffs_vnops.c or the equivalent for other
 * filesystems.
 */

int (**spec_vnodeop_p)(void *);
const struct vnodeopv_entry_desc spec_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_parsepath_desc, genfs_parsepath },	/* parsepath */
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, genfs_badop },		/* create */
	{ &vop_mknod_desc, genfs_badop },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, spec_close },		/* close */
	{ &vop_access_desc, genfs_ebadf },		/* access */
	{ &vop_accessx_desc, genfs_ebadf },		/* accessx */
	{ &vop_getattr_desc, genfs_ebadf },		/* getattr */
	{ &vop_setattr_desc, genfs_ebadf },		/* setattr */
	{ &vop_read_desc, spec_read },			/* read */
	{ &vop_write_desc, spec_write },		/* write */
	{ &vop_fallocate_desc, genfs_eopnotsupp },	/* fallocate */
	{ &vop_fdiscard_desc, spec_fdiscard },		/* fdiscard */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, genfs_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, genfs_badop },		/* remove */
	{ &vop_link_desc, genfs_badop },		/* link */
	{ &vop_rename_desc, genfs_badop },		/* rename */
	{ &vop_mkdir_desc, genfs_badop },		/* mkdir */
	{ &vop_rmdir_desc, genfs_badop },		/* rmdir */
	{ &vop_symlink_desc, genfs_badop },		/* symlink */
	{ &vop_readdir_desc, genfs_badop },		/* readdir */
	{ &vop_readlink_desc, genfs_badop },		/* readlink */
	{ &vop_abortop_desc, genfs_badop },		/* abortop */
	{ &vop_inactive_desc, spec_inactive },		/* inactive */
	{ &vop_reclaim_desc, spec_reclaim },		/* reclaim */
	{ &vop_lock_desc, genfs_lock },			/* lock */
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, spec_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, genfs_getpages },		/* getpages */
	{ &vop_putpages_desc, genfs_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc spec_vnodeop_opv_desc =
	{ &spec_vnodeop_p, spec_vnodeop_entries };

static kauth_listener_t rawio_listener;
static struct kcondvar specfs_iocv;

/* Returns true if vnode is /dev/mem or /dev/kmem. */
bool
iskmemvp(struct vnode *vp)
{
	return ((vp->v_type == VCHR) && iskmemdev(vp->v_rdev));
}

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev_t dev)
{
	/* mem_no is emitted by config(8) to generated devsw.c */
	extern const int mem_no;

	/* minor 14 is /dev/io on i386 with COMPAT_10 */
	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
}

static int
rawio_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	if ((action != KAUTH_DEVICE_RAWIO_SPEC) &&
	    (action != KAUTH_DEVICE_RAWIO_PASSTHRU))
		return result;

	/* Access is mandated by permissions. */
	result = KAUTH_RESULT_ALLOW;

	return result;
}

void
spec_init(void)
{

	rawio_listener = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    rawio_listener_cb, NULL);
	cv_init(&specfs_iocv, "specio");
}

/*
 * spec_io_enter(vp, &sn, &dev)
 *
 *	Enter an operation that may not hold vp's vnode lock or an
 *	fstrans on vp's mount.  Until spec_io_exit, the vnode will not
 *	be revoked.
 *
 *	On success, set sn to the specnode pointer and dev to the dev_t
 *	number and return zero.  Caller must later call spec_io_exit
 *	when done.
 *
 *	On failure, return ENXIO -- the device has been revoked and no
 *	longer exists.
 */
static int
spec_io_enter(struct vnode *vp, struct specnode **snp, dev_t *devp)
{
	dev_t dev;
	struct specnode *sn;
	unsigned iocnt;
	int error = 0;

	mutex_enter(vp->v_interlock);

	/*
	 * Extract all the info we need from the vnode, unless the
	 * vnode has already been reclaimed.  This can happen if the
	 * underlying device has been removed and all the device nodes
	 * for it have been revoked.  The caller may not hold a vnode
	 * lock or fstrans to prevent this from happening before it has
	 * had an opportunity to notice the vnode is dead.
	 */
	if (vdead_check(vp, VDEAD_NOWAIT) != 0 ||
	    (sn = vp->v_specnode) == NULL ||
	    (dev = vp->v_rdev) == NODEV) {
		error = ENXIO;
		goto out;
	}

	/*
	 * Notify spec_close that we are doing an I/O operation which
	 * may not be not bracketed by fstrans(9) and thus is not
	 * blocked by vfs suspension.
	 *
	 * We could hold this reference with psref(9) instead, but we
	 * already have to take the interlock for vdead_check, so
	 * there's not much more cost here to another atomic operation.
	 */
	do {
		iocnt = atomic_load_relaxed(&sn->sn_dev->sd_iocnt);
		if (__predict_false(iocnt == UINT_MAX)) {
			/*
			 * The I/O count is limited by the number of
			 * LWPs (which will never overflow this) --
			 * unless one driver uses another driver via
			 * specfs, which is rather unusual, but which
			 * could happen via pud(4) userspace drivers.
			 * We could use a 64-bit count, but can't use
			 * atomics for that on all platforms.
			 * (Probably better to switch to psref or
			 * localcount instead.)
			 */
			error = EBUSY;
			goto out;
		}
	} while (atomic_cas_uint(&sn->sn_dev->sd_iocnt, iocnt, iocnt + 1)
	    != iocnt);

	/* Success!  */
	*snp = sn;
	*devp = dev;
	error = 0;

out:	mutex_exit(vp->v_interlock);
	return error;
}

/*
 * spec_io_exit(vp, sn)
 *
 *	Exit an operation entered with a successful spec_io_enter --
 *	allow concurrent spec_node_revoke to proceed.  The argument sn
 *	must match the struct specnode pointer returned by spec_io_exit
 *	for vp.
 */
static void
spec_io_exit(struct vnode *vp, struct specnode *sn)
{
	struct specdev *sd = sn->sn_dev;
	unsigned iocnt;

	KASSERT(vp->v_specnode == sn);

	/*
	 * We are done.  Notify spec_close if appropriate.  The
	 * transition of 1 -> 0 must happen under device_lock so
	 * spec_close doesn't miss a wakeup.
	 */
	do {
		iocnt = atomic_load_relaxed(&sd->sd_iocnt);
		KASSERT(iocnt > 0);
		if (iocnt == 1) {
			mutex_enter(&device_lock);
			if (atomic_dec_uint_nv(&sd->sd_iocnt) == 0)
				cv_broadcast(&specfs_iocv);
			mutex_exit(&device_lock);
			break;
		}
	} while (atomic_cas_uint(&sd->sd_iocnt, iocnt, iocnt - 1) != iocnt);
}

/*
 * spec_io_drain(sd)
 *
 *	Wait for all existing spec_io_enter/exit sections to complete.
 *	Caller must ensure spec_io_enter will fail at this point.
 */
static void
spec_io_drain(struct specdev *sd)
{

	/*
	 * I/O at the same time as closing is unlikely -- it often
	 * indicates an application bug.
	 */
	if (__predict_true(atomic_load_relaxed(&sd->sd_iocnt) == 0))
		return;

	mutex_enter(&device_lock);
	while (atomic_load_relaxed(&sd->sd_iocnt) > 0)
		cv_wait(&specfs_iocv, &device_lock);
	mutex_exit(&device_lock);
}

/*
 * Initialize a vnode that represents a device.
 */
void
spec_node_init(vnode_t *vp, dev_t rdev)
{
	specnode_t *sn;
	specdev_t *sd;
	vnode_t *vp2;
	vnode_t **vpp;

	KASSERT(vp->v_type == VBLK || vp->v_type == VCHR);
	KASSERT(vp->v_specnode == NULL);

	/*
	 * Search the hash table for this device.  If known, add a
	 * reference to the device structure.  If not known, create
	 * a new entry to represent the device.  In all cases add
	 * the vnode to the hash table.
	 */
	sn = kmem_alloc(sizeof(*sn), KM_SLEEP);
	sd = kmem_alloc(sizeof(*sd), KM_SLEEP);
	mutex_enter(&device_lock);
	vpp = &specfs_hash[SPECHASH(rdev)];
	for (vp2 = *vpp; vp2 != NULL; vp2 = vp2->v_specnext) {
		KASSERT(vp2->v_specnode != NULL);
		if (rdev == vp2->v_rdev && vp->v_type == vp2->v_type) {
			break;
		}
	}
	if (vp2 == NULL) {
		/* No existing record, create a new one. */
		sd->sd_rdev = rdev;
		sd->sd_mountpoint = NULL;
		sd->sd_lockf = NULL;
		sd->sd_refcnt = 1;
		sd->sd_opencnt = 0;
		sd->sd_bdevvp = NULL;
		sd->sd_iocnt = 0;
		sd->sd_opened = false;
		sd->sd_closing = false;
		sn->sn_dev = sd;
		sd = NULL;
	} else {
		/* Use the existing record. */
		sn->sn_dev = vp2->v_specnode->sn_dev;
		sn->sn_dev->sd_refcnt++;
	}
	/* Insert vnode into the hash chain. */
	sn->sn_opencnt = 0;
	sn->sn_rdev = rdev;
	sn->sn_gone = false;
	vp->v_specnode = sn;
	vp->v_specnext = *vpp;
	*vpp = vp;
	mutex_exit(&device_lock);

	/* Free the record we allocated if unused. */
	if (sd != NULL) {
		kmem_free(sd, sizeof(*sd));
	}
}

/*
 * Lookup a vnode by device number and return it referenced.
 */
int
spec_node_lookup_by_dev(enum vtype type, dev_t dev, vnode_t **vpp)
{
	int error;
	vnode_t *vp;

	mutex_enter(&device_lock);
	for (vp = specfs_hash[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (type == vp->v_type && dev == vp->v_rdev) {
			mutex_enter(vp->v_interlock);
			/* If clean or being cleaned, then ignore it. */
			if (vdead_check(vp, VDEAD_NOWAIT) == 0)
				break;
			mutex_exit(vp->v_interlock);
		}
	}
	KASSERT(vp == NULL || mutex_owned(vp->v_interlock));
	if (vp == NULL) {
		mutex_exit(&device_lock);
		return ENOENT;
	}
	/*
	 * If it is an opened block device return the opened vnode.
	 */
	if (type == VBLK && vp->v_specnode->sn_dev->sd_bdevvp != NULL) {
		mutex_exit(vp->v_interlock);
		vp = vp->v_specnode->sn_dev->sd_bdevvp;
		mutex_enter(vp->v_interlock);
	}
	mutex_exit(&device_lock);
	error = vcache_vget(vp);
	if (error != 0)
		return error;
	*vpp = vp;

	return 0;
}

/*
 * Lookup a vnode by file system mounted on and return it referenced.
 */
int
spec_node_lookup_by_mount(struct mount *mp, vnode_t **vpp)
{
	int i, error;
	vnode_t *vp, *vq;

	mutex_enter(&device_lock);
	for (i = 0, vq = NULL; i < SPECHSZ && vq == NULL; i++) {
		for (vp = specfs_hash[i]; vp; vp = vp->v_specnext) {
			if (vp->v_type != VBLK)
				continue;
			vq = vp->v_specnode->sn_dev->sd_bdevvp;
			if (vq != NULL &&
			    vq->v_specnode->sn_dev->sd_mountpoint == mp)
				break;
			vq = NULL;
		}
	}
	if (vq == NULL) {
		mutex_exit(&device_lock);
		return ENOENT;
	}
	mutex_enter(vq->v_interlock);
	mutex_exit(&device_lock);
	error = vcache_vget(vq);
	if (error != 0)
		return error;
	*vpp = vq;

	return 0;

}

/*
 * Get the file system mounted on this block device.
 *
 * XXX Caller should hold the vnode lock -- shared or exclusive -- so
 * that this can't changed, and the vnode can't be revoked while we
 * examine it.  But not all callers do, and they're scattered through a
 * lot of file systems, so we can't assert this yet.
 */
struct mount *
spec_node_getmountedfs(vnode_t *devvp)
{
	struct mount *mp;

	KASSERT(devvp->v_type == VBLK);
	mp = devvp->v_specnode->sn_dev->sd_mountpoint;

	return mp;
}

/*
 * Set the file system mounted on this block device.
 *
 * XXX Caller should hold the vnode lock exclusively so this can't be
 * changed or assumed by spec_node_getmountedfs while we change it, and
 * the vnode can't be revoked while we handle it.  But not all callers
 * do, and they're scattered through a lot of file systems, so we can't
 * assert this yet.  Instead, for now, we'll take an I/O reference so
 * at least the ioctl doesn't race with revoke/detach.
 *
 * If you do change this to assert an exclusive vnode lock, you must
 * also do vdead_check before trying bdev_ioctl, because the vnode may
 * have been revoked by the time the caller locked it, and this is
 * _not_ a vop -- calls to spec_node_setmountedfs don't go through
 * v_op, so revoking the vnode doesn't prevent further calls.
 *
 * XXX Caller should additionally have the vnode open, at least if mp
 * is nonnull, but I'm not sure all callers do that -- need to audit.
 * Currently udf closes the vnode before clearing the mount.
 */
void
spec_node_setmountedfs(vnode_t *devvp, struct mount *mp)
{
	struct dkwedge_info dkw;
	struct specnode *sn;
	dev_t dev;
	int error;

	KASSERT(devvp->v_type == VBLK);

	error = spec_io_enter(devvp, &sn, &dev);
	if (error)
		return;

	KASSERT(sn->sn_dev->sd_mountpoint == NULL || mp == NULL);
	sn->sn_dev->sd_mountpoint = mp;
	if (mp == NULL)
		goto out;

	error = bdev_ioctl(dev, DIOCGWEDGEINFO, &dkw, FREAD, curlwp);
	if (error)
		goto out;

	strlcpy(mp->mnt_stat.f_mntfromlabel, dkw.dkw_wname,
	    sizeof(mp->mnt_stat.f_mntfromlabel));

out:	spec_io_exit(devvp, sn);
}

/*
 * A vnode representing a special device is going away.  Close
 * the device if the vnode holds it open.
 */
void
spec_node_revoke(vnode_t *vp)
{
	specnode_t *sn;
	specdev_t *sd;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	sn = vp->v_specnode;
	sd = sn->sn_dev;

	KASSERT(vp->v_type == VBLK || vp->v_type == VCHR);
	KASSERT(vp->v_specnode != NULL);
	KASSERT(sn->sn_gone == false);

	mutex_enter(&device_lock);
	KASSERT(sn->sn_opencnt <= sd->sd_opencnt);
	if (sn->sn_opencnt != 0) {
		sd->sd_opencnt -= (sn->sn_opencnt - 1);
		sn->sn_opencnt = 1;
		sn->sn_gone = true;
		mutex_exit(&device_lock);

		VOP_CLOSE(vp, FNONBLOCK, NOCRED);

		mutex_enter(&device_lock);
		KASSERT(sn->sn_opencnt == 0);
	}

	/*
	 * We may have revoked the vnode in this thread while another
	 * thread was in the middle of spec_close, in the window when
	 * spec_close releases the vnode lock to call .d_close for the
	 * last close.  In that case, wait for the concurrent
	 * spec_close to complete.
	 */
	while (sd->sd_closing)
		cv_wait(&specfs_iocv, &device_lock);
	mutex_exit(&device_lock);
}

/*
 * A vnode representing a special device is being recycled.
 * Destroy the specfs component.
 */
void
spec_node_destroy(vnode_t *vp)
{
	specnode_t *sn;
	specdev_t *sd;
	vnode_t **vpp, *vp2;
	int refcnt;

	sn = vp->v_specnode;
	sd = sn->sn_dev;

	KASSERT(vp->v_type == VBLK || vp->v_type == VCHR);
	KASSERT(vp->v_specnode != NULL);
	KASSERT(sn->sn_opencnt == 0);

	mutex_enter(&device_lock);
	/* Remove from the hash and destroy the node. */
	vpp = &specfs_hash[SPECHASH(vp->v_rdev)];
	for (vp2 = *vpp;; vp2 = vp2->v_specnext) {
		if (vp2 == NULL) {
			panic("spec_node_destroy: corrupt hash");
		}
		if (vp2 == vp) {
			KASSERT(vp == *vpp);
			*vpp = vp->v_specnext;
			break;
		}
		if (vp2->v_specnext == vp) {
			vp2->v_specnext = vp->v_specnext;
			break;
		}
	}
	sn = vp->v_specnode;
	vp->v_specnode = NULL;
	refcnt = sd->sd_refcnt--;
	KASSERT(refcnt > 0);
	mutex_exit(&device_lock);

	/* If the device is no longer in use, destroy our record. */
	if (refcnt == 1) {
		KASSERT(sd->sd_iocnt == 0);
		KASSERT(sd->sd_opencnt == 0);
		KASSERT(sd->sd_bdevvp == NULL);
		kmem_free(sd, sizeof(*sd));
	}
	kmem_free(sn, sizeof(*sn));
}

/*
 * Trivial lookup routine that always fails.
 */
int
spec_lookup(void *v)
{
	struct vop_lookup_v2_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

typedef int (*spec_ioctl_t)(dev_t, u_long, void *, int, struct lwp *);

/*
 * Open a special file.
 */
/* ARGSUSED */
int
spec_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct lwp *l = curlwp;
	struct vnode *vp = ap->a_vp;
	dev_t dev;
	int error;
	enum kauth_device_req req;
	specnode_t *sn;
	specdev_t *sd;
	spec_ioctl_t ioctl;
	u_int gen = 0;
	const char *name = NULL;
	bool needclose = false;
	struct partinfo pi;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERTMSG(vp->v_type == VBLK || vp->v_type == VCHR, "type=%d",
	    vp->v_type);

	dev = vp->v_rdev;
	sn = vp->v_specnode;
	sd = sn->sn_dev;

	/*
	 * Don't allow open if fs is mounted -nodev.
	 */
	if (vp->v_mount && (vp->v_mount->mnt_flag & MNT_NODEV))
		return (ENXIO);

	switch (ap->a_mode & (FREAD | FWRITE)) {
	case FREAD | FWRITE:
		req = KAUTH_REQ_DEVICE_RAWIO_SPEC_RW;
		break;
	case FWRITE:
		req = KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE;
		break;
	default:
		req = KAUTH_REQ_DEVICE_RAWIO_SPEC_READ;
		break;
	}
	error = kauth_authorize_device_spec(ap->a_cred, req, vp);
	if (error != 0)
		return (error);

	/*
	 * Acquire an open reference -- as long as we hold onto it, and
	 * the vnode isn't revoked, it can't be closed, and the vnode
	 * can't be revoked until we release the vnode lock.
	 */
	mutex_enter(&device_lock);
	KASSERT(!sn->sn_gone);
	switch (vp->v_type) {
	case VCHR:
		/*
		 * Character devices can accept opens from multiple
		 * vnodes.  But first, wait for any close to finish.
		 * Wait under the vnode lock so we don't have to worry
		 * about the vnode being revoked while we wait.
		 */
		while (sd->sd_closing) {
			error = cv_wait_sig(&specfs_iocv, &device_lock);
			if (error)
				break;
		}
		if (error)
			break;
		sd->sd_opencnt++;
		sn->sn_opencnt++;
		break;
	case VBLK:
		/*
		 * For block devices, permit only one open.  The buffer
		 * cache cannot remain self-consistent with multiple
		 * vnodes holding a block device open.
		 *
		 * Treat zero opencnt with non-NULL mountpoint as open.
		 * This may happen after forced detach of a mounted device.
		 */
		if (sd->sd_opencnt != 0 || sd->sd_mountpoint != NULL) {
			error = EBUSY;
			break;
		}
		KASSERTMSG(sn->sn_opencnt == 0, "%u", sn->sn_opencnt);
		sn->sn_opencnt = 1;
		sd->sd_opencnt = 1;
		sd->sd_bdevvp = vp;
		break;
	default:
		panic("invalid specfs vnode type: %d", vp->v_type);
	}
	mutex_exit(&device_lock);
	if (error)
		return error;

	/*
	 * Set VV_ISTTY if this is a tty cdev.
	 *
	 * XXX This does the wrong thing if the module has to be
	 * autoloaded.  We should maybe set this after autoloading
	 * modules and calling .d_open successfully, except (a) we need
	 * the vnode lock to touch it, and (b) once we acquire the
	 * vnode lock again, the vnode may have been revoked, and
	 * deadfs's dead_read needs VV_ISTTY to be already set in order
	 * to return the right answer.  So this needs some additional
	 * synchronization to be made to work correctly with tty driver
	 * module autoload.  For now, let's just hope it doesn't cause
	 * too much trouble for a tty from an autoloaded driver module
	 * to fail with EIO instead of returning EOF.
	 */
	if (vp->v_type == VCHR) {
		if (cdev_type(dev) == D_TTY)
			vp->v_vflag |= VV_ISTTY;
	}

	/*
	 * Open the device.  If .d_open returns ENXIO (device not
	 * configured), the driver may not be loaded, so try
	 * autoloading a module and then try .d_open again if anything
	 * got loaded.
	 *
	 * Because opening the device may block indefinitely, e.g. when
	 * opening a tty, and loading a module may cross into many
	 * other subsystems, we must not hold the vnode lock while
	 * calling .d_open, so release it now and reacquire it when
	 * done.
	 */
	VOP_UNLOCK(vp);
	switch (vp->v_type) {
	case VCHR:
		do {
			const struct cdevsw *cdev;

			gen = module_gen;
			error = cdev_open(dev, ap->a_mode, S_IFCHR, l);
			if (error != ENXIO)
				break;
			
			/* Check if we already have a valid driver */
			mutex_enter(&device_lock);
			cdev = cdevsw_lookup(dev);
			mutex_exit(&device_lock);
			if (cdev != NULL)
				break;

			/* Get device name from devsw_conv array */
			if ((name = cdevsw_getname(major(dev))) == NULL)
				break;
			
			/* Try to autoload device module */
			(void) module_autoload(name, MODULE_CLASS_DRIVER);
		} while (gen != module_gen);
		break;

	case VBLK:
		do {
			const struct bdevsw *bdev;

			gen = module_gen;
			error = bdev_open(dev, ap->a_mode, S_IFBLK, l);
			if (error != ENXIO)
				break;

			/* Check if we already have a valid driver */
			mutex_enter(&device_lock);
			bdev = bdevsw_lookup(dev);
			mutex_exit(&device_lock);
			if (bdev != NULL)
				break;

			/* Get device name from devsw_conv array */
			if ((name = bdevsw_getname(major(dev))) == NULL)
				break;

                        /* Try to autoload device module */
			(void) module_autoload(name, MODULE_CLASS_DRIVER);
		} while (gen != module_gen);
		break;

	default:
		__unreachable();
	}
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	/*
	 * If it has been revoked since we released the vnode lock and
	 * reacquired it, then spec_node_revoke has closed it, and we
	 * must fail with EBADF.
	 *
	 * Otherwise, if opening it failed, back out and release the
	 * open reference.  If it was ever successfully opened and we
	 * got the last reference this way, it's now our job to close
	 * it.  This might happen in the following scenario:
	 *
	 *	Thread 1		Thread 2
	 *	VOP_OPEN
	 *	  ...
	 *	  .d_open -> 0 (success)
	 *	  acquire vnode lock
	 *	  do stuff		VOP_OPEN
	 *	  release vnode lock	...
	 *				  .d_open -> EBUSY
	 *	VOP_CLOSE
	 *	  acquire vnode lock
	 *	  --sd_opencnt != 0
	 *	  => no .d_close
	 *	  release vnode lock
	 *				  acquire vnode lock
	 *				  --sd_opencnt == 0
	 *
	 * We can't resolve this by making spec_close wait for .d_open
	 * to complete before examining sd_opencnt, because .d_open can
	 * hang indefinitely, e.g. for a tty.
	 */
	mutex_enter(&device_lock);
	if (sn->sn_gone) {
		if (error == 0)
			error = EBADF;
	} else if (error == 0) {
		sd->sd_opened = true;
	} else if (sd->sd_opencnt == 1 && sd->sd_opened) {
		/*
		 * We're the last reference to a _previous_ open even
		 * though this one failed, so we have to close it.
		 * Don't decrement the reference count here --
		 * spec_close will do that.
		 */
		KASSERT(sn->sn_opencnt == 1);
		needclose = true;
	} else {
		sd->sd_opencnt--;
		sn->sn_opencnt--;
		if (vp->v_type == VBLK)
			sd->sd_bdevvp = NULL;
	}
	mutex_exit(&device_lock);

	/*
	 * If this open failed, but the device was previously opened,
	 * and another thread concurrently closed the vnode while we
	 * were in the middle of reopening it, the other thread will
	 * see sd_opencnt > 0 and thus decide not to call .d_close --
	 * it is now our responsibility to do so.
	 *
	 * XXX The flags passed to VOP_CLOSE here are wrong, but
	 * drivers can't rely on FREAD|FWRITE anyway -- e.g., consider
	 * a device opened by thread 0 with O_READ, then opened by
	 * thread 1 with O_WRITE, then closed by thread 0, and finally
	 * closed by thread 1; the last .d_close call will have FWRITE
	 * but not FREAD.  We should just eliminate the FREAD/FWRITE
	 * parameter to .d_close altogether.
	 */
	if (needclose) {
		KASSERT(error);
		VOP_CLOSE(vp, FNONBLOCK, NOCRED);
	}

	/* If anything went wrong, we're done.  */
	if (error)
		return error;

	/*
	 * For disk devices, automagically set the vnode size to the
	 * partition size, if we can.  This applies to block devices
	 * and character devices alike -- every block device must have
	 * a corresponding character device.  And if the module is
	 * loaded it will remain loaded until we're done here (it is
	 * forbidden to devsw_detach until closed).  So it is safe to
	 * query cdev_type unconditionally here.
	 */
	if (cdev_type(dev) == D_DISK) {
		ioctl = vp->v_type == VCHR ? cdev_ioctl : bdev_ioctl;
		if ((*ioctl)(dev, DIOCGPARTINFO, &pi, FREAD, curlwp) == 0)
			uvm_vnp_setsize(vp,
			    (voff_t)pi.pi_secsize * pi.pi_size);
	}

	/* Success!  */
	return 0;
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
int
spec_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
 	struct lwp *l = curlwp;
	struct specnode *sn;
	dev_t dev;
	struct buf *bp;
	daddr_t bn;
	int bsize, bscale;
	struct partinfo pi;
	int n, on;
	int error = 0;
	int i, nra;
	daddr_t lastbn, *rablks;
	int *rasizes;
	int nrablks, ratogo;

	KASSERT(uio->uio_rw == UIO_READ);
	KASSERTMSG(VMSPACE_IS_KERNEL_P(uio->uio_vmspace) ||
		   uio->uio_vmspace == curproc->p_vmspace,
		"vmspace belongs to neither kernel nor curproc");

	if (uio->uio_resid == 0)
		return (0);

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Release the lock while we sleep -- possibly
		 * indefinitely, if this is, e.g., a tty -- in
		 * cdev_read, so we don't hold up everything else that
		 * might want access to the vnode.
		 *
		 * But before we issue the read, take an I/O reference
		 * to the specnode so close will know when we're done
		 * reading.  Note that the moment we release the lock,
		 * the vnode's identity may change; hence spec_io_enter
		 * may fail, and the caller may have a dead vnode on
		 * their hands, if the file system on which vp lived
		 * has been unmounted.
		 */
		VOP_UNLOCK(vp);
		error = spec_io_enter(vp, &sn, &dev);
		if (error)
			goto out;
		error = cdev_read(dev, uio, ap->a_ioflag);
		spec_io_exit(vp, sn);
out:		vn_lock(vp, LK_SHARED | LK_RETRY);
		return (error);

	case VBLK:
		KASSERT(vp == vp->v_specnode->sn_dev->sd_bdevvp);
		if (uio->uio_offset < 0)
			return (EINVAL);

		if (bdev_ioctl(vp->v_rdev, DIOCGPARTINFO, &pi, FREAD, l) == 0)
			bsize = imin(imax(pi.pi_bsize, DEV_BSIZE), MAXBSIZE);
		else
			bsize = BLKDEV_IOSIZE;

		bscale = bsize >> DEV_BSHIFT;

		nra = uimax(16 * MAXPHYS / bsize - 1, 511);
		rablks = kmem_alloc(nra * sizeof(*rablks), KM_SLEEP);
		rasizes = kmem_alloc(nra * sizeof(*rasizes), KM_SLEEP);
		lastbn = ((uio->uio_offset + uio->uio_resid - 1) >> DEV_BSHIFT)
		    &~ (bscale - 1);
		nrablks = ratogo = 0;
		do {
			bn = (uio->uio_offset >> DEV_BSHIFT) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = uimin((unsigned)(bsize - on), uio->uio_resid);

			if (ratogo == 0) {
				nrablks = uimin((lastbn - bn) / bscale, nra);
				ratogo = nrablks;

				for (i = 0; i < nrablks; ++i) {
					rablks[i] = bn + (i+1) * bscale;
					rasizes[i] = bsize;
				}

				error = breadn(vp, bn, bsize,
					       rablks, rasizes, nrablks,
					       0, &bp);
			} else {
				if (ratogo > 0)
					--ratogo;
				error = bread(vp, bn, bsize, 0, &bp);
			}
			if (error)
				break;
			n = uimin(n, bsize - bp->b_resid);
			error = uiomove((char *)bp->b_data + on, n, uio);
			brelse(bp, 0);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);

		kmem_free(rablks, nra * sizeof(*rablks));
		kmem_free(rasizes, nra * sizeof(*rasizes));

		return (error);

	default:
		panic("spec_read type");
	}
	/* NOTREACHED */
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
int
spec_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct lwp *l = curlwp;
	struct specnode *sn;
	dev_t dev;
	struct buf *bp;
	daddr_t bn;
	int bsize, bscale;
	struct partinfo pi;
	int n, on;
	int error = 0;

	KASSERT(uio->uio_rw == UIO_WRITE);
	KASSERTMSG(VMSPACE_IS_KERNEL_P(uio->uio_vmspace) ||
		   uio->uio_vmspace == curproc->p_vmspace,
		"vmspace belongs to neither kernel nor curproc");

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Release the lock while we sleep -- possibly
		 * indefinitely, if this is, e.g., a tty -- in
		 * cdev_write, so we don't hold up everything else that
		 * might want access to the vnode.
		 *
		 * But before we issue the write, take an I/O reference
		 * to the specnode so close will know when we're done
		 * writing.  Note that the moment we release the lock,
		 * the vnode's identity may change; hence spec_io_enter
		 * may fail, and the caller may have a dead vnode on
		 * their hands, if the file system on which vp lived
		 * has been unmounted.
		 */
		VOP_UNLOCK(vp);
		error = spec_io_enter(vp, &sn, &dev);
		if (error)
			goto out;
		error = cdev_write(dev, uio, ap->a_ioflag);
		spec_io_exit(vp, sn);
out:		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		return (error);

	case VBLK:
		KASSERT(vp == vp->v_specnode->sn_dev->sd_bdevvp);
		if (uio->uio_resid == 0)
			return (0);
		if (uio->uio_offset < 0)
			return (EINVAL);

		if (bdev_ioctl(vp->v_rdev, DIOCGPARTINFO, &pi, FREAD, l) == 0)
			bsize = imin(imax(pi.pi_bsize, DEV_BSIZE), MAXBSIZE);
		else
			bsize = BLKDEV_IOSIZE;

		bscale = bsize >> DEV_BSHIFT;
		do {
			bn = (uio->uio_offset >> DEV_BSHIFT) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = uimin((unsigned)(bsize - on), uio->uio_resid);
			if (n == bsize)
				bp = getblk(vp, bn, bsize, 0, 0);
			else
				error = bread(vp, bn, bsize, B_MODIFY, &bp);
			if (error) {
				return (error);
			}
			n = uimin(n, bsize - bp->b_resid);
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (error)
				brelse(bp, 0);
			else {
				if (n + on == bsize)
					bawrite(bp);
				else
					bdwrite(bp);
				error = bp->b_error;
			}
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
		return (error);

	default:
		panic("spec_write type");
	}
	/* NOTREACHED */
}

/*
 * fdiscard, which on disk devices becomes TRIM.
 */
int
spec_fdiscard(void *v)
{
	struct vop_fdiscard_args /* {
		struct vnode *a_vp;
		off_t a_pos;
		off_t a_len;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	dev_t dev;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	dev = vp->v_rdev;

	switch (vp->v_type) {
	    case VCHR:
		// this is not stored for character devices
		//KASSERT(vp == vp->v_specnode->sn_dev->sd_cdevvp);
		return cdev_discard(dev, ap->a_pos, ap->a_len);
	    case VBLK:
		KASSERT(vp == vp->v_specnode->sn_dev->sd_bdevvp);
		return bdev_discard(dev, ap->a_pos, ap->a_len);
	    default:
		panic("spec_fdiscard: not a device\n");
	}
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
int
spec_ioctl(void *v)
{
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		void  *a_data;
		int  a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct specnode *sn;
	dev_t dev;
	int error;

	error = spec_io_enter(vp, &sn, &dev);
	if (error)
		return error;

	switch (vp->v_type) {
	case VCHR:
		error = cdev_ioctl(dev, ap->a_command, ap->a_data,
		    ap->a_fflag, curlwp);
		break;
	case VBLK:
		KASSERT(vp == vp->v_specnode->sn_dev->sd_bdevvp);
		error = bdev_ioctl(dev, ap->a_command, ap->a_data,
		   ap->a_fflag, curlwp);
		break;
	default:
		panic("spec_ioctl");
		/* NOTREACHED */
	}

	spec_io_exit(vp, sn);
	return error;
}

/* ARGSUSED */
int
spec_poll(void *v)
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct specnode *sn;
	dev_t dev;
	int revents;

	if (spec_io_enter(vp, &sn, &dev) != 0)
		return POLLERR;

	switch (vp->v_type) {
	case VCHR:
		revents = cdev_poll(dev, ap->a_events, curlwp);
		break;
	default:
		revents = genfs_poll(v);
		break;
	}

	spec_io_exit(vp, sn);
	return revents;
}

/* ARGSUSED */
int
spec_kqfilter(void *v)
{
	struct vop_kqfilter_args /* {
		struct vnode	*a_vp;
		struct proc	*a_kn;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct specnode *sn;
	dev_t dev;
	int error;

	error = spec_io_enter(vp, &sn, &dev);
	if (error)
		return error;

	switch (vp->v_type) {
	case VCHR:
		error = cdev_kqfilter(dev, ap->a_kn);
		break;
	default:
		/*
		 * Block devices don't support kqfilter, and refuse it
		 * for any other files (like those vflush()ed) too.
		 */
		error = EOPNOTSUPP;
		break;
	}

	spec_io_exit(vp, sn);
	return error;
}

/*
 * Allow mapping of only D_DISK.  This is called only for VBLK.
 */
int
spec_mmap(void *v)
{
	struct vop_mmap_args /* {
		struct vnode *a_vp;
		vm_prot_t a_prot;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct specnode *sn;
	dev_t dev;
	int error;

	KASSERT(vp->v_type == VBLK);

	error = spec_io_enter(vp, &sn, &dev);
	if (error)
		return error;

	error = bdev_type(dev) == D_DISK ? 0 : EINVAL;

	spec_io_exit(vp, sn);
	return 0;
}

/*
 * Synch buffers associated with a block device
 */
/* ARGSUSED */
int
spec_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int  a_flags;
		off_t offlo;
		off_t offhi;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct mount *mp;
	int error;

	if (vp->v_type == VBLK) {
		if ((mp = spec_node_getmountedfs(vp)) != NULL) {
			error = VFS_FSYNC(mp, vp, ap->a_flags);
			if (error != EOPNOTSUPP)
				return error;
		}
		return vflushbuf(vp, ap->a_flags);
	}
	return (0);
}

/*
 * Just call the device strategy routine
 */
int
spec_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp = ap->a_bp;
	struct specnode *sn = NULL;
	dev_t dev;
	int error;

	error = spec_io_enter(vp, &sn, &dev);
	if (error)
		goto out;

	bp->b_dev = dev;

	if (!(bp->b_flags & B_READ)) {
#ifdef DIAGNOSTIC
		if (bp->b_vp && bp->b_vp->v_type == VBLK) {
			struct mount *mp = spec_node_getmountedfs(bp->b_vp);

			if (mp && (mp->mnt_flag & MNT_RDONLY)) {
				printf("%s blk %"PRId64" written while ro!\n",
				    mp->mnt_stat.f_mntonname, bp->b_blkno);
			}
		}
#endif /* DIAGNOSTIC */
		error = fscow_run(bp, false);
		if (error)
			goto out;
	}
	bdev_strategy(bp);

	error = 0;

out:	if (sn)
		spec_io_exit(vp, sn);
	if (error) {
		bp->b_error = error;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	return error;
}

int
spec_inactive(void *v)
{
	struct vop_inactive_v2_args /* {
		struct vnode *a_vp;
		struct bool *a_recycle;
	} */ *ap = v;

	KASSERT(ap->a_vp->v_mount == dead_rootmount);
	*ap->a_recycle = true;

	return 0;
}

int
spec_reclaim(void *v)
{
	struct vop_reclaim_v2_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	KASSERT(vp->v_specnode->sn_opencnt == 0);

	VOP_UNLOCK(vp);

	KASSERT(vp->v_mount == dead_rootmount);
	return 0;
}

/*
 * This is a noop, simply returning what one has been given.
 */
int
spec_bmap(void *v)
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;

	if (ap->a_vpp != NULL)
		*ap->a_vpp = ap->a_vp;
	if (ap->a_bnp != NULL)
		*ap->a_bnp = ap->a_bn;
	if (ap->a_runp != NULL)
		*ap->a_runp = (MAXBSIZE >> DEV_BSHIFT) - 1;
	return (0);
}

/*
 * Device close routine
 */
/* ARGSUSED */
int
spec_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct session *sess;
	dev_t dev;
	int flags = ap->a_fflag;
	int mode, error, count;
	specnode_t *sn;
	specdev_t *sd;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	mutex_enter(vp->v_interlock);
	sn = vp->v_specnode;
	dev = vp->v_rdev;
	sd = sn->sn_dev;
	/*
	 * If we're going away soon, make this non-blocking.
	 * Also ensures that we won't wedge in vn_lock below.
	 */
	if (vdead_check(vp, VDEAD_NOWAIT) != 0)
		flags |= FNONBLOCK;
	mutex_exit(vp->v_interlock);

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Hack: a tty device that is a controlling terminal
		 * has a reference from the session structure.  We
		 * cannot easily tell that a character device is a
		 * controlling terminal, unless it is the closing
		 * process' controlling terminal.  In that case, if the
		 * open count is 1 release the reference from the
		 * session.  Also, remove the link from the tty back to
		 * the session and pgrp.
		 *
		 * XXX V. fishy.
		 */
		mutex_enter(&proc_lock);
		sess = curlwp->l_proc->p_session;
		if (sn->sn_opencnt == 1 && vp == sess->s_ttyvp) {
			mutex_spin_enter(&tty_lock);
			sess->s_ttyvp = NULL;
			if (sess->s_ttyp->t_session != NULL) {
				sess->s_ttyp->t_pgrp = NULL;
				sess->s_ttyp->t_session = NULL;
				mutex_spin_exit(&tty_lock);
				/* Releases proc_lock. */
				proc_sessrele(sess);
			} else {
				mutex_spin_exit(&tty_lock);
				if (sess->s_ttyp->t_pgrp != NULL)
					panic("spec_close: spurious pgrp ref");
				mutex_exit(&proc_lock);
			}
			vrele(vp);
		} else
			mutex_exit(&proc_lock);

		/*
		 * If the vnode is locked, then we are in the midst
		 * of forcably closing the device, otherwise we only
		 * close on last reference.
		 */
		mode = S_IFCHR;
		break;

	case VBLK:
		KASSERT(vp == vp->v_specnode->sn_dev->sd_bdevvp);
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		error = vinvalbuf(vp, V_SAVE, ap->a_cred, curlwp, 0, 0);
		if (error)
			return (error);
		/*
		 * We do not want to really close the device if it
		 * is still in use unless we are trying to close it
		 * forcibly. Since every use (buffer, vnode, swap, cmap)
		 * holds a reference to the vnode, and because we mark
		 * any other vnodes that alias this device, when the
		 * sum of the reference counts on all the aliased
		 * vnodes descends to one, we are on last close.
		 */
		mode = S_IFBLK;
		break;

	default:
		panic("spec_close: not special");
	}

	/*
	 * Decrement the open reference count of this node and the
	 * device.  For block devices, the open reference count must be
	 * 1 at this point.  If the device's open reference count goes
	 * to zero, we're the last one out so get the lights.
	 *
	 * We may find --sd->sd_opencnt gives zero, and yet
	 * sd->sd_opened is false.  This happens if the vnode is
	 * revoked at the same time as it is being opened, which can
	 * happen when opening a tty blocks indefinitely.  In that
	 * case, we still must call close -- it is the job of close to
	 * interrupt the open.  Either way, the device will be no
	 * longer opened, so we have to clear sd->sd_opened; subsequent
	 * opens will have responsibility for issuing close.
	 *
	 * This has the side effect that the sequence of opens might
	 * happen out of order -- we might end up doing open, open,
	 * close, close, instead of open, close, open, close.  This is
	 * unavoidable with the current devsw API, where open is
	 * allowed to block and close must be able to run concurrently
	 * to interrupt it.  It is the driver's responsibility to
	 * ensure that close is idempotent so that this works.  Drivers
	 * requiring per-open state and exact 1:1 correspondence
	 * between open and close can use fd_clone.
	 */
	mutex_enter(&device_lock);
	sn->sn_opencnt--;
	count = --sd->sd_opencnt;
	if (vp->v_type == VBLK) {
		KASSERTMSG(count == 0, "block device with %u opens",
		    count + 1);
		sd->sd_bdevvp = NULL;
	}
	if (count == 0) {
		sd->sd_opened = false;
		sd->sd_closing = true;
	}
	mutex_exit(&device_lock);

	if (count != 0)
		return 0;

	/*
	 * If we're able to block, release the vnode lock & reacquire. We
	 * might end up sleeping for someone else who wants our queues. They
	 * won't get them if we hold the vnode locked.
	 */
	if (!(flags & FNONBLOCK))
		VOP_UNLOCK(vp);

	if (vp->v_type == VBLK)
		error = bdev_close(dev, flags, mode, curlwp);
	else
		error = cdev_close(dev, flags, mode, curlwp);

	/*
	 * Wait for all other devsw operations to drain.  After this
	 * point, no bdev/cdev_* can be active for this specdev.
	 */
	spec_io_drain(sd);

	/*
	 * Wake any spec_open calls waiting for close to finish -- do
	 * this before reacquiring the vnode lock, because spec_open
	 * holds the vnode lock while waiting, so doing this after
	 * reacquiring the lock would deadlock.
	 */
	mutex_enter(&device_lock);
	KASSERT(sd->sd_closing);
	sd->sd_closing = false;
	cv_broadcast(&specfs_iocv);
	mutex_exit(&device_lock);

	if (!(flags & FNONBLOCK))
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	return (error);
}

/*
 * Print out the contents of a special device vnode.
 */
int
spec_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;

	printf("dev %llu, %llu\n", (unsigned long long)major(ap->a_vp->v_rdev),
	    (unsigned long long)minor(ap->a_vp->v_rdev));
	return 0;
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
int
spec_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		return (0);
	default:
		return genfs_pathconf(ap);
	}
	/* NOTREACHED */
}

/*
 * Advisory record locking support.
 */
int
spec_advlock(void *v)
{
	struct vop_advlock_args /* {
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return lf_advlock(ap, &vp->v_speclockf, (off_t)0);
}
