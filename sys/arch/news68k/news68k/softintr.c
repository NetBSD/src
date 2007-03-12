/*	$NetBSD: softintr.c,v 1.2.2.1 2007/03/12 05:49:40 rmind Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford and Jason R. Thorpe.
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

/*
 * Generic soft interrupt implementation for news68k.
 * Taken from mvme68k one, which is based heavily on
 * the alpha implementation by Jason Thorpe.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/sched.h>
#include <sys/vmmeter.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <news68k/news68k/isr.h>

struct news68k_soft_intrhand	*softnet_intrhand;

static struct news68k_soft_intr news68k_soft_intrs[SI_NQUEUES];

extern void _softintr(void);	/* locore.s */

static void netintr(void);

static inline int
ssir_pending(volatile unsigned char *ssptr)
{
	int __rv;

	__asm volatile(
		"	moveq	#0, %0	\n"
		"	tas	%1	\n"
		"	jne	1f	\n"
		"	moveq	#1, %0	\n"
		"1:			\n"
		: "=d" (__rv)
		: "m" (*ssptr));

	return __rv;
}

/*
 * softintr_init()
 *
 *	Initialise news68k software interrupt subsystem.
 */
void
softintr_init(void)
{
	static const char *softintr_names[] = SI_QUEUENAMES;
	struct news68k_soft_intr *nsi;
	int i;

	for (i = 0; i < SI_NQUEUES; i++) {
		nsi = &news68k_soft_intrs[i];
		LIST_INIT(&nsi->nsi_q);
		nsi->nsi_ssir = 1;
		evcnt_attach_dynamic(&nsi->nsi_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}
	isrlink_custom(SOFTINTR_IPL, (void *)_softintr);

	/* Establish legacy software interrupt handlers */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	KDASSERT(softnet_intrhand != NULL);
}

/*
 * softintr_dispatch()
 *
 *	Internal function for running queued soft interrupts.
 */
void
softintr_dispatch(void)
{
	struct news68k_soft_intr *nsi;
	struct news68k_soft_intrhand *sih;
	int handled;

	softintr_clear();

	do {
		for (nsi = news68k_soft_intrs, handled = 0;
		    nsi < &news68k_soft_intrs[SI_NQUEUES];
		    nsi++) {

			if (ssir_pending(&nsi->nsi_ssir) == 0)
				continue;

			nsi->nsi_evcnt.ev_count++;
			handled++;

			for (sih = LIST_FIRST(&nsi->nsi_q);
			     sih != NULL;
			     sih = LIST_NEXT(sih, sih_q)) {
				if (sih->sih_pending) {
					uvmexp.softs++;
					sih->sih_pending = 0;
					(*sih->sih_fn)(sih->sih_arg);
				}
			}
		}
	} while (handled);
}

static int
ipl2si(int ipl)
{
	int si;

	switch (ipl) {
	case IPL_SOFTSERIAL:
		si = SI_SOFTSERIAL;
		break;
	case IPL_SOFTNET:
		si = SI_SOFTNET;
		break;
	case IPL_SOFTCLOCK:
		si = SI_SOFTCLOCK;
		break;
	case IPL_SOFT:
		si = SI_SOFT;
		break;
	default:
		panic("ipl2si: %d", ipl);
	}
	return si;
}

/*
 * softintr_establish()		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct news68k_soft_intr *nsi;
	struct news68k_soft_intrhand *sih;
	int si;
	int s;

	si = ipl2si(ipl);
	nsi = &news68k_soft_intrs[si];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = nsi;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
		s = splsoft();
		LIST_INSERT_HEAD(&nsi->nsi_q, sih, sih_q);
		splx(s);
	}
	return sih;
}

/*
 * softintr_disestablish()	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct news68k_soft_intrhand *sih;
	int s;

	sih = arg;
	s = splsoft();
	LIST_REMOVE(sih, sih_q);
	splx(s);

	free(sih, M_DEVBUF);
}

void
netintr(void)
{
	int s, isr;

	s = splnet();
	isr = netisr;
	netisr = 0;
	splx(s);

#define DONETISR(bit, func)						\
	do {								\
		if (isr & (1 << bit)) {					\
			func();						\
		}							\
	} while (/* CONSTCOND */0)

#include <net/netisr_dispatch.h>

#undef DONETISR

}
