#include <sys/cdefs.h>
 __RCSID("$NetBSD: eloop.c,v 1.10 2015/05/02 15:18:37 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
 * All rights reserved

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

#include <sys/time.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcpcd.h"
#include "eloop.h"

#if defined(HAVE_KQUEUE)
#include <sys/event.h>
#include <fcntl.h>
#ifdef __NetBSD__
/* udata is void * except on NetBSD
 * lengths are int except on NetBSD */
#define UPTR(x)	((intptr_t)(x))
#define LENC(x)	(x)
#else
#define UPTR(x)	(x)
#define LENC(x)	((int)(x))
#endif
#define eloop_event_setup_fds(ctx)
#elif defined(HAVE_EPOLL)
#include <sys/epoll.h>
#define eloop_event_setup_fds(ctx)
#else
#include <poll.h>
static void
eloop_event_setup_fds(struct eloop_ctx *ctx)
{
	struct eloop_event *e;
	size_t i;

	i = 0;
	TAILQ_FOREACH(e, &ctx->events, next) {
		ctx->fds[i].fd = e->fd;
		ctx->fds[i].events = 0;
		if (e->read_cb)
			ctx->fds[i].events |= POLLIN;
		if (e->write_cb)
			ctx->fds[i].events |= POLLOUT;
		ctx->fds[i].revents = 0;
		e->pollfd = &ctx->fds[i];
		i++;
	}
}
#endif

int
eloop_event_add(struct eloop_ctx *ctx, int fd,
    void (*read_cb)(void *), void *read_cb_arg,
    void (*write_cb)(void *), void *write_cb_arg)
{
	struct eloop_event *e;
#if defined(HAVE_KQUEUE)
	struct kevent ke[2];
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#else
	struct pollfd *nfds;
#endif

#ifdef HAVE_EPOLL
	memset(&epe, 0, sizeof(epe));
	epe.data.fd = fd;
	epe.events = EPOLLIN;
	if (write_cb)
		epe.events |= EPOLLOUT;
#endif

	/* We should only have one callback monitoring the fd */
	TAILQ_FOREACH(e, &ctx->events, next) {
		if (e->fd == fd) {
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
			error = kevent(ctx->poll_fd, ke,
			    e->write_cb || write_cb ? 2 : 1, NULL, 0, NULL);
#elif defined(HAVE_EPOLL)
			epe.data.ptr = e;
			error = epoll_ctl(ctx->poll_fd, EPOLL_CTL_MOD,
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
			eloop_event_setup_fds(ctx);
			return error;
		}
	}

	/* Allocate a new event if no free ones already allocated */
	if ((e = TAILQ_FIRST(&ctx->free_events))) {
		TAILQ_REMOVE(&ctx->free_events, e, next);
	} else {
		e = malloc(sizeof(*e));
		if (e == NULL)
			goto err;
	}

	/* Ensure we can actually listen to it */
	ctx->events_len++;
#if !defined(HAVE_KQUEUE) && !defined(HAVE_EPOLL)
	if (ctx->events_len > ctx->fds_len) {
		nfds = realloc(ctx->fds, sizeof(*ctx->fds) * (ctx->fds_len+5));
		if (nfds == NULL)
			goto err;
		ctx->fds_len += 5;
		ctx->fds = nfds;
	}
#endif

	/* Now populate the structure and add it to the list */
	e->fd = fd;
	e->read_cb = read_cb;
	e->read_cb_arg = read_cb_arg;
	e->write_cb = write_cb;
	e->write_cb_arg = write_cb_arg;

#if defined(HAVE_KQUEUE)
	EV_SET(&ke[0], (uintptr_t)fd, EVFILT_READ, EV_ADD, 0, 0, UPTR(e));
	if (write_cb)
		EV_SET(&ke[1], (uintptr_t)fd, EVFILT_WRITE,
		    EV_ADD, 0, 0, UPTR(e));
	if (kevent(ctx->poll_fd, ke, write_cb ? 2 : 1, NULL, 0, NULL) == -1)
		goto err;
#elif defined(HAVE_EPOLL)
	epe.data.ptr = e;
	if (epoll_ctl(ctx->poll_fd, EPOLL_CTL_ADD, fd, &epe) == -1)
		goto err;
#endif

	/* The order of events should not matter.
	 * However, some PPP servers love to close the link right after
	 * sending their final message. So to ensure dhcpcd processes this
	 * message (which is likely to be that the DHCP addresses are wrong)
	 * we insert new events at the queue head as the link fd will be
	 * the first event added. */
	TAILQ_INSERT_HEAD(&ctx->events, e, next);
	eloop_event_setup_fds(ctx);
	return 0;

err:
	logger(ctx->ctx, LOG_ERR, "%s: %m", __func__);
	if (e) {
		ctx->events_len--;
		TAILQ_INSERT_TAIL(&ctx->free_events, e, next);
	}
	return -1;
}

void
eloop_event_delete(struct eloop_ctx *ctx, int fd, int write_only)
{
	struct eloop_event *e;
#if defined(HAVE_KQUEUE)
	struct kevent ke[2];
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif

	TAILQ_FOREACH(e, &ctx->events, next) {
		if (e->fd == fd) {
			if (write_only) {
				if (e->write_cb) {
					e->write_cb = NULL;
					e->write_cb_arg = NULL;
#if defined(HAVE_KQUEUE)
					EV_SET(&ke[0], (uintptr_t)fd,
					    EVFILT_WRITE, EV_DELETE,
					    0, 0, UPTR(NULL));
					kevent(ctx->poll_fd, ke, 1, NULL, 0,
					    NULL);
#elif defined(HAVE_EPOLL)
					memset(&epe, 0, sizeof(epe));
					epe.data.fd = e->fd;
					epe.data.ptr = e;
					epe.events = EPOLLIN;
					epoll_ctl(ctx->poll_fd, EPOLL_CTL_MOD,
					    fd, &epe);
#endif
				}

			} else {
				TAILQ_REMOVE(&ctx->events, e, next);
#if defined(HAVE_KQUEUE)
				EV_SET(&ke[0], (uintptr_t)fd, EVFILT_READ,
				    EV_DELETE, 0, 0, UPTR(NULL));
				if (e->write_cb)
					EV_SET(&ke[1], (uintptr_t)fd,
					    EVFILT_WRITE, EV_DELETE,
					    0, 0, UPTR(NULL));
				kevent(ctx->poll_fd, ke, e->write_cb ? 2 : 1,
				    NULL, 0, NULL);
#elif defined(HAVE_EPOLL)
				/* NULL event is safe because we
				 * rely on epoll_pwait which as added
				 * after the delete without event was fixed. */
				epoll_ctl(ctx->poll_fd, EPOLL_CTL_DEL,
				    fd, NULL);
#endif
				TAILQ_INSERT_TAIL(&ctx->free_events, e, next);
				ctx->events_len--;
			}
			eloop_event_setup_fds(ctx);
			break;
		}
	}
}

int
eloop_q_timeout_add_tv(struct eloop_ctx *ctx, int queue,
    const struct timespec *when, void (*callback)(void *), void *arg)
{
	struct timespec now, w;
	struct eloop_timeout *t, *tt = NULL;

	get_monotonic(&now);
	timespecadd(&now, when, &w);
	/* Check for time_t overflow. */
	if (timespeccmp(&w, &now, <)) {
		errno = ERANGE;
		return -1;
	}

	/* Remove existing timeout if present */
	TAILQ_FOREACH(t, &ctx->timeouts, next) {
		if (t->callback == callback && t->arg == arg) {
			TAILQ_REMOVE(&ctx->timeouts, t, next);
			break;
		}
	}

	if (t == NULL) {
		/* No existing, so allocate or grab one from the free pool */
		if ((t = TAILQ_FIRST(&ctx->free_timeouts))) {
			TAILQ_REMOVE(&ctx->free_timeouts, t, next);
		} else {
			t = malloc(sizeof(*t));
			if (t == NULL) {
				logger(ctx->ctx, LOG_ERR, "%s: %m", __func__);
				return -1;
			}
		}
	}

	t->when = w;
	t->callback = callback;
	t->arg = arg;
	t->queue = queue;

	/* The timeout list should be in chronological order,
	 * soonest first. */
	TAILQ_FOREACH(tt, &ctx->timeouts, next) {
		if (timespeccmp(&t->when, &tt->when, <)) {
			TAILQ_INSERT_BEFORE(tt, t, next);
			return 0;
		}
	}
	TAILQ_INSERT_TAIL(&ctx->timeouts, t, next);
	return 0;
}

int
eloop_q_timeout_add_sec(struct eloop_ctx *ctx, int queue, time_t when,
    void (*callback)(void *), void *arg)
{
	struct timespec tv;

	tv.tv_sec = when;
	tv.tv_nsec = 0;
	return eloop_q_timeout_add_tv(ctx, queue, &tv, callback, arg);
}

#if !defined(HAVE_KQUEUE)
int
eloop_timeout_add_now(struct eloop_ctx *ctx,
    void (*callback)(void *), void *arg)
{

	if (ctx->timeout0 != NULL) {
		logger(ctx->ctx, LOG_WARNING,
		    "%s: timeout0 already set", __func__);
		return eloop_q_timeout_add_sec(ctx, 0, 0, callback, arg);
	}

	ctx->timeout0 = callback;
	ctx->timeout0_arg = arg;
	return 0;
}
#endif

void
eloop_q_timeout_delete(struct eloop_ctx *ctx, int queue,
    void (*callback)(void *), void *arg)
{
	struct eloop_timeout *t, *tt;

	TAILQ_FOREACH_SAFE(t, &ctx->timeouts, next, tt) {
		if ((queue == 0 || t->queue == queue) &&
		    t->arg == arg &&
		    (!callback || t->callback == callback))
		{
			TAILQ_REMOVE(&ctx->timeouts, t, next);
			TAILQ_INSERT_TAIL(&ctx->free_timeouts, t, next);
		}
	}
}

void
eloop_exit(struct eloop_ctx *ctx, int code)
{

	ctx->exitcode = code;
	ctx->exitnow = 1;
}

#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
static int
eloop_open(struct eloop_ctx *ctx)
{
#if defined(HAVE_KQUEUE1)
	return (ctx->poll_fd = kqueue1(O_CLOEXEC));
#elif defined(HAVE_KQUEUE)
	int i;

	if ((ctx->poll_fd = kqueue()) == -1)
		return -1;
	if ((i = fcntl(ctx->poll_fd, F_GETFD, 0)) == -1 ||
	    fcntl(ctx->poll_fd, F_SETFD, i | FD_CLOEXEC) == -1)
	{
		close(ctx->poll_fd);
		ctx->poll_fd = -1;
		return -1;
	}

	return ctx->poll_fd;
#elif defined (HAVE_EPOLL)
	return (ctx->poll_fd = epoll_create1(EPOLL_CLOEXEC));
#endif
}

int
eloop_requeue(struct eloop_ctx *ctx)
{
	struct eloop_event *e;
	int error;
#if defined(HAVE_KQUEUE)
	size_t i;
	struct kevent *ke;
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif

	if (ctx->poll_fd != -1)
		close(ctx->poll_fd);
	if (eloop_open(ctx) == -1)
		return -1;
#if defined (HAVE_KQUEUE)
	i = 0;
	while (dhcpcd_handlesigs[i])
		i++;
	TAILQ_FOREACH(e, &ctx->events, next) {
		i++;
		if (e->write_cb)
			i++;
	}

	if ((ke = malloc(sizeof(*ke) * i)) == NULL)
		return -1;

	for (i = 0; dhcpcd_handlesigs[i]; i++)
		EV_SET(&ke[i], (uintptr_t)dhcpcd_handlesigs[i],
		    EVFILT_SIGNAL, EV_ADD, 0, 0, UPTR(NULL));

	TAILQ_FOREACH(e, &ctx->events, next) {
		EV_SET(&ke[i], (uintptr_t)e->fd, EVFILT_READ,
		    EV_ADD, 0, 0, UPTR(e));
		i++;
		if (e->write_cb) {
			EV_SET(&ke[i], (uintptr_t)e->fd, EVFILT_WRITE,
			    EV_ADD, 0, 0, UPTR(e));
			i++;
		}
	}

	error =  kevent(ctx->poll_fd, ke, LENC(i), NULL, 0, NULL);
	free(ke);

#elif defined(HAVE_EPOLL)

	error = 0;
	TAILQ_FOREACH(e, &ctx->events, next) {
		memset(&epe, 0, sizeof(epe));
		epe.data.fd = e->fd;
		epe.events = EPOLLIN;
		if (e->write_cb)
			epe.events |= EPOLLOUT;
		epe.data.ptr = e;
		if (epoll_ctl(ctx->poll_fd, EPOLL_CTL_ADD, e->fd, &epe) == -1)
			error = -1;
	}
#endif

	return error;
}
#endif

struct eloop_ctx *
eloop_init(struct dhcpcd_ctx *dctx)
{
	struct eloop_ctx *ctx;
	struct timespec now;

	/* Check we have a working monotonic clock. */
	if (get_monotonic(&now) == -1)
		return NULL;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx) {
		ctx->ctx = dctx;
		TAILQ_INIT(&ctx->events);
		TAILQ_INIT(&ctx->free_events);
		TAILQ_INIT(&ctx->timeouts);
		TAILQ_INIT(&ctx->free_timeouts);
		ctx->exitcode = EXIT_FAILURE;
#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
		ctx->poll_fd = -1;
#endif
		if (eloop_requeue(ctx) == -1) {
			free(ctx);
			return NULL;
		}
	}

	return ctx;
}

void eloop_free(struct eloop_ctx *ctx)
{
	struct eloop_event *e;
	struct eloop_timeout *t;

	if (ctx == NULL)
		return;

	while ((e = TAILQ_FIRST(&ctx->events))) {
		TAILQ_REMOVE(&ctx->events, e, next);
		free(e);
	}
	while ((e = TAILQ_FIRST(&ctx->free_events))) {
		TAILQ_REMOVE(&ctx->free_events, e, next);
		free(e);
	}
	while ((t = TAILQ_FIRST(&ctx->timeouts))) {
		TAILQ_REMOVE(&ctx->timeouts, t, next);
		free(t);
	}
	while ((t = TAILQ_FIRST(&ctx->free_timeouts))) {
		TAILQ_REMOVE(&ctx->free_timeouts, t, next);
		free(t);
	}
#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
	close(ctx->poll_fd);
#else
	free(ctx->fds);
#endif
	free(ctx);
}

int
eloop_start(struct eloop_ctx *ctx)
{
	int n;
	struct eloop_event *e;
	struct eloop_timeout *t;
	struct timespec now, ts, *tsp;
	void (*t0)(void *);
#if defined(HAVE_EPOLL) || !defined(USE_SIGNALS)
	int timeout;
#endif
#if defined(HAVE_KQUEUE)
	struct kevent ke;
#elif defined(HAVE_EPOLL)
	struct epoll_event epe;
#endif

	for (;;) {
		if (ctx->exitnow)
			break;

		/* Run all timeouts first */
		if (ctx->timeout0) {
			t0 = ctx->timeout0;
			ctx->timeout0 = NULL;
			t0(ctx->timeout0_arg);
			continue;
		}
		if ((t = TAILQ_FIRST(&ctx->timeouts))) {
			get_monotonic(&now);
			if (timespeccmp(&now, &t->when, >)) {
				TAILQ_REMOVE(&ctx->timeouts, t, next);
				t->callback(t->arg);
				TAILQ_INSERT_TAIL(&ctx->free_timeouts, t, next);
				continue;
			}
			timespecsub(&t->when, &now, &ts);
			tsp = &ts;
		} else
			/* No timeouts, so wait forever */
			tsp = NULL;

		if (tsp == NULL && ctx->events_len == 0) {
			logger(ctx->ctx, LOG_ERR, "nothing to do");
			break;
		}

#if defined(HAVE_EPOLL) || !defined(USE_SIGNALS)
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
		n = kevent(ctx->poll_fd, NULL, 0, &ke, 1, tsp);
#elif defined(HAVE_EPOLL)
#ifdef USE_SIGNALS
		n = epoll_pwait(ctx->poll_fd, &epe, 1, timeout,
		    &ctx->ctx->sigset);
#else
		n = epoll_wait(ctx->poll_fd, &epe, 1, timeout);
#endif
#else
#ifdef USE_SIGNALS
		n = pollts(ctx->fds, (nfds_t)ctx->events_len, tsp,
		    &ctx->ctx->sigset);
#else
		n = poll(ctx->fds, (nfds_t)ctx->events_len, timeout);
#endif
#endif
		if (n == -1) {
			if (errno == EINTR)
				continue;
			logger(ctx->ctx, LOG_ERR, "poll: %m");
			break;
		}

		/* Process any triggered events.
		 * We go back to the start after calling each callback incase
		 * the current event or next event is removed. */
#if defined(HAVE_KQUEUE)
		if (n) {
			if (ke.filter == EVFILT_SIGNAL) {
				struct dhcpcd_siginfo si;

				si.signo = (int)ke.ident;
				dhcpcd_handle_signal(&si);
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
			if (epe.events & EPOLLOUT && e->write_cb) {
				e->write_cb(e->write_cb_arg);
				continue;
			}
			if (epe.events &
			    (EPOLLIN | EPOLLERR | EPOLLHUP))
			{
				e->read_cb(e->read_cb_arg);
				continue;
			}
		}
#else
		if (n > 0) {
			TAILQ_FOREACH(e, &ctx->events, next) {
				if (e->pollfd->revents & POLLOUT &&
				    e->write_cb)
				{
					e->write_cb(e->write_cb_arg);
					break;
				}
				if (e->pollfd->revents) {
					e->read_cb(e->read_cb_arg);
					break;
				}
			}
		}
#endif
	}

	return ctx->exitcode;
}
