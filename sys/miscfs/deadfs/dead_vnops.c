/*	$NetBSD: dead_vnops.c,v 1.33 2003/08/07 16:32:32 agc Exp $	*/

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
 *	@(#)dead_vnops.c	8.2 (Berkeley) 11/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dead_vnops.c,v 1.33 2003/08/07 16:32:32 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/proc.h>

#include <miscfs/genfs/genfs.h>

/*
 * Prototypes for dead operations on vnodes.
 */
int	dead_lookup	__P((void *));
#define dead_create	genfs_badop
#define dead_mknod	genfs_badop
int	dead_open	__P((void *));
#define dead_close	genfs_nullop
#define dead_access	genfs_ebadf
#define dead_getattr	genfs_ebadf
#define dead_setattr	genfs_ebadf
int	dead_read	__P((void *));
int	dead_write	__P((void *));
#define dead_lease_check genfs_nullop
#define dead_fcntl	genfs_nullop
int	dead_ioctl	__P((void *));
int	dead_poll	__P((void *));
#define dead_mmap	genfs_badop
#define dead_fsync	genfs_nullop
#define dead_seek	genfs_nullop
#define dead_remove	genfs_badop
#define dead_link	genfs_badop
#define dead_rename	genfs_badop
#define dead_mkdir	genfs_badop
#define dead_rmdir	genfs_badop
#define dead_symlink	genfs_badop
#define dead_readdir	genfs_ebadf
#define dead_readlink	genfs_ebadf
#define dead_abortop	genfs_badop
#define dead_inactive	genfs_nullop
#define dead_reclaim	genfs_nullop
int	dead_lock	__P((void *));
#define dead_unlock	genfs_nullop
int	dead_bmap	__P((void *));
int	dead_strategy	__P((void *));
int	dead_print	__P((void *));
#define dead_islocked	genfs_nullop
#define dead_pathconf	genfs_ebadf
#define dead_advlock	genfs_ebadf
#define dead_blkatoff	genfs_badop
#define dead_valloc	genfs_badop
#define dead_vfree	genfs_badop
#define dead_truncate	genfs_nullop
#define dead_update	genfs_nullop
#define dead_bwrite	genfs_nullop
#define dead_revoke	genfs_nullop
#define dead_putpages	genfs_null_putpages

int	chkvnlock __P((struct vnode *));

int (**dead_vnodeop_p) __P((void *));

const struct vnodeopv_entry_desc dead_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, dead_lookup },		/* lookup */
	{ &vop_create_desc, dead_create },		/* create */
	{ &vop_mknod_desc, dead_mknod },		/* mknod */
	{ &vop_open_desc, dead_open },			/* open */
	{ &vop_close_desc, dead_close },		/* close */
	{ &vop_access_desc, dead_access },		/* access */
	{ &vop_getattr_desc, dead_getattr },		/* getattr */
	{ &vop_setattr_desc, dead_setattr },		/* setattr */
	{ &vop_read_desc, dead_read },			/* read */
	{ &vop_write_desc, dead_write },		/* write */
	{ &vop_lease_desc, dead_lease_check },		/* lease */
	{ &vop_fcntl_desc, dead_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, dead_ioctl },		/* ioctl */
	{ &vop_poll_desc, dead_poll },			/* poll */
	{ &vop_revoke_desc, dead_revoke },		/* revoke */
	{ &vop_mmap_desc, dead_mmap },			/* mmap */
	{ &vop_fsync_desc, dead_fsync },		/* fsync */
	{ &vop_seek_desc, dead_seek },			/* seek */
	{ &vop_remove_desc, dead_remove },		/* remove */
	{ &vop_link_desc, dead_link },			/* link */
	{ &vop_rename_desc, dead_rename },		/* rename */
	{ &vop_mkdir_desc, dead_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, dead_rmdir },		/* rmdir */
	{ &vop_symlink_desc, dead_symlink },		/* symlink */
	{ &vop_readdir_desc, dead_readdir },		/* readdir */
	{ &vop_readlink_desc, dead_readlink },		/* readlink */
	{ &vop_abortop_desc, dead_abortop },		/* abortop */
	{ &vop_inactive_desc, dead_inactive },		/* inactive */
	{ &vop_reclaim_desc, dead_reclaim },		/* reclaim */
	{ &vop_lock_desc, dead_lock },			/* lock */
	{ &vop_unlock_desc, dead_unlock },		/* unlock */
	{ &vop_bmap_desc, dead_bmap },			/* bmap */
	{ &vop_strategy_desc, dead_strategy },		/* strategy */
	{ &vop_print_desc, dead_print },		/* print */
	{ &vop_islocked_desc, dead_islocked },		/* islocked */
	{ &vop_pathconf_desc, dead_pathconf },		/* pathconf */
	{ &vop_advlock_desc, dead_advlock },		/* advlock */
	{ &vop_blkatoff_desc, dead_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, dead_valloc },		/* valloc */
	{ &vop_vfree_desc, dead_vfree },		/* vfree */
	{ &vop_truncate_desc, dead_truncate },		/* truncate */
	{ &vop_update_desc, dead_update },		/* update */
	{ &vop_bwrite_desc, dead_bwrite },		/* bwrite */
	{ &vop_putpages_desc, dead_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc dead_vnodeop_opv_desc =
	{ &dead_vnodeop_p, dead_vnodeop_entries };

/*
 * Trivial lookup routine that always fails.
 */
/* ARGSUSED */
int
dead_lookup(v)
	void *v;
{
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap = v;

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

/*
 * Open always fails as if device did not exist.
 */
/* ARGSUSED */
int
dead_open(v)
	void *v;
{

	return (ENXIO);
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
int
dead_read(v)
	void *v;
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;

	if (chkvnlock(ap->a_vp))
		panic("dead_read: lock");
	/*
	 * Return EOF for tty devices, EIO for others
	 */
	if ((ap->a_vp->v_flag & VISTTY) == 0)
		return (EIO);
	return (0);
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
int
dead_write(v)
	void *v;
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;

	if (chkvnlock(ap->a_vp))
		panic("dead_write: lock");
	return (EIO);
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
int
dead_ioctl(v)
	void *v;
{
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;

	if (!chkvnlock(ap->a_vp))
		return (EBADF);
	return (VCALL(ap->a_vp, VOFFSET(vop_ioctl), ap));
}

/* ARGSUSED */
int
dead_poll(v)
	void *v;
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct proc *a_p;
	} */ *ap = v;

	/*
	 * Let the user find out that the descriptor is gone.
	 */
	return (ap->a_events);
}

/*
 * Just call the device strategy routine
 */
int
dead_strategy(v)
	void *v;
{

	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	if (ap->a_bp->b_vp == NULL || !chkvnlock(ap->a_bp->b_vp)) {
		ap->a_bp->b_flags |= B_ERROR;
		biodone(ap->a_bp);
		return (EIO);
	}
	return (VOP_STRATEGY(ap->a_bp));
}

/*
 * Wait until the vnode has finished changing state.
 */
int
dead_lock(v)
	void *v;
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;

	if (ap->a_flags & LK_INTERLOCK) {
		simple_unlock(&ap->a_vp->v_interlock);
		ap->a_flags &= ~LK_INTERLOCK;
	}
	if (!chkvnlock(ap->a_vp))
		return (0);
	return (VCALL(ap->a_vp, VOFFSET(vop_lock), ap));
}

/*
 * Wait until the vnode has finished changing state.
 */
int
dead_bmap(v)
	void *v;
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;

	if (!chkvnlock(ap->a_vp))
		return (EIO);
	return (VOP_BMAP(ap->a_vp, ap->a_bn, ap->a_vpp, ap->a_bnp, ap->a_runp));
}

/*
 * Print out the contents of a dead vnode.
 */
/* ARGSUSED */
int
dead_print(v)
	void *v;
{
	printf("tag VT_NON, dead vnode\n");
	return 0;
}

/*
 * We have to wait during times when the vnode is
 * in a state of change.
 */
int
chkvnlock(vp)
	struct vnode *vp;
{
	int locked = 0;

	while (vp->v_flag & VXLOCK) {
		vp->v_flag |= VXWANT;
		(void) tsleep(vp, PINOD, "deadchk", 0);
		locked = 1;
	}
	return (locked);
}
