/*
 * eloop - portable event based main loop.
 * Copyright (c) 2006-2017 Roy Marples <roy@marples.name>
 * All rights reserved.

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

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif
#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* config.h should define HAVE_KQUEUE, HAVE_EPOLL, etc. */
#if defined(HAVE_CONFIG_H) && !defined(NO_CONFIG_H)
#include "config.h"
#endif

/* Attempt to autodetect kqueue or epoll.
 * Failing that, fall back to pselect. */
#if !defined(HAVE_KQUEUE) && !defined(HAVE_EPOLL) && !defined(HAVE_PSELECT) && \
    !defined(HAVE_POLLTS) && !defined(HAVE_PPOLL)
#if defined(BSD)
/* Assume BSD has a working sys/queue.h and kqueue(2) interface. */
#define HAVE_SYS_QUEUE_H
#define HAVE_KQUEUE
#define WARN_SELECT
#elif defined(__linux__) || defined(__sun)
/* Assume Linux and Solaris have a working epoll(3) interface. */
#define HAVE_EPOLL
#define WARN_SELECT
#else
/* pselect(2) is a POSIX standard. */
#define HAVE_PSELECT
#define WARN_SELECT
#endif
#endif

/* pollts and ppoll require poll.
 * pselect is wrapped in a pollts/ppoll style interface
 * and as such require poll as well. */
#if defined(HAVE_PSELECT) || defined(HAVE_POLLTS) || defined(HAVE_PPOLL)
#ifndef HAVE_POLL
#define HAVE_POLL
#endif
#if defined(HAVE_POLLTS)
#define POLLTS pollts
#elif defined(HAVE_PPOLL)
#define POLLTS ppoll
#else
#define POLLTS eloop_pollts
#define ELOOP_NEED_POLLTS
#endif
#endif

#include "eloop.h"

#ifndef UNUSED
#define UNUSED(a) (void)((a))
#endif
#ifndef __unused
#ifdef __GNUC__
#define __unused   __attribute__((__unused__))
#else
#define __unused
#endif
#endif

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC	1000L
#define NSEC_PER_MSEC	1000000L
#endif

#if defined(HAVE_KQUEUE)
#include <sys/event.h>
#include <fcntl.h>
#ifdef __NetBSD__
/* udata is void * except on NetBSD.
 * lengths are int except on NetBSD. */
#define UPTR(x)	((intptr_t)(x))
#define LENC(x)	(x)
#else
#define UPTR(x)	(x)
#define LENC(x)	((int)(x))
#endif
#elif defined(HAVE_EPOLL)
#include <sys/epoll.h>
#elif defined(HAVE_POLL)
#if defined(HAVE_PSELECT)
#include <sys/select.h>
#endif
#include <poll.h>
#endif

#ifdef WARN_SELECT
#if defined(HAVE_KQUEUE)
#pragma message("Compiling eloop with kqueue(2) support.")
#elif defined(HAVE_EPOLL)
#pragma message("Compiling eloop with epoll(7) support.")
#elif defined(HAVE_PSELECT)
#pragma message("Compiling eloop with pselect(2) support.")
#elif defined(HAVE_PPOLL)
#pragma message("Compiling eloop with ppoll(2) support.")
#elif defined(HAVE_POLLTS)
#pragma message("Compiling eloop with pollts(2) support.")
#else
#error Unknown select mechanism for eloop
#endif
#endif

/* Our structures require TAILQ macros, which really every libc should
 * ship as they are useful beyond belief.
 * Sadly some libc's don't have sys/queue.h and some that do don't have
 * the TAILQ_FOREACH macro. For those that don't, the application using
 * this implementation will need to ship a working queue.h somewhere.
 * If we don't have sys/queue.h found in config.h, then
 * allow QUEUE_H to override loading queue.h in the current directory. */
#ifndef TAILQ_FOREACH
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#elif defined(QUEUE_H)
#define __QUEUE_HEADER(x) #x
#define _QUEUE_HEADER(x) __QUEUE_HEADER(x)
#include _QUEUE_HEADER(QUEUE_H)
#else
#include "queue.h"
#endif
#endif

struct eloop_event {
	TAILQ_ENTRY(eloop_event) next;
	int fd;
	void (*read_cb)(void *);
	void *read_cb_arg;
	void (*write_cb)(void *);
	void *write_cb_arg;
};

struct eloop_timeout {
	TAILQ_ENTRY(eloop_timeout) next;
	struct timespec when;
	void (*callback)(void *);
	void *arg;
	int queue;
};

struct eloop {
	size_t events_len;
	TAILQ_HEAD (event_head, eloop_event) events;
	struct event_head free_events;
	int events_maxfd;
	struct eloop_event **event_fds;

	TAILQ_HEAD (timeout_head, eloop_timeout) timeouts;
	struct timeout_head free_timeouts;

	void (*timeout0)(void *);
	void *timeout0_arg;
	const int *signals;
	size_t signals_len;
	void (*signal_cb)(int, void *);
	void *signal_cb_ctx;

#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
	int poll_fd;
#elif defined(HAVE_POLL)
	struct pollfd *fds;
	size_t fds_len;
#endif

	int exitnow;
	int exitcode;
};

#ifdef HAVE_REALLOCARRAY
#define	eloop_realloca	reallocarray
#else
/* Handy routing to check for potential overflow.
 * reallocarray(3) and reallocarr(3) are not portable. */
#define SQRT_SIZE_MAX (((size_t)1) << (sizeof(size_t) * CHAR_BIT / 2))
static void *
eloop_realloca(void *ptr, size_t n, size_t size)
{

	if ((n | size) >= SQRT_SIZE_MAX && n > SIZE_MAX / size) {
		errno = EOVERFLOW;
		return NULL;
	}
	return realloc(ptr, n * size);
}
#endif

#ifdef HAVE_POLL
static void
eloop_event_setup_fds(struct eloop *eloop)
{
	struct eloop_event *e;
	size_t i;

	i = 0;
	TAILQ_FOREACH(e, &eloop->events, next) {
		eloop->fds[i].fd = e->fd;
		eloop->fds[i].events = 0;
		if (e->read_cb)
			eloop->fds[i].events |= POLLIN;
		if (e->write_cb)
			eloop->fds[i].events |= POLLOUT;
		eloop->fds[i].revents = 0;
		i++;
	}
}

#ifdef ELOOP_NEED_POLLTS
/* Wrapper around pselect, to imitate the NetBSD pollts call. */
static int
eloop_pollts(struct pollfd * fds, nfds_t nfds,
    const struct timespec *ts, const sigset_t *sigmask)
{
	fd_set read_fds, write_fds;
	nfds_t n;
	int maxfd, r;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	maxfd = 0;
	for (n = 0; n < nfds; n++) {
		if (fds[n].events & POLLIN) {
			FD_SET(fds[n].fd, &read_fds);
			if (fds[n].fd > maxfd)
				maxfd = fds[n].fd;
		}
		if (fds[n].events & POLLOUT) {
			FD_SET(fds[n].fd, &write_fds);
			if (fds[n].fd > maxfd)
				maxfd = fds[n].fd;
		}
	}

	r = pselect(maxfd + 1, &read_fds, &write_fds, NULL, ts, sigmask);
	if (r > 0) {
		for (n = 0; n < nfds; n++) {
			fds[n].revents =
			    FD_ISSET(fds[n].fd, &read_fds) ? POLLIN : 0;
			if (FD_ISSET(fds[n].fd, &write_fds))
				fds[n].revents |= POLLOUT;
		}
	}

	return r;
}
#endif /* pollts */
#else /* !HAVE_POLL */
#define eloop_event_setup_fds(a) {}
#endif /* HAVE_POLL */

int
eloop_event_add_rw(struct eloop *eloop, int fd,
    void (*read_cb)(void *), void *read_cb_arg,
    void (*write_cb)(void *), void *write_cb_arg)
{
	struct eloop_event *e;
#if defined(HAVE_KQUEUE)
	struct kevent ke[2];
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#elif defined(HAVE_POLL)
	struct pollfd *nfds;
#endif

	assert(eloop != NULL);
	assert(read_cb != NULL || write_cb != NULL);
	if (fd == -1) {
		errno = EINVAL;
		return -1;
	}

#ifdef HAVE_EPOLL
	memset(&epe, 0, sizeof(epe));
	epe.data.fd = fd;
	epe.events = EPOLLIN;
	if (write_cb)
		epe.events |= EPOLLOUT;
#endif

	/* We should only have one callback monitoring the fd. */
	if (fd <= eloop->events_maxfd) {
		if ((e = eloop->event_fds[fd]) != NULL) {
			int error;

#if defined(HAVE_KQUEUE)
			EV_SET(&ke[0], (uintptr_t)fd, EVFILT_READ, EV_ADD,
			    0, 0, UPTR(e));
			if (write_cb)
				EV_SET(&ke[1], (uintptr_t)fd, EVFILT_WRITE,
				    EV_ADD, 0, 0, UPTR(e));
			else if (e->write_cb)
				EV_SET(&ke[1], (uintptr_t)fd, EVFILT_WRITE,
				    EV_DELETE, 0, 0, UPTR(e));
			error = kevent(eloop->poll_fd, ke,
			    e->write_cb || write_cb ? 2 : 1, NULL, 0, NULL);
#elif defined(HAVE_EPOLL)
			epe.data.ptr = e;
			error = epoll_ctl(eloop->poll_fd, EPOLL_CTL_MOD,
			    fd, &epe);
#else
			error = 0;
#endif
			if (read_cb) {
				e->read_cb = read_cb;
				e->read_cb_arg = read_cb_arg;
			}
			if (write_cb) {
				e->write_cb = write_cb;
				e->write_cb_arg = write_cb_arg;
			}
			eloop_event_setup_fds(eloop);
			return error;
		}
	} else {
		struct eloop_event **new_fds;
		int maxfd, i;

		/* Reserve ourself and 4 more. */
		maxfd = fd + 4;
		new_fds = eloop_realloca(eloop->event_fds,
		    ((size_t)maxfd + 1), sizeof(*eloop->event_fds));
		if (new_fds == NULL)
			return -1;

		/* set new entries NULL as the fd's may not be contiguous. */
		for (i = maxfd; i > eloop->events_maxfd; i--)
			new_fds[i] = NULL;

		eloop->event_fds = new_fds;
		eloop->events_maxfd = maxfd;
	}

	/* Allocate a new event if no free ones already allocated. */
	if ((e = TAILQ_FIRST(&eloop->free_events))) {
		TAILQ_REMOVE(&eloop->free_events, e, next);
	} else {
		e = malloc(sizeof(*e));
		if (e == NULL)
			goto err;
	}

	/* Ensure we can actually listen to it. */
	eloop->events_len++;
#ifdef HAVE_POLL
	if (eloop->events_len > eloop->fds_len) {
		nfds = eloop_realloca(eloop->fds,
		    (eloop->fds_len + 5), sizeof(*eloop->fds));
		if (nfds == NULL)
			goto err;
		eloop->fds_len += 5;
		eloop->fds = nfds;
	}
#endif

	/* Now populate the structure and add it to the list. */
	e->fd = fd;
	e->read_cb = read_cb;
	e->read_cb_arg = read_cb_arg;
	e->write_cb = write_cb;
	e->write_cb_arg = write_cb_arg;

#if defined(HAVE_KQUEUE)
	if (read_cb != NULL)
		EV_SET(&ke[0], (uintptr_t)fd, EVFILT_READ,
		    EV_ADD, 0, 0, UPTR(e));
	if (write_cb != NULL)
		EV_SET(&ke[1], (uintptr_t)fd, EVFILT_WRITE,
		    EV_ADD, 0, 0, UPTR(e));
	if (kevent(eloop->poll_fd, ke, write_cb ? 2 : 1, NULL, 0, NULL) == -1)
		goto err;
#elif defined(HAVE_EPOLL)
	epe.data.ptr = e;
	if (epoll_ctl(eloop->poll_fd, EPOLL_CTL_ADD, fd, &epe) == -1)
		goto err;
#endif

	TAILQ_INSERT_HEAD(&eloop->events, e, next);
	eloop->event_fds[e->fd] = e;
	eloop_event_setup_fds(eloop);
	return 0;

err:
	if (e) {
		eloop->events_len--;
		TAILQ_INSERT_TAIL(&eloop->free_events, e, next);
	}
	return -1;
}

int
eloop_event_add(struct eloop *eloop, int fd,
    void (*read_cb)(void *), void *read_cb_arg)
{

	return eloop_event_add_rw(eloop, fd, read_cb, read_cb_arg, NULL, NULL);
}

int
eloop_event_add_w(struct eloop *eloop, int fd,
    void (*write_cb)(void *), void *write_cb_arg)
{

	return eloop_event_add_rw(eloop, fd, NULL,NULL, write_cb, write_cb_arg);
}

int
eloop_event_delete_write(struct eloop *eloop, int fd, int write_only)
{
	struct eloop_event *e;
#if defined(HAVE_KQUEUE)
	struct kevent ke[2];
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif

	assert(eloop != NULL);

	if (fd > eloop->events_maxfd ||
	    (e = eloop->event_fds[fd]) == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	if (write_only) {
		if (e->write_cb == NULL)
			return 0;
		if (e->read_cb == NULL)
			goto remove;
		e->write_cb = NULL;
		e->write_cb_arg = NULL;
#if defined(HAVE_KQUEUE)
		EV_SET(&ke[0], (uintptr_t)e->fd,
		    EVFILT_WRITE, EV_DELETE, 0, 0, UPTR(NULL));
		kevent(eloop->poll_fd, ke, 1, NULL, 0, NULL);
#elif defined(HAVE_EPOLL)
		memset(&epe, 0, sizeof(epe));
		epe.data.fd = e->fd;
		epe.data.ptr = e;
		epe.events = EPOLLIN;
		epoll_ctl(eloop->poll_fd, EPOLL_CTL_MOD, fd, &epe);
#endif
		eloop_event_setup_fds(eloop);
		return 1;
	}

remove:
	TAILQ_REMOVE(&eloop->events, e, next);
	eloop->event_fds[e->fd] = NULL;
	TAILQ_INSERT_TAIL(&eloop->free_events, e, next);
	eloop->events_len--;

#if defined(HAVE_KQUEUE)
	EV_SET(&ke[0], (uintptr_t)fd, EVFILT_READ,
	    EV_DELETE, 0, 0, UPTR(NULL));
	if (e->write_cb)
		EV_SET(&ke[1], (uintptr_t)fd,
		    EVFILT_WRITE, EV_DELETE, 0, 0, UPTR(NULL));
	kevent(eloop->poll_fd, ke, e->write_cb ? 2 : 1, NULL, 0, NULL);
#elif defined(HAVE_EPOLL)
	/* NULL event is safe because we
	 * rely on epoll_pwait which as added
	 * after the delete without event was fixed. */
	epoll_ctl(eloop->poll_fd, EPOLL_CTL_DEL, fd, NULL);
#endif

	eloop_event_setup_fds(eloop);
	return 1;
}

int
eloop_q_timeout_add_tv(struct eloop *eloop, int queue,
    const struct timespec *when, void (*callback)(void *), void *arg)
{
	struct timespec now, w;
	struct eloop_timeout *t, *tt = NULL;

	assert(eloop != NULL);
	assert(when != NULL);
	assert(callback != NULL);

	clock_gettime(CLOCK_MONOTONIC, &now);
	timespecadd(&now, when, &w);
	/* Check for time_t overflow. */
	if (timespeccmp(&w, &now, <)) {
		errno = ERANGE;
		return -1;
	}

	/* Remove existing timeout if present. */
	TAILQ_FOREACH(t, &eloop->timeouts, next) {
		if (t->callback == callback && t->arg == arg) {
			TAILQ_REMOVE(&eloop->timeouts, t, next);
			break;
		}
	}

	if (t == NULL) {
		/* No existing, so allocate or grab one from the free pool. */
		if ((t = TAILQ_FIRST(&eloop->free_timeouts))) {
			TAILQ_REMOVE(&eloop->free_timeouts, t, next);
		} else {
			if ((t = malloc(sizeof(*t))) == NULL)
				return -1;
		}
	}

	t->when = w;
	t->callback = callback;
	t->arg = arg;
	t->queue = queue;

	/* The timeout list should be in chronological order,
	 * soonest first. */
	TAILQ_FOREACH(tt, &eloop->timeouts, next) {
		if (timespeccmp(&t->when, &tt->when, <)) {
			TAILQ_INSERT_BEFORE(tt, t, next);
			return 0;
		}
	}
	TAILQ_INSERT_TAIL(&eloop->timeouts, t, next);
	return 0;
}

int
eloop_q_timeout_add_sec(struct eloop *eloop, int queue, time_t when,
    void (*callback)(void *), void *arg)
{
	struct timespec tv;

	tv.tv_sec = when;
	tv.tv_nsec = 0;
	return eloop_q_timeout_add_tv(eloop, queue, &tv, callback, arg);
}

int
eloop_q_timeout_add_msec(struct eloop *eloop, int queue, long when,
    void (*callback)(void *), void *arg)
{
	struct timespec tv;

	tv.tv_sec = when / MSEC_PER_SEC;
	tv.tv_nsec = (when % MSEC_PER_SEC) * NSEC_PER_MSEC;
	return eloop_q_timeout_add_tv(eloop, queue, &tv, callback, arg);
}

#if !defined(HAVE_KQUEUE)
static int
eloop_timeout_add_now(struct eloop *eloop,
    void (*callback)(void *), void *arg)
{

	assert(eloop->timeout0 == NULL);
	eloop->timeout0 = callback;
	eloop->timeout0_arg = arg;
	return 0;
}
#endif

int
eloop_q_timeout_delete(struct eloop *eloop, int queue,
    void (*callback)(void *), void *arg)
{
	struct eloop_timeout *t, *tt;
	int n;

	assert(eloop != NULL);

	n = 0;
	TAILQ_FOREACH_SAFE(t, &eloop->timeouts, next, tt) {
		if ((queue == 0 || t->queue == queue) &&
		    t->arg == arg &&
		    (!callback || t->callback == callback))
		{
			TAILQ_REMOVE(&eloop->timeouts, t, next);
			TAILQ_INSERT_TAIL(&eloop->free_timeouts, t, next);
			n++;
		}
	}
	return n;
}

void
eloop_exit(struct eloop *eloop, int code)
{

	assert(eloop != NULL);

	eloop->exitcode = code;
	eloop->exitnow = 1;
}

#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
static int
eloop_open(struct eloop *eloop)
{

#if defined(HAVE_KQUEUE1)
	return (eloop->poll_fd = kqueue1(O_CLOEXEC));
#elif defined(HAVE_KQUEUE)
	int i;

	if ((eloop->poll_fd = kqueue()) == -1)
		return -1;
	if ((i = fcntl(eloop->poll_fd, F_GETFD, 0)) == -1 ||
	    fcntl(eloop->poll_fd, F_SETFD, i | FD_CLOEXEC) == -1)
	{
		close(eloop->poll_fd);
		eloop->poll_fd = -1;
	}

	return eloop->poll_fd;
#elif defined (HAVE_EPOLL)
	return (eloop->poll_fd = epoll_create1(EPOLL_CLOEXEC));
#else
	return (eloop->poll_fd = -1);
#endif
}
#endif

int
eloop_requeue(struct eloop *eloop)
{
#if defined(HAVE_POLL)

	UNUSED(eloop);
	return 0;
#else /* !HAVE_POLL */
	struct eloop_event *e;
	int error;
#if defined(HAVE_KQUEUE)
	size_t i;
	struct kevent *ke;
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif

	assert(eloop != NULL);

	if (eloop->poll_fd != -1)
		close(eloop->poll_fd);
	if (eloop_open(eloop) == -1)
		return -1;
#if defined (HAVE_KQUEUE)
	i = eloop->signals_len;
	TAILQ_FOREACH(e, &eloop->events, next) {
		i++;
		if (e->write_cb)
			i++;
	}

	if ((ke = malloc(sizeof(*ke) * i)) == NULL)
		return -1;

	for (i = 0; i < eloop->signals_len; i++)
		EV_SET(&ke[i], (uintptr_t)eloop->signals[i],
		    EVFILT_SIGNAL, EV_ADD, 0, 0, UPTR(NULL));

	TAILQ_FOREACH(e, &eloop->events, next) {
		EV_SET(&ke[i], (uintptr_t)e->fd, EVFILT_READ,
		    EV_ADD, 0, 0, UPTR(e));
		i++;
		if (e->write_cb) {
			EV_SET(&ke[i], (uintptr_t)e->fd, EVFILT_WRITE,
			    EV_ADD, 0, 0, UPTR(e));
			i++;
		}
	}

	error =  kevent(eloop->poll_fd, ke, LENC(i), NULL, 0, NULL);
	free(ke);

#elif defined(HAVE_EPOLL)

	error = 0;
	TAILQ_FOREACH(e, &eloop->events, next) {
		memset(&epe, 0, sizeof(epe));
		epe.data.fd = e->fd;
		epe.events = EPOLLIN;
		if (e->write_cb)
			epe.events |= EPOLLOUT;
		epe.data.ptr = e;
		if (epoll_ctl(eloop->poll_fd, EPOLL_CTL_ADD, e->fd, &epe) == -1)
			error = -1;
	}
#endif

	return error;
#endif /* HAVE_POLL */
}

int
eloop_signal_set_cb(struct eloop *eloop,
    const int *signals, size_t signals_len,
    void (*signal_cb)(int, void *), void *signal_cb_ctx)
{

	assert(eloop != NULL);

	eloop->signals = signals;
	eloop->signals_len = signals_len;
	eloop->signal_cb = signal_cb;
	eloop->signal_cb_ctx = signal_cb_ctx;
	return eloop_requeue(eloop);
}

#ifndef HAVE_KQUEUE
struct eloop_siginfo {
	int sig;
	struct eloop *eloop;
};
static struct eloop_siginfo _eloop_siginfo;
static struct eloop *_eloop;

static void
eloop_signal1(void *arg)
{
	struct eloop_siginfo *si = arg;

	si->eloop->signal_cb(si->sig, si->eloop->signal_cb_ctx);
}

static void
eloop_signal3(int sig, __unused siginfo_t *siginfo, __unused void *arg)
{

	/* So that we can operate safely under a signal we instruct
	 * eloop to pass a copy of the siginfo structure to handle_signal1
	 * as the very first thing to do. */
	_eloop_siginfo.eloop = _eloop;
	_eloop_siginfo.sig = sig;
	eloop_timeout_add_now(_eloop_siginfo.eloop,
	    eloop_signal1, &_eloop_siginfo);
}
#endif

int
eloop_signal_mask(struct eloop *eloop, sigset_t *oldset)
{
	sigset_t newset;
	size_t i;
#ifndef HAVE_KQUEUE
	struct sigaction sa;
#endif

	assert(eloop != NULL);

	sigemptyset(&newset);
	for (i = 0; i < eloop->signals_len; i++)
		sigaddset(&newset, eloop->signals[i]);
	if (sigprocmask(SIG_SETMASK, &newset, oldset) == -1)
		return -1;

#ifndef HAVE_KQUEUE
	_eloop = eloop;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = eloop_signal3;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	for (i = 0; i < eloop->signals_len; i++) {
		if (sigaction(eloop->signals[i], &sa, NULL) == -1)
			return -1;
	}
#endif
	return 0;
}

struct eloop *
eloop_new(void)
{
	struct eloop *eloop;
	struct timespec now;

	/* Check we have a working monotonic clock. */
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
		return NULL;

	eloop = calloc(1, sizeof(*eloop));
	if (eloop) {
		TAILQ_INIT(&eloop->events);
		eloop->events_maxfd = -1;
		TAILQ_INIT(&eloop->free_events);
		TAILQ_INIT(&eloop->timeouts);
		TAILQ_INIT(&eloop->free_timeouts);
		eloop->exitcode = EXIT_FAILURE;
#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
		if (eloop_open(eloop) == -1) {
			eloop_free(eloop);
			return NULL;
		}
#endif
	}

	return eloop;
}

void eloop_free(struct eloop *eloop)
{
	struct eloop_event *e;
	struct eloop_timeout *t;

	if (eloop == NULL)
		return;

	free(eloop->event_fds);
	while ((e = TAILQ_FIRST(&eloop->events))) {
		TAILQ_REMOVE(&eloop->events, e, next);
		free(e);
	}
	while ((e = TAILQ_FIRST(&eloop->free_events))) {
		TAILQ_REMOVE(&eloop->free_events, e, next);
		free(e);
	}
	while ((t = TAILQ_FIRST(&eloop->timeouts))) {
		TAILQ_REMOVE(&eloop->timeouts, t, next);
		free(t);
	}
	while ((t = TAILQ_FIRST(&eloop->free_timeouts))) {
		TAILQ_REMOVE(&eloop->free_timeouts, t, next);
		free(t);
	}
#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
	close(eloop->poll_fd);
#elif defined(HAVE_POLL)
	free(eloop->fds);
#endif
	free(eloop);
}

int
eloop_start(struct eloop *eloop, sigset_t *signals)
{
	int n;
	struct eloop_event *e;
	struct eloop_timeout *t;
	struct timespec now, ts, *tsp;
	void (*t0)(void *);
#if defined(HAVE_KQUEUE)
	struct kevent ke;
	UNUSED(signals);
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif
#ifndef HAVE_KQUEUE
	int timeout;
#endif

	assert(eloop != NULL);

	eloop->exitnow = 0;
	for (;;) {
		if (eloop->exitnow)
			break;

		/* Run all timeouts first. */
		if (eloop->timeout0) {
			t0 = eloop->timeout0;
			eloop->timeout0 = NULL;
			t0(eloop->timeout0_arg);
			continue;
		}
		if ((t = TAILQ_FIRST(&eloop->timeouts))) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (timespeccmp(&now, &t->when, >)) {
				TAILQ_REMOVE(&eloop->timeouts, t, next);
				t->callback(t->arg);
				TAILQ_INSERT_TAIL(&eloop->free_timeouts, t, next);
				continue;
			}
			timespecsub(&t->when, &now, &ts);
			tsp = &ts;
		} else
			/* No timeouts, so wait forever. */
			tsp = NULL;

		if (tsp == NULL && eloop->events_len == 0)
			break;

#ifndef HAVE_KQUEUE
		if (tsp == NULL)
			timeout = -1;
		else if (tsp->tv_sec > INT_MAX / 1000 ||
		    (tsp->tv_sec == INT_MAX / 1000 &&
		    (tsp->tv_nsec + 999999) / 1000000 > INT_MAX % 1000000))
			timeout = INT_MAX;
		else
			timeout = (int)(tsp->tv_sec * 1000 +
			    (tsp->tv_nsec + 999999) / 1000000);
#endif

#if defined(HAVE_KQUEUE)
		n = kevent(eloop->poll_fd, NULL, 0, &ke, 1, tsp);
#elif defined(HAVE_EPOLL)
		if (signals)
			n = epoll_pwait(eloop->poll_fd, &epe, 1,
			    timeout, signals);
		else
			n = epoll_wait(eloop->poll_fd, &epe, 1, timeout);
#elif defined(HAVE_POLL)
		if (signals)
			n = POLLTS(eloop->fds, (nfds_t)eloop->events_len,
			    tsp, signals);
		else
			n = poll(eloop->fds, (nfds_t)eloop->events_len,
			    timeout);
#endif
		if (n == -1) {
			if (errno == EINTR)
				continue;
			return -errno;
		}

		/* Process any triggered events.
		 * We go back to the start after calling each callback incase
		 * the current event or next event is removed. */
#if defined(HAVE_KQUEUE)
		if (n) {
			if (ke.filter == EVFILT_SIGNAL) {
				eloop->signal_cb((int)ke.ident,
				    eloop->signal_cb_ctx);
				continue;
			}
			e = (struct eloop_event *)ke.udata;
			if (ke.filter == EVFILT_WRITE) {
				e->write_cb(e->write_cb_arg);
				continue;
			} else if (ke.filter == EVFILT_READ) {
				e->read_cb(e->read_cb_arg);
				continue;
			}
		}
#elif defined(HAVE_EPOLL)
		if (n) {
			e = (struct eloop_event *)epe.data.ptr;
			if (epe.events & EPOLLOUT && e->write_cb != NULL) {
				e->write_cb(e->write_cb_arg);
				continue;
			}
			if (epe.events &
			    (EPOLLIN | EPOLLERR | EPOLLHUP) &&
			    e->read_cb != NULL)
			{
				e->read_cb(e->read_cb_arg);
				continue;
			}
		}
#elif defined(HAVE_POLL)
		if (n > 0) {
			size_t i;

			for (i = 0; i < eloop->events_len; i++) {
				if (eloop->fds[i].revents & POLLOUT) {
					e = eloop->event_fds[eloop->fds[i].fd];
					if (e->write_cb != NULL) {
						e->write_cb(e->write_cb_arg);
						break;
					}
				}
				if (eloop->fds[i].revents) {
					e = eloop->event_fds[eloop->fds[i].fd];
					if (e->read_cb != NULL) {
						e->read_cb(e->read_cb_arg);
						break;
					}
				}
			}
		}
#endif
	}

	return eloop->exitcode;
}
