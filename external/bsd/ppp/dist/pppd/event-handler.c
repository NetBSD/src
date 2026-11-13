/*
 * event-handler.c - generic select() based event handler.  Should be system
 * independent.
 *
 * Copyright (c) 1994-2025 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Derived from sys-linux.c and sys-solaris.c by Jaco Kroon <jaco@uls.co.za>.
 */
#include <limits.h>
#include <stddef.h>
#include <errno.h>
#include <sys/select.h>

#include "pppd.h"
#include "pppd-private.h"

struct event_handler {
    struct event_handler* next;
    int fd;
    event_cb cb;
    void* ctx;
};

static fd_set in_fds;		/* set of fds that wait_input waits for */
static int max_in_fd;		/* highest fd set in in_fds */
static struct event_handler* handlers;
static int called_remove;

/********************************************************************
 *
 * wait_input - wait until there is data available,
 * for the length of time specified by *timo (indefinite
 * if timo is NULL).
 */

void wait_input(struct timeval *timo)
{
    fd_set ready, exc;
    int n;
    struct event_handler* h = handlers, *nh;

    called_remove = 0;
    ready = in_fds;
    exc = in_fds;
    n = select(max_in_fd + 1, &ready, NULL, &exc, timo);
    if (n < 0 && errno != EINTR)
	fatal("select: %m");

    while (h) {
	nh = h->next;
	if (FD_ISSET(h->fd, &ready)) {
	    FD_CLR(h->fd, &ready); /* clear so that if we need to re-iterate we won't call again */
	    h->cb(h->fd, h->ctx);

	    if (called_remove) {
		nh = handlers;
		called_remove = 0;
	    }
	}
	h = nh;
    }
}

/*
 * add_fd - add an fd to the set that wait_input waits for.
 */
void add_fd(int fd)
{
    if (fd >= FD_SETSIZE)
	fatal("internal error: file descriptor too large (%d)", fd);
    FD_SET(fd, &in_fds);
    if (fd > max_in_fd)
	max_in_fd = fd;
}

void add_fd_callback(int fd, event_cb cb, void* ctx)
{
    struct event_handler *n = malloc(sizeof(*n));
    n->next = handlers;
    n->fd = fd;
    n->cb = cb;
    n->ctx = ctx;
    handlers = n;
    add_fd(fd);
}

/*
 * remove_fd - remove an fd from the set that wait_input waits for.
 */
void remove_fd(int fd)
{
    struct event_handler** h, *t;
    FD_CLR(fd, &in_fds);

    for (h = &handlers; *h; h = &(*h)->next) {
	if (fd == (*h)->fd) {
	    called_remove = 1;
	    t = (*h)->next;
	    free(*h);
	    *h = t;
	    return;
	}
    }
}

void event_handler_init()
{
    FD_ZERO(&in_fds);
    max_in_fd = 0;
}
