/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

/* Some systems don't define timespec macros */
#ifndef timespecclear
#define timespecclear(tsp)      (tsp)->tv_sec = (time_t)((tsp)->tv_nsec = 0L)
#define timespecisset(tsp)      ((tsp)->tv_sec || (tsp)->tv_nsec)
#define timespeccmp(tsp, usp, cmp)                                      \
        (((tsp)->tv_sec == (usp)->tv_sec) ?                             \
            ((tsp)->tv_nsec cmp (usp)->tv_nsec) :                       \
            ((tsp)->tv_sec cmp (usp)->tv_sec))
#define timespecadd(tsp, usp, vsp)                                      \
        do {                                                            \
                (vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;          \
                (vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;       \
                if ((vsp)->tv_nsec >= 1000000000L) {                    \
                        (vsp)->tv_sec++;                                \
                        (vsp)->tv_nsec -= 1000000000L;                  \
                }                                                       \
        } while (/* CONSTCOND */ 0)
#define timespecsub(tsp, usp, vsp)                                      \
        do {                                                            \
                (vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;          \
                (vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;       \
                if ((vsp)->tv_nsec < 0) {                               \
                        (vsp)->tv_sec--;                                \
                        (vsp)->tv_nsec += 1000000000L;                  \
                }                                                       \
        } while (/* CONSTCOND */ 0)
#endif

/* eloop queues are really only for deleting timeouts registered
 * for a function or object.
 * The idea being that one interface has different timeouts for
 * say DHCP and DHCPv6. */
#ifndef ELOOP_QUEUE
  #define ELOOP_QUEUE 1
#endif

/* Forward declare eloop - the content should be invisible to the outside */
struct eloop;

int eloop_event_add_rw(struct eloop *, int,
    void (*)(void *), void *,
    void (*)(void *), void *);
int eloop_event_add(struct eloop *, int,
    void (*)(void *), void *);
int eloop_event_add_w(struct eloop *, int,
    void (*)(void *), void *);
#define eloop_event_delete(eloop, fd) \
    eloop_event_delete_write((eloop), (fd), 0)
#define eloop_event_remove_writecb(eloop, fd) \
    eloop_event_delete_write((eloop), (fd), 1)
int eloop_event_delete_write(struct eloop *, int, int);

#define eloop_timeout_add_tv(eloop, tv, cb, ctx) \
    eloop_q_timeout_add_tv((eloop), ELOOP_QUEUE, (tv), (cb), (ctx))
#define eloop_timeout_add_sec(eloop, tv, cb, ctx) \
    eloop_q_timeout_add_sec((eloop), ELOOP_QUEUE, (tv), (cb), (ctx))
#define eloop_timeout_add_msec(eloop, ms, cb, ctx) \
    eloop_q_timeout_add_msec((eloop), ELOOP_QUEUE, (ms), (cb), (ctx))
#define eloop_timeout_delete(eloop, cb, ctx) \
    eloop_q_timeout_delete((eloop), ELOOP_QUEUE, (cb), (ctx))
int eloop_q_timeout_add_tv(struct eloop *, int,
    const struct timespec *, void (*)(void *), void *);
int eloop_q_timeout_add_sec(struct eloop *, int,
    time_t, void (*)(void *), void *);
int eloop_q_timeout_add_msec(struct eloop *, int,
    long, void (*)(void *), void *);
int eloop_q_timeout_delete(struct eloop *, int, void (*)(void *), void *);

int eloop_signal_set_cb(struct eloop *, const int *, size_t,
    void (*)(int, void *), void *);
int eloop_signal_mask(struct eloop *, sigset_t *oldset);

struct eloop * eloop_new(void);
int eloop_requeue(struct eloop *);
void eloop_free(struct eloop *);
void eloop_exit(struct eloop *, int);
int eloop_start(struct eloop *, sigset_t *);

#endif
