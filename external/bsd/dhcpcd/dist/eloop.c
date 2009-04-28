/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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
#include <poll.h>
#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

#include "common.h"
#include "eloop.h"

static struct timeval now;

static struct event {
	int fd;
	void (*callback)(void *);
	void *arg;
	struct event *next;
} *events;
static struct event *free_events;

static struct timeout {
	struct timeval when;
	void (*callback)(void *);
	void *arg;
	struct timeout *next;
} *timeouts;
static struct timeout *free_timeouts;

static struct pollfd *fds;
static size_t fds_len;

void
add_event(int fd, void (*callback)(void *), void *arg)
{
	struct event *e, *last = NULL;

	/* We should only have one callback monitoring the fd */
	for (e = events; e; e = e->next) {
		if (e->fd == fd) {
			e->callback = callback;
			e->arg = arg;
			return;
		}
		last = e;
	}

	/* Allocate a new event if no free ones already allocated */
	if (free_events) {
		e = free_events;
		free_events = e->next;
	} else
		e = xmalloc(sizeof(*e));
	e->fd = fd;
	e->callback = callback;
	e->arg = arg;
	e->next = NULL;
	if (last)
		last->next = e;
	else
		events = e;
}

void
delete_event(int fd)
{
	struct event *e, *last = NULL;

	for (e = events; e; e = e->next) {
		if (e->fd == fd) {
			if (last)
				last->next = e->next;
			else
				events = e->next;
			e->next = free_events;
			free_events = e;
			break;
		}
		last = e;
	}
}

void
add_timeout_tv(const struct timeval *when, void (*callback)(void *), void *arg)
{
	struct timeval w;
	struct timeout *t, *tt = NULL;

	get_monotonic(&now);
	timeradd(&now, when, &w);
	/* Check for time_t overflow. */
	if (timercmp(&w, &now, <)) {
		errno = ERANGE;
		return;
	}

	/* Remove existing timeout if present */
	for (t = timeouts; t; t = t->next) {
		if (t->callback == callback && t->arg == arg) {
			if (tt)
				tt->next = t->next;
			else
				timeouts = t->next;
			break;
		}
		tt = t;
	}

	if (!t) {
		/* No existing, so allocate or grab one from the free pool */
		if (free_timeouts) {
			t = free_timeouts;
			free_timeouts = t->next;
		} else
			t = xmalloc(sizeof(*t));
	}

	t->when.tv_sec = w.tv_sec;
	t->when.tv_usec = w.tv_usec;
	t->callback = callback;
	t->arg = arg;

	/* The timeout list should be in chronological order,
	 * soonest first.
	 * This is the easiest algorithm - check the head, then middle
	 * and finally the end. */
	if (!timeouts || timercmp(&t->when, &timeouts->when, <)) {
		t->next = timeouts;
		timeouts = t;
		return;
	} 
	for (tt = timeouts; tt->next; tt = tt->next)
		if (timercmp(&t->when, &tt->next->when, <)) {
			t->next = tt->next;
			tt->next = t;
			return;
		}
	tt->next = t;
	t->next = NULL;
}

void
add_timeout_sec(time_t when, void (*callback)(void *), void *arg)
{
	struct timeval tv;

	tv.tv_sec = when;
	tv.tv_usec = 0;
	add_timeout_tv(&tv, callback, arg);
}

/* This deletes all timeouts for the interface EXCEPT for ones with the
 * callbacks given. Handy for deleting everything apart from the expire
 * timeout. */
void
delete_timeouts(void *arg, void (*callback)(void *), ...)
{
	struct timeout *t, *tt, *last = NULL;
	va_list va;
	void (*f)(void *);

	for (t = timeouts; t && (tt = t->next, 1); t = tt) {
		if (t->arg == arg && t->callback != callback) {
			va_start(va, callback);
			while ((f = va_arg(va, void (*)(void *))))
				if (f == t->callback)
					break;
			va_end(va);
			if (!f) {
				if (last)
					last->next = t->next;
				else
					timeouts = t->next;
				t->next = free_timeouts;
				free_timeouts = t;
				continue;
			}
		}
		last = t;
	}
}

void
delete_timeout(void (*callback)(void *), void *arg)
{
	struct timeout *t, *tt, *last = NULL;

	for (t = timeouts; t && (tt = t->next, 1); t = tt) {
		if (t->arg == arg &&
		    (!callback || t->callback == callback))
		{
			if (last)
				last->next = t->next;
			else
				timeouts = t->next;
			t->next = free_timeouts;
			free_timeouts = t;
			continue;
		}
		last = t;
	}
}

#ifdef DEBUG_MEMORY
/* Define this to free all malloced memory.
 * Normally we don't do this as the OS will do it for us at exit,
 * but it's handy for debugging other leaks in valgrind. */
static void
cleanup(void)
{
	struct event *e;
	struct timeout *t;

	while (events) {
		e = events->next;
		free(events);
		events = e;
	}
	while (free_events) {
		e = free_events->next;
		free(free_events);
		free_events = e;
	}
	while (timeouts) {
		t = timeouts->next;
		free(timeouts);
		timeouts = t;
	}
	while (free_timeouts) {
		t = free_timeouts->next;
		free(free_timeouts);
		free_timeouts = t;
	}
	free(fds);
}
#endif

_noreturn void
start_eloop(void)
{
	int msecs, n;
	nfds_t nfds, i;
	struct event *e;
	struct timeout *t;
	struct timeval tv;

#ifdef DEBUG_MEMORY
	atexit(cleanup);
#endif

	for (;;) {
		/* Run all timeouts first.
		 * When we have one that has not yet occured,
		 * calculate milliseconds until it does for use in poll. */
		if (timeouts) {
			if (timercmp(&now, &timeouts->when, >)) {
				t = timeouts;
				timeouts = timeouts->next;
				t->callback(t->arg);
				t->next = free_timeouts;
				free_timeouts = t;
				continue;
			}
			timersub(&timeouts->when, &now, &tv);
			if (tv.tv_sec > INT_MAX / 1000 ||
			    (tv.tv_sec == INT_MAX / 1000 &&
				(tv.tv_usec + 999) / 1000 > INT_MAX % 1000))
				msecs = INT_MAX;
			else
				msecs = tv.tv_sec * 1000 +
				    (tv.tv_usec + 999) / 1000;
		} else
			/* No timeouts, so wait forever. */
			msecs = -1;

		/* Allocate memory for our pollfds as and when needed.
		 * We don't bother shrinking it. */
		nfds = 0;
		for (e = events; e; e = e->next)
			nfds++;
		if (msecs == -1 && nfds == 0) {
			syslog(LOG_ERR, "nothing to do");
			exit(EXIT_FAILURE);
		}
		if (nfds > fds_len) {
			free(fds);
			/* Allocate 5 more than we need for future use */
			fds_len = nfds + 5;
			fds = xmalloc(sizeof(*fds) * fds_len);
		}
		nfds = 0;
		for (e = events; e; e = e->next) {
			fds[nfds].fd = e->fd;
			fds[nfds].events = POLLIN;
			fds[nfds].revents = 0;
			nfds++;
		}
		n = poll(fds, nfds, msecs);
		if (n == -1) {
			if (errno == EAGAIN || errno == EINTR) {
				get_monotonic(&now);
				continue;
			}
			syslog(LOG_ERR, "poll: %m");
			exit(EXIT_FAILURE);
		}

		/* Get the now time and process any triggered events. */
		get_monotonic(&now);
		if (n == 0)
			continue;
		for (i = 0; i < nfds; i++) {
			if (!(fds[i].revents & (POLLIN | POLLHUP)))
				continue;
			for (e = events; e; e = e->next) {
				if (e->fd == fds[i].fd) {
					e->callback(e->arg);
					break;
				}
			}
		}
	}
}
