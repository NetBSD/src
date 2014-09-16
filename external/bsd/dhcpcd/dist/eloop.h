/* $NetBSD: eloop.h,v 1.1.1.10 2014/09/16 22:23:21 roy Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

#ifndef ELOOP_H
#define ELOOP_H

#include <time.h>

#ifndef ELOOP_QUEUE
  #define ELOOP_QUEUE 1
#endif

/* EXIT_FAILURE is a non zero value and EXIT_SUCCESS is zero.
 * To add a CONTINUE definition, simply do the opposite of EXIT_FAILURE. */
#define ELOOP_CONTINUE	-EXIT_FAILURE

struct eloop_event {
	TAILQ_ENTRY(eloop_event) next;
	int fd;
	void (*read_cb)(void *);
	void *read_cb_arg;
	void (*write_cb)(void *);
	void *write_cb_arg;
	struct pollfd *pollfd;
};

struct eloop_timeout {
	TAILQ_ENTRY(eloop_timeout) next;
	struct timeval when;
	void (*callback)(void *);
	void *arg;
	int queue;
};

struct eloop_ctx {
	size_t events_len;
	TAILQ_HEAD (event_head, eloop_event) events;
	struct event_head free_events;

	TAILQ_HEAD (timeout_head, eloop_timeout) timeouts;
	struct timeout_head free_timeouts;

	void (*timeout0)(void *);
	void *timeout0_arg;

	struct pollfd *fds;
	size_t fds_len;

	int exitnow;
	int exitcode;
};

#define eloop_timeout_add_tv(a, b, c, d) \
    eloop_q_timeout_add_tv(a, ELOOP_QUEUE, b, c, d)
#define eloop_timeout_add_sec(a, b, c, d) \
    eloop_q_timeout_add_sec(a, ELOOP_QUEUE, b, c, d)
#define eloop_timeout_delete(a, b, c) \
    eloop_q_timeout_delete(a, ELOOP_QUEUE, b, c)

int eloop_event_add(struct eloop_ctx *, int,
    void (*)(void *), void *,
    void (*)(void *), void *);
void eloop_event_delete(struct eloop_ctx *, int, int);
int eloop_q_timeout_add_sec(struct eloop_ctx *, int queue,
    time_t, void (*)(void *), void *);
int eloop_q_timeout_add_tv(struct eloop_ctx *, int queue,
    const struct timeval *, void (*)(void *), void *);
int eloop_timeout_add_now(struct eloop_ctx *, void (*)(void *), void *);
void eloop_q_timeout_delete(struct eloop_ctx *, int, void (*)(void *), void *);
struct eloop_ctx * eloop_init(void);
void eloop_free(struct eloop_ctx *);
void eloop_exit(struct eloop_ctx *, int);
int eloop_start(struct dhcpcd_ctx *);

#endif
