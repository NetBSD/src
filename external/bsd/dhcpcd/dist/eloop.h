/* $NetBSD: eloop.h,v 1.1.1.5.10.2 2013/06/23 06:26:31 tls Exp $ */

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

#ifndef ELOOP_H
#define ELOOP_H

#include <time.h>

#ifndef ELOOP_QUEUE
  #define ELOOP_QUEUE 1
#endif

#define eloop_timeout_add_tv(a, b, c) \
    eloop_q_timeout_add_tv(ELOOP_QUEUE, a, b, c)
#define eloop_timeout_add_sec(a, b, c) \
    eloop_q_timeout_add_sec(ELOOP_QUEUE, a, b, c)
#define eloop_timeout_delete(a, b) \
    eloop_q_timeout_delete(ELOOP_QUEUE, a, b)
#define eloop_timeouts_delete(a, ...) \
    eloop_q_timeouts_delete(ELOOP_QUEUE, a, __VA_ARGS__)

int eloop_event_add(int fd, void (*)(void *), void *);
void eloop_event_delete(int fd);
int eloop_q_timeout_add_sec(int queue, time_t, void (*)(void *), void *);
int eloop_q_timeout_add_tv(int queue, const struct timeval *, void (*)(void *),
    void *);
int eloop_timeout_add_now(void (*)(void *), void *);
void eloop_q_timeout_delete(int, void (*)(void *), void *);
void eloop_q_timeouts_delete(int, void *, void (*)(void *), ...);
void eloop_init(void);
void eloop_start(const sigset_t *);

#endif
