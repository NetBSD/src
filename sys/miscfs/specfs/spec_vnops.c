/*	$NetBSD: spec_vnops.c,v 1.87 2006/05/14 21:32:21 elad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spec_vnops.c,v 1.87 2006/05/14 21:32:21 elad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/lockf.h>
#include <sys/tty.h>
#include <sys/kauth.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

/* symbolic sleep message strings for devices */
const char	devopn[] = "devopn";
const char	devio[] = "devio";
const char	devwait[] = "devwait";
const char	devin[] = "devin";
const char	devout[] = "devout";
const char	devioc[] = "devioc";
const char	devcls[] = "devcls";

struct vnode	*speclisth[SPECHSZ];

/*
 * This vnode operations vector is used for two things only:
 * - special device nodes created from whole cloth by the kernel.
 * - as a temporary vnodeops replacement for vnodes which were found to
 *	be aliased by callers of checkalias().
 * For the ops vector for vnodes built from special devices found in a
 * filesystem, see (e.g) ffs_specop_entries[] in ffs_vnops.c or the
 * equivalent for other filesystems.
 */

int (**spec_vnodeop_p)(void *);
const struct vnodeopv_entry_desc spec_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, spec_close },		/* close */
	{ &vop_access_desc, spec_access },		/* access */
	{ &vop_getattr_desc, spec_getattr },		/* getattr */
	{ &vop_setattr_desc, spec_setattr },		/* setattr */
	{ &vop_read_desc, spec_read },			/* read */
	{ &vop_write_desc, spec_write },		/* write */
	{ &vop_lease_desc, spec_lease_check },		/* lease */
	{ &vop_fcntl_desc, spec_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, spec_inactive },		/* inactive */
	{ &vop_reclaim_desc, spec_reclaim },		/* reclaim */
	{ &vop_lock_desc, spec_lock },			/* lock */
	{ &vop_unlock_desc, spec_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, spec_print },		/* print */
	{ &vop_islocked_desc, spec_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_bwrite_desc, spec_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* getpages */
	{ &vop_putpages_desc, spec_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc spec_vnodeop_opv_desc =
	{ &spec_vnodeop_p, spec_vnodeop_entries };

/*
 * Trivial lookup routine that always fails.
 */
int
spec_lookup(v)
	void *v;
{
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
static int
iskmemdev(dev_t dev)
{
	/* mem_no is emitted by config(8) to generated devsw.c */
	extern const int mem_no;

	/* minor 14 is /dev/io on i386 with COMPAT_10 */
	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
}

/*
 * Open a special file.
 */
/* ARGSUSED */
int
spec_open(v)
	void *v;
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct lwp *l = ap->a_l;
	struct vnode *bvp, *vp = ap->a_vp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	dev_t blkdev, dev = (dev_t)vp->v_rdev;
	int error;
	struct partinfo pi;
	int (*d_ioctl)(dev_t, u_long, caddr_t, int, struct lwp *);

	/*
	 * Don't allow open if fs is mounted -nodev.
	 */
	if (vp->v_mount && (vp->v_mount->mnt_flag & MNT_NODEV))
		return (ENXIO);

	switch (vp->v_type) {

	case VCHR:
		cdev = cdevsw_lookup(dev);
		if (cdev == NULL)
			return (ENXIO);
		if (ap->a_cred != FSCRED && (ap->a_mode & FWRITE)) {
			/*
			 * When running in very secure mode, do not allow
			 * opens for writing of any disk character devices.
			 */
			if (securelevel >= 2 && cdev->d_type == D_DISK)
				return (EPERM);
			/*
			 * When running in secure mode, do not allow opens
			 * for writing of /dev/mem, /dev/kmem, or character
			 * devices whose corresponding block devices are
			 * currently mounted.
			 */
			if (securelevel >= 1) {
				blkdev = devsw_chr2blk(dev);
				if (blkdev != (dev_t)NODEV &&
				    vfinddev(blkdev, VBLK, &bvp) &&
				    (error = vfs_mountedon(bvp)))
					return (error);
				if (iskmemdev(dev))
					return (EPERM);
			}
		}
		if (cdev->d_type == D_TTY)
			vp->v_flag |= VISTTY;
		VOP_UNLOCK(vp, 0);
		error = (*cdev->d_open)(dev, ap->a_mode, S_IFCHR, l);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		if (cdev->d_type != D_DISK)
			return error;
		d_ioctl = cdev->d_ioctl;
		break;

	case VBLK:
		bdev = bdevsw_lookup(dev);
		if (bdev == NULL)
			return (ENXIO);
		/*
		 * When running in very secure mode, do not allow
		 * opens for writing of any disk block devices.
		 */
		if (securelevel >= 2 && ap->a_cred != FSCRED &&
		    (ap->a_mode & FWRITE) && bdev->d_type == D_DISK)
			return (EPERM);
		/*
		 * Do not allow opens of block devices that are
		 * currently mounted.
		 */
		if ((error = vfs_mountedon(vp)) != 0)
			return (error);
		error = (*bdev->d_open)(dev, ap->a_mode, S_IFBLK, l);
		d_ioctl = bdev->d_ioctl;
		break;

	case VNON:
	case VLNK:
	case VDIR:
	case VREG:
	case VBAD:
	case VFIFO:
	case VSOCK:
	default:
		return 0;
	}

	if (error)
		return error;
	if (!(*d_ioctl)(vp->v_rdev, DIOCGPART, (caddr_t)&pi, FREAD, curlwp))
		vp->v_size = (voff_t)pi.disklab->d_secsize * pi.part->p_size;
	return 0;
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
int
spec_read(v)
	void *v;
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
	struct buf *bp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	daddr_t bn;
	int bsize, bscale;
	struct partinfo dpart;
	int n, on;
	int error = 0;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_READ)
		panic("spec_read mode");
	if (&uio->uio_vmspace->vm_map != kernel_map &&
	    uio->uio_vmspace != curproc->p_vmspace)
		panic("spec_read proc");
#endif
	if (uio->uio_resid == 0)
		return (0);

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0);
		cdev = cdevsw_lookup(vp->v_rdev);
		if (cdev != NULL)
			error = (*cdev->d_read)(vp->v_rdev, uio, ap->a_ioflag);
		else
			error = ENXIO;
		vn_lock(vp, LK_SHARED | LK_RETRY);
		return (error);

	case VBLK:
		if (uio->uio_offset < 0)
			return (EINVAL);
		bsize = BLKDEV_IOSIZE;
		bdev = bdevsw_lookup(vp->v_rdev);
		if (bdev != NULL &&
		    (*bdev->d_ioctl)(vp->v_rdev, DIOCGPART, (caddr_t)&dpart,
				     FREAD, l) == 0) {
			if (dpart.part->p_fstype == FS_BSDFFS &&
			    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0)
				bsize = dpart.part->p_frag *
				    dpart.part->p_fsize;
		}
		bscale = bsize >> DEV_BSHIFT;
		do {
			bn = (uio->uio_offset >> DEV_BSHIFT) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			error = bread(vp, bn, bsize, NOCRED, &bp);
			n = min(n, bsize - bp->b_resid);
			if (error) {
				brelse(bp);
				return (error);
			}
			error = uiomove((char *)bp->b_data + on, n, uio);
			brelse(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
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
spec_write(v)
	void *v;
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
	struct buf *bp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	daddr_t bn;
	int bsize, bscale;
	struct partinfo dpart;
	int n, on;
	int error = 0;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_WRITE)
		panic("spec_write mode");
	if (&uio->uio_vmspace->vm_map != kernel_map &&
	    uio->uio_vmspace != curproc->p_vmspace)
		panic("spec_write proc");
#endif

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0);
		cdev = cdevsw_lookup(vp->v_rdev);
		if (cdev != NULL)
			error = (*cdev->d_write)(vp->v_rdev, uio, ap->a_ioflag);
		else
			error = ENXIO;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		return (error);

	case VBLK:
		if (uio->uio_resid == 0)
			return (0);
		if (uio->uio_offset < 0)
			return (EINVAL);
		bsize = BLKDEV_IOSIZE;
		bdev = bdevsw_lookup(vp->v_rdev);
		if (bdev != NULL &&
		    (*bdev->d_ioctl)(vp->v_rdev, DIOCGPART, (caddr_t)&dpart,
				    FREAD, l) == 0) {
			if (dpart.part->p_fstype == FS_BSDFFS &&
			    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0)
				bsize = dpart.part->p_frag *
				    dpart.part->p_fsize;
		}
		bscale = bsize >> DEV_BSHIFT;
		do {
			bn = (uio->uio_offset >> DEV_BSHIFT) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			if (n == bsize)
				bp = getblk(vp, bn, bsize, 0, 0);
			else
				error = bread(vp, bn, bsize, NOCRED, &bp);
			if (error) {
				brelse(bp);
				return (error);
			}
			n = min(n, bsize - bp->b_resid);
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (error)
				brelse(bp);
			else {
				if (n + on == bsize)
					bawrite(bp);
				else
					bdwrite(bp);
				if (bp->b_flags & B_ERROR)
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
 * Device ioctl operation.
 */
/* ARGSUSED */
int
spec_ioctl(v)
	void *v;
{
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		void  *a_data;
		int  a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	struct vnode *vp;
	dev_t dev;

	/*
	 * Extract all the info we need from the vnode, taking care to
	 * avoid a race with VOP_REVOKE().
	 */

	vp = ap->a_vp;
	dev = NODEV;
	simple_lock(&vp->v_interlock);
	if ((vp->v_flag & VXLOCK) == 0 && vp->v_specinfo) {
		dev = vp->v_rdev;
	}
	simple_unlock(&vp->v_interlock);
	if (dev == NODEV) {
		return ENXIO;
	}

	switch (vp->v_type) {

	case VCHR:
		cdev = cdevsw_lookup(dev);
		if (cdev == NULL)
			return (ENXIO);
		return ((*cdev->d_ioctl)(dev, ap->a_command, ap->a_data,
		    ap->a_fflag, ap->a_l));

	case VBLK:
		bdev = bdevsw_lookup(dev);
		if (bdev == NULL)
			return (ENXIO);
		if (ap->a_command == 0 && (long)ap->a_data == B_TAPE) {
			if (bdev->d_type == D_TAPE)
				return (0);
			else
				return (1);
		}
		return ((*bdev->d_ioctl)(dev, ap->a_command, ap->a_data,
		   ap->a_fflag, ap->a_l));

	default:
		panic("spec_ioctl");
		/* NOTREACHED */
	}
}

/* ARGSUSED */
int
spec_poll(v)
	void *v;
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct lwp *a_l;
	} */ *ap = v;
	const struct cdevsw *cdev;
	dev_t dev;

	switch (ap->a_vp->v_type) {

	case VCHR:
		dev = ap->a_vp->v_rdev;
		cdev = cdevsw_lookup(dev);
		if (cdev == NULL)
			return (POLLERR);
		return (*cdev->d_poll)(dev, ap->a_events, ap->a_l);

	default:
		return (genfs_poll(v));
	}
}

/* ARGSUSED */
int
spec_kqfilter(v)
	void *v;
{
	struct vop_kqfilter_args /* {
		struct vnode	*a_vp;
		struct proc	*a_kn;
	} */ *ap = v;
	const struct cdevsw *cdev;
	dev_t dev;

	switch (ap->a_vp->v_type) {

	case VCHR:
		dev = ap->a_vp->v_rdev;
		cdev = cdevsw_lookup(dev);
		if (cdev == NULL)
			return (ENXIO);
		return (*cdev->d_kqfilter)(dev, ap->a_kn);
	default:
		/*
		 * Block devices don't support kqfilter, and refuse it
		 * for any other files (like those vflush()ed) too.
		 */
		return (EOPNOTSUPP);
	}
}

/*
 * Synch buffers associated with a block device
 */
/* ARGSUSED */
int
spec_fsync(v)
	void *v;
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int  a_flags;
		off_t offlo;
		off_t offhi;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	if (vp->v_type == VBLK)
		vflushbuf(vp, (ap->a_flags & FSYNC_WAIT) != 0);
	return (0);
}

/*
 * Just call the device strategy routine
 */
int
spec_strategy(v)
	void *v;
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp = ap->a_bp;
	int error, s;
	struct spec_cow_entry *e;

	error = 0;
	bp->b_dev = vp->v_rdev;
	if (!(bp->b_flags & B_READ) &&
	    (LIST_FIRST(&bp->b_dep)) != NULL && bioops.io_start)
		(*bioops.io_start)(bp);

	if (!(bp->b_flags & B_READ) && !SLIST_EMPTY(&vp->v_spec_cow_head)) {
		SPEC_COW_LOCK(vp->v_specinfo, s);
		while (vp->v_spec_cow_req > 0)
			ltsleep(&vp->v_spec_cow_req, PRIBIO, "cowlist", 0,
			    &vp->v_spec_cow_slock);
		vp->v_spec_cow_count++;
		SPEC_COW_UNLOCK(vp->v_specinfo, s);

		SLIST_FOREACH(e, &vp->v_spec_cow_head, ce_list) {
			if ((error = (*e->ce_func)(e->ce_cookie, bp)) != 0)
				break;
		}

		SPEC_COW_LOCK(vp->v_specinfo, s);
		vp->v_spec_cow_count--;
		if (vp->v_spec_cow_req && vp->v_spec_cow_count == 0)
			wakeup(&vp->v_spec_cow_req);
		SPEC_COW_UNLOCK(vp->v_specinfo, s);
	}

	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return (error);
	}

	DEV_STRATEGY(bp);

	return (0);
}

int
spec_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_l;
	} */ *ap = v;

	VOP_UNLOCK(ap->a_vp, 0);
	return (0);
}

/*
 * This is a noop, simply returning what one has been given.
 */
int
spec_bmap(v)
	void *v;
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
spec_close(v)
	void *v;
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	struct session *sess;
	dev_t dev = vp->v_rdev;
	int (*devclose)(dev_t, int, int, struct lwp *);
	int mode, error, count, flags, flags1;

	count = vcount(vp);
	flags = vp->v_flag;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Hack: a tty device that is a controlling terminal
		 * has a reference from the session structure.
		 * We cannot easily tell that a character device is
		 * a controlling terminal, unless it is the closing
		 * process' controlling terminal.  In that case,
		 * if the reference count is 2 (this last descriptor
		 * plus the session), release the reference from the session.
		 * Also remove the link from the tty back to the session
		 * and pgrp - due to the way consoles are handled we cannot
		 * guarantee that the vrele() will do the final close on the
		 * actual tty device.
		 */
		if (count == 2 && ap->a_l &&
		    vp == (sess = ap->a_l->l_proc->p_session)->s_ttyvp) {
			sess->s_ttyvp = NULL;
			if (sess->s_ttyp->t_session != NULL) {
				sess->s_ttyp->t_pgrp = NULL;
				sess->s_ttyp->t_session = NULL;
				SESSRELE(sess);
			} else if (sess->s_ttyp->t_pgrp != NULL)
				panic("spec_close: spurious pgrp ref");
			vrele(vp);
			count--;
		}
		/*
		 * If the vnode is locked, then we are in the midst
		 * of forcably closing the device, otherwise we only
		 * close on last reference.
		 */
		if (count > 1 && (flags & VXLOCK) == 0)
			return (0);
		cdev = cdevsw_lookup(dev);
		if (cdev != NULL)
			devclose = cdev->d_close;
		else
			devclose = NULL;
		mode = S_IFCHR;
		break;

	case VBLK:
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		error = vinvalbuf(vp, V_SAVE, ap->a_cred, ap->a_l, 0, 0);
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
		if (count > 1 && (flags & VXLOCK) == 0)
			return (0);
		bdev = bdevsw_lookup(dev);
		if (bdev != NULL)
			devclose = bdev->d_close;
		else
			devclose = NULL;
		mode = S_IFBLK;
		break;

	default:
		panic("spec_close: not special");
	}

	flags1 = ap->a_fflag;

	/*
	 * if VXLOCK is set, then we're going away soon, so make this
	 * non-blocking. Also ensures that we won't wedge in vn_lock below.
	 */
	if (flags & VXLOCK)
		flags1 |= FNONBLOCK;

	/*
	 * If we're able to block, release the vnode lock & reacquire. We
	 * might end up sleeping for someone else who wants our queues. They
	 * won't get them if we hold the vnode locked. Also, if VXLOCK is set,
	 * don't release the lock as we won't be able to regain it.
	 */
	if (!(flags1 & FNONBLOCK))
		VOP_UNLOCK(vp, 0);

	if (devclose != NULL)
		error = (*devclose)(dev, flags1, mode, ap->a_l);
	else
		error = ENXIO;

	if (!(flags1 & FNONBLOCK))
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	return (error);
}

/*
 * Print out the contents of a special device vnode.
 */
int
spec_print(v)
	void *v;
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;

	printf("tag VT_NON, dev %d, %d\n", major(ap->a_vp->v_rdev),
	    minor(ap->a_vp->v_rdev));
	return 0;
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
int
spec_pathconf(v)
	void *v;
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
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Advisory record locking support.
 */
int
spec_advlock(v)
	void *v;
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
