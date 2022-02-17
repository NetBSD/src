/*	$NetBSD: sys_timerfd.c,v 1.8 2022/02/17 16:28:29 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_timerfd.c,v 1.8 2022/02/17 16:28:29 thorpej Exp $");

/*
 * timerfd
 *
 * Timerfd objects are similar to POSIX timers, except they are associated
 * with a file descriptor rather than a process.  Timerfd objects are
 * created with the timerfd_create(2) system call, similar to timer_create(2).
 * The timerfd analogues for timer_gettime(2) and timer_settime(2) are
 * timerfd_gettime(2) and timerfd_settime(2), respectively.
 *
 * When a timerfd object's timer fires, an internal counter is incremented.
 * When this counter is non-zero, the descriptor associated with the timerfd
 * object is "readable".  Note that this is slightly different than the
 * POSIX timer "overrun" counter, which only increments if the timer fires
 * again while the notification signal is already pending.  Thus, we are
 * responsible for incrementing the "overrun" counter each time the timerfd
 * timer fires.
 *
 * This implementation is API compatible with the Linux timerfd interface.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/timerfd.h>
#include <sys/uio.h>

/* N.B. all timerfd state is protected by itimer_lock() */
struct timerfd {
	struct itimer	tfd_itimer;
	kcondvar_t	tfd_read_wait;
	struct selinfo	tfd_read_sel;
	int64_t		tfd_nwaiters;
	bool		tfd_cancel_on_set;
	bool		tfd_cancelled;
	bool		tfd_restarting;

	/*
	 * Information kept for stat(2).
	 */
	struct timespec tfd_btime;	/* time created */
	struct timespec	tfd_mtime;	/* last timerfd_settime() */
	struct timespec	tfd_atime;	/* last read */
};

static void	timerfd_wake(struct timerfd *);

static inline uint64_t
timerfd_fire_count(const struct timerfd * const tfd)
{
	return (unsigned int)tfd->tfd_itimer.it_overruns;
}

static inline bool
timerfd_is_readable(const struct timerfd * const tfd)
{
	return tfd->tfd_itimer.it_overruns != 0 || tfd->tfd_cancelled;
}

/*
 * timerfd_fire:
 *
 *	Called when the timerfd's timer fires.
 *
 *	Called from a callout with itimer lock held.
 */
static void
timerfd_fire(struct itimer * const it)
{
	struct timerfd * const tfd =
	    container_of(it, struct timerfd, tfd_itimer);

	it->it_overruns++;
	timerfd_wake(tfd);
}

/*
 * timerfd_realtime_changed:
 *
 *	Called when CLOCK_REALTIME is changed with clock_settime()
 *	or settimeofday().
 *
 *	Called with itimer lock held.
 */
static void
timerfd_realtime_changed(struct itimer * const it)
{
	struct timerfd * const tfd =
	    container_of(it, struct timerfd, tfd_itimer);

	/* Should only be called when timer is armed. */
	KASSERT(timespecisset(&it->it_time.it_value));

	if (tfd->tfd_cancel_on_set) {
		tfd->tfd_cancelled = true;
		timerfd_wake(tfd);
	}
}

static const struct itimer_ops timerfd_itimer_monotonic_ops = {
	.ito_fire = timerfd_fire,
};

static const struct itimer_ops timerfd_itimer_realtime_ops = {
	.ito_fire = timerfd_fire,
	.ito_realtime_changed = timerfd_realtime_changed,
};

/*
 * timerfd_create:
 *
 *	Create a timerfd object.
 */
static struct timerfd *
timerfd_create(clockid_t const clock_id, int const flags)
{
	struct timerfd * const tfd = kmem_zalloc(sizeof(*tfd), KM_SLEEP);

	KASSERT(clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC);

	cv_init(&tfd->tfd_read_wait, "tfdread");
	selinit(&tfd->tfd_read_sel);
	getnanotime(&tfd->tfd_btime);

	/* Caller deals with TFD_CLOEXEC and TFD_NONBLOCK. */

	itimer_lock();
	itimer_init(&tfd->tfd_itimer,
	    clock_id == CLOCK_REALTIME ? &timerfd_itimer_realtime_ops
				       : &timerfd_itimer_monotonic_ops,
	    clock_id, NULL);
	itimer_unlock();

	return tfd;
}

/*
 * timerfd_destroy:
 *
 *	Destroy a timerfd object.
 */
static void
timerfd_destroy(struct timerfd * const tfd)
{

	KASSERT(tfd->tfd_nwaiters == 0);

	itimer_lock();
	itimer_poison(&tfd->tfd_itimer);
	itimer_fini(&tfd->tfd_itimer);	/* drops itimer lock */

	cv_destroy(&tfd->tfd_read_wait);

	seldestroy(&tfd->tfd_read_sel);

	kmem_free(tfd, sizeof(*tfd));
}

/*
 * timerfd_wait:
 *
 *	Block on a timerfd.  Handles non-blocking, as well as
 *	the restart cases.
 */
static int
timerfd_wait(struct timerfd * const tfd, int const fflag)
{
	extern kmutex_t	itimer_mutex;	/* XXX */
	int error;

	if (fflag & FNONBLOCK) {
		return EAGAIN;
	}

	/*
	 * We're going to block.  Check if we need to return ERESTART.
	 */
	if (tfd->tfd_restarting) {
		return ERESTART;
	}

	tfd->tfd_nwaiters++;
	KASSERT(tfd->tfd_nwaiters > 0);
	error = cv_wait_sig(&tfd->tfd_read_wait, &itimer_mutex);
	tfd->tfd_nwaiters--;
	KASSERT(tfd->tfd_nwaiters >= 0);

	/*
	 * If a restart was triggered while we were asleep, we need
	 * to return ERESTART if no other error was returned.
	 */
	if (tfd->tfd_restarting) {
		if (error == 0) {
			error = ERESTART;
		}
	}

	return error;
}

/*
 * timerfd_wake:
 *
 *	Wake LWPs blocked on a timerfd.
 */
static void
timerfd_wake(struct timerfd * const tfd)
{

	if (tfd->tfd_nwaiters) {
		cv_broadcast(&tfd->tfd_read_wait);
	}
	selnotify(&tfd->tfd_read_sel, POLLIN | POLLRDNORM, NOTE_SUBMIT);
}

/*
 * timerfd file operations
 */

static int
timerfd_fop_read(file_t * const fp, off_t * const offset,
    struct uio * const uio, kauth_cred_t const cred, int const flags)
{
	struct timerfd * const tfd = fp->f_timerfd;
	struct itimer * const it = &tfd->tfd_itimer;
	int const fflag = fp->f_flag;
	uint64_t return_value;
	int error;

	if (uio->uio_resid < sizeof(uint64_t)) {
		return EINVAL;
	}

	itimer_lock();

	while (!timerfd_is_readable(tfd)) {
		if ((error = timerfd_wait(tfd, fflag)) != 0) {
			itimer_unlock();
			return error;
		}
	}

	if (tfd->tfd_cancelled) {
		itimer_unlock();
		return ECANCELED;
	}

	return_value = timerfd_fire_count(tfd);
	it->it_overruns = 0;

	getnanotime(&tfd->tfd_atime);

	itimer_unlock();

	error = uiomove(&return_value, sizeof(return_value), uio);

	return error;
}

static int
timerfd_fop_ioctl(file_t * const fp, unsigned long const cmd, void * const data)
{
	struct timerfd * const tfd = fp->f_timerfd;
	int error = 0;

	switch (cmd) {
	case FIONBIO:
		break;

	case FIONREAD:
		itimer_lock();
		*(int *)data = timerfd_is_readable(tfd) ? sizeof(uint64_t) : 0;
		itimer_unlock();
		break;

	case TFD_IOC_SET_TICKS: {
		const uint64_t * const new_ticksp = data;
		if (*new_ticksp > INT_MAX) {
			return EINVAL;
		}
		itimer_lock();
		tfd->tfd_itimer.it_overruns = (int)*new_ticksp;
		itimer_unlock();
		break;
	    }

	default:
		error = EPASSTHROUGH;
	}

	return error;
}

static int
timerfd_fop_poll(file_t * const fp, int const events)
{
	struct timerfd * const tfd = fp->f_timerfd;
	int revents = events & (POLLOUT | POLLWRNORM);

	if (events & (POLLIN | POLLRDNORM)) {
		itimer_lock();
		if (timerfd_is_readable(tfd)) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(curlwp, &tfd->tfd_read_sel);
		}
		itimer_unlock();
	}

	return revents;
}

static int
timerfd_fop_stat(file_t * const fp, struct stat * const st)
{
	struct timerfd * const tfd = fp->f_timerfd;

	memset(st, 0, sizeof(*st));

	itimer_lock();
	st->st_size = (off_t)timerfd_fire_count(tfd);
	st->st_atimespec = tfd->tfd_atime;
	st->st_mtimespec = tfd->tfd_mtime;
	itimer_unlock();

	st->st_blksize = sizeof(uint64_t);
	st->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	st->st_blocks = 1;
	st->st_birthtimespec = tfd->tfd_btime;
	st->st_ctimespec = st->st_mtimespec;
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);

	return 0;
}

static int
timerfd_fop_close(file_t * const fp)
{
	struct timerfd * const tfd = fp->f_timerfd;

	fp->f_timerfd = NULL;
	timerfd_destroy(tfd);

	return 0;
}

static void
timerfd_filt_read_detach(struct knote * const kn)
{
	struct timerfd * const tfd = ((file_t *)kn->kn_obj)->f_timerfd;

	itimer_lock();
	KASSERT(kn->kn_hook == tfd);
	selremove_knote(&tfd->tfd_read_sel, kn);
	itimer_unlock();
}

static int
timerfd_filt_read(struct knote * const kn, long const hint)
{
	struct timerfd * const tfd = ((file_t *)kn->kn_obj)->f_timerfd;
	int rv;

	if (hint & NOTE_SUBMIT) {
		KASSERT(itimer_lock_held());
	} else {
		itimer_lock();
	}

	kn->kn_data = (int64_t)timerfd_fire_count(tfd);
	rv = kn->kn_data != 0;

	if ((hint & NOTE_SUBMIT) == 0) {
		itimer_unlock();
	}

	return rv;
}

static const struct filterops timerfd_read_filterops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_detach = timerfd_filt_read_detach,
	.f_event = timerfd_filt_read,
};

static int
timerfd_fop_kqfilter(file_t * const fp, struct knote * const kn)
{
	struct timerfd * const tfd = ((file_t *)kn->kn_obj)->f_timerfd;
	struct selinfo *sel;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		sel = &tfd->tfd_read_sel;
		kn->kn_fop = &timerfd_read_filterops;
		break;

	default:
		return EINVAL;
	}

	kn->kn_hook = tfd;

	itimer_lock();
	selrecord_knote(sel, kn);
	itimer_unlock();

	return 0;
}

static void
timerfd_fop_restart(file_t * const fp)
{
	struct timerfd * const tfd = fp->f_timerfd;

	/*
	 * Unblock blocked reads in order to allow close() to complete.
	 * System calls return ERESTART so that the fd is revalidated.
	 */

	itimer_lock();

	if (tfd->tfd_nwaiters != 0) {
		tfd->tfd_restarting = true;
		cv_broadcast(&tfd->tfd_read_wait);
	}

	itimer_unlock();
}

static const struct fileops timerfd_fileops = {
	.fo_name = "timerfd",
	.fo_read = timerfd_fop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = timerfd_fop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = timerfd_fop_poll,
	.fo_stat = timerfd_fop_stat,
	.fo_close = timerfd_fop_close,
	.fo_kqfilter = timerfd_fop_kqfilter,
	.fo_restart = timerfd_fop_restart,
};

/*
 * timerfd_create(2) system call
 */
int
do_timerfd_create(struct lwp * const l, clockid_t const clock_id,
    int const flags, register_t *retval)
{
	file_t *fp;
	int fd, error;

	if (flags & ~(TFD_CLOEXEC | TFD_NONBLOCK)) {
		return EINVAL;
	}

	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		/* allowed */
		break;

	default:
		return EINVAL;
	}

	if ((error = fd_allocfile(&fp, &fd)) != 0) {
		return error;
	}

	fp->f_flag = FREAD;
	if (flags & TFD_NONBLOCK) {
		fp->f_flag |= FNONBLOCK;
	}
	fp->f_type = DTYPE_TIMERFD;
	fp->f_ops = &timerfd_fileops;
	fp->f_timerfd = timerfd_create(clock_id, flags);
	fd_set_exclose(l, fd, !!(flags & TFD_CLOEXEC));
	fd_affix(curproc, fp, fd);

	*retval = fd;
	return 0;
}

int
sys_timerfd_create(struct lwp *l, const struct sys_timerfd_create_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(int) flags;
	} */

	return do_timerfd_create(l, SCARG(uap, clock_id), SCARG(uap, flags),
	    retval);
}

/*
 * timerfd_gettime(2) system call.
 */
int
do_timerfd_gettime(struct lwp *l, int fd, struct itimerspec *curr_value,
    register_t *retval)
{
	file_t *fp;

	if ((fp = fd_getfile(fd)) == NULL) {
		return EBADF;
	}

	if (fp->f_ops != &timerfd_fileops) {
		fd_putfile(fd);
		return EINVAL;
	}

	struct timerfd * const tfd = fp->f_timerfd;
	itimer_lock();
	itimer_gettime(&tfd->tfd_itimer, curr_value);
	itimer_unlock();

	fd_putfile(fd);
	return 0;
}

int
sys_timerfd_gettime(struct lwp *l, const struct sys_timerfd_gettime_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct itimerspec *) curr_value;
	} */

	struct itimerspec oits;
	int error;

	error = do_timerfd_gettime(l, SCARG(uap, fd), &oits, retval);
	if (error == 0) {
		error = copyout(&oits, SCARG(uap, curr_value), sizeof(oits));
	}
	return error;
}

/*
 * timerfd_settime(2) system call.
 */
int
do_timerfd_settime(struct lwp *l, int fd, int flags,
    const struct itimerspec *new_value, struct itimerspec *old_value,
    register_t *retval)
{
	file_t *fp;
	int error;

	if (flags & ~(TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET)) {
		return EINVAL;
	}

	if ((fp = fd_getfile(fd)) == NULL) {
		return EBADF;
	}

	if (fp->f_ops != &timerfd_fileops) {
		fd_putfile(fd);
		return EINVAL;
	}

	struct timerfd * const tfd = fp->f_timerfd;
	struct itimer * const it = &tfd->tfd_itimer;

	itimer_lock();

 restart:
	if (old_value != NULL) {
		*old_value = it->it_time;
	}
	it->it_time = *new_value;

	/*
	 * If we've been passed a relative value, convert it to an
	 * absolute, as that's what the itimer facility expects for
	 * non-virtual timers.  Also ensure that this doesn't set it
	 * to zero or lets it go negative.
	 * XXXJRT re-factor.
	 */
	if (timespecisset(&it->it_time.it_value) &&
	    (flags & TFD_TIMER_ABSTIME) == 0) {
		struct timespec now;
		if (it->it_clockid == CLOCK_REALTIME) {
			getnanotime(&now);
		} else { /* CLOCK_MONOTONIC */
			getnanouptime(&now);
		}
		timespecadd(&it->it_time.it_value, &now,
		    &it->it_time.it_value);
	}

	error = itimer_settime(it);
	if (error == ERESTART) {
		goto restart;
	}
	KASSERT(error == 0);

	/* Reset the expirations counter. */
	it->it_overruns = 0;

	if (it->it_clockid == CLOCK_REALTIME) {
		tfd->tfd_cancelled = false;
		tfd->tfd_cancel_on_set = !!(flags & TFD_TIMER_CANCEL_ON_SET);
	}

	getnanotime(&tfd->tfd_mtime);
	itimer_unlock();

	fd_putfile(fd);
	return error;
}

int
sys_timerfd_settime(struct lwp *l, const struct sys_timerfd_settime_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(const struct itimerspec *) new_value;
		syscallarg(struct itimerspec *) old_value;
	} */

	struct itimerspec nits, oits, *oitsp = NULL;
	int error;

	error = copyin(SCARG(uap, new_value), &nits, sizeof(nits));
	if (error) {
		return error;
	}

	if (SCARG(uap, old_value) != NULL) {
		oitsp = &oits;
	}

	error = do_timerfd_settime(l, SCARG(uap, fd), SCARG(uap, flags),
	    &nits, oitsp, retval);
	if (error == 0 && oitsp != NULL) {
		error = copyout(oitsp, SCARG(uap, old_value), sizeof(*oitsp));
	}
	return error;
}
