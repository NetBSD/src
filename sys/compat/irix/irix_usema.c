/*	$NetBSD: irix_usema.c,v 1.2 2002/05/22 05:14:03 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: irix_usema.c,v 1.2 2002/05/22 05:14:03 manu Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/queue.h>

#include <miscfs/genfs/genfs.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_extern.h>

#include <compat/irix/irix_usema.h>
#include <compat/irix/irix_ioctl.h>

#include <machine/irix_machdep.h>

/*
 * dev_t for the usemaclone device
 */
const dev_t irix_usemaclonedev = 
    makedev(IRIX_USEMADEV_MAJOR, IRIX_USEMACLNDEV_MINOR);

/*
 * THis table keeps track of the semaphore address for a given vnode.
 */
static LIST_HEAD(irix_usema_reclist, irix_usema_rec) irix_usema_reclist;

/*
 * In order to define a custom vnode operation vector for the usemaclone
 * driver, we need to define a dummy filesystem, featuring just a null
 * init function and the vnode operation vector. This is defined by 
 * irix_usema_dummy_vfsops, and registered to the kernel using vfs_attach
 * at driver attach time, in irix_usemaattach().
 */
struct vfsops irix_usema_dummy_vfsops = {
	"usema_dummy", 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, irix_usema_dummy_vfs_init, NULL, NULL, NULL, NULL, NULL,
	irix_usema_vnodeopv_descs,
};
void irix_usema_dummy_vfs_init(void) { return; } /* Do nothing */
const struct vnodeopv_desc * const irix_usema_vnodeopv_descs[] = { 
	&irix_usema_opv_desc,
	NULL,
};	
const struct vnodeopv_desc irix_usema_opv_desc = 
	{ &irix_usema_vnodeop_p, irix_usema_vnodeop_entries };
int (**irix_usema_vnodeop_p) __P((void *));

/*
 * Vnode operations on the usemaclone device
 */
const struct vnodeopv_entry_desc irix_usema_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, genfs_nullop },
	{ &vop_open_desc, genfs_nullop },
	{ &vop_close_desc, irix_usema_close },
	{ &vop_access_desc, irix_usema_access },
	{ &vop_getattr_desc, irix_usema_getattr },
	{ &vop_setattr_desc, irix_usema_setattr },
	{ &vop_ioctl_desc, irix_usema_ioctl },
	{ &vop_fcntl_desc, irix_usema_fcntl },
	{ &vop_poll_desc, irix_usema_poll },
	{ &vop_abortop_desc, genfs_abortop },
	{ &vop_lease_desc, genfs_nullop },
	{ &vop_lock_desc, genfs_lock },
	{ &vop_unlock_desc, genfs_unlock },
	{ &vop_islocked_desc, genfs_islocked },
	{ &vop_advlock_desc, genfs_nullop },
	{ &vop_fsync_desc, genfs_nullop },
	{ &vop_reclaim_desc, genfs_nullop },
	{ &vop_revoke_desc, genfs_revoke },
	{ &vop_inactive_desc, irix_usema_inactive },
	{ NULL, NULL},
};

struct irix_usema_softc {
	struct device irix_usema_dev;
};

/*
 * Initialize the usema driver: prepare the irix_usema_reclist chained list,
 * and attach the dummy filesystem we need to use custom vnode operations.
 */
void
irix_usemaattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux; 
{
	int error;

	LIST_INIT(&irix_usema_reclist);
	if ((error = vfs_attach(&irix_usema_dummy_vfsops)) != 0)
		panic("irix_usemaattach: vfs_attach() failed");

	return;
}

/* 
 * Standard device operations. Unused here.
 */
int irix_usemaopen(dev_t dev, int flags, int fmp, struct proc *p) { return 0; }
int irix_usemaread(dev_t dev, struct uio *uio, int flag) { return ENXIO; }
int irix_usemawrite(dev_t dev, struct uio *uio, int flag) { return ENXIO; }
int irix_usemaioctl(dev_t dev, u_long cmd, caddr_t data, int fl, struct proc *p)
    { return ENXIO; }
int irix_usemapoll(dev_t dev, int events, struct proc *p) { return ENXIO; }
int irix_usemaclose(dev_t dev, int flags, int fmt, struct proc *p) { return 0; }

/*
 * vnode operations on the device
 */
int
irix_usema_ioctl(v)
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
	u_long cmd = ap->a_command;
	caddr_t data = ap->a_data;
	struct vnode *vp = ap->a_vp;
	struct irix_usema_rec *iur;
	struct irix_ioctl_usrdata iiu;
	register_t *retval;
	int error;

#ifdef DEBUG_IRIX
	printf("irix_usema_ioctl(): vp = %p, cmd = %lx, data = %p\n", 
	    vp, cmd, data);
#endif
	
	/*
	 * Some ioctl commands need to set the ioctl return value. In
	 * irix_sys_ioctl(), we copy the return value address and the 
	 * data argument to the stackgap in a struct irix_ioctl_usrdata. 
	 * The address of this structure is passed as the data argument
	 * to the vnode layer. We therefore need to read this structure 
	 * to get the real data argument and the retval address.
	 */
	if ((error = copyin(data, &iiu, sizeof(iiu))) != 0)
		return error;
	data = iiu.iiu_data;
	retval = iiu.iiu_retval;

	switch (cmd) {
	case IRIX_UIOCABLOCKQ: /* semaphore has been blocked */
		return 0;
		break;

	case IRIX_UIOCAUNBLOCKQ: /* semaphore has been unblocked */
		LIST_FOREACH(iur, &irix_usema_reclist, iur_list)
			if (iur->iur_vn == vp) {
				iur->iur_wakeup = 1;
				wakeup((void *)&selwait);
				break;
			}
		if (iur == NULL) /* Nothing was found */
			return EBADF;
		return 0;

		break;

	case IRIX_UIOCGETCOUNT: { /* get semaphore value */
		struct irix_semaphore is;

		LIST_FOREACH(iur, &irix_usema_reclist, iur_list) {
			if (iur->iur_vn == vp) {
				if ((error = copyin(iur->iur_sem, 
				    &is, sizeof(is))) != 0)
					return error;
				if (is.is_val < 0)
					*retval = -1;
				break;
			}
		}
		if (iur == NULL) /* Nothing found */
			return EBADF;

		return 0;
		break;
	}

	case IRIX_UIOCIDADDR: { /* register address of sem. owner field */
		struct irix_usema_idaddr iui;
		struct irix_semaphore *isp;

		if ((error = copyin(data, &iui, sizeof(iui))) != 0)
			return error;

		/* 
		 * iui.iui_oidp points to the is_oid field of struct
		 * irix_semaphore. We want the structre address itself.
		 */
		isp = NULL;
		isp = (struct irix_semaphore *)((u_long)(isp) -
		    (u_long)(&isp->is_oid) + (u_long)iui.iui_oidp);

		iur = (struct irix_usema_rec *)
		    malloc(sizeof(struct irix_usema_rec), M_DEVBUF, M_WAITOK);
		iur->iur_vn = vp;
		iur->iur_sem = isp;
		iur->iur_wakeup = 0;
		LIST_INSERT_HEAD(&irix_usema_reclist, iur, iur_list);
		
		return 0;
		break;
	}
	default:
		printf("Warning: unimplemented IRIX usema ioctl command %ld\n",
		    (cmd & 0xff));
		break;
	}
	return 0;
}


int
irix_usema_poll(v)
	void *v;
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct proc *a_p;
	} */ *ap = v;
	int events = ap->a_events;
	struct vnode *vp = ap->a_vp;
	int ret = 0;
	struct irix_usema_rec *iur;
	int check = POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI;

#ifdef DEBUG_IRIX
	printf("irix_usema_poll() vn = %p, events = %d\n", vp, events);
#endif
	if (events & check) 
		LIST_FOREACH(iur, &irix_usema_reclist, iur_list) {
			if (iur->iur_vn == vp && iur->iur_wakeup != 0) {
				ret = events & check;
				iur->iur_wakeup = 0;
				break;
			}
		}
	return ret;
}

int
irix_usema_close(v)
	void *v;
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *rvp;
	struct irix_usema_rec *iur;
	int error;
	
#ifdef DEBUG_IRIX
	printf("irix_usema_close() vn = %p\n", vp);
#endif

	simple_lock(&vp->v_interlock);

	/* vp is a vnode duplicated from rvp. eventually also close rvp */
	rvp = (struct vnode *)(vp->v_data);
	vrele(rvp); 		/* for vref() in irix_sys_open() */
	vp->v_data = NULL;

	if (ap->a_fflag & FWRITE)
		rvp->v_writecount--;
	vn_lock(rvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(rvp, ap->a_fflag, ap->a_cred, ap->a_p);
	vput(rvp);

restart:
	LIST_FOREACH(iur, &irix_usema_reclist, iur_list) {
		if (iur->iur_vn == vp) {
			LIST_REMOVE(iur, iur_list);
			free(iur, M_DEVBUF);
			/* current iur is now invalid, restart */
			goto restart;
		}
	}

	simple_unlock(&vp->v_interlock);

	return error;
}

/* 
 * Try to apply setattr to the original vnode, not the duplicated one,
 * but still return 0 in case of failure (IRIX libc rely on this).
 */
int
irix_usema_setattr(v)
	void *v;
{
	struct vop_setattr_args /* {
		struct vnode    *a_vp;
		struct vattr    *a_vap;
		struct ucred    *a_cred;
		struct proc     *a_p;
	} */ *ap = v;
	struct vnode *vp = (struct vnode *)(ap->a_vp->v_data);
	int error;

#ifdef DEBUG_IRIX
	printf("irix_usema_setattr()\n");
#endif
	error = VOP_SETATTR(vp, ap->a_vap, ap->a_cred, ap->a_p);

	/* Silently ignore any error */
	return 0;
}

int
irix_usema_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode    *a_vp;
		struct proc     *a_p;
	} */ *ap = v;

	VOP_UNLOCK(ap->a_vp, 0);
	vrecycle(ap->a_vp, NULL, ap->a_p);

	return 0;
}


/* 
 * For fcntl, access and getattr vnode operations, we want to do the 
 * operation on the original vnode, not the duplicated one.
 */
#define ___CONCAT(x,y) __CONCAT(x,y)
#define __CONCAT3(x,y,z) ___CONCAT(__CONCAT(x,y),z)

#define IRIX_USEMA_VNOP_WRAP(op)				\
int								\
__CONCAT(irix_usema_,op)(v)					\
	void *v;						\
{								\
	struct __CONCAT3(vop_,op,_args) *ap = v;		\
	struct vnode *vp = (struct vnode *)(ap->a_vp->v_data);	\
	struct __CONCAT3(vop_,op,_args) a;			\
								\
	(void)memcpy(&a, ap, sizeof(a));			\
	a.a_vp = vp;						\
								\
	return VCALL(vp,VOFFSET(__CONCAT(vop_,op)),&a);		\
}

IRIX_USEMA_VNOP_WRAP(access)
IRIX_USEMA_VNOP_WRAP(getattr)
IRIX_USEMA_VNOP_WRAP(fcntl)
