/*	$NetBSD: sys_eventfd.c,v 1.9 2022/02/17 16:28:29 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: sys_eventfd.c,v 1.9 2022/02/17 16:28:29 thorpej Exp $");

/*
 * eventfd
 *
 * Eventfd objects present a simple counting object associated with a
 * file descriptor.  Writes and reads to this file descriptor increment
 * and decrement the count, respectively.  When the count is non-zero,
 * the descriptor is considered "readable", and when less than the max
 * value (EVENTFD_MAXVAL), is considered "writable".
 *
 * This implementation is API compatible with the Linux eventfd(2)
 * interface.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/eventfd.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/uio.h>

struct eventfd {
	kmutex_t	efd_lock;
	kcondvar_t	efd_read_wait;
	kcondvar_t	efd_write_wait;
	struct selinfo	efd_read_sel;
	struct selinfo	efd_write_sel;
	eventfd_t	efd_val;
	int64_t		efd_nwaiters;
	bool		efd_restarting;
	bool		efd_has_read_waiters;
	bool		efd_has_write_waiters;
	bool		efd_is_semaphore;

	/*
	 * Information kept for stat(2).
	 */
	struct timespec efd_btime;	/* time created */
	struct timespec	efd_mtime;	/* last write */
	struct timespec	efd_atime;	/* last read */
};

#define	EVENTFD_MAXVAL	(UINT64_MAX - 1)

/*
 * eventfd_create:
 *
 *	Create an eventfd object.
 */
static struct eventfd *
eventfd_create(unsigned int const val, int const flags)
{
	struct eventfd * const efd = kmem_zalloc(sizeof(*efd), KM_SLEEP);

	mutex_init(&efd->efd_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&efd->efd_read_wait, "efdread");
	cv_init(&efd->efd_write_wait, "efdwrite");
	selinit(&efd->efd_read_sel);
	selinit(&efd->efd_write_sel);
	efd->efd_val = val;
	efd->efd_is_semaphore = !!(flags & EFD_SEMAPHORE);
	getnanotime(&efd->efd_btime);

	/* Caller deals with EFD_CLOEXEC and EFD_NONBLOCK. */

	return efd;
}

/*
 * eventfd_destroy:
 *
 *	Destroy an eventfd object.
 */
static void
eventfd_destroy(struct eventfd * const efd)
{

	KASSERT(efd->efd_nwaiters == 0);
	KASSERT(efd->efd_has_read_waiters == false);
	KASSERT(efd->efd_has_write_waiters == false);

	cv_destroy(&efd->efd_read_wait);
	cv_destroy(&efd->efd_write_wait);

	seldestroy(&efd->efd_read_sel);
	seldestroy(&efd->efd_write_sel);

	mutex_destroy(&efd->efd_lock);

	kmem_free(efd, sizeof(*efd));
}

/*
 * eventfd_wait:
 *
 *	Block on an eventfd.  Handles non-blocking, as well as
 *	the restart cases.
 */
static int
eventfd_wait(struct eventfd * const efd, int const fflag, bool const is_write)
{
	kcondvar_t *waitcv;
	int error;

	if (fflag & FNONBLOCK) {
		return EAGAIN;
	}

	/*
	 * We're going to block.  Check if we need to return ERESTART.
	 */
	if (efd->efd_restarting) {
		return ERESTART;
	}

	if (is_write) {
		efd->efd_has_write_waiters = true;
		waitcv = &efd->efd_write_wait;
	} else {
		efd->efd_has_read_waiters = true;
		waitcv = &efd->efd_read_wait;
	}

	efd->efd_nwaiters++;
	KASSERT(efd->efd_nwaiters > 0);
	error = cv_wait_sig(waitcv, &efd->efd_lock);
	efd->efd_nwaiters--;
	KASSERT(efd->efd_nwaiters >= 0);

	/*
	 * If a restart was triggered while we were asleep, we need
	 * to return ERESTART if no other error was returned.
	 */
	if (efd->efd_restarting) {
		if (error == 0) {
			error = ERESTART;
		}
	}

	return error;
}

/*
 * eventfd_wake:
 *
 *	Wake LWPs block on an eventfd.
 */
static void
eventfd_wake(struct eventfd * const efd, bool const is_write)
{
	kcondvar_t *waitcv = NULL;
	struct selinfo *sel;
	int pollev;

	if (is_write) {
		if (efd->efd_has_read_waiters) {
			waitcv = &efd->efd_read_wait;
			efd->efd_has_read_waiters = false;
		}
		sel = &efd->efd_read_sel;
		pollev = POLLIN | POLLRDNORM;
	} else {
		if (efd->efd_has_write_waiters) {
			waitcv = &efd->efd_write_wait;
			efd->efd_has_write_waiters = false;
		}
		sel = &efd->efd_write_sel;
		pollev = POLLOUT | POLLWRNORM;
	}
	if (waitcv != NULL) {
		cv_broadcast(waitcv);
	}
	selnotify(sel, pollev, NOTE_SUBMIT);
}

/*
 * eventfd file operations
 */

static int
eventfd_fop_read(file_t * const fp, off_t * const offset,
    struct uio * const uio, kauth_cred_t const cred, int const flags)
{
	struct eventfd * const efd = fp->f_eventfd;
	int const fflag = fp->f_flag;
	eventfd_t return_value;
	int error;

	if (uio->uio_resid < sizeof(eventfd_t)) {
		return EINVAL;
	}

	mutex_enter(&efd->efd_lock);

	while (efd->efd_val == 0) {
		if ((error = eventfd_wait(efd, fflag, false)) != 0) {
			mutex_exit(&efd->efd_lock);
			return error;
		}
	}

	if (efd->efd_is_semaphore) {
		return_value = 1;
		efd->efd_val--;
	} else {
		return_value = efd->efd_val;
		efd->efd_val = 0;
	}

	getnanotime(&efd->efd_atime);
	eventfd_wake(efd, false);

	mutex_exit(&efd->efd_lock);

	error = uiomove(&return_value, sizeof(return_value), uio);

	return error;
}

static int
eventfd_fop_write(file_t * const fp, off_t * const offset,
    struct uio * const uio, kauth_cred_t const cred, int const flags)
{
	struct eventfd * const efd = fp->f_eventfd;
	int const fflag = fp->f_flag;
	eventfd_t write_value;
	int error;

	if (uio->uio_resid < sizeof(eventfd_t)) {
		return EINVAL;
	}

	if ((error = uiomove(&write_value, sizeof(write_value), uio)) != 0) {
		return error;
	}

	if (write_value > EVENTFD_MAXVAL) {
		error = EINVAL;
		goto out;
	}

	mutex_enter(&efd->efd_lock);

	KASSERT(efd->efd_val <= EVENTFD_MAXVAL);
	while ((EVENTFD_MAXVAL - efd->efd_val) < write_value) {
		if ((error = eventfd_wait(efd, fflag, true)) != 0) {
			mutex_exit(&efd->efd_lock);
			goto out;
		}
	}

	efd->efd_val += write_value;
	KASSERT(efd->efd_val <= EVENTFD_MAXVAL);

	getnanotime(&efd->efd_mtime);
	eventfd_wake(efd, true);

	mutex_exit(&efd->efd_lock);

 out:
	if (error) {
		/*
		 * Undo the effect of uiomove() so that the error
		 * gets reported correctly; see dofilewrite().
		 */
		uio->uio_resid += sizeof(write_value);
	}
	return error;
}

static int
eventfd_ioctl(file_t * const fp, u_long const cmd, void * const data)
{
	struct eventfd * const efd = fp->f_eventfd;

	switch (cmd) {
	case FIONBIO:
		return 0;
	
	case FIONREAD:
		mutex_enter(&efd->efd_lock);
		*(int *)data = efd->efd_val != 0 ? sizeof(eventfd_t) : 0;
		mutex_exit(&efd->efd_lock);
		return 0;
	
	case FIONWRITE:
		*(int *)data = 0;
		return 0;

	case FIONSPACE:
		/*
		 * FIONSPACE doesn't really work for eventfd, because the
		 * writability depends on the contents (value) being written.
		 */
		break;

	default:
		break;
	}

	return EPASSTHROUGH;
}

static int
eventfd_fop_poll(file_t * const fp, int const events)
{
	struct eventfd * const efd = fp->f_eventfd;
	int revents = 0;

	/*
	 * Note that Linux will return POLLERR if the eventfd count
	 * overflows, but that is not possible in the normal read/write
	 * API, only with Linux kernel-internal interfaces.  So, this
	 * implementation never returns POLLERR.
	 *
	 * Also note that the Linux eventfd(2) man page does not
	 * specifically discuss returning POLLRDNORM, but we check
	 * for that event in addition to POLLIN.
	 */

	mutex_enter(&efd->efd_lock);

	if (events & (POLLIN | POLLRDNORM)) {
		if (efd->efd_val != 0) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(curlwp, &efd->efd_read_sel);
		}
	}

	if (events & (POLLOUT | POLLWRNORM)) {
		if (efd->efd_val < EVENTFD_MAXVAL) {
			revents |= events & (POLLOUT | POLLWRNORM);
		} else {
			selrecord(curlwp, &efd->efd_write_sel);
		}
	}

	mutex_exit(&efd->efd_lock);

	return revents;
}

static int
eventfd_fop_stat(file_t * const fp, struct stat * const st)
{
	struct eventfd * const efd = fp->f_eventfd;

	memset(st, 0, sizeof(*st));

	mutex_enter(&efd->efd_lock);
	st->st_size = (off_t)efd->efd_val;
	st->st_blksize = sizeof(eventfd_t);
	st->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	st->st_blocks = 1;
	st->st_birthtimespec = st->st_ctimespec = efd->efd_btime;
	st->st_atimespec = efd->efd_atime;
	st->st_mtimespec = efd->efd_mtime;
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	mutex_exit(&efd->efd_lock);

	return 0;
}

static int
eventfd_fop_close(file_t * const fp)
{
	struct eventfd * const efd = fp->f_eventfd;

	fp->f_eventfd = NULL;
	eventfd_destroy(efd);

	return 0;
}

static void
eventfd_filt_read_detach(struct knote * const kn)
{
	struct eventfd * const efd = ((file_t *)kn->kn_obj)->f_eventfd;

	mutex_enter(&efd->efd_lock);
	KASSERT(kn->kn_hook == efd);
	selremove_knote(&efd->efd_read_sel, kn);
	mutex_exit(&efd->efd_lock);
}

static int
eventfd_filt_read(struct knote * const kn, long const hint)
{
	struct eventfd * const efd = ((file_t *)kn->kn_obj)->f_eventfd;
	int rv;

	if (hint & NOTE_SUBMIT) {
		KASSERT(mutex_owned(&efd->efd_lock));
	} else {
		mutex_enter(&efd->efd_lock);
	}

	kn->kn_data = (int64_t)efd->efd_val;
	rv = (eventfd_t)kn->kn_data > 0;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(&efd->efd_lock);
	}

	return rv;
}

static const struct filterops eventfd_read_filterops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_detach = eventfd_filt_read_detach,
	.f_event = eventfd_filt_read,
};

static void
eventfd_filt_write_detach(struct knote * const kn)
{
	struct eventfd * const efd = ((file_t *)kn->kn_obj)->f_eventfd;

	mutex_enter(&efd->efd_lock);
	KASSERT(kn->kn_hook == efd);
	selremove_knote(&efd->efd_write_sel, kn);
	mutex_exit(&efd->efd_lock);
}

static int
eventfd_filt_write(struct knote * const kn, long const hint)
{
	struct eventfd * const efd = ((file_t *)kn->kn_obj)->f_eventfd;
	int rv;

	if (hint & NOTE_SUBMIT) {
		KASSERT(mutex_owned(&efd->efd_lock));
	} else {
		mutex_enter(&efd->efd_lock);
	}

	kn->kn_data = (int64_t)efd->efd_val;
	rv = (eventfd_t)kn->kn_data < EVENTFD_MAXVAL;

	if ((hint & NOTE_SUBMIT) == 0) {
		mutex_exit(&efd->efd_lock);
	}

	return rv;
}

static const struct filterops eventfd_write_filterops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_detach = eventfd_filt_write_detach,
	.f_event = eventfd_filt_write,
};

static int
eventfd_fop_kqfilter(file_t * const fp, struct knote * const kn)
{
	struct eventfd * const efd = ((file_t *)kn->kn_obj)->f_eventfd;
	struct selinfo *sel;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		sel = &efd->efd_read_sel;
		kn->kn_fop = &eventfd_read_filterops;
		break;

	case EVFILT_WRITE:
		sel = &efd->efd_write_sel;
		kn->kn_fop = &eventfd_write_filterops;
		break;

	default:
		return EINVAL;
	}

	kn->kn_hook = efd;

	mutex_enter(&efd->efd_lock);
	selrecord_knote(sel, kn);
	mutex_exit(&efd->efd_lock);

	return 0;
}

static void
eventfd_fop_restart(file_t * const fp)
{
	struct eventfd * const efd = fp->f_eventfd;

	/*
	 * Unblock blocked reads/writes in order to allow close() to complete.
	 * System calls return ERESTART so that the fd is revalidated.
	 */

	mutex_enter(&efd->efd_lock);

	if (efd->efd_nwaiters != 0) {
		efd->efd_restarting = true;
		if (efd->efd_has_read_waiters) {
			cv_broadcast(&efd->efd_read_wait);
			efd->efd_has_read_waiters = false;
		}
		if (efd->efd_has_write_waiters) {
			cv_broadcast(&efd->efd_write_wait);
			efd->efd_has_write_waiters = false;
		}
	}

	mutex_exit(&efd->efd_lock);
}

static const struct fileops eventfd_fileops = {
	.fo_name = "eventfd",
	.fo_read = eventfd_fop_read,
	.fo_write = eventfd_fop_write,
	.fo_ioctl = eventfd_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = eventfd_fop_poll,
	.fo_stat = eventfd_fop_stat,
	.fo_close = eventfd_fop_close,
	.fo_kqfilter = eventfd_fop_kqfilter,
	.fo_restart = eventfd_fop_restart,
};

/*
 * eventfd(2) system call
 */
int
do_eventfd(struct lwp * const l, unsigned int const val, int const flags,
    register_t *retval)
{
	file_t *fp;
	int fd, error;

	if (flags & ~(EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE)) {
		return EINVAL;
	}

	if ((error = fd_allocfile(&fp, &fd)) != 0) {
		return error;
	}

	fp->f_flag = FREAD | FWRITE;
	if (flags & EFD_NONBLOCK) {
		fp->f_flag |= FNONBLOCK;
	}
	fp->f_type = DTYPE_EVENTFD;
	fp->f_ops = &eventfd_fileops;
	fp->f_eventfd = eventfd_create(val, flags);
	fd_set_exclose(l, fd, !!(flags & EFD_CLOEXEC));
	fd_affix(curproc, fp, fd);

	*retval = fd;
	return 0;
}

int
sys_eventfd(struct lwp *l, const struct sys_eventfd_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(unsigned int) val;
		syscallarg(int) flags;
	} */

	return do_eventfd(l, SCARG(uap, val), SCARG(uap, flags), retval);
}
