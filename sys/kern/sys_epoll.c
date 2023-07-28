/*	$NetBSD: sys_epoll.c,v 1.1 2023/07/28 18:19:01 christos Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2007 Roman Divacky
 * Copyright (c) 2014 Dmitry Chagin <dchagin@FreeBSD.org>
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
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_epoll.c,v 1.1 2023/07/28 18:19:01 christos Exp $");


#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/epoll.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/vnode.h>

#include <sys/syscallargs.h>

#define	EPOLL_MAX_DEPTH		5

#define	EPOLL_EVRD	(EPOLLIN|EPOLLRDNORM)
#define	EPOLL_EVWR	(EPOLLOUT|EPOLLWRNORM)
#define	EPOLL_EVSUP	(EPOLLET|EPOLLONESHOT|EPOLLHUP|EPOLLERR|EPOLLPRI \
			|EPOLL_EVRD|EPOLL_EVWR|EPOLLRDHUP)

#define	kext_data	ext[0]
#define	kext_epfd	ext[1]
#define	kext_fd		ext[2]

#if DEBUG
#define	DPRINTF(x) uprintf x
#else
#define	DPRINTF(x) __nothing
#endif

struct epoll_edge {
	int epfd;
	int fd;
};

__BITMAP_TYPE(epoll_seen, char, 1);

static int	epoll_to_kevent(int, int, struct epoll_event *, struct kevent *,
    int *);
static void	kevent_to_epoll(struct kevent *, struct epoll_event *);
static int      epoll_kev_put_events(void *, struct kevent *, struct kevent *,
    size_t, int);
static int	epoll_kev_fetch_changes(void *, const struct kevent *,
    struct kevent *, size_t, int);
static int	epoll_kev_fetch_timeout(const void *, void *, size_t);
static int	epoll_register_kevent(register_t *, int, int, int,
    unsigned int);
static int	epoll_fd_registered(register_t *, int, int);
static int	epoll_delete_all_events(register_t *, int, int);
static int	epoll_recover_watch_tree(struct epoll_edge *, size_t, size_t);
static int	epoll_dfs(struct epoll_edge *, size_t, struct epoll_seen *,
    size_t, int, int);
static int	epoll_check_loop_and_depth(struct lwp *, int, int);

/*
 * epoll_create1(2).  Parse the flags and then create a kqueue instance.
 */
int
sys_epoll_create1(struct lwp *l, const struct sys_epoll_create1_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) flags;
	} */
	struct sys_kqueue1_args kqa;

	if ((SCARG(uap, flags) & ~(O_CLOEXEC)) != 0)
		return EINVAL;

	SCARG(&kqa, flags) = SCARG(uap, flags);

	return sys_kqueue1(l, &kqa, retval);
}

/*
 * Structure converting function from epoll to kevent.
 */
static int
epoll_to_kevent(int epfd, int fd, struct epoll_event *l_event,
    struct kevent *kevent, int *nkevents)
{
	uint32_t levents = l_event->events;
	uint32_t kev_flags = EV_ADD | EV_ENABLE;

	/* flags related to how event is registered */
	if ((levents & EPOLLONESHOT) != 0)
		kev_flags |= EV_DISPATCH;
	if ((levents & EPOLLET) != 0)
		kev_flags |= EV_CLEAR;
	if ((levents & EPOLLERR) != 0)
		kev_flags |= EV_ERROR;
	if ((levents & EPOLLRDHUP) != 0)
		kev_flags |= EV_EOF;

	/* flags related to what event is registered */
	if ((levents & EPOLL_EVRD) != 0) {
		EV_SET(kevent, fd, EVFILT_READ, kev_flags, 0, 0, 0);
		kevent->kext_data = l_event->data;
		kevent->kext_epfd = epfd;
		kevent->kext_fd = fd;
		++kevent;
		++(*nkevents);
	}
	if ((levents & EPOLL_EVWR) != 0) {
		EV_SET(kevent, fd, EVFILT_WRITE, kev_flags, 0, 0, 0);
		kevent->kext_data = l_event->data;
		kevent->kext_epfd = epfd;
		kevent->kext_fd = fd;
		++kevent;
		++(*nkevents);
	}
	/* zero event mask is legal */
	if ((levents & (EPOLL_EVRD | EPOLL_EVWR)) == 0) {
		EV_SET(kevent++, fd, EVFILT_READ, EV_ADD|EV_DISABLE, 0, 0, 0);
		++(*nkevents);
	}

	if ((levents & ~(EPOLL_EVSUP)) != 0) {
		return EINVAL;
	}

	return 0;
}

/*
 * Structure converting function from kevent to epoll. In a case
 * this is called on error in registration we store the error in
 * event->data and pick it up later in sys_epoll_ctl().
 */
static void
kevent_to_epoll(struct kevent *kevent, struct epoll_event *l_event)
{

	l_event->data = kevent->kext_data;

	if ((kevent->flags & EV_ERROR) != 0) {
		l_event->events = EPOLLERR;
		return;
	}

	/* XXX EPOLLPRI, EPOLLHUP */
	switch (kevent->filter) {
	case EVFILT_READ:
		l_event->events = EPOLLIN;
		if ((kevent->flags & EV_EOF) != 0)
			l_event->events |= EPOLLRDHUP;
		break;
	case EVFILT_WRITE:
		l_event->events = EPOLLOUT;
		break;
	default:
		DPRINTF(("%s: unhandled kevent filter %d\n", __func__,
		    kevent->filter));
		break;
	}
}

/*
 * Copyout callback used by kevent.  This converts kevent events to
 * epoll events that are located in args->eventlist.
 */
static int
epoll_kev_put_events(void *ctx, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{
	int i;
	struct epoll_event *eep = (struct epoll_event *)eventlist;

	KASSERT(n >= 0 && n < EPOLL_MAX_EVENTS);

	for (i = 0; i < n; i++)
		kevent_to_epoll(events + i, eep + index + i);

	return 0;
}

/*
 * Copyin callback used by kevent. This copies already
 * converted filters from kernel memory to the kevent
 * internal kernel memory. Hence the memcpy instead of
 * copyin.
 */
static int
epoll_kev_fetch_changes(void *ctx, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{
	KASSERT(n >= 0 && n < EPOLL_MAX_EVENTS);

	memcpy(changes, changelist + index, n * sizeof(*changes));

	return 0;
}

/*
 * Timer copy callback used by kevent.  Copies a converted timeout
 * from kernel memory to kevent memory.  Hence the memcpy instead of
 * just using copyin.
 */
static int
epoll_kev_fetch_timeout(const void *src, void *dest, size_t size)
{
	memcpy(dest, src, size);

	return 0;
}

/*
 * Load epoll filter, convert it to kevent filter and load it into
 * kevent subsystem.
 *
 * event must point to kernel memory or be NULL.
 */
int
epoll_ctl_common(struct lwp *l, register_t *retval, int epfd, int op, int fd,
    struct epoll_event *event)
{
	struct kevent kev[2];
        struct kevent_ops k_ops = {
		.keo_private = NULL,
		.keo_fetch_timeout = NULL,
		.keo_fetch_changes = epoll_kev_fetch_changes,
		.keo_put_events = NULL,
	};
	file_t *epfp, *fp;
	int error = 0;
	int nchanges = 0;

	/*
	 * Need to validate epfd and fd separately from kevent1 to match
	 * Linux's errno behaviour.
	 */
	epfp = fd_getfile(epfd);
	if (epfp == NULL)
		return EBADF;
	if (epfp->f_type != DTYPE_KQUEUE)
		error = EINVAL;
	fd_putfile(epfd);
	if (error != 0)
		return error;

	fp = fd_getfile(fd);
	if (fp == NULL)
		return EBADF;
	if (fp->f_type == DTYPE_VNODE) {
		switch (fp->f_vnode->v_type) {
		case VREG:
		case VDIR:
		case VBLK:
		case VLNK:
			error = EPERM;
			break;

		default:
			break;
		}
	}
	fd_putfile(fd);
	if (error != 0)
		return error;

	/* Linux disallows spying on himself */
	if (epfd == fd) {
		return EINVAL;
	}

	if (op != EPOLL_CTL_DEL) {
		error = epoll_to_kevent(epfd, fd, event, kev, &nchanges);
		if (error != 0)
			return error;
	}

	switch (op) {
	case EPOLL_CTL_MOD:
		error = epoll_delete_all_events(retval, epfd, fd);
		if (error != 0)
			return error;
		break;

	case EPOLL_CTL_ADD:
		if (epoll_fd_registered(retval, epfd, fd))
			return EEXIST;
		error = epoll_check_loop_and_depth(l, epfd, fd);
		if (error != 0)
			return error;
		break;

	case EPOLL_CTL_DEL:
		/* CTL_DEL means unregister this fd with this epoll */
		return epoll_delete_all_events(retval, epfd, fd);

	default:
		DPRINTF(("%s: invalid op %d\n", ___func__, op));
		return EINVAL;
	}

	error = kevent1(retval, epfd, kev, nchanges, NULL, 0, NULL, &k_ops);

	if (error == EOPNOTSUPP) {
		error = EPERM;
	}

	return error;
}

/*
 * epoll_ctl(2).  Copyin event if necessary and then call
 * epoll_ctl_common().
 */
int
sys_epoll_ctl(struct lwp *l, const struct sys_epoll_ctl_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) epfd;
		syscallarg(int) op;
		syscallarg(int) fd;
		syscallarg(struct epoll_event *) event;
	} */
	struct epoll_event ee;
	struct epoll_event *eep;
	int error;

	if (SCARG(uap, op) != EPOLL_CTL_DEL) {
		error = copyin(SCARG(uap, event), &ee, sizeof(ee));
		if (error != 0)
			return error;

		eep = &ee;
	} else
		eep = NULL;

	return epoll_ctl_common(l, retval, SCARG(uap, epfd), SCARG(uap, op),
	    SCARG(uap, fd), eep);
}

/*
 * Wait for a filter to be triggered on the epoll file descriptor.
 * All of the epoll_*wait* syscalls eventually end up here.
 *
 * events, nss, and ssp must point to kernel memory (or be NULL).
 */
int
epoll_wait_common(struct lwp *l, register_t *retval, int epfd,
    struct epoll_event *events, int maxevents, struct timespec *tsp,
    const sigset_t *nssp)
{
	struct kevent_ops k_ops = {
	        .keo_private = NULL,
		.keo_fetch_timeout = epoll_kev_fetch_timeout,
		.keo_fetch_changes = NULL,
		.keo_put_events = epoll_kev_put_events,
	};
	struct proc *p = l->l_proc;
	file_t *epfp;
	sigset_t oss;
	int error = 0;

	if (maxevents <= 0 || maxevents > EPOLL_MAX_EVENTS)
		return EINVAL;

	/*
	 * Need to validate epfd separately from kevent1 to match
	 * Linux's errno behaviour.
	 */
	epfp = fd_getfile(epfd);
	if (epfp == NULL)
		return EBADF;
	if (epfp->f_type != DTYPE_KQUEUE)
		error = EINVAL;
	fd_putfile(epfd);
	if (error != 0)
		return error;

	if (nssp != NULL) {
		mutex_enter(p->p_lock);
		error = sigprocmask1(l, SIG_SETMASK, nssp, &oss);
		mutex_exit(p->p_lock);
		if (error != 0)
			return error;
	}

	error = kevent1(retval, epfd, NULL, 0, (struct kevent *)events,
	    maxevents, tsp, &k_ops);
	/*
	 * Since we're not registering nay events, ENOMEM should not
	 * be possible for this specific kevent1 call.
	 */
	KASSERT(error != ENOMEM);

	if (nssp != NULL) {
	        mutex_enter(p->p_lock);
		error = sigprocmask1(l, SIG_SETMASK, &oss, NULL);
		mutex_exit(p->p_lock);
	}

	return error;
}

/*
 * epoll_pwait2(2).
 */
int
sys_epoll_pwait2(struct lwp *l, const struct sys_epoll_pwait2_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) epfd;
		syscallarg(struct epoll_event *) events;
		syscallarg(int) maxevents;
		syscallarg(struct timespec *) timeout;
		syscallarg(sigset_t *) sigmask;
	} */
	struct epoll_event *events;
	struct timespec ts, *tsp;
	sigset_t ss, *ssp;
	int error;
	const int maxevents = SCARG(uap, maxevents);

	if (maxevents <= 0 || maxevents >= EPOLL_MAX_EVENTS)
		return EINVAL;

	if (SCARG(uap, timeout) != NULL) {
		error = copyin(SCARG(uap, timeout), &ts, sizeof(ts));
		if (error != 0)
			return error;

		tsp = &ts;
	} else
		tsp = NULL;

	if (SCARG(uap, sigmask) != NULL) {
		error = copyin(SCARG(uap, sigmask), &ss, sizeof(ss));
		if (error != 0)
			return error;

		ssp = &ss;
	} else
		ssp = NULL;

	events = kmem_alloc(maxevents * sizeof(*events), KM_SLEEP);

	error = epoll_wait_common(l, retval, SCARG(uap, epfd), events,
	    maxevents, tsp, ssp);
	if (error == 0)
		error = copyout(events, SCARG(uap, events),
		    *retval * sizeof(*events));

	kmem_free(events, maxevents * sizeof(*events));
	return error;
}

/*
 * Helper that registers a single kevent.
 */
static int
epoll_register_kevent(register_t *retval, int epfd, int fd, int filter,
    unsigned int flags)
{
	struct kevent kev;
	struct kevent_ops k_ops = {
		.keo_private = NULL,
		.keo_fetch_timeout = NULL,
		.keo_fetch_changes = epoll_kev_fetch_changes,
		.keo_put_events = NULL,
	};

	EV_SET(&kev, fd, filter, flags, 0, 0, 0);

        return kevent1(retval, epfd, &kev, 1, NULL, 0, NULL, &k_ops);
}

/*
 * Check if an fd is already registered in the kqueue referenced by epfd.
 */
static int
epoll_fd_registered(register_t *retval, int epfd, int fd)
{
	/*
	 * Set empty filter flags to avoid accidental modification of already
	 * registered events. In the case of event re-registration:
	 * 1. If event does not exists kevent() does nothing and returns ENOENT
	 * 2. If event does exists, it's enabled/disabled state is preserved
	 *    but fflags, data and udata fields are overwritten. So we can not
	 *    set socket lowats and store user's context pointer in udata.
	 */
	if (epoll_register_kevent(retval, epfd, fd, EVFILT_READ, 0) != ENOENT ||
	    epoll_register_kevent(retval, epfd, fd, EVFILT_WRITE, 0) != ENOENT)
		return 1;

	return 0;
}

/*
 * Remove all events in the kqueue referenced by epfd that depend on
 * fd.
 */
static int
epoll_delete_all_events(register_t *retval, int epfd, int fd)
{
	int error1, error2;

	error1 = epoll_register_kevent(retval, epfd, fd, EVFILT_READ,
	    EV_DELETE);
	error2 = epoll_register_kevent(retval, epfd, fd, EVFILT_WRITE,
	    EV_DELETE);

	/* return 0 if at least one result positive */
	return error1 == 0 ? 0 : error2;
}

/*
 * Interate through all the knotes and recover a directed graph on
 * which kqueues are watching each other.
 *
 * If edges is NULL, the number of edges is still counted but no graph
 * is assembled.
 */
static int
epoll_recover_watch_tree(struct epoll_edge *edges, size_t nedges, size_t nfds) {
	file_t *currfp, *targetfp;
	struct knote *kn, *tmpkn;
	size_t i, nedges_so_far = 0;

	for (i = 0; i < nfds && (edges == NULL || nedges_so_far < nedges); i++)
	{
		currfp = fd_getfile(i);
		if (currfp == NULL)
			continue;
		if (currfp->f_type != DTYPE_KQUEUE)
			goto continue_count_outer;

		SLIST_FOREACH_SAFE(kn, &currfp->f_kqueue->kq_sel.sel_klist,
		    kn_selnext, tmpkn) {
			targetfp = fd_getfile(kn->kn_kevent.kext_epfd);
			if (targetfp == NULL)
				continue;
			if (targetfp->f_type == DTYPE_KQUEUE) {
				if (edges != NULL) {
					edges[nedges_so_far].epfd =
					    kn->kn_kevent.kext_epfd;
					edges[nedges_so_far].fd =
					    kn->kn_kevent.kext_fd;
				}
				nedges_so_far++;
			}

			fd_putfile(kn->kn_kevent.kext_epfd);
		}

continue_count_outer:
		fd_putfile(i);
	}

	return nedges_so_far;
}

/*
 * Run dfs on the graph described by edges, checking for loops and a
 * depth greater than EPOLL_MAX_DEPTH.
 */
static int
epoll_dfs(struct epoll_edge *edges, size_t nedges, struct epoll_seen *seen,
    size_t nseen, int currfd, int depth)
{
	int error;
	size_t i;

	KASSERT(edges != NULL);
	KASSERT(seen != NULL);
	KASSERT(nedges > 0);
	KASSERT(currfd < nseen);
	KASSERT(0 <= depth && depth <= EPOLL_MAX_DEPTH + 1);

	if (__BITMAP_ISSET(currfd, seen))
		return ELOOP;

	__BITMAP_SET(currfd, seen);

	depth++;
	if (depth > EPOLL_MAX_DEPTH)
		return EINVAL;

	for (i = 0; i < nedges; i++) {
		if (edges[i].epfd != currfd)
			continue;

		error = epoll_dfs(edges, nedges, seen, nseen,
		    edges[i].fd, depth);
		if (error != 0)
			return error;
	}

	return 0;
}

/*
 * Check if adding fd to epfd would violate the maximum depth or
 * create a loop.
 */
static int
epoll_check_loop_and_depth(struct lwp *l, int epfd, int fd)
{
	int error;
	file_t *fp;
	struct epoll_edge *edges;
	struct epoll_seen *seen;
	size_t nedges, nfds, seen_size;
	bool fdirrelevant;

	/* If the target isn't another kqueue, we can skip this check */
	fp = fd_getfile(fd);
	if (fp == NULL)
		return 0;
	fdirrelevant = fp->f_type != DTYPE_KQUEUE;
	fd_putfile(fd);
	if (fdirrelevant)
		return 0;

	nfds = l->l_proc->p_fd->fd_lastfile + 1;

	/*
	 * We call epoll_recover_watch_tree twice, once to find the
	 * number of edges, and once to actually fill them in.  We add one
	 * because we want to include the edge epfd->fd.
	 */
        nedges = 1 + epoll_recover_watch_tree(NULL, 0, nfds);

	edges = kmem_zalloc(nedges * sizeof(*edges), KM_SLEEP);

	epoll_recover_watch_tree(edges + 1, nedges - 1, nfds);

	edges[0].epfd = epfd;
	edges[0].fd = fd;

	seen_size = __BITMAP_SIZE(char, nfds);
	seen = kmem_zalloc(seen_size, KM_SLEEP);

	error = epoll_dfs(edges, nedges, seen, nfds, epfd, 0);

	kmem_free(seen, seen_size);
	kmem_free(edges, nedges * sizeof(*edges));

	return error;
}
