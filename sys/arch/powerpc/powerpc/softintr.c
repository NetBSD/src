/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.1 2004/03/24 23:39:39 matt Exp $");

#include <sys/param.h>
#include <lib/libkern/libkern.h>
#include <sys/pool.h>
#include <machine/intr.h>
#include <net/netisr.h>

static struct softintr_qh
#ifdef IPL_SOFTI2C
       softintr_softi2c = SIMPLEQ_HEAD_INITIALIZER(softintr_softi2c),
#endif
       softintr_softnet = SIMPLEQ_HEAD_INITIALIZER(softintr_softnet),
     softintr_softclock = SIMPLEQ_HEAD_INITIALIZER(softintr_softclock),
    softintr_softserial = SIMPLEQ_HEAD_INITIALIZER(softintr_softserial);

static struct pool softintr_pool;
struct softintr *softnet_handlers[32];

static __inline struct softintr_qh *
softintr_queue(int ipl)
{
	switch (ipl) {
	case IPL_SOFTSERIAL:	return &softintr_softserial;
	case IPL_SOFTNET:	return &softintr_softnet;
	case IPL_SOFTCLOCK:	return &softintr_softclock;
#ifdef IPL_SOFTI2C
	case IPL_SOFTI2C:	return &softintr_softi2c;
#endif
	default:
		KASSERT(ipl == IPL_SOFTSERIAL || ipl == IPL_SOFTNET || ipl == IPL_SOFTCLOCK);
	}
	return NULL;
}

void
softintr__init(void)
{
	pool_init(&softintr_pool, sizeof(struct softintr), 0, 0, 0,
	   "sipl", &pool_allocator_nointr);

#define DONETISR(n, f) \
	softnet_handlers[(n)] = \
	    softintr_establish(IPL_SOFTNET, (void (*)(void *))(f), NULL)
#include <net/netisr_dispatch.h>
#undef DONETISR
}

/*
 * This routine is called by the platform dependent intrcode to
 * run the queue of softintr schedules.  It is expected to already be
 * at the appropriate IPL upon entry.
 */
void
softintr__run(int ipl)
{
	struct softintr_qh * const qh = softintr_queue(ipl);
	struct softintr *si;
	int s;

	for (;;) {
		s = splvm();
		si = SIMPLEQ_FIRST(qh);
		if (si == NULL) {
			splx(s);
			return;
		}
		SIMPLEQ_REMOVE_HEAD(qh, si_link);
		si->si_refs--;
		KASSERT(si->si_refs > 0);
		splx(s);

		(*si->si_func)(si->si_arg);
	}
}

/*
 * This schedules a software interrupt handler.
 * It may be called from any IPL >= the IPL of the handler.
 */
void
softintr_schedule(void *cookie)
{
	struct softintr * const si = cookie;
	struct softintr_qh * const qh = softintr_queue(si->si_ipl);
	int s;

	/*
	 * Assume checking a single integer field is atomic.
	 */
	if (si->si_refs > 1)
		return;

	/*
	 * Raise IPL and insert onto queue.
	 */
	s = splvm();
	SIMPLEQ_INSERT_TAIL(qh, si, si_link);
	si->si_refs++;
	switch (si->si_ipl) {
	case IPL_SOFTSERIAL:	setsoftserial(); break;
	case IPL_SOFTCLOCK:	setsoftclock(); break;
	case IPL_SOFTNET:	setsoftnet(); break;
#ifdef IPL_SOFTI2C
	case IPL_SOFTNET:	setsofti2c(); break;
#endif
	}
	splx(s);
}

/*
 * Establish (allocate) a software interrupt handler.
 * This must be called from a threaded context at an IPL <= IPL_VM
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct softintr *si;
	int s;

	s = splvm();
	si = pool_get(&softintr_pool, PR_WAITOK);
	splx(s);
	if (si == NULL)
		return NULL;

	si->si_ipl = ipl;
	si->si_func = func;
	si->si_arg = arg;
	si->si_refs = 1;
	return si;
}

/*
 * Disestablish (free) a software interrupt handler.
 * This must be called from a threaded context at an IPL <= IPL_VM
 */
void
softintr_disestablish(void *cookie)
{
	struct softintr * const si = cookie;
	int s;

	s = splvm();
	/*
	 * If queued, dequeue the entry.
	 */
	if (si->si_refs > 1) {
		struct softintr_qh * const qh = softintr_queue(si->si_ipl);
		SIMPLEQ_REMOVE(qh, si, softintr, si_link);
		si->si_refs--;
	}

	/*
	 * Make sure we always put to the pool at a consistent IPL.
	 */
	si->si_refs--;
	KASSERT(si->si_refs == 0);
	pool_put(&softintr_pool, si);
	splx(s);
}
