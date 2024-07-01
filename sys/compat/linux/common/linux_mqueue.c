/*	$NetBSD	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_mqueue.c,v 1.1 2024/07/01 01:35:53 christos Exp $");

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/mqueue.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_sigevent.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_mqueue.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

static void
linux_to_bsd_mq_attr(const struct linux_mq_attr *lmp, struct mq_attr *bmp)
{
	memset(bmp, 0, sizeof(*bmp));
	bmp->mq_flags = cvtto_bsd_mask(lmp->mq_flags, LINUX_O_NONBLOCK,
	    O_NONBLOCK);
	bmp->mq_maxmsg = lmp->mq_maxmsg;
	bmp->mq_msgsize = lmp->mq_msgsize;
	bmp->mq_curmsgs = lmp->mq_curmsgs;
}

static void
bsd_to_linux_mq_attr(const struct mq_attr *bmp, struct linux_mq_attr *lmp)
{
	memset(lmp, 0, sizeof(*lmp));
	lmp->mq_flags = cvtto_linux_mask(bmp->mq_flags, O_NONBLOCK,
	    LINUX_O_NONBLOCK);
	lmp->mq_maxmsg = bmp->mq_maxmsg;
	lmp->mq_msgsize = bmp->mq_msgsize;
	lmp->mq_curmsgs = bmp->mq_curmsgs;
}

/* Adapted from sys_mq_open */
int
linux_sys_mq_open(struct lwp *l, const struct linux_sys_mq_open_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) name;
		syscallarg(int) oflag;
		syscallarg(linux_umode_t) mode;
		syscallarg(struct linux_mq_attr *) attr;
	} */
	struct linux_mq_attr lattr;
	struct mq_attr *attr = NULL, a;
	int error, oflag;

	oflag = linux_to_bsd_ioflags(SCARG(uap, oflag));

	if ((oflag & O_CREAT) != 0 && SCARG(uap, attr) != NULL) {
		error = copyin(SCARG(uap, attr), &lattr, sizeof(lattr));
		if (error)
			return error;
		linux_to_bsd_mq_attr(&lattr, &a);
		attr = &a;
	}

	return mq_handle_open(l, SCARG(uap, name), oflag,
	    (mode_t)SCARG(uap, mode), attr, retval);
}

int
linux_sys_mq_unlink(struct lwp *l, const struct linux_sys_mq_unlink_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) name;
	} */
	struct sys_mq_unlink_args args;

	SCARG(&args, name) = SCARG(uap, name);

	return sys_mq_unlink(l, &args, retval);
}

/* Adapted from sys___mq_timedsend50 */
int
linux_sys_mq_timedsend(struct lwp *l, const struct linux_sys_mq_timedsend_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(const char *) msg_ptr;
		syscallarg(size_t) msg_len;
		syscallarg(unsigned int) msg_prio;
		syscallarg(const struct linux_timespec *) abs_timeout;
	} */
	struct linux_timespec lts;
	struct timespec ts, *tsp;
	int error;

	/* Get and convert time value */
	if (SCARG(uap, abs_timeout)) {
		error = copyin(SCARG(uap, abs_timeout), &lts, sizeof(lts));
		if (error)
			return error;
		linux_to_native_timespec(&ts, &lts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return mq_send1((mqd_t)SCARG(uap, mqdes), SCARG(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp);
}

/* Adapted from sys___mq_timedreceive50 */
int
linux_sys_mq_timedreceive(struct lwp *l,
    const struct linux_sys_mq_timedreceive_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(char *) msg_ptr;
		syscallarg(size_t) msg_len;
		syscallarg(unsigned int *) msg_prio;
		syscallarg(const struct linux_timespec *) abs_timeout;
	}; */
	struct linux_timespec lts;
	struct timespec ts, *tsp;
	ssize_t mlen;
	int error;

	/* Get and convert time value */
	if (SCARG(uap, abs_timeout)) {
		error = copyin(SCARG(uap, abs_timeout), &lts, sizeof(lts));
		if (error)
			return error;
		linux_to_native_timespec(&ts, &lts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	error = mq_recv1((mqd_t)SCARG(uap, mqdes), SCARG(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp, &mlen);
	if (error == 0)
		*retval = mlen;

	return error;
}

/* Adapted from sys_mq_notify */
int
linux_sys_mq_notify(struct lwp *l, const struct linux_sys_mq_notify_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(const struct linux_sigevent *) sevp;
	} */
	struct mqueue *mq;
	struct sigevent sig;
	int error;

	if (SCARG(uap, sevp)) {
		/* Get the signal from user-space */
		error = linux_sigevent_copyin(SCARG(uap, sevp), &sig,
		    sizeof(sig));
		if (error)
			return error;
		if (sig.sigev_notify == SIGEV_SIGNAL &&
		    (sig.sigev_signo <= 0 || sig.sigev_signo >= NSIG))
			return EINVAL;
	}

	error = mqueue_get((mqd_t)SCARG(uap, mqdes), 0, &mq);
	if (error)
		return error;
	if (SCARG(uap, sevp)) {
		/* Register notification: set the signal and target process */
		if (mq->mq_notify_proc == NULL) {
			memcpy(&mq->mq_sig_notify, &sig,
			    sizeof(struct sigevent));
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

/* Adapted from sys_mq_getattr */
static int
linux_sys_mq_getattr(struct lwp *l,
    const struct linux_sys_mq_getsetattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(const struct linux_mq_attr *) newattr;
		syscallarg(struct linux_mq_attr *) oldattr;
	} */
	struct linux_mq_attr lattr;
	struct mq_attr attr;
	struct mqueue *mq;
	int error;

	error = mqueue_get((mqd_t)SCARG(uap, mqdes), 0, &mq);
	if (error)
		return error;
	memcpy(&attr, &mq->mq_attrib, sizeof(struct mq_attr));
	bsd_to_linux_mq_attr(&attr, &lattr);
	mutex_exit(&mq->mq_mtx);
	fd_putfile((int)SCARG(uap, mqdes));

	return copyout(&lattr, SCARG(uap, oldattr), sizeof(lattr));
}

/* Adapted from sys_mq_setattr */
static int
linux_sys_mq_setattr(struct lwp *l,
    const struct linux_sys_mq_getsetattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(const struct linux_mq_attr *) newattr;
		syscallarg(struct linux_mq_attr *) oldattr;
	} */
	struct linux_mq_attr lattr;
	struct mq_attr attr;
	struct mqueue *mq;
	int error, nonblock;

	error = copyin(SCARG(uap, newattr), &lattr, sizeof(lattr));
	if (error)
		return error;
	linux_to_bsd_mq_attr(&lattr, &attr);
	nonblock = (attr.mq_flags & O_NONBLOCK);

	error = mqueue_get((mqd_t)SCARG(uap, mqdes), 0, &mq);
	if (error)
		return error;

	/* Copy the old attributes, if needed */
	if (SCARG(uap, oldattr)) {
		memcpy(&attr, &mq->mq_attrib, sizeof(struct mq_attr));
		bsd_to_linux_mq_attr(&attr, &lattr);
	}

	/* Ignore everything, except O_NONBLOCK */
	if (nonblock)
		mq->mq_attrib.mq_flags |= O_NONBLOCK;
	else
		mq->mq_attrib.mq_flags &= ~O_NONBLOCK;

	mutex_exit(&mq->mq_mtx);
	fd_putfile((int)SCARG(uap, mqdes));

	/* Copy the data to the user-space. */
	if (SCARG(uap, oldattr))
		return copyout(&lattr, SCARG(uap, oldattr), sizeof(lattr));

	return 0;
}

int
linux_sys_mq_getsetattr(struct lwp *l,
    const struct linux_sys_mq_getsetattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_mqd_t) mqdes;
		syscallarg(const struct linux_mq_attr *) newattr;
		syscallarg(struct linux_mq_attr *) oldattr;
	} */
	if (SCARG(uap, newattr) == NULL)
		return linux_sys_mq_getattr(l, uap, retval);
	return linux_sys_mq_setattr(l, uap, retval);
}
