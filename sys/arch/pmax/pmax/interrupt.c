/*	$NetBSD: interrupt.c,v 1.2 2001/08/27 02:00:19 nisimura Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <mips/psl.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/intr.h>

const u_int32_t ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTSERIAL */
};

struct pmax_soft_intr pmax_soft_intrs[_IPL_NSOFT];

struct evcnt pmax_clock_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");
struct evcnt pmax_fpu_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "fpu", "intr");
struct evcnt pmax_memerr_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "memerr", "intr");

/* XXX XXX XXX */
extern void MachFPInterrupt(unsigned, unsigned, unsigned, struct frame *);
#define	dealfpu(sr,cr,pc) \
		MachFPInterrupt((sr), (cr), (pc), curproc->p_md.md_regs)
/* XXX XXX XXX */

/*
 * pmax uses standard mips1 convention, wiring FPU to hard interupt 5.
 */
void
cpu_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{

	int i, s;
	struct pmax_soft_intr *asi;
	struct pmax_soft_intrhand *sih;

	uvmexp.intrs++;

	/* device interrupts */
	if (ipending & (MIPS_INT_MASK_0|MIPS_INT_MASK_1|MIPS_INT_MASK_2|
			MIPS_INT_MASK_3|MIPS_INT_MASK_4)) {
		(*platform.iointr)(status, cause, pc, ipending);
	}
	/* FPU notification */
	if (ipending & MIPS_INT_MASK_5) {
		if (!USERMODE(status))
			goto kerneltouchedFPU;
		pmax_fpu_evcnt.ev_count++;
		dealfpu(status, cause, pc);
	}

	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	for (i = _IPL_NSOFT - 1; i >= 0; i--) {
		if ((ipending & ipl_si_to_sr[i]) == 0)
			continue;

		asi = &pmax_soft_intrs[i];

		if (TAILQ_FIRST(&asi->softintr_q) != NULL)
			asi->softintr_evcnt.ev_count++;

		for (;;) {
			s = splhigh();

			sih = TAILQ_FIRST(&asi->softintr_q);
			if (sih != NULL) {
				TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
				sih->sih_pending = 0;
			}

			splx(s);

			if (sih == NULL)
				break;

			uvmexp.softs++;
			(*sih->sih_fn)(sih->sih_arg);
		}
	}
	return;
kerneltouchedFPU:
	panic("kernel used FPU: PC %x, CR %x, SR %x", pc, cause, status);
}

/* XXX For legacy software interrupts. */
struct pmax_soft_intrhand *softnet_intrhand;

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init(void)
{
	static const char *softintr_names[] = IPL_SOFTNAMES;
	struct pmax_soft_intr *asi;
	int i;

	for (i = 0; i < _IPL_NSOFT; i++) {
		asi = &pmax_soft_intrs[i];
		TAILQ_INIT(&asi->softintr_q);
		asi->softintr_ipl = IPL_SOFT + i;
		evcnt_attach_dynamic(&asi->softintr_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* XXX Establish legacy soft interrupt handlers. */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	assert(softnet_intrhand != NULL);
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct pmax_soft_intr *asi;
	struct pmax_soft_intrhand *sih;

	if (__predict_false(ipl >= (IPL_SOFT + _IPL_NSOFT) ||
			    ipl < IPL_SOFT))
		panic("softintr_establish");

	asi = &pmax_soft_intrs[ipl - IPL_SOFT];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = asi;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return (sih);
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct pmax_soft_intrhand *sih = arg;
	struct pmax_soft_intr *asi = sih->sih_intrhead;
	int s;

	s = splhigh();
	if (sih->sih_pending) {
		TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	splx(s);

	free(sih, M_DEVBUF);
}
