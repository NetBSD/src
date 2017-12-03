/*	$NetBSD: netbsd32_mqueue.c,v 1.6.16.2 2017/12/03 11:36:56 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_mqueue.c,v 1.6.16.2 2017/12/03 11:36:56 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/module.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

extern struct emul emul_netbsd32;

int
netbsd32_mq_open(struct lwp *l, const struct netbsd32_mq_open_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) name;
		syscallarg(int) oflag;
		syscallarg(mode_t) mode;
		syscallarg(struct netbsd32_mq_attrp_t) attr;
	} */
	struct netbsd32_mq_attr attr32;
	struct mq_attr *attr = NULL, a;
	int error;

	if ((SCARG(uap, oflag) & O_CREAT) && SCARG_P32(uap, attr) != NULL) {
		error = copyin(SCARG_P32(uap, attr), &attr32, sizeof(attr32));
		if (error)
			return error;
		netbsd32_to_mq_attr(&attr32, &a);
		attr = &a;
	}

	return mq_handle_open(l, SCARG_P32(uap, name), SCARG(uap, oflag),
	    SCARG(uap, mode), attr, retval);
}

int
netbsd32_mq_close(struct lwp *l, const struct netbsd32_mq_close_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
	} */

	return netbsd32_close(l, (const void*)uap, retval);
}

int
netbsd32_mq_unlink(struct lwp *l, const struct netbsd32_mq_unlink_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys_mq_unlink_args ua;

	NETBSD32TOP_UAP(name, const char);
	return sys_mq_unlink(l, &ua, retval);
}

int
netbsd32_mq_getattr(struct lwp *l, const struct netbsd32_mq_getattr_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(netbsd32_mq_attrp_t) mqstat;
	} */
	struct mqueue *mq;
	struct mq_attr attr;
	struct netbsd32_mq_attr a32;
	int error;

	error = mqueue_get(SCARG(uap, mqdes), 0, &mq);
	if (error)
		return error;

	memcpy(&attr, &mq->mq_attrib, sizeof(struct mq_attr));
	mutex_exit(&mq->mq_mtx);
	fd_putfile((int)SCARG(uap, mqdes));
	netbsd32_from_mq_attr(&attr, &a32);
	return copyout(&a32, SCARG_P32(uap,mqstat), sizeof(a32));
}

int
netbsd32_mq_setattr(struct lwp *l, const struct netbsd32_mq_setattr_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const netbsd32_mq_attrp_t) mqstat;
		syscallarg(netbsd32_mq_attrp_t) omqstat;
	} */
	struct mqueue *mq;
	struct netbsd32_mq_attr attr32;
	struct mq_attr attr;
	int error, nonblock;

	error = copyin(SCARG_P32(uap, mqstat), &attr32, sizeof(attr32));
	if (error)
		return error;
	netbsd32_to_mq_attr(&attr32, &attr);
	nonblock = (attr.mq_flags & O_NONBLOCK);

	error = mqueue_get(SCARG(uap, mqdes), 0, &mq);
	if (error)
		return error;

	/* Copy the old attributes, if needed */
	if (SCARG_P32(uap, omqstat))
		memcpy(&attr, &mq->mq_attrib, sizeof(struct mq_attr));

	/* Ignore everything, except O_NONBLOCK */
	if (nonblock)
		mq->mq_attrib.mq_flags |= O_NONBLOCK;
	else
		mq->mq_attrib.mq_flags &= ~O_NONBLOCK;

	mutex_exit(&mq->mq_mtx);
	fd_putfile((int)SCARG(uap, mqdes));

	/*
	 * Copy the data to the user-space.
	 * Note: According to POSIX, the new attributes should not be set in
	 * case of fail - this would be violated.
	 */
	if (SCARG_P32(uap, omqstat)) {
		netbsd32_from_mq_attr(&attr, &attr32);
		error = copyout(&attr32, SCARG_P32(uap, omqstat),
		    sizeof(attr32));
	}

	return error;
}

int
netbsd32_mq_notify(struct lwp *l, const struct netbsd32_mq_notify_args *uap,
    register_t *result)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const netbsd32_sigeventp_t) notification;
	} */
	struct mqueue *mq;
	struct netbsd32_sigevent sig32;
	int error;

	if (SCARG_P32(uap, notification)) {
		/* Get the signal from user-space */
		error = copyin(SCARG_P32(uap, notification), &sig32,
		    sizeof(sig32));
		if (error)
			return error;
		if (sig32.sigev_notify == SIGEV_SIGNAL &&
		    (sig32.sigev_signo <=0 || sig32.sigev_signo >= NSIG))
			return EINVAL;
	}

	error = mqueue_get(SCARG(uap, mqdes), 0, &mq);
	if (error) {
		return error;
	}
	if (SCARG_P32(uap, notification)) {
		/* Register notification: set the signal and target process */
		if (mq->mq_notify_proc == NULL) {
			netbsd32_to_sigevent(&sig32, &mq->mq_sig_notify);
			mq->mq_notify_proc = l->l_proc;
		} else {
			/* Fail if someone else already registered */
			error = EBUSY;
		}
	} else {
		/* Unregister the notification */
		mq->mq_notify_proc = NULL;
	}
	mutex_exit(&mq->mq_mtx);
	fd_putfile((int)SCARG(uap, mqdes));

	return error;
}

int
netbsd32_mq_send(struct lwp *l, const struct netbsd32_mq_send_args *uap,
    register_t *result)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(unsigned) msg_prio;
	} */


	return mq_send1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), NULL);
}

int
netbsd32_mq_receive(struct lwp *l, const struct netbsd32_mq_receive_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(netbsd32_uintp) msg_prio;
	} */
	ssize_t mlen;
	int error;

	error = mq_recv1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG_P32(uap, msg_prio), NULL, &mlen);
	if (error == 0)
		*retval = mlen;

	return error;
}

int
netbsd32___mq_timedsend50(struct lwp *l,
     const struct netbsd32___mq_timedsend50_args *uap, register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(unsigned) msg_prio;
		syscallarg(const netbsd32_timespecp_t) abs_timeout;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec ts32;
	int error;

	/* Get and convert time value */
	if (SCARG_P32(uap, abs_timeout)) {
		error = copyin(SCARG_P32(uap, abs_timeout), &ts32,
		     sizeof(ts32));
		if (error)
			return error;
		netbsd32_to_timespec(&ts32, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return mq_send1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp);
}

int
netbsd32___mq_timedreceive50(struct lwp *l,
    const struct netbsd32___mq_timedreceive50_args *uap, register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(netbsd32_uintp) msg_prio;
		syscallarg(const netbsd32_timespecp_t) abs_timeout;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec ts32;
	ssize_t mlen;
	int error;

	/* Get and convert time value */
	if (SCARG_P32(uap, abs_timeout)) {
		error = copyin(SCARG_P32(uap, abs_timeout), &ts32,
		    sizeof(ts32));
		if (error)
			return error;
		netbsd32_to_timespec(&ts32, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	error = mq_recv1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG_P32(uap, msg_prio), tsp, &mlen);
	if (error == 0)
		*retval = mlen;

	return error;
}

#ifdef COMPAT_50

int
compat_50_netbsd32_mq_timedsend(struct lwp *l,
    const struct compat_50_netbsd32_mq_timedsend_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(unsigned) msg_prio;
		syscallarg(const netbsd32_timespec50p_t) abs_timeout;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec50 ts32;
	int error;

	/* Get and convert time value */
	if (SCARG_P32(uap, abs_timeout)) {
		error = copyin(SCARG_P32(uap, abs_timeout), &ts32,
		     sizeof(ts32));
		if (error)
			return error;
		netbsd32_to_timespec50(&ts32, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return mq_send1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp);
}

int
compat_50_netbsd32_mq_timedreceive(struct lwp *l,
    const struct compat_50_netbsd32_mq_timedreceive_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(netbsd32_charp) msg_ptr;
		syscallarg(netbsd32_size_t) msg_len;
		syscallarg(netbsd32_uintp) msg_prio;
		syscallarg(const netbsd32_timespec50p_t) abs_timeout;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec50 ts32;
	ssize_t mlen;
	int error;

	/* Get and convert time value */
	if (SCARG_P32(uap, abs_timeout)) {
		error = copyin(SCARG_P32(uap, abs_timeout), &ts32,
		    sizeof(ts32));
		if (error)
			return error;
		netbsd32_to_timespec50(&ts32, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	error = mq_recv1(SCARG(uap, mqdes), SCARG_P32(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG_P32(uap, msg_prio), tsp, &mlen);
	if (error == 0)
		*retval = mlen;

	return error;
}

#endif /* COMPAT_50 */

#define _PKG_ENTRY(name)	\
	{ NETBSD32_SYS_ ## name, 0, (sy_call_t *)name }

static const struct syscall_package compat_mqueue_syscalls[] = {
	_PKG_ENTRY(netbsd32_mq_open),
	_PKG_ENTRY(netbsd32_mq_close),
	_PKG_ENTRY(netbsd32_mq_unlink),
	_PKG_ENTRY(netbsd32_mq_getattr),
	_PKG_ENTRY(netbsd32_mq_setattr),
	_PKG_ENTRY(netbsd32_mq_notify),
	_PKG_ENTRY(netbsd32_mq_send),
	_PKG_ENTRY(netbsd32_mq_receive),
	_PKG_ENTRY(netbsd32___mq_timedsend50),
	_PKG_ENTRY(netbsd32___mq_timedreceive50),
#ifdef COMPAT_50
	_PKG_ENTRY(compat_50_netbsd32_mq_timedsend),
	_PKG_ENTRY(compat_50_netbsd32_mq_timedreceive),
#endif
	{0, 0, NULL}
};

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_mqueue, "mqueue,compat_netbsd32");

static int      
compat_netbsd32_mqueue_modcmd(modcmd_t cmd, void *arg)
{               
	int error;      
                
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(&emul_netbsd32,
		    compat_mqueue_syscalls);
		break;
	case MODULE_CMD_FINI:
		error = syscall_disestablish(&emul_netbsd32,
		    compat_mqueue_syscalls);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
