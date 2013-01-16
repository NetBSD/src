/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2010 Roy Marples <roy@marples.name>
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
  #define ELOOP_QUEUE 0
#endif

#define add_timeout_tv(a, b, c) add_q_timeout_tv(ELOOP_QUEUE, a, b, c)
#define add_timeout_sec(a, b, c) add_q_timeout_sec(ELOOP_QUEUE, a, b, c)
#define delete_timeout(a, b) delete_q_timeout(ELOOP_QUEUE, a, b)
#define delete_timeouts(a, ...) delete_q_timeouts(ELOOP_QUEUE, a, __VA_ARGS__)

void add_event(int fd, void (*)(void *), void *);
void delete_event(int fd);
void add_q_timeout_sec(int queue, time_t, void (*)(void *), void *);
void add_q_timeout_tv(int queue, const struct timeval *, void (*)(void *),
    void *);
void delete_q_timeout(int, void (*)(void *), void *);
void delete_q_timeouts(int, void *, void (*)(void *), ...);
void eloop_init(void);
void start_eloop(void);

#endif
