/*	$NetBSD: irix_usema.c,v 1.28 2008/01/26 15:48:11 tsutsui Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_usema.c,v 1.28 2008/01/26 15:48:11 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/rwlock.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/simplelock.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/conf.h>

#include <miscfs/genfs/genfs.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_extern.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_usema.h>
#include <compat/irix/irix_ioctl.h>
#include <compat/irix/irix_syscallargs.h>

const struct cdevsw irix_usema_cdevsw = {
	nullopen, nullclose, noread, nowrite,
	noioctl, nostop, notty, nopoll, nommap, nokqfilter,
};

/*
 * semaphore list, and operations on the list
 */
static LIST_HEAD(irix_usema_reclist, irix_usema_rec) irix_usema_reclist;
static krwlock_t irix_usema_reclist_lock;

static struct irix_usema_rec *iur_lookup_by_vn(struct vnode *);
static struct irix_usema_rec *iur_lookup_by_sem(struct irix_semaphore *);
static struct irix_usema_rec *iur_insert
(struct irix_semaphore *, struct vnode *, struct proc *);
static void iur_remove(struct irix_usema_rec *);
static struct irix_waiting_proc_rec *iur_proc_queue
(struct irix_usema_rec *, struct proc *);
static void iur_proc_dequeue
(struct irix_usema_rec *, struct irix_waiting_proc_rec *);
static void iur_proc_release
(struct irix_usema_rec *, struct irix_waiting_proc_rec *);
static int iur_proc_isreleased(struct irix_usema_rec *, struct proc *);
static struct irix_waiting_proc_rec *iur_proc_getfirst
(struct irix_usema_rec *);

/*
 * In order to define a custom vnode operation vector for the usemaclone
 * driver, we need to define a dummy filesystem, featuring just a null
 * init function and the vnode operation vector. This is defined by
 * irix_usema_dummy_vfsops, and registered to the kernel using vfs_attach
 * at driver attach time, in irix_usemaattach().
 */
struct vfsops irix_usema_dummy_vfsops = {
	"usema_dummy", 0,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, irix_usema_dummy_vfs_init, NULL, NULL, NULL, NULL, NULL,
	NULL,
	irix_usema_vnodeopv_descs,
};
void irix_usema_dummy_vfs_init(void) { return; } /* Do nothing */
const struct vnodeopv_desc * const irix_usema_vnodeopv_descs[] = {
	&irix_usema_opv_desc,
	NULL,
};
const struct vnodeopv_desc irix_usema_opv_desc =
	{ &irix_usema_vnodeop_p, irix_usema_vnodeop_entries };
int (**irix_usema_vnodeop_p)(void *);

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
 * Initialize the usema driver: prepare the chained lists
 * and attach the dummy filesystem we need to use custom vnode operations.
 */
void
irix_usemaattach(struct device *parent, struct device *self, void *aux)
{
	int error;

	rw_init(&irix_usema_reclist_lock);
	LIST_INIT(&irix_usema_reclist);
	if ((error = vfs_attach(&irix_usema_dummy_vfsops)) != 0)
		panic("irix_usemaattach: vfs_attach() failed");

	return;
}

/*
 * vnode operations on the device
 */
int
irix_usema_ioctl(void *v)
{
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		void * a_data;
		int  a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	u_long cmd = ap->a_command;
	struct irix_ioctl_usrdata *iiu = ap->a_data;
	struct vnode *vp = ap->a_vp;
	struct irix_usema_rec *iur;
	struct irix_waiting_proc_rec *iwpr;
	void *data;
	register_t *retval;
	int error;

#ifdef DEBUG_IRIX
	printf("irix_usema_ioctl(): vp = %p, cmd = %lx, data = %p\n",
	    vp, cmd, data);
#endif

	/*
	 * Some ioctl commands need to set the ioctl return value. In
	 * irix_sys_ioctl(), we copy the return value address and the
	 * original data argument to a struct irix_ioctl_usrdata.
	 * The address of this structure is passed as the data argument
	 * to the vnode layer. We therefore need to read this structure
	 * to get the real data argument and the retval address.
	 */
	data = iiu->iiu_data;
	retval = iiu->iiu_retval;

	switch (cmd) {
	case IRIX_UIOCABLOCKQ: /* semaphore has been blocked */
		if ((iur = iur_lookup_by_vn(vp)) == NULL)
			return EBADF;

		iwpr = iur_proc_queue(iur, curlwp->l_proc);
		break;

	case IRIX_UIOCAUNBLOCKQ: /* semaphore has been unblocked */
		if ((iur = iur_lookup_by_vn(vp)) == NULL)
			return EBADF;

		if ((iwpr = iur_proc_getfirst(iur)) != NULL) {
			extern kcondvar_t select_cv;
			iur_proc_release(iur, iwpr);
			cv_broadcast(&select_cv);
		}
		break;

	case IRIX_UIOCGETCOUNT: /* get semaphore value */
		if ((iur = iur_lookup_by_vn(vp)) == NULL)
			return EBADF;

		*retval = -iur->iur_waiting_count;
		break;

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

		if ((iur_insert(isp, vp, curlwp->l_proc)) == NULL)
			return EFAULT;
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
irix_usema_poll(void *v)
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
	} */ *ap = v;
	int events = ap->a_events;
	struct vnode *vp = ap->a_vp;
	struct irix_usema_rec *iur;
	int check = POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI;

#ifdef DEBUG_IRIX
	printf("irix_usema_poll() vn = %p, events = 0x%x\n", vp, events);
#endif
	if ((events & check) == 0)
		return 0;

	if ((iur = iur_lookup_by_vn(vp)) == NULL)
		return 0;

	if (iur_proc_isreleased(iur, curlwp->l_proc) == 0)
		return 0;

	return (events & check);
}

int
irix_usema_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		kauth_cred_t a_cred;
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
	error = VOP_CLOSE(rvp, ap->a_fflag, ap->a_cred);
	vput(rvp);

	if ((iur = iur_lookup_by_vn(vp)) != NULL)
		iur_remove(iur);

	simple_unlock(&vp->v_interlock);

	return error;
}

/*
 * Try to apply setattr to the original vnode, not the duplicated one,
 * but still return 0 in case of failure (IRIX libc rely on this).
 */
int
irix_usema_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode    *a_vp;
		struct vattr    *a_vap;
		kauth_cred_t	 a_cred;
	} */ *ap = v;
	struct vnode *vp = (struct vnode *)(ap->a_vp->v_data);
	int error;

#ifdef DEBUG_IRIX
	printf("irix_usema_setattr()\n");
#endif
	error = VOP_SETATTR(vp, ap->a_vap, ap->a_cred);

	/* Silently ignore any error */
	return 0;
}

int
irix_usema_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode    *a_vp;
	} */ *ap = v;

	VOP_UNLOCK(ap->a_vp, 0);
	vrecycle(ap->a_vp, NULL, curlwp);

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

/*
 * The usync_ctnl system call is not part of the usema driver,
 * but it is closely related to it.
 */
int
irix_sys_usync_cntl(struct lwp *l, const struct irix_sys_usync_cntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */
	struct proc *p = l->l_proc;
	int error;
	struct irix_usync_arg iua;
	struct irix_usema_rec *iur;
	struct irix_waiting_proc_rec *iwpr;

	switch (SCARG(uap, cmd)) {
	case IRIX_USYNC_BLOCK:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		if ((iur = iur_insert(iua.iua_sem, NULL, p)) == NULL)
			return EFAULT;

		iwpr = iur_proc_queue(iur, p);
		(void)tsleep(iwpr, PZERO, "irix_usema", 0);
		break;

	case IRIX_USYNC_INTR_BLOCK:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		if ((iur = iur_insert(iua.iua_sem, NULL, p)) == NULL)
			return EFAULT;

		iwpr = iur_proc_queue(iur, p);
		(void)tsleep(iwpr, PZERO|PCATCH, "irix_usema", 0);
		break;

	case IRIX_USYNC_UNBLOCK_ALL:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		if ((iur = iur_lookup_by_sem(iua.iua_sem)) == 0)
			return EINVAL;

		rw_enter(&iur->iur_lock, RW_READER);
		TAILQ_FOREACH(iwpr, &iur->iur_waiting_p, iwpr_list) {
			wakeup((void *)iwpr);
			iur_proc_dequeue(iur, iwpr);
		}
		iur_remove(iur);
		rw_exit(&iur->iur_lock);
		break;

	case IRIX_USYNC_UNBLOCK:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		if ((iur = iur_lookup_by_sem(iua.iua_sem)) == 0)
			return EINVAL;

		if ((iwpr = iur_proc_getfirst(iur)) != NULL) {
			wakeup((void *)iwpr);
			iur_proc_dequeue(iur, iwpr);
		}
		if ((iwpr = iur_proc_getfirst(iur)) == NULL)
			iur_remove(iur);
		break;

	case IRIX_USYNC_GET_STATE:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		if ((iur = iur_lookup_by_sem(iua.iua_sem)) == NULL)
			return 0; /* Not blocked, return 0 */

		*retval = -iur->iur_waiting_count;
		break;
	default:
		printf("Warning: unimplemented IRIX usync_cntl command %d\n",
		    SCARG(uap, cmd));
		return EINVAL;
	}

	return 0;
}

/* Operations on irix_usema_reclist */
static struct irix_usema_rec *
iur_lookup_by_vn(struct vnode *vp)
{
	struct irix_usema_rec *iur;

	rw_enter(&irix_usema_reclist_lock, RW_READER);
	LIST_FOREACH(iur, &irix_usema_reclist, iur_list)
		if (iur->iur_vn == vp)
			break;
	rw_exit(&irix_usema_reclist_lock);
	return iur;
}

static struct irix_usema_rec *
iur_lookup_by_sem(struct irix_semaphore *sem)
{
	struct irix_usema_rec *iur;
	struct irix_semaphore is;
	int error;

	if ((error = copyin(sem, &is, sizeof(is))) != 0)
		return NULL;

	rw_enter(&irix_usema_reclist_lock, RW_READER);
	LIST_FOREACH(iur, &irix_usema_reclist, iur_list)
		if (iur->iur_sem == sem && iur->iur_shid == is.is_shid)
			break;
	rw_exit(&irix_usema_reclist_lock);

	return iur;
}

static struct irix_usema_rec *
iur_insert(struct irix_semaphore *sem, struct vnode *vp, struct proc *p)
{
	struct irix_usema_rec *iur;
	struct irix_semaphore is;
	int error;

	if ((iur = iur_lookup_by_sem(sem)) != NULL)
		return iur;

	if ((error = copyin(sem, &is, sizeof(is))) != 0)
		return NULL;

	iur = malloc(sizeof(struct irix_usema_rec), M_DEVBUF, M_WAITOK);
	iur->iur_sem = sem;
	iur->iur_vn = vp;
	iur->iur_shid = is.is_shid;
	iur->iur_p = p;
	iur->iur_waiting_count = 0;
	rw_init(&iur->iur_lock);
	TAILQ_INIT(&iur->iur_waiting_p);
	TAILQ_INIT(&iur->iur_released_p);
	rw_enter(&irix_usema_reclist_lock, RW_WRITER);
	LIST_INSERT_HEAD(&irix_usema_reclist, iur, iur_list);
	rw_exit(&irix_usema_reclist_lock);
	return iur;
}

static void
iur_remove(struct irix_usema_rec *iur)
{
	struct irix_waiting_proc_rec *iwpr;

	rw_enter(&iur->iur_lock, RW_WRITER);
waiting_restart:
	TAILQ_FOREACH(iwpr, &iur->iur_waiting_p, iwpr_list) {
		TAILQ_REMOVE(&iur->iur_waiting_p, iwpr, iwpr_list);
		free(iwpr, M_DEVBUF);
		/* iwpr is now invalid, restart */
		goto waiting_restart;
	}

released_restart:
	TAILQ_FOREACH(iwpr, &iur->iur_released_p, iwpr_list) {
		TAILQ_REMOVE(&iur->iur_released_p, iwpr, iwpr_list);
		free(iwpr, M_DEVBUF);
		/* iwpr is now invalid, restart */
		goto released_restart;
	}

	rw_enter(&irix_usema_reclist_lock, RW_WRITER);
	LIST_REMOVE(iur, iur_list);
	rw_exit(&irix_usema_reclist_lock);

	rw_exit(&iur->iur_lock);
	rw_destroy(&iur->iur_lock);
	free(iur, M_DEVBUF);
	return;
}

static struct irix_waiting_proc_rec *
iur_proc_queue(struct irix_usema_rec *iur, struct proc *p)
{
	struct irix_waiting_proc_rec *iwpr;

	/* Do we have this iwpr on the released list? If we do, reuse it */
	rw_enter(&iur->iur_lock, RW_WRITER);
	TAILQ_FOREACH(iwpr, &iur->iur_released_p, iwpr_list) {
		if (iwpr->iwpr_p == p) {
			TAILQ_REMOVE(&iur->iur_released_p, iwpr, iwpr_list);
			goto got_iwpr;
		}
	}

	/* Otherwise, create a new one */
	iwpr = malloc(sizeof(struct irix_waiting_proc_rec), M_DEVBUF, M_WAITOK);
	iwpr->iwpr_p = p;
got_iwpr:
	TAILQ_INSERT_TAIL(&iur->iur_waiting_p, iwpr, iwpr_list);
	iur->iur_waiting_count++;
	rw_exit(&iur->iur_lock);
	return iwpr;
}

static void
iur_proc_dequeue(struct irix_usema_rec *iur, struct irix_waiting_proc_rec *iwpr)
{
	rw_enter(&iur->iur_lock, RW_WRITER);
	iur->iur_waiting_count--;
	TAILQ_REMOVE(&iur->iur_waiting_p, iwpr, iwpr_list);
	rw_exit(&iur->iur_lock);
	free(iwpr, M_DEVBUF);
	return;
}

static void
iur_proc_release(struct irix_usema_rec *iur, struct irix_waiting_proc_rec *iwpr)
{
	rw_enter(&iur->iur_lock, RW_WRITER);
	iur->iur_waiting_count--;
	TAILQ_REMOVE(&iur->iur_waiting_p, iwpr, iwpr_list);
	TAILQ_INSERT_TAIL(&iur->iur_released_p, iwpr, iwpr_list);
	rw_exit(&iur->iur_lock);
	return;
}


static int
iur_proc_isreleased(struct irix_usema_rec *iur, struct proc *p)
{
	struct irix_waiting_proc_rec *iwpr;
	int res = 0;

	rw_enter(&iur->iur_lock, RW_READER);
	TAILQ_FOREACH(iwpr, &iur->iur_released_p, iwpr_list) {
		if (iwpr->iwpr_p == p) {
			res = 1;
			break;
		}
	}
	rw_exit(&iur->iur_lock);
	return res;
}

static struct irix_waiting_proc_rec *
iur_proc_getfirst(struct irix_usema_rec *iur)
{
	struct irix_waiting_proc_rec *iwpr;

	rw_enter(&iur->iur_lock, RW_READER);
	iwpr = TAILQ_FIRST(&iur->iur_waiting_p);
	rw_exit(&iur->iur_lock);
	return iwpr;
}

/*
 * Cleanup irix_usema_usync_cntl() allocations
 * if new_p is NULL, free any structure allocated for process p
 * otherwise change ownership of structure allocated for process p to new_p
 */
void
irix_usema_exit_cleanup(struct proc *p, struct proc *new_p)
{
	struct irix_usema_rec *iur;

#ifdef DEBUG_IRIX
	printf("irix_usema_exit_cleanup(): p = %p, new_p = %p\n", p, new_p);
#endif
remove_restart:
	rw_enter(&irix_usema_reclist_lock, RW_WRITER);
	LIST_FOREACH(iur, &irix_usema_reclist, iur_list) {
		if (iur->iur_p != p)
			continue;
		if (new_p == NULL) {
			/*
			 * Release the lock now since iur_remove() needs to
			 * acquire an exclusive lock.
			 */
			rw_exit(&irix_usema_reclist_lock);
			iur_remove(iur);
			/*
			 * iur is now invalid and we lost the lock, restart
			 */
			goto remove_restart;
		} else {
			iur->iur_p = new_p;
		}
	}
	rw_exit(&irix_usema_reclist_lock);

	return;
}

#ifdef DEBUG_IRIX
/*
 * This dumps all in-kernel information about processes waiting for
 * semaphores and process that have been released by an operation
 * on a semaphore.
 */
void
irix_usema_debug(void)
{
	struct irix_usema_rec *iur;
	struct irix_waiting_proc_rec *iwpr;

	LIST_FOREACH(iur, &irix_usema_reclist, iur_list) {
		printf("iur %p\n", iur);
		printf("  iur->iur_vn = %p\n", iur->iur_vn);
		printf("  iur->iur_sem = %p\n", iur->iur_sem);
		printf("  iur->iur_shid = 0x%08x\n", iur->iur_shid);
		printf("  iur->iur_p = %p\n", iur->iur_p);
		printf("  iur->iur_waiting_count = %d\n",
		    iur->iur_waiting_count);
		printf("  Waiting processes\n");
		TAILQ_FOREACH(iwpr, &iur->iur_waiting_p, iwpr_list) {
			printf("    iwpr %p: iwpr->iwpr_p = %p (pid %d)\n",
			    iwpr, iwpr->iwpr_p, iwpr->iwpr_p->p_pid);
		}
		printf("  Released processes\n");
		TAILQ_FOREACH(iwpr, &iur->iur_released_p, iwpr_list) {
			printf("    iwpr %p: iwpr->iwpr_p = %p (pid %d)\n",
			    iwpr, iwpr->iwpr_p, iwpr->iwpr_p->p_pid);
		}
	}
}
#endif /* DEBUG_IRIX */
