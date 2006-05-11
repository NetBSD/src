/*	$NetBSD: extintr.c,v 1.3.10.1 2006/05/11 23:26:59 elad Exp $ */

/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/trap.h>

#include <dev/isa/isavar.h>

static void intr_calculatemasks(void);
static int fakeintr(void *);
static void ext_intr_ivr(void);
static void install_extint(void (*)(void));

int imen = 0xffffffff;
int imask[NIPL];

struct intrsource intrsources[ICU_LEN];

static int
fakeintr(void *arg)
{

	return 0;
}

/*
 * ext_interrupts using the board's interrupt vector register.
 */
static void
ext_intr_ivr(void)
{
	u_int8_t irq;
	int r_imen, pcpl, msr;
	struct cpu_info *ci = curcpu();
	struct intrhand *ih;
	struct intrsource *is;

	pcpl = ci->ci_cpl;
	msr = mfmsr();

	irq = *((u_char *)prep_intr_reg + INTR_VECTOR_REG);
	is = &intrsources[irq];

	r_imen = 1 << irq;

	if ((pcpl & r_imen) != 0) {
		ci->ci_ipending |= r_imen; /* Masked! Mark this as pending */
		imen |= r_imen;
		isa_intr_mask(imen);
	} else {
		splraise(is->is_mask);
		mtmsr(msr | PSL_EE);
		ih = is->is_hand;
		if (ih == NULL)
			printf("spurious interrupt %d\n", irq);
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		isa_intr_clr(irq);
		mtmsr(msr);
		ci->ci_cpl = pcpl;

		isa_intr_clr(irq);

		uvmexp.intrs++;
		is->is_ev.ev_count++;
	}

	mtmsr(msr | PSL_EE);
	splx(pcpl);	/* Process pendings. */
	mtmsr(msr);
}

void *
intr_establish(int irq, int type, int level, int (*ih_fun)(void *), void *ih_arg)
{
	struct intrhand **p, *q, *ih;
	struct intrsource *is;
	static struct intrhand fakehand = {fakeintr};

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	is = &intrsources[irq];
	is->is_hwirq = irq;

	switch (is->is_type) {
	case IST_NONE:
		is->is_type = type;
		break;
	case IST_LEVEL:
	case IST_EDGE:
		if (type == is->is_type)
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s irq %d",
			    isa_intr_typename(is->is_type),
			    isa_intr_typename(type), irq);
		break;
	}


	if (is->is_hand == NULL) {
		snprintf(is->is_source, sizeof(is->is_source), "irq %d",
		    is->is_hwirq);
		evcnt_attach_dynamic(&is->is_ev, EVCNT_TYPE_INTR, NULL,
		    "8259", is->is_source);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &is->is_hand; (q = *p) != NULL; p = &q->ih_next)
		continue;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	if (irq < I8259_INTR_NUM)
		isa_setirqstat(irq, 1, type);

	return (ih);
}

void
intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq;
	struct intrsource *is = &intrsources[irq];
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq");

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &is->is_hand; (q = *p) != NULL && q != ih; p = &q->ih_next)
		continue;
	if (q == NULL)
		panic("intr_disestablish: handler not registered");

	*p = q->ih_next;

	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (is->is_hand == NULL) {
		is->is_type = IST_NONE;
		evcnt_detach(&is->is_ev);
	}
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
static void
intr_calculatemasks(void)
{
	int irq, level;
	struct intrsource *is;
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0, is = intrsources; irq < ICU_LEN; irq++, is++) {
		register int levels = 0;
		for (q = is->is_hand; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		is->is_level = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0, is = intrsources; irq < ICU_LEN; irq++, is++)
			if (is->is_level & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = SINT_CLOCK;
	imask[IPL_SOFTNET] = SINT_NET;
	imask[IPL_SOFTSERIAL] = SINT_SERIAL;

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
         * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= SPL_CLOCK;          /* block the clock */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

	/* And eventually calculate the complete masks. */
	for (irq = 0, is = intrsources; irq < ICU_LEN; irq++, is++) {
		register int irqs = 1 << irq;
		for (q = is->is_hand; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		is->is_mask = irqs;
	}

	{
		register int irqs = 0;
		for (irq = 0, is = intrsources; irq < I8259_INTR_NUM;
		     irq++, is++)
			if (is->is_hand)
				irqs |= 1 << irq;
		if (irqs >= 0x100)	/* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		imen = ~irqs;
		isa_intr_mask(imen);
	}
}

void
do_pending_int(void)
{
	struct cpu_info * const ci = curcpu();
	struct intrhand *ih;
	struct intrsource *is;
	int irq, pcpl, hwpend, emsr, dmsr;

	if (ci->ci_iactive)
		return;

	ci->ci_iactive = 1;
	emsr = mfmsr();
	dmsr = emsr & ~PSL_EE;
	mtmsr(dmsr);

	pcpl = ci->ci_cpl;		/* Turn off all */
again:
	hwpend = ci->ci_ipending & ~pcpl; /* Do now unmasked pendings */
	imen &= ~hwpend;
	hwpend &= ~SINT_MASK;
	while (hwpend) {
		irq = ffs(hwpend) - 1;
		is = &intrsources[irq];
		hwpend &= ~(1L << irq);
		ih = is->is_hand;
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		isa_intr_clr(irq); /* XXX */

		uvmexp.intrs++;
		is->is_ev.ev_count++;
	}
	if ((ci->ci_ipending & ~pcpl) & SINT_CLOCK) {
		ci->ci_ipending &= ~SINT_CLOCK;
		splsoftclock();
		mtmsr(emsr);
		softintr__run(IPL_SOFTCLOCK);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softclock.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & SINT_NET) {
		ci->ci_ipending &= ~SINT_NET;
		splsoftnet();
		mtmsr(emsr);
		softintr__run(IPL_SOFTNET);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softnet.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & SINT_SERIAL) {
		ci->ci_ipending &= ~SINT_SERIAL;
		splsoftserial();
		mtmsr(emsr);
		softintr__run(IPL_SOFTSERIAL);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softserial.ev_count++;
		goto again;
	}

	ci->ci_ipending &= pcpl;
	ci->ci_cpl = pcpl;	/* Don't use splx... we are here already! */
	isa_intr_mask(imen);
	ci->ci_iactive = 0;
	mtmsr(emsr);
}

static void
install_extint(void (*handler)(void))
{
	extern u_char extint[];
	extern u_long extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int omsr, msr;

#ifdef DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(omsr), "=r"(msr) : "K"((u_short)~PSL_EE));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	mtmsr(omsr);
}

void
init_intr_ivr(void)
{
	install_extint(ext_intr_ivr);
}

/*
 *  Reorder protection in the following inline functions is
 * achieved with the "eieio" instruction which the assembler
 * seems to detect and then doesn't move instructions past....
 */

int
splraise(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int oldcpl;

	__asm volatile("sync; eieio\n");	/* don't reorder.... */
	oldcpl = ci->ci_cpl;
	ci->ci_cpl = oldcpl | newcpl;
	__asm volatile("sync; eieio\n");	/* reorder protect */
	return(oldcpl);
}

int
spllower(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio\n");	/* reorder protect */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = newcpl;
	if(ci->ci_ipending & ~newcpl)
	        do_pending_int();
	__asm volatile("sync; eieio\n");	/* reorder protect */
	return ocpl;
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
void
softintr(int pending)
{
	int msrsave;

	msrsave = mfmsr();
	mtmsr(msrsave & ~PSL_EE);
	curcpu()->ci_ipending |= pending;
	mtmsr(msrsave);
}
