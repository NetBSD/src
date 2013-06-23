#include <sys/cdefs.h>
 __RCSID("$NetBSD: eloop.c,v 1.1.1.5.10.2 2013/06/23 06:26:31 tls Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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

/* Needed for ppoll(2) */
#define _GNU_SOURCE

#include <sys/queue.h>
#include <sys/time.h>

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

#include "common.h"
#include "dhcpcd.h"
#include "eloop.h"

static struct timeval now;

struct event {
	TAILQ_ENTRY(event) next;
	int fd;
	void (*callback)(void *);
	void *arg;
	struct pollfd *pollfd;
};
static size_t events_len;
static TAILQ_HEAD (event_head, event) events = TAILQ_HEAD_INITIALIZER(events);
static struct event_head free_events = TAILQ_HEAD_INITIALIZER(free_events);

struct timeout {
	TAILQ_ENTRY(timeout) next;
	struct timeval when;
	void (*callback)(void *);
	void *arg;
	int queue;
};
static TAILQ_HEAD (timeout_head, timeout) timeouts
    = TAILQ_HEAD_INITIALIZER(timeouts);
static struct timeout_head free_timeouts
    = TAILQ_HEAD_INITIALIZER(free_timeouts);

static void (*volatile timeout0)(void *);
static void *volatile timeout0_arg;

static struct pollfd *fds;
static size_t fds_len;

static void
eloop_event_setup_fds(void)
{
	struct event *e;
	size_t i;

	i = 0;
	TAILQ_FOREACH(e, &events, next) {
		fds[i].fd = e->fd;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
		e->pollfd = &fds[i];
		i++;
	}
}

int
eloop_event_add(int fd, void (*callback)(void *), void *arg)
{
	struct event *e;

	/* We should only have one callback monitoring the fd */
	TAILQ_FOREACH(e, &events, next) {
		if (e->fd == fd) {
			e->callback = callback;
			e->arg = arg;
			return 0;
		}
	}

	/* Allocate a new event if no free ones already allocated */
	if ((e = TAILQ_FIRST(&free_events))) {
		TAILQ_REMOVE(&free_events, e, next);
	} else {
		e = malloc(sizeof(*e));
		if (e == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
	}

	/* Ensure we can actually listen to it */
	events_len++;
	if (events_len > fds_len) {
		fds_len += 5;
		free(fds);
		fds = malloc(sizeof(*fds) * fds_len);
		if (fds == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			free(e);
			return -1;
		}
	}

	/* Now populate the structure and add it to the list */
	e->fd = fd;
	e->callback = callback;
	e->arg = arg;
	/* The order of events should not matter.
	 * However, some PPP servers love to close the link right after
	 * sending their final message. So to ensure dhcpcd processes this
	 * message (which is likely to be that the DHCP addresses are wrong)
	 * we insert new events at the queue head as the link fd will be
	 * the first event added. */
	TAILQ_INSERT_HEAD(&events, e, next);
	eloop_event_setup_fds();
	return 0;
}

void
eloop_event_delete(int fd)
{
	struct event *e;

	TAILQ_FOREACH(e, &events, next) {
		if (e->fd == fd) {
			TAILQ_REMOVE(&events, e, next);
			TAILQ_INSERT_TAIL(&free_events, e, next);
			events_len--;
			eloop_event_setup_fds();
			break;
		}
	}
}

int
eloop_q_timeout_add_tv(int queue,
    const struct timeval *when, void (*callback)(void *), void *arg)
{
	struct timeval w;
	struct timeout *t, *tt = NULL;

	get_monotonic(&now);
	timeradd(&now, when, &w);
	/* Check for time_t overflow. */
	if (timercmp(&w, &now, <)) {
		errno = ERANGE;
		return -1;
	}

	/* Remove existing timeout if present */
	TAILQ_FOREACH(t, &timeouts, next) {
		if (t->callback == callback && t->arg == arg) {
			TAILQ_REMOVE(&timeouts, t, next);
			break;
		}
	}

	if (t == NULL) {
		/* No existing, so allocate or grab one from the free pool */
		if ((t = TAILQ_FIRST(&free_timeouts))) {
			TAILQ_REMOVE(&free_timeouts, t, next);
		} else {
			t = malloc(sizeof(*t));
			if (t == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
		}
	}

	t->when.tv_sec = w.tv_sec;
	t->when.tv_usec = w.tv_usec;
	t->callback = callback;
	t->arg = arg;
	t->queue = queue;

	/* The timeout list should be in chronological order,
	 * soonest first. */
	TAILQ_FOREACH(tt, &timeouts, next) {
		if (timercmp(&t->when, &tt->when, <)) {
			TAILQ_INSERT_BEFORE(tt, t, next);
			return 0;
		}
	}
	TAILQ_INSERT_TAIL(&timeouts, t, next);
	return 0;
}

int
eloop_q_timeout_add_sec(int queue, time_t when,
    void (*callback)(void *), void *arg)
{
	struct timeval tv;

	tv.tv_sec = when;
	tv.tv_usec = 0;
	return eloop_q_timeout_add_tv(queue, &tv, callback, arg);
}

int
eloop_timeout_add_now(void (*callback)(void *), void *arg)
{

	if (timeout0 != NULL) {
		syslog(LOG_WARNING, "%s: timeout0 already set", __func__);
		return eloop_q_timeout_add_sec(0, 0, callback, arg);
	}

	timeout0 = callback;
	timeout0_arg = arg;
	return 0;
}

/* This deletes all timeouts for the interface EXCEPT for ones with the
 * callbacks given. Handy for deleting everything apart from the expire
 * timeout. */
static void
eloop_q_timeouts_delete_v(int queue, void *arg,
    void (*callback)(void *), va_list v)
{
	struct timeout *t, *tt;
	va_list va;
	void (*f)(void *);

	TAILQ_FOREACH_SAFE(t, &timeouts, next, tt) {
		if ((queue == 0 || t->queue == queue) && t->arg == arg &&
		    t->callback != callback)
		{
			va_copy(va, v);
			while ((f = va_arg(va, void (*)(void *)))) {
				if (f == t->callback)
					break;
			}
			va_end(va);
			if (f == NULL) {
				TAILQ_REMOVE(&timeouts, t, next);
				TAILQ_INSERT_TAIL(&free_timeouts, t, next);
			}
		}
	}
}

void
eloop_q_timeouts_delete(int queue, void *arg, void (*callback)(void *), ...)
{
	va_list va;

	va_start(va, callback);
	eloop_q_timeouts_delete_v(queue, arg, callback, va);
	va_end(va);
}

void
eloop_q_timeout_delete(int queue, void (*callback)(void *), void *arg)
{
	struct timeout *t, *tt;

	TAILQ_FOREACH_SAFE(t, &timeouts, next, tt) {
		if (t->queue == queue && t->arg == arg &&
		    (!callback || t->callback == callback))
		{
			TAILQ_REMOVE(&timeouts, t, next);
			TAILQ_INSERT_TAIL(&free_timeouts, t, next);
		}
	}
}

#ifdef DEBUG_MEMORY
/* Define this to free all malloced memory.
 * Normally we don't do this as the OS will do it for us at exit,
 * but it's handy for debugging other leaks in valgrind. */
static void
eloop_cleanup(void)
{
	struct event *e;
	struct timeout *t;

	while ((e = TAILQ_FIRST(&events))) {
		TAILQ_REMOVE(&events, e, next);
		free(e);
	}
	while ((e = TAILQ_FIRST(&free_events))) {
		TAILQ_REMOVE(&free_events, e, next);
		free(e);
	}
	while ((t = TAILQ_FIRST(&timeouts))) {
		TAILQ_REMOVE(&timeouts, t, next);
		free(t);
	}
	while ((t = TAILQ_FIRST(&free_timeouts))) {
		TAILQ_REMOVE(&free_timeouts, t, next);
		free(t);
	}
	free(fds);
}

void
eloop_init(void)
{

	atexit(eloop_cleanup);
}
#endif

__dead void
eloop_start(const sigset_t *sigmask)
{
	int n;
	struct event *e;
	struct timeout *t;
	struct timeval tv;
	struct timespec ts, *tsp;
	void (*t0)(void *);

	for (;;) {
		/* Run all timeouts first */
		if (timeout0) {
			t0 = timeout0;
			timeout0 = NULL;
			t0(timeout0_arg);
			continue;
		}
		if ((t = TAILQ_FIRST(&timeouts))) {
			get_monotonic(&now);
			if (timercmp(&now, &t->when, >)) {
				TAILQ_REMOVE(&timeouts, t, next);
				t->callback(t->arg);
				TAILQ_INSERT_TAIL(&free_timeouts, t, next);
				continue;
			}
			timersub(&t->when, &now, &tv);
			TIMEVAL_TO_TIMESPEC(&tv, &ts);
			tsp = &ts;
		} else
			/* No timeouts, so wait forever */
			tsp = NULL;

		if (tsp == NULL && events_len == 0) {
			syslog(LOG_ERR, "nothing to do");
			exit(EXIT_FAILURE);
		}

		n = pollts(fds, events_len, tsp, sigmask);
		if (n == -1) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			syslog(LOG_ERR, "poll: %m");
			exit(EXIT_FAILURE);
		}

		/* Process any triggered events. */
		if (n > 0) {
			TAILQ_FOREACH(e, &events, next) {
				if (e->pollfd->revents & (POLLIN | POLLHUP)) {
					e->callback(e->arg);
					/* We need to break here as the
					 * callback could destroy the next
					 * fd to process. */
					break;
				}
			}
		}
	}
}
