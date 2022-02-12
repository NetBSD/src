/*	$NetBSD: kern_event.c,v 1.140 2022/02/12 15:51:29 thorpej Exp $	*/

/*-
 * Copyright (c) 2008, 2009, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
 * Copyright (c) 2009 Apple, Inc
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/kern/kern_event.c,v 1.27 2001/07/05 17:10:44 rwatson Exp
 */

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif /* _KERNEL_OPT */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_event.c,v 1.140 2022/02/12 15:51:29 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/wait.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/poll.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/conf.h>
#include <sys/atomic.h>

static int	kqueue_scan(file_t *, size_t, struct kevent *,
			    const struct timespec *, register_t *,
			    const struct kevent_ops *, struct kevent *,
			    size_t);
static int	kqueue_ioctl(file_t *, u_long, void *);
static int	kqueue_fcntl(file_t *, u_int, void *);
static int	kqueue_poll(file_t *, int);
static int	kqueue_kqfilter(file_t *, struct knote *);
static int	kqueue_stat(file_t *, struct stat *);
static int	kqueue_close(file_t *);
static void	kqueue_restart(file_t *);
static int	kqueue_register(struct kqueue *, struct kevent *);
static void	kqueue_doclose(struct kqueue *, struct klist *, int);

static void	knote_detach(struct knote *, filedesc_t *fdp, bool);
static void	knote_enqueue(struct knote *);
static void	knote_activate(struct knote *);
static void	knote_activate_locked(struct knote *);
static void	knote_deactivate_locked(struct knote *);

static void	filt_kqdetach(struct knote *);
static int	filt_kqueue(struct knote *, long hint);
static int	filt_procattach(struct knote *);
static void	filt_procdetach(struct knote *);
static int	filt_proc(struct knote *, long hint);
static int	filt_fileattach(struct knote *);
static void	filt_timerexpire(void *x);
static int	filt_timerattach(struct knote *);
static void	filt_timerdetach(struct knote *);
static int	filt_timer(struct knote *, long hint);
static int	filt_timertouch(struct knote *, struct kevent *, long type);
static int	filt_userattach(struct knote *);
static void	filt_userdetach(struct knote *);
static int	filt_user(struct knote *, long hint);
static int	filt_usertouch(struct knote *, struct kevent *, long type);

static const struct fileops kqueueops = {
	.fo_name = "kqueue",
	.fo_read = (void *)enxio,
	.fo_write = (void *)enxio,
	.fo_ioctl = kqueue_ioctl,
	.fo_fcntl = kqueue_fcntl,
	.fo_poll = kqueue_poll,
	.fo_stat = kqueue_stat,
	.fo_close = kqueue_close,
	.fo_kqfilter = kqueue_kqfilter,
	.fo_restart = kqueue_restart,
};

static const struct filterops kqread_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_kqdetach,
	.f_event = filt_kqueue,
};

static const struct filterops proc_filtops = {
	.f_flags = FILTEROP_MPSAFE,
	.f_attach = filt_procattach,
	.f_detach = filt_procdetach,
	.f_event = filt_proc,
};

/*
 * file_filtops is not marked MPSAFE because it's going to call
 * fileops::fo_kqfilter(), which might not be.  That function,
 * however, will override the knote's filterops, and thus will
 * inherit the MPSAFE-ness of the back-end at that time.
 */
static const struct filterops file_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = filt_fileattach,
	.f_detach = NULL,
	.f_event = NULL,
};

static const struct filterops timer_filtops = {
	.f_flags = FILTEROP_MPSAFE,
	.f_attach = filt_timerattach,
	.f_detach = filt_timerdetach,
	.f_event = filt_timer,
	.f_touch = filt_timertouch,
};

static const struct filterops user_filtops = {
	.f_flags = FILTEROP_MPSAFE,
	.f_attach = filt_userattach,
	.f_detach = filt_userdetach,
	.f_event = filt_user,
	.f_touch = filt_usertouch,
};

static u_int	kq_ncallouts = 0;
static int	kq_calloutmax = (4 * 1024);

#define	KN_HASHSIZE		64		/* XXX should be tunable */
#define	KN_HASH(val, mask)	(((val) ^ (val >> 8)) & (mask))

extern const struct filterops fs_filtops;	/* vfs_syscalls.c */
extern const struct filterops sig_filtops;	/* kern_sig.c */

/*
 * Table for for all system-defined filters.
 * These should be listed in the numeric order of the EVFILT_* defines.
 * If filtops is NULL, the filter isn't implemented in NetBSD.
 * End of list is when name is NULL.
 *
 * Note that 'refcnt' is meaningless for built-in filters.
 */
struct kfilter {
	const char	*name;		/* name of filter */
	uint32_t	filter;		/* id of filter */
	unsigned	refcnt;		/* reference count */
	const struct filterops *filtops;/* operations for filter */
	size_t		namelen;	/* length of name string */
};

/* System defined filters */
static struct kfilter sys_kfilters[] = {
	{ "EVFILT_READ",	EVFILT_READ,	0, &file_filtops, 0 },
	{ "EVFILT_WRITE",	EVFILT_WRITE,	0, &file_filtops, 0, },
	{ "EVFILT_AIO",		EVFILT_AIO,	0, NULL, 0 },
	{ "EVFILT_VNODE",	EVFILT_VNODE,	0, &file_filtops, 0 },
	{ "EVFILT_PROC",	EVFILT_PROC,	0, &proc_filtops, 0 },
	{ "EVFILT_SIGNAL",	EVFILT_SIGNAL,	0, &sig_filtops, 0 },
	{ "EVFILT_TIMER",	EVFILT_TIMER,	0, &timer_filtops, 0 },
	{ "EVFILT_FS",		EVFILT_FS,	0, &fs_filtops, 0 },
	{ "EVFILT_USER",	EVFILT_USER,	0, &user_filtops, 0 },
	{ "EVFILT_EMPTY",	EVFILT_EMPTY,	0, &file_filtops, 0 },
	{ NULL,			0,		0, NULL, 0 },
};

/* User defined kfilters */
static struct kfilter	*user_kfilters;		/* array */
static int		user_kfilterc;		/* current offset */
static int		user_kfiltermaxc;	/* max size so far */
static size_t		user_kfiltersz;		/* size of allocated memory */

/*
 * Global Locks.
 *
 * Lock order:
 *
 *	kqueue_filter_lock
 *	-> kn_kq->kq_fdp->fd_lock
 *	-> object lock (e.g., device driver lock, &c.)
 *	-> kn_kq->kq_lock
 *
 * Locking rules:
 *
 *	f_attach: fdp->fd_lock, KERNEL_LOCK
 *	f_detach: fdp->fd_lock, KERNEL_LOCK
 *	f_event(!NOTE_SUBMIT) via kevent: fdp->fd_lock, _no_ object lock
 *	f_event via knote: whatever caller guarantees
 *		Typically,	f_event(NOTE_SUBMIT) via knote: object lock
 *				f_event(!NOTE_SUBMIT) via knote: nothing,
 *					acquires/releases object lock inside.
 *
 * Locking rules when detaching knotes:
 *
 * There are some situations where knote submission may require dropping
 * locks (see knote_proc_fork()).  In order to support this, it's possible
 * to mark a knote as being 'in-flux'.  Such a knote is guaranteed not to
 * be detached while it remains in-flux.  Because it will not be detached,
 * locks can be dropped so e.g. memory can be allocated, locks on other
 * data structures can be acquired, etc.  During this time, any attempt to
 * detach an in-flux knote must wait until the knote is no longer in-flux.
 * When this happens, the knote is marked for death (KN_WILLDETACH) and the
 * LWP who gets to finish the detach operation is recorded in the knote's
 * 'udata' field (which is no longer required for its original purpose once
 * a knote is so marked).  Code paths that lead to knote_detach() must ensure
 * that their LWP is the one tasked with its final demise after waiting for
 * the in-flux status of the knote to clear.  Note that once a knote is
 * marked KN_WILLDETACH, no code paths may put it into an in-flux state.
 *
 * Once the special circumstances have been handled, the locks are re-
 * acquired in the proper order (object lock -> kq_lock), the knote taken
 * out of flux, and any waiters are notified.  Because waiters must have
 * also dropped *their* locks in order to safely block, they must re-
 * validate all of their assumptions; see knote_detach_quiesce().  See also
 * the kqueue_register() (EV_ADD, EV_DELETE) and kqueue_scan() (EV_ONESHOT)
 * cases.
 *
 * When kqueue_scan() encounters an in-flux knote, the situation is
 * treated like another LWP's list marker.
 *
 * LISTEN WELL: It is important to not hold knotes in flux for an
 * extended period of time! In-flux knotes effectively block any
 * progress of the kqueue_scan() operation.  Any code paths that place
 * knotes in-flux should be careful to not block for indefinite periods
 * of time, such as for memory allocation (i.e. KM_NOSLEEP is OK, but
 * KM_SLEEP is not).
 */
static krwlock_t	kqueue_filter_lock;	/* lock on filter lists */

#define	KQ_FLUX_WAIT(kq)	(void)cv_wait(&kq->kq_cv, &kq->kq_lock)
#define	KQ_FLUX_WAKEUP(kq)	cv_broadcast(&kq->kq_cv)

static inline bool
kn_in_flux(struct knote *kn)
{
	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));
	return kn->kn_influx != 0;
}

static inline bool
kn_enter_flux(struct knote *kn)
{
	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));

	if (kn->kn_status & KN_WILLDETACH) {
		return false;
	}

	KASSERT(kn->kn_influx < UINT_MAX);
	kn->kn_influx++;

	return true;
}

static inline bool
kn_leave_flux(struct knote *kn)
{
	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));
	KASSERT(kn->kn_influx > 0);
	kn->kn_influx--;
	return kn->kn_influx == 0;
}

static void
kn_wait_flux(struct knote *kn, bool can_loop)
{
	bool loop;

	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));

	/*
	 * It may not be safe for us to touch the knote again after
	 * dropping the kq_lock.  The caller has let us know in
	 * 'can_loop'.
	 */
	for (loop = true; loop && kn->kn_influx != 0; loop = can_loop) {
		KQ_FLUX_WAIT(kn->kn_kq);
	}
}

#define	KNOTE_WILLDETACH(kn)						\
do {									\
	(kn)->kn_status |= KN_WILLDETACH;				\
	(kn)->kn_kevent.udata = curlwp;					\
} while (/*CONSTCOND*/0)

/*
 * Wait until the specified knote is in a quiescent state and
 * safe to detach.  Returns true if we potentially blocked (and
 * thus dropped our locks).
 */
static bool
knote_detach_quiesce(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;
	filedesc_t *fdp = kq->kq_fdp;

	KASSERT(mutex_owned(&fdp->fd_lock));

	mutex_spin_enter(&kq->kq_lock);
	/*
	 * There are two cases where we might see KN_WILLDETACH here:
	 *
	 * 1. Someone else has already started detaching the knote but
	 *    had to wait for it to settle first.
	 *
	 * 2. We had to wait for it to settle, and had to come back
	 *    around after re-acquiring the locks.
	 *
	 * When KN_WILLDETACH is set, we also set the LWP that claimed
	 * the prize of finishing the detach in the 'udata' field of the
	 * knote (which will never be used again for its usual purpose
	 * once the note is in this state).  If it doesn't point to us,
	 * we must drop the locks and let them in to finish the job.
	 *
	 * Otherwise, once we have claimed the knote for ourselves, we
	 * can finish waiting for it to settle.  The is the only scenario
	 * where touching a detaching knote is safe after dropping the
	 * locks.
	 */
	if ((kn->kn_status & KN_WILLDETACH) != 0 &&
	    kn->kn_kevent.udata != curlwp) {
		/*
		 * N.B. it is NOT safe for us to touch the knote again
		 * after dropping the locks here.  The caller must go
		 * back around and re-validate everything.  However, if
		 * the knote is in-flux, we want to block to minimize
		 * busy-looping.
		 */
		mutex_exit(&fdp->fd_lock);
		if (kn_in_flux(kn)) {
			kn_wait_flux(kn, false);
			mutex_spin_exit(&kq->kq_lock);
			return true;
		}
		mutex_spin_exit(&kq->kq_lock);
		preempt_point();
		return true;
	}
	/*
	 * If we get here, we know that we will be claiming the
	 * detach responsibilies, or that we already have and
	 * this is the second attempt after re-validation.
	 */
	KASSERT((kn->kn_status & KN_WILLDETACH) == 0 ||
		kn->kn_kevent.udata == curlwp);
	/*
	 * Similarly, if we get here, either we are just claiming it
	 * and may have to wait for it to settle, or if this is the
	 * second attempt after re-validation that no other code paths
	 * have put it in-flux.
	 */
	KASSERT((kn->kn_status & KN_WILLDETACH) == 0 ||
		kn_in_flux(kn) == false);
	KNOTE_WILLDETACH(kn);
	if (kn_in_flux(kn)) {
		mutex_exit(&fdp->fd_lock);
		kn_wait_flux(kn, true);
		/*
		 * It is safe for us to touch the knote again after
		 * dropping the locks, but the caller must still
		 * re-validate everything because other aspects of
		 * the environment may have changed while we blocked.
		 */
		KASSERT(kn_in_flux(kn) == false);
		mutex_spin_exit(&kq->kq_lock);
		return true;
	}
	mutex_spin_exit(&kq->kq_lock);

	return false;
}

static int
filter_attach(struct knote *kn)
{
	int rv;

	KASSERT(kn->kn_fop != NULL);
	KASSERT(kn->kn_fop->f_attach != NULL);

	/*
	 * N.B. that kn->kn_fop may change as the result of calling
	 * f_attach().
	 */
	if (kn->kn_fop->f_flags & FILTEROP_MPSAFE) {
		rv = kn->kn_fop->f_attach(kn);
	} else {
		KERNEL_LOCK(1, NULL);
		rv = kn->kn_fop->f_attach(kn);
		KERNEL_UNLOCK_ONE(NULL);
	}

	return rv;
}

static void
filter_detach(struct knote *kn)
{
	KASSERT(kn->kn_fop != NULL);
	KASSERT(kn->kn_fop->f_detach != NULL);

	if (kn->kn_fop->f_flags & FILTEROP_MPSAFE) {
		kn->kn_fop->f_detach(kn);
	} else {
		KERNEL_LOCK(1, NULL);
		kn->kn_fop->f_detach(kn);
		KERNEL_UNLOCK_ONE(NULL);
	}
}

static int
filter_event(struct knote *kn, long hint)
{
	int rv;

	KASSERT(kn->kn_fop != NULL);
	KASSERT(kn->kn_fop->f_event != NULL);

	if (kn->kn_fop->f_flags & FILTEROP_MPSAFE) {
		rv = kn->kn_fop->f_event(kn, hint);
	} else {
		KERNEL_LOCK(1, NULL);
		rv = kn->kn_fop->f_event(kn, hint);
		KERNEL_UNLOCK_ONE(NULL);
	}

	return rv;
}

static int
filter_touch(struct knote *kn, struct kevent *kev, long type)
{
	return kn->kn_fop->f_touch(kn, kev, type);
}

static kauth_listener_t	kqueue_listener;

static int
kqueue_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	int result;

	result = KAUTH_RESULT_DEFER;
	p = arg0;

	if (action != KAUTH_PROCESS_KEVENT_FILTER)
		return result;

	if ((kauth_cred_getuid(p->p_cred) != kauth_cred_getuid(cred) ||
	    ISSET(p->p_flag, PK_SUGID)))
		return result;

	result = KAUTH_RESULT_ALLOW;

	return result;
}

/*
 * Initialize the kqueue subsystem.
 */
void
kqueue_init(void)
{

	rw_init(&kqueue_filter_lock);

	kqueue_listener = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    kqueue_listener_cb, NULL);
}

/*
 * Find kfilter entry by name, or NULL if not found.
 */
static struct kfilter *
kfilter_byname_sys(const char *name)
{
	int i;

	KASSERT(rw_lock_held(&kqueue_filter_lock));

	for (i = 0; sys_kfilters[i].name != NULL; i++) {
		if (strcmp(name, sys_kfilters[i].name) == 0)
			return &sys_kfilters[i];
	}
	return NULL;
}

static struct kfilter *
kfilter_byname_user(const char *name)
{
	int i;

	KASSERT(rw_lock_held(&kqueue_filter_lock));

	/* user filter slots have a NULL name if previously deregistered */
	for (i = 0; i < user_kfilterc ; i++) {
		if (user_kfilters[i].name != NULL &&
		    strcmp(name, user_kfilters[i].name) == 0)
			return &user_kfilters[i];
	}
	return NULL;
}

static struct kfilter *
kfilter_byname(const char *name)
{
	struct kfilter *kfilter;

	KASSERT(rw_lock_held(&kqueue_filter_lock));

	if ((kfilter = kfilter_byname_sys(name)) != NULL)
		return kfilter;

	return kfilter_byname_user(name);
}

/*
 * Find kfilter entry by filter id, or NULL if not found.
 * Assumes entries are indexed in filter id order, for speed.
 */
static struct kfilter *
kfilter_byfilter(uint32_t filter)
{
	struct kfilter *kfilter;

	KASSERT(rw_lock_held(&kqueue_filter_lock));

	if (filter < EVFILT_SYSCOUNT)	/* it's a system filter */
		kfilter = &sys_kfilters[filter];
	else if (user_kfilters != NULL &&
	    filter < EVFILT_SYSCOUNT + user_kfilterc)
					/* it's a user filter */
		kfilter = &user_kfilters[filter - EVFILT_SYSCOUNT];
	else
		return (NULL);		/* out of range */
	KASSERT(kfilter->filter == filter);	/* sanity check! */
	return (kfilter);
}

/*
 * Register a new kfilter. Stores the entry in user_kfilters.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 * If retfilter != NULL, the new filterid is returned in it.
 */
int
kfilter_register(const char *name, const struct filterops *filtops,
		 int *retfilter)
{
	struct kfilter *kfilter;
	size_t len;
	int i;

	if (name == NULL || name[0] == '\0' || filtops == NULL)
		return (EINVAL);	/* invalid args */

	rw_enter(&kqueue_filter_lock, RW_WRITER);
	if (kfilter_byname(name) != NULL) {
		rw_exit(&kqueue_filter_lock);
		return (EEXIST);	/* already exists */
	}
	if (user_kfilterc > 0xffffffff - EVFILT_SYSCOUNT) {
		rw_exit(&kqueue_filter_lock);
		return (EINVAL);	/* too many */
	}

	for (i = 0; i < user_kfilterc; i++) {
		kfilter = &user_kfilters[i];
		if (kfilter->name == NULL) {
			/* Previously deregistered slot.  Reuse. */
			goto reuse;
		}
	}

	/* check if need to grow user_kfilters */
	if (user_kfilterc + 1 > user_kfiltermaxc) {
		/* Grow in KFILTER_EXTENT chunks. */
		user_kfiltermaxc += KFILTER_EXTENT;
		len = user_kfiltermaxc * sizeof(*kfilter);
		kfilter = kmem_alloc(len, KM_SLEEP);
		memset((char *)kfilter + user_kfiltersz, 0, len - user_kfiltersz);
		if (user_kfilters != NULL) {
			memcpy(kfilter, user_kfilters, user_kfiltersz);
			kmem_free(user_kfilters, user_kfiltersz);
		}
		user_kfiltersz = len;
		user_kfilters = kfilter;
	}
	/* Adding new slot */
	kfilter = &user_kfilters[user_kfilterc++];
reuse:
	kfilter->name = kmem_strdupsize(name, &kfilter->namelen, KM_SLEEP);

	kfilter->filter = (kfilter - user_kfilters) + EVFILT_SYSCOUNT;

	kfilter->filtops = kmem_alloc(sizeof(*filtops), KM_SLEEP);
	memcpy(__UNCONST(kfilter->filtops), filtops, sizeof(*filtops));

	if (retfilter != NULL)
		*retfilter = kfilter->filter;
	rw_exit(&kqueue_filter_lock);

	return (0);
}

/*
 * Unregister a kfilter previously registered with kfilter_register.
 * This retains the filter id, but clears the name and frees filtops (filter
 * operations), so that the number isn't reused during a boot.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 */
int
kfilter_unregister(const char *name)
{
	struct kfilter *kfilter;

	if (name == NULL || name[0] == '\0')
		return (EINVAL);	/* invalid name */

	rw_enter(&kqueue_filter_lock, RW_WRITER);
	if (kfilter_byname_sys(name) != NULL) {
		rw_exit(&kqueue_filter_lock);
		return (EINVAL);	/* can't detach system filters */
	}

	kfilter = kfilter_byname_user(name);
	if (kfilter == NULL) {
		rw_exit(&kqueue_filter_lock);
		return (ENOENT);
	}
	if (kfilter->refcnt != 0) {
		rw_exit(&kqueue_filter_lock);
		return (EBUSY);
	}

	/* Cast away const (but we know it's safe. */
	kmem_free(__UNCONST(kfilter->name), kfilter->namelen);
	kfilter->name = NULL;	/* mark as `not implemented' */

	if (kfilter->filtops != NULL) {
		/* Cast away const (but we know it's safe. */
		kmem_free(__UNCONST(kfilter->filtops),
		    sizeof(*kfilter->filtops));
		kfilter->filtops = NULL; /* mark as `not implemented' */
	}
	rw_exit(&kqueue_filter_lock);

	return (0);
}


/*
 * Filter attach method for EVFILT_READ and EVFILT_WRITE on normal file
 * descriptors. Calls fileops kqfilter method for given file descriptor.
 */
static int
filt_fileattach(struct knote *kn)
{
	file_t *fp;

	fp = kn->kn_obj;

	return (*fp->f_ops->fo_kqfilter)(fp, kn);
}

/*
 * Filter detach method for EVFILT_READ on kqueue descriptor.
 */
static void
filt_kqdetach(struct knote *kn)
{
	struct kqueue *kq;

	kq = ((file_t *)kn->kn_obj)->f_kqueue;

	mutex_spin_enter(&kq->kq_lock);
	selremove_knote(&kq->kq_sel, kn);
	mutex_spin_exit(&kq->kq_lock);
}

/*
 * Filter event method for EVFILT_READ on kqueue descriptor.
 */
/*ARGSUSED*/
static int
filt_kqueue(struct knote *kn, long hint)
{
	struct kqueue *kq;
	int rv;

	kq = ((file_t *)kn->kn_obj)->f_kqueue;

	if (hint != NOTE_SUBMIT)
		mutex_spin_enter(&kq->kq_lock);
	kn->kn_data = KQ_COUNT(kq);
	rv = (kn->kn_data > 0);
	if (hint != NOTE_SUBMIT)
		mutex_spin_exit(&kq->kq_lock);

	return rv;
}

/*
 * Filter attach method for EVFILT_PROC.
 */
static int
filt_procattach(struct knote *kn)
{
	struct proc *p;

	mutex_enter(&proc_lock);
	p = proc_find(kn->kn_id);
	if (p == NULL) {
		mutex_exit(&proc_lock);
		return ESRCH;
	}

	/*
	 * Fail if it's not owned by you, or the last exec gave us
	 * setuid/setgid privs (unless you're root).
	 */
	mutex_enter(p->p_lock);
	mutex_exit(&proc_lock);
	if (kauth_authorize_process(curlwp->l_cred,
	    KAUTH_PROCESS_KEVENT_FILTER, p, NULL, NULL, NULL) != 0) {
	    	mutex_exit(p->p_lock);
		return EACCES;
	}

	kn->kn_obj = p;
	kn->kn_flags |= EV_CLEAR;	/* automatically set */

	/*
	 * NOTE_CHILD is only ever generated internally; don't let it
	 * leak in from user-space.  See knote_proc_fork_track().
	 */
	kn->kn_sfflags &= ~NOTE_CHILD;

	klist_insert(&p->p_klist, kn);
    	mutex_exit(p->p_lock);

	return 0;
}

/*
 * Filter detach method for EVFILT_PROC.
 *
 * The knote may be attached to a different process, which may exit,
 * leaving nothing for the knote to be attached to.  So when the process
 * exits, the knote is marked as DETACHED and also flagged as ONESHOT so
 * it will be deleted when read out.  However, as part of the knote deletion,
 * this routine is called, so a check is needed to avoid actually performing
 * a detach, because the original process might not exist any more.
 */
static void
filt_procdetach(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;
	struct proc *p;

	/*
	 * We have to synchronize with knote_proc_exit(), but we
	 * are forced to acquire the locks in the wrong order here
	 * because we can't be sure kn->kn_obj is valid unless
	 * KN_DETACHED is not set.
	 */
 again:
	mutex_spin_enter(&kq->kq_lock);
	if ((kn->kn_status & KN_DETACHED) == 0) {
		p = kn->kn_obj;
		if (!mutex_tryenter(p->p_lock)) {
			mutex_spin_exit(&kq->kq_lock);
			preempt_point();
			goto again;
		}
		kn->kn_status |= KN_DETACHED;
		klist_remove(&p->p_klist, kn);
		mutex_exit(p->p_lock);
	}
	mutex_spin_exit(&kq->kq_lock);
}

/*
 * Filter event method for EVFILT_PROC.
 *
 * Due to some of the complexities of process locking, we have special
 * entry points for delivering knote submissions.  filt_proc() is used
 * only to check for activation from kqueue_register() and kqueue_scan().
 */
static int
filt_proc(struct knote *kn, long hint)
{
	struct kqueue *kq = kn->kn_kq;
	uint32_t fflags;

	/*
	 * Because we share the same klist with signal knotes, just
	 * ensure that we're not being invoked for the proc-related
	 * submissions.
	 */
	KASSERT((hint & (NOTE_EXEC | NOTE_EXIT | NOTE_FORK)) == 0);

	mutex_spin_enter(&kq->kq_lock);
	fflags = kn->kn_fflags;
	mutex_spin_exit(&kq->kq_lock);

	return fflags != 0;
}

void
knote_proc_exec(struct proc *p)
{
	struct knote *kn, *tmpkn;
	struct kqueue *kq;
	uint32_t fflags;

	mutex_enter(p->p_lock);

	SLIST_FOREACH_SAFE(kn, &p->p_klist, kn_selnext, tmpkn) {
		/* N.B. EVFILT_SIGNAL knotes are on this same list. */
		if (kn->kn_fop == &sig_filtops) {
			continue;
		}
		KASSERT(kn->kn_fop == &proc_filtops);

		kq = kn->kn_kq;
		mutex_spin_enter(&kq->kq_lock);
		fflags = (kn->kn_fflags |= (kn->kn_sfflags & NOTE_EXEC));
		if (fflags) {
			knote_activate_locked(kn);
		}
		mutex_spin_exit(&kq->kq_lock);
	}

	mutex_exit(p->p_lock);
}

static int __noinline
knote_proc_fork_track(struct proc *p1, struct proc *p2, struct knote *okn)
{
	struct kqueue *kq = okn->kn_kq;

	KASSERT(mutex_owned(&kq->kq_lock));
	KASSERT(mutex_owned(p1->p_lock));

	/*
	 * We're going to put this knote into flux while we drop
	 * the locks and create and attach a new knote to track the
	 * child.  If we are not able to enter flux, then this knote
	 * is about to go away, so skip the notification.
	 */
	if (!kn_enter_flux(okn)) {
		return 0;
	}

	mutex_spin_exit(&kq->kq_lock);
	mutex_exit(p1->p_lock);

	/*
	 * We actually have to register *two* new knotes:
	 *
	 * ==> One for the NOTE_CHILD notification.  This is a forced
	 *     ONESHOT note.
	 *
	 * ==> One to actually track the child process as it subsequently
	 *     forks, execs, and, ultimately, exits.
	 *
	 * If we only register a single knote, then it's possible for
	 * for the NOTE_CHILD and NOTE_EXIT to be collapsed into a single
	 * notification if the child exits before the tracking process
	 * has received the NOTE_CHILD notification, which applications
	 * aren't expecting (the event's 'data' field would be clobbered,
	 * for exmaple).
	 *
	 * To do this, what we have here is an **extremely** stripped-down
	 * version of kqueue_register() that has the following properties:
	 *
	 * ==> Does not block to allocate memory.  If we are unable
	 *     to allocate memory, we return ENOMEM.
	 *
	 * ==> Does not search for existing knotes; we know there
	 *     are not any because this is a new process that isn't
	 *     even visible to other processes yet.
	 *
	 * ==> Assumes that the knhash for our kq's descriptor table
	 *     already exists (after all, we're already tracking
	 *     processes with knotes if we got here).
	 *
	 * ==> Directly attaches the new tracking knote to the child
	 *     process.
	 *
	 * The whole point is to do the minimum amount of work while the
	 * knote is held in-flux, and to avoid doing extra work in general
	 * (we already have the new child process; why bother looking it
	 * up again?).
	 */
	filedesc_t *fdp = kq->kq_fdp;
	struct knote *knchild, *kntrack;
	int error = 0;

	knchild = kmem_zalloc(sizeof(*knchild), KM_NOSLEEP);
	kntrack = kmem_zalloc(sizeof(*knchild), KM_NOSLEEP);
	if (__predict_false(knchild == NULL || kntrack == NULL)) {
		error = ENOMEM;
		goto out;
	}

	kntrack->kn_obj = p2;
	kntrack->kn_id = p2->p_pid;
	kntrack->kn_kq = kq;
	kntrack->kn_fop = okn->kn_fop;
	kntrack->kn_kfilter = okn->kn_kfilter;
	kntrack->kn_sfflags = okn->kn_sfflags;
	kntrack->kn_sdata = p1->p_pid;

	kntrack->kn_kevent.ident = p2->p_pid;
	kntrack->kn_kevent.filter = okn->kn_filter;
	kntrack->kn_kevent.flags =
	    okn->kn_flags | EV_ADD | EV_ENABLE | EV_CLEAR;
	kntrack->kn_kevent.fflags = 0;
	kntrack->kn_kevent.data = 0;
	kntrack->kn_kevent.udata = okn->kn_kevent.udata; /* preserve udata */

	/*
	 * The child note does not need to be attached to the
	 * new proc's klist at all.
	 */
	*knchild = *kntrack;
	knchild->kn_status = KN_DETACHED;
	knchild->kn_sfflags = 0;
	knchild->kn_kevent.flags |= EV_ONESHOT;
	knchild->kn_kevent.fflags = NOTE_CHILD;
	knchild->kn_kevent.data = p1->p_pid;		 /* parent */

	mutex_enter(&fdp->fd_lock);

	/*
	 * We need to check to see if the kq is closing, and skip
	 * attaching the knote if so.  Normally, this isn't necessary
	 * when coming in the front door because the file descriptor
	 * layer will synchronize this.
	 *
	 * It's safe to test KQ_CLOSING without taking the kq_lock
	 * here because that flag is only ever set when the fd_lock
	 * is also held.
	 */
	if (__predict_false(kq->kq_count & KQ_CLOSING)) {
		mutex_exit(&fdp->fd_lock);
		goto out;
	}

	/*
	 * We do the "insert into FD table" and "attach to klist" steps
	 * in the opposite order of kqueue_register() here to avoid
	 * having to take p2->p_lock twice.  But this is OK because we
	 * hold fd_lock across the entire operation.
	 */

	mutex_enter(p2->p_lock);
	error = kauth_authorize_process(curlwp->l_cred,
	    KAUTH_PROCESS_KEVENT_FILTER, p2, NULL, NULL, NULL);
	if (__predict_false(error != 0)) {
		mutex_exit(p2->p_lock);
		mutex_exit(&fdp->fd_lock);
		error = EACCES;
		goto out;
	}
	klist_insert(&p2->p_klist, kntrack);
	mutex_exit(p2->p_lock);

	KASSERT(fdp->fd_knhashmask != 0);
	KASSERT(fdp->fd_knhash != NULL);
	struct klist *list = &fdp->fd_knhash[KN_HASH(kntrack->kn_id,
	    fdp->fd_knhashmask)];
	SLIST_INSERT_HEAD(list, kntrack, kn_link);
	SLIST_INSERT_HEAD(list, knchild, kn_link);

	/* This adds references for knchild *and* kntrack. */
	atomic_add_int(&kntrack->kn_kfilter->refcnt, 2);

	knote_activate(knchild);

	kntrack = NULL;
	knchild = NULL;

	mutex_exit(&fdp->fd_lock);

 out:
	if (__predict_false(knchild != NULL)) {
		kmem_free(knchild, sizeof(*knchild));
	}
	if (__predict_false(kntrack != NULL)) {
		kmem_free(kntrack, sizeof(*kntrack));
	}
	mutex_enter(p1->p_lock);
	mutex_spin_enter(&kq->kq_lock);

	if (kn_leave_flux(okn)) {
		KQ_FLUX_WAKEUP(kq);
	}

	return error;
}

void
knote_proc_fork(struct proc *p1, struct proc *p2)
{
	struct knote *kn;
	struct kqueue *kq;
	uint32_t fflags;

	mutex_enter(p1->p_lock);

	/*
	 * N.B. We DO NOT use SLIST_FOREACH_SAFE() here because we
	 * don't want to pre-fetch the next knote; in the event we
	 * have to drop p_lock, we will have put the knote in-flux,
	 * meaning that no one will be able to detach it until we
	 * have taken the knote out of flux.  However, that does
	 * NOT stop someone else from detaching the next note in the
	 * list while we have it unlocked.  Thus, we want to fetch
	 * the next note in the list only after we have re-acquired
	 * the lock, and using SLIST_FOREACH() will satisfy that.
	 */
	SLIST_FOREACH(kn, &p1->p_klist, kn_selnext) {
		/* N.B. EVFILT_SIGNAL knotes are on this same list. */
		if (kn->kn_fop == &sig_filtops) {
			continue;
		}
		KASSERT(kn->kn_fop == &proc_filtops);

		kq = kn->kn_kq;
		mutex_spin_enter(&kq->kq_lock);
		kn->kn_fflags |= (kn->kn_sfflags & NOTE_FORK);
		if (__predict_false(kn->kn_sfflags & NOTE_TRACK)) {
			/*
			 * This will drop kq_lock and p_lock and
			 * re-acquire them before it returns.
			 */
			if (knote_proc_fork_track(p1, p2, kn)) {
				kn->kn_fflags |= NOTE_TRACKERR;
			}
			KASSERT(mutex_owned(p1->p_lock));
			KASSERT(mutex_owned(&kq->kq_lock));
		}
		fflags = kn->kn_fflags;
		if (fflags) {
			knote_activate_locked(kn);
		}
		mutex_spin_exit(&kq->kq_lock);
	}

	mutex_exit(p1->p_lock);
}

void
knote_proc_exit(struct proc *p)
{
	struct knote *kn;
	struct kqueue *kq;

	KASSERT(mutex_owned(p->p_lock));

	while (!SLIST_EMPTY(&p->p_klist)) {
		kn = SLIST_FIRST(&p->p_klist);
		kq = kn->kn_kq;

		KASSERT(kn->kn_obj == p);

		mutex_spin_enter(&kq->kq_lock);
		kn->kn_data = P_WAITSTATUS(p);
		/*
		 * Mark as ONESHOT, so that the knote is g/c'ed
		 * when read.
		 */
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		kn->kn_fflags |= kn->kn_sfflags & NOTE_EXIT;

		/*
		 * Detach the knote from the process and mark it as such.
		 * N.B. EVFILT_SIGNAL are also on p_klist, but by the
		 * time we get here, all open file descriptors for this
		 * process have been released, meaning that signal knotes
		 * will have already been detached.
		 *
		 * We need to synchronize this with filt_procdetach().
		 */
		KASSERT(kn->kn_fop == &proc_filtops);
		if ((kn->kn_status & KN_DETACHED) == 0) {
			kn->kn_status |= KN_DETACHED;
			SLIST_REMOVE_HEAD(&p->p_klist, kn_selnext);
		}

		/*
		 * Always activate the knote for NOTE_EXIT regardless
		 * of whether or not the listener cares about it.
		 * This matches historical behavior.
		 */
		knote_activate_locked(kn);
		mutex_spin_exit(&kq->kq_lock);
	}
}

#define	FILT_TIMER_NOSCHED	((uintptr_t)-1)

static int
filt_timercompute(struct kevent *kev, uintptr_t *tticksp)
{
	struct timespec ts;
	uintptr_t tticks;

	if (kev->fflags & ~(NOTE_TIMER_UNITMASK | NOTE_ABSTIME)) {
		return EINVAL;
	}

	/*
	 * Convert the event 'data' to a timespec, then convert the
	 * timespec to callout ticks.
	 */
	switch (kev->fflags & NOTE_TIMER_UNITMASK) {
	case NOTE_SECONDS:
		ts.tv_sec = kev->data;
		ts.tv_nsec = 0;
		break;

	case NOTE_MSECONDS:		/* == historical value 0 */
		ts.tv_sec = kev->data / 1000;
		ts.tv_nsec = (kev->data % 1000) * 1000000;
		break;

	case NOTE_USECONDS:
		ts.tv_sec = kev->data / 1000000;
		ts.tv_nsec = (kev->data % 1000000) * 1000;
		break;

	case NOTE_NSECONDS:
		ts.tv_sec = kev->data / 1000000000;
		ts.tv_nsec = kev->data % 1000000000;
		break;

	default:
		return EINVAL;
	}

	if (kev->fflags & NOTE_ABSTIME) {
		struct timespec deadline = ts;

		/*
		 * Get current time.
		 *
		 * XXX This is CLOCK_REALTIME.  There is no way to
		 * XXX specify CLOCK_MONOTONIC.
		 */
		nanotime(&ts);

		/* Absolute timers do not repeat. */
		kev->data = FILT_TIMER_NOSCHED;

		/* If we're past the deadline, then the event will fire. */
		if (timespeccmp(&deadline, &ts, <=)) {
			tticks = FILT_TIMER_NOSCHED;
			goto out;
		}

		/* Calculate how much time is left. */
		timespecsub(&deadline, &ts, &ts);
	} else {
		/* EV_CLEAR automatically set for relative timers. */
		kev->flags |= EV_CLEAR;
	}

	tticks = tstohz(&ts);

	/* if the supplied value is under our resolution, use 1 tick */
	if (tticks == 0) {
		if (kev->data == 0)
			return EINVAL;
		tticks = 1;
	} else if (tticks > INT_MAX) {
		return EINVAL;
	}

	if ((kev->flags & EV_ONESHOT) != 0) {
		/* Timer does not repeat. */
		kev->data = FILT_TIMER_NOSCHED;
	} else {
		KASSERT((uintptr_t)tticks != FILT_TIMER_NOSCHED);
		kev->data = tticks;
	}

 out:
	*tticksp = tticks;

	return 0;
}

static void
filt_timerexpire(void *knx)
{
	struct knote *kn = knx;
	struct kqueue *kq = kn->kn_kq;

	mutex_spin_enter(&kq->kq_lock);
	kn->kn_data++;
	knote_activate_locked(kn);
	if (kn->kn_sdata != FILT_TIMER_NOSCHED) {
		KASSERT(kn->kn_sdata > 0 && kn->kn_sdata <= INT_MAX);
		callout_schedule((callout_t *)kn->kn_hook,
		    (int)kn->kn_sdata);
	}
	mutex_spin_exit(&kq->kq_lock);
}

static inline void
filt_timerstart(struct knote *kn, uintptr_t tticks)
{
	callout_t *calloutp = kn->kn_hook;

	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));
	KASSERT(!callout_pending(calloutp));

	if (__predict_false(tticks == FILT_TIMER_NOSCHED)) {
		kn->kn_data = 1;
	} else {
		KASSERT(tticks <= INT_MAX);
		callout_reset(calloutp, (int)tticks, filt_timerexpire, kn);
	}
}

static int
filt_timerattach(struct knote *kn)
{
	callout_t *calloutp;
	struct kqueue *kq;
	uintptr_t tticks;
	int error;

	struct kevent kev = {
		.flags = kn->kn_flags,
		.fflags = kn->kn_sfflags,
		.data = kn->kn_sdata,
	};

	error = filt_timercompute(&kev, &tticks);
	if (error) {
		return error;
	}

	if (atomic_inc_uint_nv(&kq_ncallouts) >= kq_calloutmax ||
	    (calloutp = kmem_alloc(sizeof(*calloutp), KM_NOSLEEP)) == NULL) {
		atomic_dec_uint(&kq_ncallouts);
		return ENOMEM;
	}
	callout_init(calloutp, CALLOUT_MPSAFE);

	kq = kn->kn_kq;
	mutex_spin_enter(&kq->kq_lock);

	kn->kn_sdata = kev.data;
	kn->kn_flags = kev.flags;
	KASSERT(kn->kn_sfflags == kev.fflags);
	kn->kn_hook = calloutp;

	filt_timerstart(kn, tticks);

	mutex_spin_exit(&kq->kq_lock);

	return (0);
}

static void
filt_timerdetach(struct knote *kn)
{
	callout_t *calloutp;
	struct kqueue *kq = kn->kn_kq;

	/* prevent rescheduling when we expire */
	mutex_spin_enter(&kq->kq_lock);
	kn->kn_sdata = FILT_TIMER_NOSCHED;
	mutex_spin_exit(&kq->kq_lock);

	calloutp = (callout_t *)kn->kn_hook;

	/*
	 * Attempt to stop the callout.  This will block if it's
	 * already running.
	 */
	callout_halt(calloutp, NULL);

	callout_destroy(calloutp);
	kmem_free(calloutp, sizeof(*calloutp));
	atomic_dec_uint(&kq_ncallouts);
}

static int
filt_timertouch(struct knote *kn, struct kevent *kev, long type)
{
	struct kqueue *kq = kn->kn_kq;
	callout_t *calloutp;
	uintptr_t tticks;
	int error;

	KASSERT(mutex_owned(&kq->kq_lock));

	switch (type) {
	case EVENT_REGISTER:
		/* Only relevant for EV_ADD. */
		if ((kev->flags & EV_ADD) == 0) {
			return 0;
		}

		/*
		 * Stop the timer, under the assumption that if
		 * an application is re-configuring the timer,
		 * they no longer care about the old one.  We
		 * can safely drop the kq_lock while we wait
		 * because fdp->fd_lock will be held throughout,
		 * ensuring that no one can sneak in with an
		 * EV_DELETE or close the kq.
		 */
		KASSERT(mutex_owned(&kq->kq_fdp->fd_lock));

		calloutp = kn->kn_hook;
		callout_halt(calloutp, &kq->kq_lock);
		KASSERT(mutex_owned(&kq->kq_lock));
		knote_deactivate_locked(kn);
		kn->kn_data = 0;

		error = filt_timercompute(kev, &tticks);
		if (error) {
			return error;
		}
		kn->kn_sdata = kev->data;
		kn->kn_flags = kev->flags;
		kn->kn_sfflags = kev->fflags;
		filt_timerstart(kn, tticks);
		break;

	case EVENT_PROCESS:
		*kev = kn->kn_kevent;
		break;

	default:
		panic("%s: invalid type (%ld)", __func__, type);
	}

	return 0;
}

static int
filt_timer(struct knote *kn, long hint)
{
	struct kqueue *kq = kn->kn_kq;
	int rv;

	mutex_spin_enter(&kq->kq_lock);
	rv = (kn->kn_data != 0);
	mutex_spin_exit(&kq->kq_lock);

	return rv;
}

static int
filt_userattach(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;

	/*
	 * EVFILT_USER knotes are not attached to anything in the kernel.
	 */
	mutex_spin_enter(&kq->kq_lock);
	kn->kn_hook = NULL;
	if (kn->kn_fflags & NOTE_TRIGGER)
		kn->kn_hookid = 1;
	else
		kn->kn_hookid = 0;
	mutex_spin_exit(&kq->kq_lock);
	return (0);
}

static void
filt_userdetach(struct knote *kn)
{

	/*
	 * EVFILT_USER knotes are not attached to anything in the kernel.
	 */
}

static int
filt_user(struct knote *kn, long hint)
{
	struct kqueue *kq = kn->kn_kq;
	int hookid;

	mutex_spin_enter(&kq->kq_lock);
	hookid = kn->kn_hookid;
	mutex_spin_exit(&kq->kq_lock);

	return hookid;
}

static int
filt_usertouch(struct knote *kn, struct kevent *kev, long type)
{
	int ffctrl;

	KASSERT(mutex_owned(&kn->kn_kq->kq_lock));

	switch (type) {
	case EVENT_REGISTER:
		if (kev->fflags & NOTE_TRIGGER)
			kn->kn_hookid = 1;

		ffctrl = kev->fflags & NOTE_FFCTRLMASK;
		kev->fflags &= NOTE_FFLAGSMASK;
		switch (ffctrl) {
		case NOTE_FFNOP:
			break;

		case NOTE_FFAND:
			kn->kn_sfflags &= kev->fflags;
			break;

		case NOTE_FFOR:
			kn->kn_sfflags |= kev->fflags;
			break;

		case NOTE_FFCOPY:
			kn->kn_sfflags = kev->fflags;
			break;

		default:
			/* XXX Return error? */
			break;
		}
		kn->kn_sdata = kev->data;
		if (kev->flags & EV_CLEAR) {
			kn->kn_hookid = 0;
			kn->kn_data = 0;
			kn->kn_fflags = 0;
		}
		break;

	case EVENT_PROCESS:
		*kev = kn->kn_kevent;
		kev->fflags = kn->kn_sfflags;
		kev->data = kn->kn_sdata;
		if (kn->kn_flags & EV_CLEAR) {
			kn->kn_hookid = 0;
			kn->kn_data = 0;
			kn->kn_fflags = 0;
		}
		break;

	default:
		panic("filt_usertouch() - invalid type (%ld)", type);
		break;
	}

	return 0;
}

/*
 * filt_seltrue:
 *
 *	This filter "event" routine simulates seltrue().
 */
int
filt_seltrue(struct knote *kn, long hint)
{

	/*
	 * We don't know how much data can be read/written,
	 * but we know that it *can* be.  This is about as
	 * good as select/poll does as well.
	 */
	kn->kn_data = 0;
	return (1);
}

/*
 * This provides full kqfilter entry for device switch tables, which
 * has same effect as filter using filt_seltrue() as filter method.
 */
static void
filt_seltruedetach(struct knote *kn)
{
	/* Nothing to do */
}

const struct filterops seltrue_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_seltruedetach,
	.f_event = filt_seltrue,
};

int
seltrue_kqfilter(dev_t dev, struct knote *kn)
{
	switch (kn->kn_filter) {
	case EVFILT_READ:
	case EVFILT_WRITE:
		kn->kn_fop = &seltrue_filtops;
		break;
	default:
		return (EINVAL);
	}

	/* Nothing more to do */
	return (0);
}

/*
 * kqueue(2) system call.
 */
static int
kqueue1(struct lwp *l, int flags, register_t *retval)
{
	struct kqueue *kq;
	file_t *fp;
	int fd, error;

	if ((error = fd_allocfile(&fp, &fd)) != 0)
		return error;
	fp->f_flag = FREAD | FWRITE | (flags & (FNONBLOCK|FNOSIGPIPE));
	fp->f_type = DTYPE_KQUEUE;
	fp->f_ops = &kqueueops;
	kq = kmem_zalloc(sizeof(*kq), KM_SLEEP);
	mutex_init(&kq->kq_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&kq->kq_cv, "kqueue");
	selinit(&kq->kq_sel);
	TAILQ_INIT(&kq->kq_head);
	fp->f_kqueue = kq;
	*retval = fd;
	kq->kq_fdp = curlwp->l_fd;
	fd_set_exclose(l, fd, (flags & O_CLOEXEC) != 0);
	fd_affix(curproc, fp, fd);
	return error;
}

/*
 * kqueue(2) system call.
 */
int
sys_kqueue(struct lwp *l, const void *v, register_t *retval)
{
	return kqueue1(l, 0, retval);
}

int
sys_kqueue1(struct lwp *l, const struct sys_kqueue1_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) flags;
	} */
	return kqueue1(l, SCARG(uap, flags), retval);
}

/*
 * kevent(2) system call.
 */
int
kevent_fetch_changes(void *ctx, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{

	return copyin(changelist + index, changes, n * sizeof(*changes));
}

int
kevent_put_events(void *ctx, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{

	return copyout(events, eventlist + index, n * sizeof(*events));
}

static const struct kevent_ops kevent_native_ops = {
	.keo_private = NULL,
	.keo_fetch_timeout = copyin,
	.keo_fetch_changes = kevent_fetch_changes,
	.keo_put_events = kevent_put_events,
};

int
sys___kevent50(struct lwp *l, const struct sys___kevent50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct kevent *) changelist;
		syscallarg(size_t) nchanges;
		syscallarg(struct kevent *) eventlist;
		syscallarg(size_t) nevents;
		syscallarg(const struct timespec *) timeout;
	} */

	return kevent1(retval, SCARG(uap, fd), SCARG(uap, changelist),
	    SCARG(uap, nchanges), SCARG(uap, eventlist), SCARG(uap, nevents),
	    SCARG(uap, timeout), &kevent_native_ops);
}

int
kevent1(register_t *retval, int fd,
	const struct kevent *changelist, size_t nchanges,
	struct kevent *eventlist, size_t nevents,
	const struct timespec *timeout,
	const struct kevent_ops *keops)
{
	struct kevent *kevp;
	struct kqueue *kq;
	struct timespec	ts;
	size_t i, n, ichange;
	int nerrors, error;
	struct kevent kevbuf[KQ_NEVENTS];	/* approx 300 bytes on 64-bit */
	file_t *fp;

	/* check that we're dealing with a kq */
	fp = fd_getfile(fd);
	if (fp == NULL)
		return (EBADF);

	if (fp->f_type != DTYPE_KQUEUE) {
		fd_putfile(fd);
		return (EBADF);
	}

	if (timeout != NULL) {
		error = (*keops->keo_fetch_timeout)(timeout, &ts, sizeof(ts));
		if (error)
			goto done;
		timeout = &ts;
	}

	kq = fp->f_kqueue;
	nerrors = 0;
	ichange = 0;

	/* traverse list of events to register */
	while (nchanges > 0) {
		n = MIN(nchanges, __arraycount(kevbuf));
		error = (*keops->keo_fetch_changes)(keops->keo_private,
		    changelist, kevbuf, ichange, n);
		if (error)
			goto done;
		for (i = 0; i < n; i++) {
			kevp = &kevbuf[i];
			kevp->flags &= ~EV_SYSFLAGS;
			/* register each knote */
			error = kqueue_register(kq, kevp);
			if (!error && !(kevp->flags & EV_RECEIPT))
				continue;
			if (nevents == 0)
				goto done;
			kevp->flags = EV_ERROR;
			kevp->data = error;
			error = (*keops->keo_put_events)
				(keops->keo_private, kevp,
				 eventlist, nerrors, 1);
			if (error)
				goto done;
			nevents--;
			nerrors++;
		}
		nchanges -= n;	/* update the results */
		ichange += n;
	}
	if (nerrors) {
		*retval = nerrors;
		error = 0;
		goto done;
	}

	/* actually scan through the events */
	error = kqueue_scan(fp, nevents, eventlist, timeout, retval, keops,
	    kevbuf, __arraycount(kevbuf));
 done:
	fd_putfile(fd);
	return (error);
}

/*
 * Register a given kevent kev onto the kqueue
 */
static int
kqueue_register(struct kqueue *kq, struct kevent *kev)
{
	struct kfilter *kfilter;
	filedesc_t *fdp;
	file_t *fp;
	fdfile_t *ff;
	struct knote *kn, *newkn;
	struct klist *list;
	int error, fd, rv;

	fdp = kq->kq_fdp;
	fp = NULL;
	kn = NULL;
	error = 0;
	fd = 0;

	newkn = kmem_zalloc(sizeof(*newkn), KM_SLEEP);

	rw_enter(&kqueue_filter_lock, RW_READER);
	kfilter = kfilter_byfilter(kev->filter);
	if (kfilter == NULL || kfilter->filtops == NULL) {
		/* filter not found nor implemented */
		rw_exit(&kqueue_filter_lock);
		kmem_free(newkn, sizeof(*newkn));
		return (EINVAL);
	}

	/* search if knote already exists */
	if (kfilter->filtops->f_flags & FILTEROP_ISFD) {
		/* monitoring a file descriptor */
		/* validate descriptor */
		if (kev->ident > INT_MAX
		    || (fp = fd_getfile(fd = kev->ident)) == NULL) {
			rw_exit(&kqueue_filter_lock);
			kmem_free(newkn, sizeof(*newkn));
			return EBADF;
		}
		mutex_enter(&fdp->fd_lock);
		ff = fdp->fd_dt->dt_ff[fd];
		if (ff->ff_refcnt & FR_CLOSING) {
			error = EBADF;
			goto doneunlock;
		}
		if (fd <= fdp->fd_lastkqfile) {
			SLIST_FOREACH(kn, &ff->ff_knlist, kn_link) {
				if (kq == kn->kn_kq &&
				    kev->filter == kn->kn_filter)
					break;
			}
		}
	} else {
		/*
		 * not monitoring a file descriptor, so
		 * lookup knotes in internal hash table
		 */
		mutex_enter(&fdp->fd_lock);
		if (fdp->fd_knhashmask != 0) {
			list = &fdp->fd_knhash[
			    KN_HASH((u_long)kev->ident, fdp->fd_knhashmask)];
			SLIST_FOREACH(kn, list, kn_link) {
				if (kev->ident == kn->kn_id &&
				    kq == kn->kn_kq &&
				    kev->filter == kn->kn_filter)
					break;
			}
		}
	}

	/* It's safe to test KQ_CLOSING while holding only the fd_lock. */
	KASSERT(mutex_owned(&fdp->fd_lock));
	KASSERT((kq->kq_count & KQ_CLOSING) == 0);

	/*
	 * kn now contains the matching knote, or NULL if no match
	 */
	if (kn == NULL) {
		if (kev->flags & EV_ADD) {
			/* create new knote */
			kn = newkn;
			newkn = NULL;
			kn->kn_obj = fp;
			kn->kn_id = kev->ident;
			kn->kn_kq = kq;
			kn->kn_fop = kfilter->filtops;
			kn->kn_kfilter = kfilter;
			kn->kn_sfflags = kev->fflags;
			kn->kn_sdata = kev->data;
			kev->fflags = 0;
			kev->data = 0;
			kn->kn_kevent = *kev;

			KASSERT(kn->kn_fop != NULL);
			/*
			 * apply reference count to knote structure, and
			 * do not release it at the end of this routine.
			 */
			fp = NULL;

			if (!(kn->kn_fop->f_flags & FILTEROP_ISFD)) {
				/*
				 * If knote is not on an fd, store on
				 * internal hash table.
				 */
				if (fdp->fd_knhashmask == 0) {
					/* XXXAD can block with fd_lock held */
					fdp->fd_knhash = hashinit(KN_HASHSIZE,
					    HASH_LIST, true,
					    &fdp->fd_knhashmask);
				}
				list = &fdp->fd_knhash[KN_HASH(kn->kn_id,
				    fdp->fd_knhashmask)];
			} else {
				/* Otherwise, knote is on an fd. */
				list = (struct klist *)
				    &fdp->fd_dt->dt_ff[kn->kn_id]->ff_knlist;
				if ((int)kn->kn_id > fdp->fd_lastkqfile)
					fdp->fd_lastkqfile = kn->kn_id;
			}
			SLIST_INSERT_HEAD(list, kn, kn_link);

			/*
			 * N.B. kn->kn_fop may change as the result
			 * of filter_attach()!
			 */
			error = filter_attach(kn);
			if (error != 0) {
#ifdef DEBUG
				struct proc *p = curlwp->l_proc;
				const file_t *ft = kn->kn_obj;
				printf("%s: %s[%d]: event type %d not "
				    "supported for file type %d/%s "
				    "(error %d)\n", __func__,
				    p->p_comm, p->p_pid,
				    kn->kn_filter, ft ? ft->f_type : -1,
				    ft ? ft->f_ops->fo_name : "?", error);
#endif

				/*
				 * N.B. no need to check for this note to
				 * be in-flux, since it was never visible
				 * to the monitored object.
				 *
				 * knote_detach() drops fdp->fd_lock
				 */
				mutex_enter(&kq->kq_lock);
				KNOTE_WILLDETACH(kn);
				KASSERT(kn_in_flux(kn) == false);
				mutex_exit(&kq->kq_lock);
				knote_detach(kn, fdp, false);
				goto done;
			}
			atomic_inc_uint(&kfilter->refcnt);
			goto done_ev_add;
		} else {
			/* No matching knote and the EV_ADD flag is not set. */
			error = ENOENT;
			goto doneunlock;
		}
	}

	if (kev->flags & EV_DELETE) {
		/*
		 * Let the world know that this knote is about to go
		 * away, and wait for it to settle if it's currently
		 * in-flux.
		 */
		mutex_spin_enter(&kq->kq_lock);
		if (kn->kn_status & KN_WILLDETACH) {
			/*
			 * This knote is already on its way out,
			 * so just be done.
			 */
			mutex_spin_exit(&kq->kq_lock);
			goto doneunlock;
		}
		KNOTE_WILLDETACH(kn);
		if (kn_in_flux(kn)) {
			mutex_exit(&fdp->fd_lock);
			/*
			 * It's safe for us to conclusively wait for
			 * this knote to settle because we know we'll
			 * be completing the detach.
			 */
			kn_wait_flux(kn, true);
			KASSERT(kn_in_flux(kn) == false);
			mutex_spin_exit(&kq->kq_lock);
			mutex_enter(&fdp->fd_lock);
		} else {
			mutex_spin_exit(&kq->kq_lock);
		}

		/* knote_detach() drops fdp->fd_lock */
		knote_detach(kn, fdp, true);
		goto done;
	}

	/*
	 * The user may change some filter values after the
	 * initial EV_ADD, but doing so will not reset any
	 * filter which have already been triggered.
	 */
	kn->kn_kevent.udata = kev->udata;
	KASSERT(kn->kn_fop != NULL);
	if (!(kn->kn_fop->f_flags & FILTEROP_ISFD) &&
	    kn->kn_fop->f_touch != NULL) {
		mutex_spin_enter(&kq->kq_lock);
		error = filter_touch(kn, kev, EVENT_REGISTER);
		mutex_spin_exit(&kq->kq_lock);
		if (__predict_false(error != 0)) {
			/* Never a new knote (which would consume newkn). */
			KASSERT(newkn != NULL);
			goto doneunlock;
		}
	} else {
		kn->kn_sfflags = kev->fflags;
		kn->kn_sdata = kev->data;
	}

	/*
	 * We can get here if we are trying to attach
	 * an event to a file descriptor that does not
	 * support events, and the attach routine is
	 * broken and does not return an error.
	 */
 done_ev_add:
	rv = filter_event(kn, 0);
	if (rv)
		knote_activate(kn);

	/* disable knote */
	if ((kev->flags & EV_DISABLE)) {
		mutex_spin_enter(&kq->kq_lock);
		if ((kn->kn_status & KN_DISABLED) == 0)
			kn->kn_status |= KN_DISABLED;
		mutex_spin_exit(&kq->kq_lock);
	}

	/* enable knote */
	if ((kev->flags & EV_ENABLE)) {
		knote_enqueue(kn);
	}
 doneunlock:
	mutex_exit(&fdp->fd_lock);
 done:
	rw_exit(&kqueue_filter_lock);
	if (newkn != NULL)
		kmem_free(newkn, sizeof(*newkn));
	if (fp != NULL)
		fd_putfile(fd);
	return (error);
}

#define KN_FMT(buf, kn) \
    (snprintb((buf), sizeof(buf), __KN_FLAG_BITS, (kn)->kn_status), buf)

#if defined(DDB)
void
kqueue_printit(struct kqueue *kq, bool full, void (*pr)(const char *, ...))
{
	const struct knote *kn;
	u_int count;
	int nmarker;
	char buf[128];

	count = 0;
	nmarker = 0;

	(*pr)("kqueue %p (restart=%d count=%u):\n", kq,
	    !!(kq->kq_count & KQ_RESTART), KQ_COUNT(kq));
	(*pr)("  Queued knotes:\n");
	TAILQ_FOREACH(kn, &kq->kq_head, kn_tqe) {
		if (kn->kn_status & KN_MARKER) {
			nmarker++;
		} else {
			count++;
		}
		(*pr)("    knote %p: kq=%p status=%s\n",
		    kn, kn->kn_kq, KN_FMT(buf, kn));
		(*pr)("      id=0x%lx (%lu) filter=%d\n",
		    (u_long)kn->kn_id, (u_long)kn->kn_id, kn->kn_filter);
		if (kn->kn_kq != kq) {
			(*pr)("      !!! kn->kn_kq != kq\n");
		}
	}
	if (count != KQ_COUNT(kq)) {
		(*pr)("  !!! count(%u) != KQ_COUNT(%u)\n",
		    count, KQ_COUNT(kq));
	}
}
#endif /* DDB */

#if defined(DEBUG)
static void
kqueue_check(const char *func, size_t line, const struct kqueue *kq)
{
	const struct knote *kn;
	u_int count;
	int nmarker;
	char buf[128];

	KASSERT(mutex_owned(&kq->kq_lock));

	count = 0;
	nmarker = 0;
	TAILQ_FOREACH(kn, &kq->kq_head, kn_tqe) {
		if ((kn->kn_status & (KN_MARKER | KN_QUEUED)) == 0) {
			panic("%s,%zu: kq=%p kn=%p !(MARKER|QUEUED) %s",
			    func, line, kq, kn, KN_FMT(buf, kn));
		}
		if ((kn->kn_status & KN_MARKER) == 0) {
			if (kn->kn_kq != kq) {
				panic("%s,%zu: kq=%p kn(%p) != kn->kq(%p): %s",
				    func, line, kq, kn, kn->kn_kq,
				    KN_FMT(buf, kn));
			}
			if ((kn->kn_status & KN_ACTIVE) == 0) {
				panic("%s,%zu: kq=%p kn=%p: !ACTIVE %s",
				    func, line, kq, kn, KN_FMT(buf, kn));
			}
			count++;
			if (count > KQ_COUNT(kq)) {
				panic("%s,%zu: kq=%p kq->kq_count(%u) != "
				    "count(%d), nmarker=%d",
		    		    func, line, kq, KQ_COUNT(kq), count,
				    nmarker);
			}
		} else {
			nmarker++;
		}
	}
}
#define kq_check(a) kqueue_check(__func__, __LINE__, (a))
#else /* defined(DEBUG) */
#define	kq_check(a)	/* nothing */
#endif /* defined(DEBUG) */

static void
kqueue_restart(file_t *fp)
{
	struct kqueue *kq = fp->f_kqueue;
	KASSERT(kq != NULL);

	mutex_spin_enter(&kq->kq_lock);
	kq->kq_count |= KQ_RESTART;
	cv_broadcast(&kq->kq_cv);
	mutex_spin_exit(&kq->kq_lock);
}

/*
 * Scan through the list of events on fp (for a maximum of maxevents),
 * returning the results in to ulistp. Timeout is determined by tsp; if
 * NULL, wait indefinitely, if 0 valued, perform a poll, otherwise wait
 * as appropriate.
 */
static int
kqueue_scan(file_t *fp, size_t maxevents, struct kevent *ulistp,
	    const struct timespec *tsp, register_t *retval,
	    const struct kevent_ops *keops, struct kevent *kevbuf,
	    size_t kevcnt)
{
	struct kqueue	*kq;
	struct kevent	*kevp;
	struct timespec	ats, sleepts;
	struct knote	*kn, *marker, morker;
	size_t		count, nkev, nevents;
	int		timeout, error, touch, rv, influx;
	filedesc_t	*fdp;

	fdp = curlwp->l_fd;
	kq = fp->f_kqueue;
	count = maxevents;
	nkev = nevents = error = 0;
	if (count == 0) {
		*retval = 0;
		return 0;
	}

	if (tsp) {				/* timeout supplied */
		ats = *tsp;
		if (inittimeleft(&ats, &sleepts) == -1) {
			*retval = maxevents;
			return EINVAL;
		}
		timeout = tstohz(&ats);
		if (timeout <= 0)
			timeout = -1;           /* do poll */
	} else {
		/* no timeout, wait forever */
		timeout = 0;
	}

	memset(&morker, 0, sizeof(morker));
	marker = &morker;
	marker->kn_kq = kq;
	marker->kn_status = KN_MARKER;
	mutex_spin_enter(&kq->kq_lock);
 retry:
	kevp = kevbuf;
	if (KQ_COUNT(kq) == 0) {
		if (timeout >= 0) {
			error = cv_timedwait_sig(&kq->kq_cv,
			    &kq->kq_lock, timeout);
			if (error == 0) {
				if (KQ_COUNT(kq) == 0 &&
				    (kq->kq_count & KQ_RESTART)) {
					/* return to clear file reference */
					error = ERESTART;
				} else if (tsp == NULL || (timeout =
				    gettimeleft(&ats, &sleepts)) > 0) {
					goto retry;
				}
			} else {
				/* don't restart after signals... */
				if (error == ERESTART)
					error = EINTR;
				if (error == EWOULDBLOCK)
					error = 0;
			}
		}
		mutex_spin_exit(&kq->kq_lock);
		goto done;
	}

	/* mark end of knote list */
	TAILQ_INSERT_TAIL(&kq->kq_head, marker, kn_tqe);
	influx = 0;

	/*
	 * Acquire the fdp->fd_lock interlock to avoid races with
	 * file creation/destruction from other threads.
	 */
	mutex_spin_exit(&kq->kq_lock);
relock:
	mutex_enter(&fdp->fd_lock);
	mutex_spin_enter(&kq->kq_lock);

	while (count != 0) {
		/*
		 * Get next knote.  We are guaranteed this will never
		 * be NULL because of the marker we inserted above.
		 */
		kn = TAILQ_FIRST(&kq->kq_head);

		bool kn_is_other_marker =
		    (kn->kn_status & KN_MARKER) != 0 && kn != marker;
		bool kn_is_detaching = (kn->kn_status & KN_WILLDETACH) != 0;
		bool kn_is_in_flux = kn_in_flux(kn);

		/*
		 * If we found a marker that's not ours, or this knote
		 * is in a state of flux, then wait for everything to
		 * settle down and go around again.
		 */
		if (kn_is_other_marker || kn_is_detaching || kn_is_in_flux) {
			if (influx) {
				influx = 0;
				KQ_FLUX_WAKEUP(kq);
			}
			mutex_exit(&fdp->fd_lock);
			if (kn_is_other_marker || kn_is_in_flux) {
				KQ_FLUX_WAIT(kq);
				mutex_spin_exit(&kq->kq_lock);
			} else {
				/*
				 * Detaching but not in-flux?  Someone is
				 * actively trying to finish the job; just
				 * go around and try again.
				 */
				KASSERT(kn_is_detaching);
				mutex_spin_exit(&kq->kq_lock);
				preempt_point();
			}
			goto relock;
		}

		TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe);
		if (kn == marker) {
			/* it's our marker, stop */
			KQ_FLUX_WAKEUP(kq);
			if (count == maxevents) {
				mutex_exit(&fdp->fd_lock);
				goto retry;
			}
			break;
		}
		KASSERT((kn->kn_status & KN_BUSY) == 0);

		kq_check(kq);
		kn->kn_status &= ~KN_QUEUED;
		kn->kn_status |= KN_BUSY;
		kq_check(kq);
		if (kn->kn_status & KN_DISABLED) {
			kn->kn_status &= ~KN_BUSY;
			kq->kq_count--;
			/* don't want disabled events */
			continue;
		}
		if ((kn->kn_flags & EV_ONESHOT) == 0) {
			mutex_spin_exit(&kq->kq_lock);
			KASSERT(mutex_owned(&fdp->fd_lock));
			rv = filter_event(kn, 0);
			mutex_spin_enter(&kq->kq_lock);
			/* Re-poll if note was re-enqueued. */
			if ((kn->kn_status & KN_QUEUED) != 0) {
				kn->kn_status &= ~KN_BUSY;
				/* Re-enqueue raised kq_count, lower it again */
				kq->kq_count--;
				influx = 1;
				continue;
			}
			if (rv == 0) {
				/*
				 * non-ONESHOT event that hasn't triggered
				 * again, so it will remain de-queued.
				 */
				kn->kn_status &= ~(KN_ACTIVE|KN_BUSY);
				kq->kq_count--;
				influx = 1;
				continue;
			}
		} else {
			/*
			 * Must NOT drop kq_lock until we can do
			 * the KNOTE_WILLDETACH() below.
			 */
		}
		KASSERT(kn->kn_fop != NULL);
		touch = (!(kn->kn_fop->f_flags & FILTEROP_ISFD) &&
				kn->kn_fop->f_touch != NULL);
		/* XXXAD should be got from f_event if !oneshot. */
		KASSERT((kn->kn_status & KN_WILLDETACH) == 0);
		if (touch) {
			(void)filter_touch(kn, kevp, EVENT_PROCESS);
		} else {
			*kevp = kn->kn_kevent;
		}
		kevp++;
		nkev++;
		influx = 1;
		if (kn->kn_flags & EV_ONESHOT) {
			/* delete ONESHOT events after retrieval */
			KNOTE_WILLDETACH(kn);
			kn->kn_status &= ~KN_BUSY;
			kq->kq_count--;
			KASSERT(kn_in_flux(kn) == false);
			KASSERT((kn->kn_status & KN_WILLDETACH) != 0 &&
				kn->kn_kevent.udata == curlwp);
			mutex_spin_exit(&kq->kq_lock);
			knote_detach(kn, fdp, true);
			mutex_enter(&fdp->fd_lock);
			mutex_spin_enter(&kq->kq_lock);
		} else if (kn->kn_flags & EV_CLEAR) {
			/* clear state after retrieval */
			kn->kn_data = 0;
			kn->kn_fflags = 0;
			/*
			 * Manually clear knotes who weren't
			 * 'touch'ed.
			 */
			if (touch == 0) {
				kn->kn_data = 0;
				kn->kn_fflags = 0;
			}
			kn->kn_status &= ~(KN_ACTIVE|KN_BUSY);
			kq->kq_count--;
		} else if (kn->kn_flags & EV_DISPATCH) {
			kn->kn_status |= KN_DISABLED;
			kn->kn_status &= ~(KN_ACTIVE|KN_BUSY);
			kq->kq_count--;
		} else {
			/* add event back on list */
			kq_check(kq);
			kn->kn_status |= KN_QUEUED;
			kn->kn_status &= ~KN_BUSY;
			TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe);
			kq_check(kq);
		}

		if (nkev == kevcnt) {
			/* do copyouts in kevcnt chunks */
			influx = 0;
			KQ_FLUX_WAKEUP(kq);
			mutex_spin_exit(&kq->kq_lock);
			mutex_exit(&fdp->fd_lock);
			error = (*keops->keo_put_events)
			    (keops->keo_private,
			    kevbuf, ulistp, nevents, nkev);
			mutex_enter(&fdp->fd_lock);
			mutex_spin_enter(&kq->kq_lock);
			nevents += nkev;
			nkev = 0;
			kevp = kevbuf;
		}
		count--;
		if (error != 0 || count == 0) {
			/* remove marker */
			TAILQ_REMOVE(&kq->kq_head, marker, kn_tqe);
			break;
		}
	}
	KQ_FLUX_WAKEUP(kq);
	mutex_spin_exit(&kq->kq_lock);
	mutex_exit(&fdp->fd_lock);

done:
	if (nkev != 0) {
		/* copyout remaining events */
		error = (*keops->keo_put_events)(keops->keo_private,
		    kevbuf, ulistp, nevents, nkev);
	}
	*retval = maxevents - count;

	return error;
}

/*
 * fileops ioctl method for a kqueue descriptor.
 *
 * Two ioctls are currently supported. They both use struct kfilter_mapping:
 *	KFILTER_BYNAME		find name for filter, and return result in
 *				name, which is of size len.
 *	KFILTER_BYFILTER	find filter for name. len is ignored.
 */
/*ARGSUSED*/
static int
kqueue_ioctl(file_t *fp, u_long com, void *data)
{
	struct kfilter_mapping	*km;
	const struct kfilter	*kfilter;
	char			*name;
	int			error;

	km = data;
	error = 0;
	name = kmem_alloc(KFILTER_MAXNAME, KM_SLEEP);

	switch (com) {
	case KFILTER_BYFILTER:	/* convert filter -> name */
		rw_enter(&kqueue_filter_lock, RW_READER);
		kfilter = kfilter_byfilter(km->filter);
		if (kfilter != NULL) {
			strlcpy(name, kfilter->name, KFILTER_MAXNAME);
			rw_exit(&kqueue_filter_lock);
			error = copyoutstr(name, km->name, km->len, NULL);
		} else {
			rw_exit(&kqueue_filter_lock);
			error = ENOENT;
		}
		break;

	case KFILTER_BYNAME:	/* convert name -> filter */
		error = copyinstr(km->name, name, KFILTER_MAXNAME, NULL);
		if (error) {
			break;
		}
		rw_enter(&kqueue_filter_lock, RW_READER);
		kfilter = kfilter_byname(name);
		if (kfilter != NULL)
			km->filter = kfilter->filter;
		else
			error = ENOENT;
		rw_exit(&kqueue_filter_lock);
		break;

	default:
		error = ENOTTY;
		break;

	}
	kmem_free(name, KFILTER_MAXNAME);
	return (error);
}

/*
 * fileops fcntl method for a kqueue descriptor.
 */
static int
kqueue_fcntl(file_t *fp, u_int com, void *data)
{

	return (ENOTTY);
}

/*
 * fileops poll method for a kqueue descriptor.
 * Determine if kqueue has events pending.
 */
static int
kqueue_poll(file_t *fp, int events)
{
	struct kqueue	*kq;
	int		revents;

	kq = fp->f_kqueue;

	revents = 0;
	if (events & (POLLIN | POLLRDNORM)) {
		mutex_spin_enter(&kq->kq_lock);
		if (KQ_COUNT(kq) != 0) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(curlwp, &kq->kq_sel);
		}
		kq_check(kq);
		mutex_spin_exit(&kq->kq_lock);
	}

	return revents;
}

/*
 * fileops stat method for a kqueue descriptor.
 * Returns dummy info, with st_size being number of events pending.
 */
static int
kqueue_stat(file_t *fp, struct stat *st)
{
	struct kqueue *kq;

	kq = fp->f_kqueue;

	memset(st, 0, sizeof(*st));
	st->st_size = KQ_COUNT(kq);
	st->st_blksize = sizeof(struct kevent);
	st->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
	st->st_blocks = 1;
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);

	return 0;
}

static void
kqueue_doclose(struct kqueue *kq, struct klist *list, int fd)
{
	struct knote *kn;
	filedesc_t *fdp;

	fdp = kq->kq_fdp;

	KASSERT(mutex_owned(&fdp->fd_lock));

 again:
	for (kn = SLIST_FIRST(list); kn != NULL;) {
		if (kq != kn->kn_kq) {
			kn = SLIST_NEXT(kn, kn_link);
			continue;
		}
		if (knote_detach_quiesce(kn)) {
			mutex_enter(&fdp->fd_lock);
			goto again;
		}
		knote_detach(kn, fdp, true);
		mutex_enter(&fdp->fd_lock);
		kn = SLIST_FIRST(list);
	}
}

/*
 * fileops close method for a kqueue descriptor.
 */
static int
kqueue_close(file_t *fp)
{
	struct kqueue *kq;
	filedesc_t *fdp;
	fdfile_t *ff;
	int i;

	kq = fp->f_kqueue;
	fp->f_kqueue = NULL;
	fp->f_type = 0;
	fdp = curlwp->l_fd;

	KASSERT(kq->kq_fdp == fdp);

	mutex_enter(&fdp->fd_lock);

	/*
	 * We're doing to drop the fd_lock multiple times while
	 * we detach knotes.  During this time, attempts to register
	 * knotes via the back door (e.g. knote_proc_fork_track())
	 * need to fail, lest they sneak in to attach a knote after
	 * we've already drained the list it's destined for.
	 *
	 * We must acquire kq_lock here to set KQ_CLOSING (to serialize
	 * with other code paths that modify kq_count without holding
	 * the fd_lock), but once this bit is set, it's only safe to
	 * test it while holding the fd_lock, and holding kq_lock while
	 * doing so is not necessary.
	 */
	mutex_enter(&kq->kq_lock);
	kq->kq_count |= KQ_CLOSING;
	mutex_exit(&kq->kq_lock);

	for (i = 0; i <= fdp->fd_lastkqfile; i++) {
		if ((ff = fdp->fd_dt->dt_ff[i]) == NULL)
			continue;
		kqueue_doclose(kq, (struct klist *)&ff->ff_knlist, i);
	}
	if (fdp->fd_knhashmask != 0) {
		for (i = 0; i < fdp->fd_knhashmask + 1; i++) {
			kqueue_doclose(kq, &fdp->fd_knhash[i], -1);
		}
	}

	mutex_exit(&fdp->fd_lock);

#if defined(DEBUG)
	mutex_enter(&kq->kq_lock);
	kq_check(kq);
	mutex_exit(&kq->kq_lock);
#endif /* DEBUG */
	KASSERT(TAILQ_EMPTY(&kq->kq_head));
	KASSERT(KQ_COUNT(kq) == 0);
	mutex_destroy(&kq->kq_lock);
	cv_destroy(&kq->kq_cv);
	seldestroy(&kq->kq_sel);
	kmem_free(kq, sizeof(*kq));

	return (0);
}

/*
 * struct fileops kqfilter method for a kqueue descriptor.
 * Event triggered when monitored kqueue changes.
 */
static int
kqueue_kqfilter(file_t *fp, struct knote *kn)
{
	struct kqueue *kq;

	kq = ((file_t *)kn->kn_obj)->f_kqueue;

	KASSERT(fp == kn->kn_obj);

	if (kn->kn_filter != EVFILT_READ)
		return EINVAL;

	kn->kn_fop = &kqread_filtops;
	mutex_enter(&kq->kq_lock);
	selrecord_knote(&kq->kq_sel, kn);
	mutex_exit(&kq->kq_lock);

	return 0;
}


/*
 * Walk down a list of knotes, activating them if their event has
 * triggered.  The caller's object lock (e.g. device driver lock)
 * must be held.
 */
void
knote(struct klist *list, long hint)
{
	struct knote *kn, *tmpkn;

	SLIST_FOREACH_SAFE(kn, list, kn_selnext, tmpkn) {
		if (filter_event(kn, hint)) {
			knote_activate(kn);
		}
	}
}

/*
 * Remove all knotes referencing a specified fd
 */
void
knote_fdclose(int fd)
{
	struct klist *list;
	struct knote *kn;
	filedesc_t *fdp;

 again:
	fdp = curlwp->l_fd;
	mutex_enter(&fdp->fd_lock);
	list = (struct klist *)&fdp->fd_dt->dt_ff[fd]->ff_knlist;
	while ((kn = SLIST_FIRST(list)) != NULL) {
		if (knote_detach_quiesce(kn)) {
			goto again;
		}
		knote_detach(kn, fdp, true);
		mutex_enter(&fdp->fd_lock);
	}
	mutex_exit(&fdp->fd_lock);
}

/*
 * Drop knote.  Called with fdp->fd_lock held, and will drop before
 * returning.
 */
static void
knote_detach(struct knote *kn, filedesc_t *fdp, bool dofop)
{
	struct klist *list;
	struct kqueue *kq;

	kq = kn->kn_kq;

	KASSERT((kn->kn_status & KN_MARKER) == 0);
	KASSERT((kn->kn_status & KN_WILLDETACH) != 0);
	KASSERT(kn->kn_fop != NULL);
	KASSERT(mutex_owned(&fdp->fd_lock));

	/* Remove from monitored object. */
	if (dofop) {
		filter_detach(kn);
	}

	/* Remove from descriptor table. */
	if (kn->kn_fop->f_flags & FILTEROP_ISFD)
		list = (struct klist *)&fdp->fd_dt->dt_ff[kn->kn_id]->ff_knlist;
	else
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];

	SLIST_REMOVE(list, kn, knote, kn_link);

	/* Remove from kqueue. */
again:
	mutex_spin_enter(&kq->kq_lock);
	KASSERT(kn_in_flux(kn) == false);
	if ((kn->kn_status & KN_QUEUED) != 0) {
		kq_check(kq);
		KASSERT(KQ_COUNT(kq) != 0);
		kq->kq_count--;
		TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe);
		kn->kn_status &= ~KN_QUEUED;
		kq_check(kq);
	} else if (kn->kn_status & KN_BUSY) {
		mutex_spin_exit(&kq->kq_lock);
		goto again;
	}
	mutex_spin_exit(&kq->kq_lock);

	mutex_exit(&fdp->fd_lock);
	if (kn->kn_fop->f_flags & FILTEROP_ISFD)
		fd_putfile(kn->kn_id);
	atomic_dec_uint(&kn->kn_kfilter->refcnt);
	kmem_free(kn, sizeof(*kn));
}

/*
 * Queue new event for knote.
 */
static void
knote_enqueue(struct knote *kn)
{
	struct kqueue *kq;

	KASSERT((kn->kn_status & KN_MARKER) == 0);

	kq = kn->kn_kq;

	mutex_spin_enter(&kq->kq_lock);
	if (__predict_false(kn->kn_status & KN_WILLDETACH)) {
		/* Don't bother enqueueing a dying knote. */
		goto out;
	}
	if ((kn->kn_status & KN_DISABLED) != 0) {
		kn->kn_status &= ~KN_DISABLED;
	}
	if ((kn->kn_status & (KN_ACTIVE | KN_QUEUED)) == KN_ACTIVE) {
		kq_check(kq);
		kn->kn_status |= KN_QUEUED;
		TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe);
		KASSERT(KQ_COUNT(kq) < KQ_MAXCOUNT);
		kq->kq_count++;
		kq_check(kq);
		cv_broadcast(&kq->kq_cv);
		selnotify(&kq->kq_sel, 0, NOTE_SUBMIT);
	}
 out:
	mutex_spin_exit(&kq->kq_lock);
}
/*
 * Queue new event for knote.
 */
static void
knote_activate_locked(struct knote *kn)
{
	struct kqueue *kq;

	KASSERT((kn->kn_status & KN_MARKER) == 0);

	kq = kn->kn_kq;

	if (__predict_false(kn->kn_status & KN_WILLDETACH)) {
		/* Don't bother enqueueing a dying knote. */
		return;
	}
	kn->kn_status |= KN_ACTIVE;
	if ((kn->kn_status & (KN_QUEUED | KN_DISABLED)) == 0) {
		kq_check(kq);
		kn->kn_status |= KN_QUEUED;
		TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe);
		KASSERT(KQ_COUNT(kq) < KQ_MAXCOUNT);
		kq->kq_count++;
		kq_check(kq);
		cv_broadcast(&kq->kq_cv);
		selnotify(&kq->kq_sel, 0, NOTE_SUBMIT);
	}
}

static void
knote_activate(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;

	mutex_spin_enter(&kq->kq_lock);
	knote_activate_locked(kn);
	mutex_spin_exit(&kq->kq_lock);
}

static void
knote_deactivate_locked(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;

	if (kn->kn_status & KN_QUEUED) {
		kq_check(kq);
		kn->kn_status &= ~KN_QUEUED;
		TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe);
		KASSERT(KQ_COUNT(kq) > 0);
		kq->kq_count--;
		kq_check(kq);
	}
	kn->kn_status &= ~KN_ACTIVE;
}

/*
 * Set EV_EOF on the specified knote.  Also allows additional
 * EV_* flags to be set (e.g. EV_ONESHOT).
 */
void
knote_set_eof(struct knote *kn, uint32_t flags)
{
	struct kqueue *kq = kn->kn_kq;

	mutex_spin_enter(&kq->kq_lock);
	kn->kn_flags |= EV_EOF | flags;
	mutex_spin_exit(&kq->kq_lock);
}

/*
 * Clear EV_EOF on the specified knote.
 */
void
knote_clear_eof(struct knote *kn)
{
	struct kqueue *kq = kn->kn_kq;

	mutex_spin_enter(&kq->kq_lock);
	kn->kn_flags &= ~EV_EOF;
	mutex_spin_exit(&kq->kq_lock);
}
