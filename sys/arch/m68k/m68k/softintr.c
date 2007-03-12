/*	$NetBSD: softintr.c,v 1.1.4.2 2007/03/12 05:48:54 rmind Exp $	*/

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
 * Generic soft interrupt implementation for m68k ports which use
 * hp300 style simulated software interrupt with VAX REI emulation.
 *
 * Based heavily on the alpha implementation by Jason Thorpe.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.1.4.2 2007/03/12 05:48:54 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <m68k/asm_single.h>

#include <machine/cpu.h>
#include <machine/intr.h>

struct m68k_soft_intrhand *softnet_intrhand;
volatile uint8_t ssir;

static struct m68k_soft_intr m68k_soft_intrs[SI_NQUEUES];

static void netintr(void);

/*
 * softintr_init()
 *
 *	Initialise hp300 style software interrupt subsystem.
 */
void
softintr_init(void)
{
	static const char *softintr_names[] = SI_QUEUENAMES;
	struct m68k_soft_intr *msi;
	int i;

	for (i = 0; i < SI_NQUEUES; i++) {
		msi = &m68k_soft_intrs[i];
		LIST_INIT(&msi->msi_q);
		msi->msi_ipl = i;
		evcnt_attach_dynamic(&msi->msi_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* Establish legacy software interrupt handlers */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	KASSERT(softnet_intrhand != NULL);
}

/*
 * softintr_dispatch()
 *
 *	Internal function for running queued soft interrupts.
 */
void
softintr_dispatch(void)
{
	struct m68k_soft_intr *msi;
	struct m68k_soft_intrhand *sih;
	bool handled;
	uint8_t mask;

	do {
		handled = false;
		mask = 0x01;
		for (msi = m68k_soft_intrs;
		    msi < &m68k_soft_intrs[SI_NQUEUES];
		    msi++, mask <<= 1) {

			if ((ssir & mask) == 0)
				continue;

			msi->msi_evcnt.ev_count++;
			handled = true;
			single_inst_bclr_b(ssir, mask);

			for (sih = LIST_FIRST(&msi->msi_q);
			     sih != NULL;
			     sih = LIST_NEXT(sih, sih_q)) {
				if (sih->sih_pending) {
					uvmexp.softs++;
					sih->sih_pending = 0;
					(*sih->sih_func)(sih->sih_arg);
				}
			}
		}
	} while (handled);
}

static int
ipl2si(ipl_t ipl)
{
	int si;

	switch (ipl) {
	case IPL_SOFTNET:
		si = SI_SOFTNET;
		break;
	case IPL_SOFTCLOCK:
		si = SI_SOFTCLOCK;
		break;
	case IPL_SOFTSERIAL:
		si = SI_SOFTSERIAL;
		break;
	default:
		si = SI_SOFT;
		break;
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
	struct m68k_soft_intr *msi;
	struct m68k_soft_intrhand *sih;
	int s;

	if (__predict_false(ipl >= NIPL || ipl < 0 || func == NULL))
		panic("%s: invalid ipl (%d) or function", __func__, ipl);

	msi = &m68k_soft_intrs[ipl2si(ipl)];

	sih = malloc(sizeof(struct m68k_soft_intrhand), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = msi;
		sih->sih_func = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
		s = splsoft();
		LIST_INSERT_HEAD(&msi->msi_q, sih, sih_q);
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
	struct m68k_soft_intrhand *sih;
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

	for (;;) {
		s = splhigh();
		isr = netisr;
		netisr = 0;
		splx(s);

		if (isr == 0)
			return;

#define DONETISR(bit, func)						\
		do {							\
			if (isr & (1 << bit))				\
				func();					\
		} while (/* CONSTCOND */0)

		s = splsoftnet();

#include <net/netisr_dispatch.h>

#undef DONETISR

		splx(s);
	}
}
