/*	$NetBSD: softintr.c,v 1.5 2003/07/15 03:36:00 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.5 2003/07/15 03:36:00 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <net/netisr.h>


/*
 * Soft interrupt data structures
 */
struct sh5_soft_intrhand {
	TAILQ_ENTRY(sh5_soft_intrhand) sih_q;
	struct sh5_soft_intr *sih_intrhead;
	void (*sih_fn)(void *);
	void *sih_arg;
	volatile int sih_pending;
};

struct sh5_soft_intr {
	TAILQ_HEAD(, sh5_soft_intrhand) si_q;
	struct simplelock si_slock;
	struct evcnt *si_evcnt;
	int si_ipl;
};


/* Bitmap of pending simulated software interrupts */
volatile uint32_t ssir;

/* Legacy "network" software interrupt handler */
static void netintr(void *);

/* Legacy "network" software interrupt cookie */
void *softnet_cookie;

/* Linked lists of soft interrupt handlers per level */
static struct sh5_soft_intr soft_intrs[_IPL_NSOFT];

/* Relative priorities of soft interrupts. Lowest first */
static const int soft_intr_prio[_IPL_NSOFT] = {
	IPL_SOFT, IPL_SOFTCLOCK, IPL_SOFTNET, IPL_SOFTSERIAL
};

void
softintr_init(void)
{
	struct sh5_soft_intr *si;
	int i;

	for (i = 0; i < _IPL_NSOFT; i++) {
		si = &soft_intrs[i];
		TAILQ_INIT(&si->si_q);

		simple_lock_init(&si->si_slock);
		si->si_evcnt = &_sh5_intr_events[soft_intr_prio[i]];
		si->si_ipl = soft_intr_prio[i];
	}

	ssir = 0;

	/* XXX Establish legacy soft interrupt handlers. */
	softnet_cookie = softintr_establish(IPL_SOFTNET, netintr, NULL);
	KDASSERT(softnet_cookie != NULL);
}

/* Register a software interrupt handler. */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct sh5_soft_intrhand *sih;
	int i;

	for (i = _IPL_NSOFT - 1; i >= 0; i--)
		if (soft_intr_prio[i] == ipl)
			break;

	if (__predict_false(i == _IPL_NSOFT))
		panic("softintr_establish");

	sih = malloc(sizeof(*sih), M_SOFTINTR, M_NOWAIT);

	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = &soft_intrs[i];
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}

	return ((void *)sih);
}

/* Unregister a software interrupt handler. */
void
softintr_disestablish(void *cookie)
{
	struct sh5_soft_intrhand *sih = cookie;
	struct sh5_soft_intr *si = sih->sih_intrhead;
	int s;

	s = splhigh();
	if (__predict_false(sih->sih_pending)) {
		sih->sih_pending = 0;
		TAILQ_REMOVE(&si->si_q, sih, sih_q);
		if (TAILQ_EMPTY(&si->si_q))
			ssir &= ~(1 << (si->si_ipl - 1));
	}
	splx(s);

	free(sih, M_DEVBUF);
}

/* Schedule a software interrupt */
void
softintr_schedule(void *cookie)
{
	struct sh5_soft_intrhand *sih = cookie;
	struct sh5_soft_intr *si = sih->sih_intrhead;
	int s;

	s = splhigh();
	if (__predict_true(sih->sih_pending == 0)) {
		sih->sih_pending = 1;
		if (TAILQ_EMPTY(&si->si_q))
			ssir |= (1 << (si->si_ipl - 1));
		TAILQ_INSERT_TAIL(&si->si_q, sih, sih_q);
	}
	splx(s);
}

/*
 * Software (low priority) network interrupt. i.e. softnet().
 */
/*ARGSUSED*/
static void
netintr(void *arg)
{
#define	DONETISR(bit, fn)						\
	do {								\
		if (n & (1 << bit))					\
			fn();						\
	} while (/*CONSTCOND*/0)

	int s, n;

	s = splnet();
	n = netisr;
	netisr = 0;
	splx(s);

#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Dispatch all pending soft interrupts with priorities starting
 * at `softspl' down to the priority encoded in `oldspl'.
 *
 * The current interrupt mask is set up to block further soft interrupts
 * at priorities lower than or equal to `softspl'.
 *
 * Note that this routine can be called recursively, once per soft
 * interrupt level. The recursion can occur at any time, even for
 * soft interrupts with priorities which we've already dealt with
 * in the outer loop...
 */
void
softintr_dispatch(u_int oldspl, u_int softspl)
{
	struct sh5_soft_intrhand *sih;
	struct sh5_soft_intr *si;
	int i;

	KDASSERT(softspl > oldspl);

	for (i = _IPL_NSOFT - 1; i >= 0; i--)
		if (softspl == soft_intr_prio[i])
			break;

	KDASSERT(i >= 0);

	for ( ; i >= 0 && soft_intr_prio[i] > oldspl; i--) {
		si = &soft_intrs[i];

		/*
		 * No interrupts while we diddle with the queue
		 */
		(void) splhigh();

		/*
		 * Dispatch all soft interrupts at the current level
		 */
		while ((sih = TAILQ_FIRST(&si->si_q)) != NULL) {
			TAILQ_REMOVE(&si->si_q, sih, sih_q);
			sih->sih_pending = 0;

			si->si_evcnt->ev_count++;
			uvmexp.softs++;

			/*
			 * Drop to the current soft interrupt spl
			 * before calling the handler.
			 */
			_cpu_intr_set(soft_intr_prio[i]);
			(*sih->sih_fn)(sih->sih_arg);

			/*
			 * Block all interrupts again
			 */
			(void) splhigh();
		}

		/*
		 * Clear the "pending" status for soft interrupts at
		 * this level.
		 */
		ssir &= ~(1 << (si->si_ipl - 1));
	}

	/*
	 * Note: No need to use splx() here, since that will check
	 * for soft interrupts again ...
	 */
	_cpu_intr_set(oldspl);
}
