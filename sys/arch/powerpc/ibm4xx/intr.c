/*	$NetBSD: intr.c,v 1.2 2002/05/13 07:04:24 matt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/dcr.h>

#include <powerpc/spr.h>

static inline void disable_irq(int irq);
static inline void enable_irq(int irq);
static void intr_calculatemasks(void);
static char *intr_typename(int);

static int fakeintr(void *);
static inline int cntlzw(int);


volatile int cpl, ipending;
u_long imask[NIPL];

static int intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
static struct intrhand *intrhand[ICU_LEN];



static inline int
cntlzw(int x)
{
	int a;

	__asm __volatile ("cntlzw %0,%1" : "=r"(a) : "r"(x));
	return a;
}

static int
fakeintr(void *arg)
{

	return 0;
}

/*
 * Set up interrupt mapping array.
 */
void
intr_init(void)
{

}

/*
 * external interrupt handler
 */
void
ext_intr(void)
{
	int i, bits_to_clear;
	int r_imen, msr;
	int pcpl;
	struct intrhand *ih;
	u_long int_state;

	pcpl = cpl;
	asm volatile ("mfmsr %0" : "=r"(msr));
	
	int_state = mfdcr(DCR_UIC0_MSR);	/* Read non-masked interrupt status */
	bits_to_clear = int_state;

	while (int_state) {
		i = cntlzw(int_state);
		int_state &= ~IRQ_TO_MASK(i);

		r_imen = IRQ_TO_MASK(i);

		if ((pcpl & r_imen) != 0) {
			ipending |= r_imen;	/* Masked! Mark this as pending */
			disable_irq(i);
 		} else {
			splraise(intrmask[i]);
			asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			ih = intrhand[i];
			while (ih) {
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
			KERNEL_UNLOCK();
			asm volatile ("mtmsr %0" :: "r"(msr));
			cpl = pcpl;
			uvmexp.intrs++;
			intrcnt[i]++;
		}
	}
	mtdcr(DCR_UIC0_SR, bits_to_clear);	/* Acknowledge all pending interrupts */
	
	asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
	splx(pcpl);
	asm volatile ("mtmsr %0" :: "r"(msr));
}

static inline void 
disable_irq(int irq)
{
	int mask, omask;

	mask = omask = mfdcr(DCR_UIC0_ER);
	mask &= ~IRQ_TO_MASK(irq);
	if (mask == omask)
		return;
	mtdcr(DCR_UIC0_ER, mask);
#ifdef IRQ_DEBUG
	printf("irq_disable: irq=%d, mask=%08x\n",irq,mask);
#endif
}

static inline void 
enable_irq(int irq)
{
	int mask, omask;

	mask = omask = mfdcr(DCR_UIC0_ER);
	mask |= IRQ_TO_MASK(irq);
	if (mask == omask)
		return;
	mtdcr(DCR_UIC0_ER, mask);
#ifdef IRQ_DEBUG
	printf("irq_enable: irq=%d, mask=%08x\n",irq,mask);
#endif
}

static char *
intr_typename(int type)
{

	switch (type) {
	case IST_NONE :
		return ("none");
	case IST_PULSE:
		return ("pulsed");
	case IST_EDGE:
		return ("edge-triggered");
	case IST_LEVEL:
		return ("level-triggered");
	default:
		panic("isa_intr_typename: invalid type %d", type);
	}
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(int irq, int type, int level, int (*ih_fun)(void *), void *ih_arg)
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = { fakeintr };

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	switch (intrtype[irq]) {
	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrtype[irq])
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(intrtype[irq]),
			    intr_typename(type));
		break;
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

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
#ifdef IRQ_DEBUG
	printf("***** intr_establish: irq%d h=%p arg=%p\n",irq, ih_fun, ih_arg);
#endif
	return (ih);
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq;
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq");

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL && q != ih; p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (intrhand[irq] == NULL)
		intrtype[irq] = IST_NONE;
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
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int levels = 0;
		for (q = intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= IRQ_TO_MASK(irq);
		imask[level] = irqs | SINT_MASK;
	}
	
	/*
	 * IPL_CLOCK should mask clock interrupt even if interrupt handler
	 * is not registered.
	 */
	imask[IPL_CLOCK] |= SPL_CLOCK;

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
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int irqs = IRQ_TO_MASK(irq);
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		if (intrhand[irq])
			enable_irq(irq);
		else
			disable_irq(irq);
}

void
do_pending_int(void)
{
	struct intrhand *ih;
	int irq;
	int pcpl;
	int hwpend;
	int emsr, dmsr;
	static int processing;

	if (processing)
		return;

	processing = 1;
	asm volatile("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile("mtmsr %0" :: "r"(dmsr));

	pcpl = cpl;		/* Turn off all */
  again:	
	while ((hwpend = ipending & ~pcpl & HWINT_MASK)) {
		irq = cntlzw(hwpend);
		enable_irq(irq);

		ipending &= ~IRQ_TO_MASK(irq);

		splraise(intrmask[irq]);
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		ih = intrhand[irq];
		while(ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[irq]++;
	}

	if ((ipending & ~pcpl) & SINT_SERIAL) {
		ipending &= ~SINT_SERIAL;
		splsoftserial();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softserial();
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SINT_SERIAL]++;
		goto again;
	}
	if ((ipending & ~pcpl) & SINT_NET) {
		ipending &= ~SINT_NET;
		splsoftnet();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softnet();
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SINT_NET]++;
		goto again;
	}
	if ((ipending & ~pcpl) & SINT_CLOCK) {
		ipending &= ~SINT_CLOCK;
		splsoftclock();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softclock(NULL);
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SINT_CLOCK]++;
		goto again;
	}
	cpl = pcpl;	/* Don't use splx... we are here already! */
	asm volatile("mtmsr %0" :: "r"(emsr));
	processing = 0;
}
