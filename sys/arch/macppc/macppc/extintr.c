/*	$NetBSD: extintr.c,v 1.55.14.1 2006/06/19 03:44:52 chap Exp $	*/

/*-
 * Copyright (c) 2000, 2001 Tsubai Masanari.
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
 * 3. Neither the name of the University nor the names of its contributors
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

/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: extintr.c,v 1.55.14.1 2006/06/19 03:44:52 chap Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/pio.h>

#include <powerpc/atomic.h>
#include <powerpc/openpic.h>

#include <dev/ofw/openfirm.h>

#define NIRQ 32
#define HWIRQ_MAX (NIRQ - 4 - 1)
#define HWIRQ_MASK 0x0fffffff

void intr_calculatemasks __P((void));
int fakeintr __P((void *));

static inline uint32_t gc_read_irq __P((void));
static inline int mapirq __P((int));
static void gc_enable_irq __P((int));
static void gc_reenable_irq __P((int));
static void gc_disable_irq __P((int));

static void do_pending_int __P((void));
static void ext_intr_openpic __P((void));
static void legacy_int_init __P((void));

int imask[NIPL];

struct intrsource {
	int is_type;
	int is_level;
	int is_hwirq;
	int is_mask;
	struct intrhand *is_hand;
	struct evcnt is_ev;
	char is_source[16];
};

static struct intrsource intrsources[NIRQ];

static u_char virq[ICU_LEN];
static int virq_max = 0;

static u_char *obio_base;

extern u_int *heathrow_FCR;

#define IPI_VECTOR 64

#define interrupt_reg	(obio_base + 0x10)

#define INT_STATE_REG_H  (interrupt_reg + 0x00)
#define INT_ENABLE_REG_H (interrupt_reg + 0x04)
#define INT_CLEAR_REG_H  (interrupt_reg + 0x08)
#define INT_LEVEL_REG_H  (interrupt_reg + 0x0c)
#define INT_STATE_REG_L  (interrupt_reg + 0x10)
#define INT_ENABLE_REG_L (interrupt_reg + 0x14)
#define INT_CLEAR_REG_L  (interrupt_reg + 0x18)
#define INT_LEVEL_REG_L  (interrupt_reg + 0x1c)

#define have_openpic	(openpic_base != NULL)

static uint32_t		intr_level_mask;

#define INT_LEVEL_MASK_GC	0x3ff00000
#define INT_LEVEL_MASK_OHARE	0x1ff00000

/*
 * Map 64 irqs into 32 (bits).
 */
int
mapirq(irq)
	int irq;
{
	int v;

	if (irq < 0 || irq >= ICU_LEN)
		panic("invalid irq %d", irq);

	if (virq[irq])
		return virq[irq];

	virq_max++;
	v = virq_max;
	if (v > HWIRQ_MAX)
		panic("virq overflow");

	intrsources[v].is_hwirq = irq;
	virq[irq] = v;

	return v;
}

uint32_t
gc_read_irq()
{
	uint32_t rv = 0;
	uint32_t int_state;
	uint32_t events, e;
	uint32_t levels;

	/* Get the internal interrupts */
	int_state = in32rb(INT_STATE_REG_L);
	events = int_state & ~intr_level_mask;

	/* Get the enabled external interrupts */
	levels = in32rb(INT_LEVEL_REG_L) & in32rb(INT_ENABLE_REG_L);
	events = events | (levels & intr_level_mask);

	/* Clear any interrupts that we've read */
	out32rb(INT_CLEAR_REG_L, events | int_state);
	while (events) {
		e = 31 - cntlzw(events);
		rv |= 1 << virq[e];
		events &= ~(1 << e);
	}

	/* If we're on Heathrow, repeat for the secondary */
	if (heathrow_FCR) {
		events = in32rb(INT_STATE_REG_H);

		if (events)
			out32rb(INT_CLEAR_REG_H, events);

		while (events) {
			e = 31 - cntlzw(events);
			rv |= 1 << virq[e + 32];
			events &= ~(1 << e);
		}
	}

	/* 1 << 0 is invalid because virq will always be at least 1. */
	return rv & ~1;
}

void
gc_reenable_irq(irq)
	int irq;
{
	struct cpu_info *ci = curcpu();
	u_int levels, vi;
	u_int mask, irqbit;

	if (irq < 32) {
		mask = in32rb(INT_ENABLE_REG_L);
		irqbit = 1 << irq;

		/* Already enabled? */
		if (mask & irqbit)
			return;

		mask |= irqbit;
		out32rb(INT_ENABLE_REG_L, mask);	/* unmask */

		/* look for lost level interrupts */
		levels = in32rb(INT_LEVEL_REG_L);
		if (levels & irqbit) {
			vi = virq[irq];		/* map to virtual irq */
			ci->ci_ipending |= (1<<vi);	
			out32rb(INT_CLEAR_REG_L, irqbit);
		}
	} else {
		mask = in32rb(INT_ENABLE_REG_H);
		irqbit = 1 << (irq - 32);

		/* Already enabled? */
		if (mask & irqbit)
			return;

		mask |= irqbit;
		out32rb(INT_ENABLE_REG_H, mask);	/* unmask */

		/* look for lost level interrupts */
		levels = in32rb(INT_LEVEL_REG_H);
		if (levels & irqbit) {
			vi = virq[irq];		/* map to virtual irq */
			ci->ci_ipending |= (1<<vi);	
			out32rb(INT_CLEAR_REG_H, irqbit);
		}
	}
}

void
gc_enable_irq(irq)
	int irq;
{
	u_int mask;

	if (irq < 32) {
		mask = in32rb(INT_ENABLE_REG_L);
		mask |= 1 << irq;
		out32rb(INT_ENABLE_REG_L, mask);	/* unmask */
	} else {
		mask = in32rb(INT_ENABLE_REG_H);
		mask |= 1 << (irq - 32);
		out32rb(INT_ENABLE_REG_H, mask);	/* unmask */
	}
}

void
gc_disable_irq(irq)
	int irq;
{
	u_int x;

	if (irq < 32) {
		x = in32rb(INT_ENABLE_REG_L);
		x &= ~(1 << irq);
		out32rb(INT_ENABLE_REG_L, x);
	} else {
		x = in32rb(INT_ENABLE_REG_H);
		x &= ~(1 << (irq - 32));
		out32rb(INT_ENABLE_REG_H, x);
	}
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
void
intr_calculatemasks()
{
	int irq, level;
	struct intrsource *is;
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++) {
		register int levels = 0;
		for (q = is->is_hand; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		is->is_level = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++)
			if (is->is_level & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

	/*
	 * IPL_CLOCK should mask clock interrupt even if interrupt handler
	 * is not registered.
	 */
	imask[IPL_CLOCK] |= 1 << SPL_CLOCK;

	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = 1 << SIR_CLOCK;
	imask[IPL_SOFTNET] = 1 << SIR_NET;
	imask[IPL_SOFTSERIAL] = 1 << SIR_SERIAL;

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
	imask[IPL_VM] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_VM];

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
	for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++) {
		register int irqs = 1 << irq;
		for (q = is->is_hand; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		is->is_mask = irqs;
	}

	/* Lastly, enable IRQs actually in use. */
	if (have_openpic) {
		for (irq = 0; irq < ICU_LEN; irq++)
			openpic_disable_irq(irq);
		for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++) {
			if (is->is_hand != NULL)
				openpic_enable_irq(is->is_hwirq, is->is_type);
		}
	} else {
		out32rb(INT_ENABLE_REG_L, 0);
		if (heathrow_FCR)
			out32rb(INT_ENABLE_REG_H, 0);
		for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++) {
			if (is->is_hand)
				gc_enable_irq(is->is_hwirq);
		}
	}
}

int
fakeintr(arg)
	void *arg;
{

	return 0;
}

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < NIRQ)

const char *
intr_typename(type)
	int type;
{

	switch (type) {
        case IST_NONE :
		return "none";
        case IST_PULSE:
		return "pulsed";
        case IST_EDGE:
		return "edge-triggered";
        case IST_LEVEL:
		return "level-triggered";
	default:
		panic("intr_typename: invalid type %d", type);
	}
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(hwirq, type, level, ih_fun, ih_arg)
	int hwirq;
	int type;
	int level;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	struct intrhand **p, *q, *ih;
	struct intrsource *is;
	static struct intrhand fakehand = {fakeintr};
	int irq;

	irq = mapirq(hwirq);

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq (%d) or type (%d)", irq, type);

	is = &intrsources[irq];

	switch (is->is_type) {
	case IST_NONE:
		is->is_type = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (type == is->is_type)
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(is->is_type),
			    intr_typename(type));
		break;
	}
	if (is->is_hand == NULL) {
		snprintf(is->is_source, sizeof(is->is_source), "irq %d",
		    is->is_hwirq);
		evcnt_attach_dynamic(&is->is_ev, EVCNT_TYPE_INTR, NULL,
		    have_openpic ? "openpic" : "pic", is->is_source);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &is->is_hand; (q = *p) != NULL; p = &q->ih_next)
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
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	if (!have_openpic) {
		uint32_t msr = mfmsr();
		mtmsr(msr & ~PSL_EE);
		gc_reenable_irq(hwirq);
		mtmsr(msr);
	}

	return ih;
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(arg)
	void *arg;
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq;
	struct intrsource *is = &intrsources[irq];
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq %d", irq);

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &is->is_hand; (q = *p) != NULL && q != ih; p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (is->is_hand == NULL) {
		is->is_type = IST_NONE;
		evcnt_detach(&is->is_ev);
	}
}

#define HH_INTR_SECONDARY 0xf80000c0
#define GC_IPI_IRQ	  30
extern int cpuintr(void *);

/*
 * external interrupt handler
 */
void
ext_intr()
{
	int irq;
	int pcpl, msr, r_imen;
	struct cpu_info *ci = curcpu();
	struct intrsource *is;
	struct intrhand *ih;
	uint32_t int_state;
#if DIAGNOSTIC
	uint32_t oint_state;
	int spincount=0;
#endif

#ifdef MULTIPROCESSOR
	/* Only cpu0 can handle external interrupts. */
	if (cpu_number() != 0) {
		out32(HH_INTR_SECONDARY, ~0);
		cpuintr(NULL);
		return;
	}
#endif

	pcpl = ci->ci_cpl;
	msr = mfmsr();

#if DIAGNOSTIC
	oint_state = 0;
#endif
	while ((int_state = gc_read_irq()) != 0) {

#if DIAGNOSTIC
		/*
		 * Paranoia....
		 */
		if (int_state == oint_state) {
			if (spincount++ > 0x80) {
				uint32_t	stuck;
				const char	*comma="";

				stuck = int_state;
				printf("Disabling stuck interrupt(s): ");
				while (stuck) {
					irq = 31 - cntlzw(int_state);
					r_imen = 1 << irq;
					is = &intrsources[irq];
					printf("%s%d", comma, is->is_hwirq);
					gc_disable_irq(is->is_hwirq);
					ci->ci_ipending &= ~r_imen;
					stuck &= ~r_imen;
					comma = ", ";
				}
				printf("\n");
				spincount = 0;
				continue;
			}
		}
		oint_state = int_state;
#endif

#ifdef MULTIPROCESSOR
		r_imen = 1 << virq[GC_IPI_IRQ];
		if (int_state & r_imen) {
			int_state &= ~r_imen;
			cpuintr(NULL);
		}
#endif

		while (int_state) {
			irq = 31 - cntlzw(int_state);
			r_imen = 1 << irq;

			is = &intrsources[irq];

			int_state &= ~r_imen;

			if ((pcpl & r_imen) != 0) {
				ci->ci_ipending |= r_imen;
				gc_disable_irq(is->is_hwirq);
				continue;
			}

			/* This one is no longer pending */
			ci->ci_ipending &= ~r_imen;

			splraise(is->is_mask);
			mtmsr(msr | PSL_EE);
			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			ih = is->is_hand;
			while (ih) {
#ifdef DIAGNOSTIC
				if (!ih->ih_fun) {
					printf("NULL interrupt handler!\n");
					panic("irq %02d, hwirq %02d, is %p\n",
						irq, is->is_hwirq, is);
				}
#endif
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
			KERNEL_UNLOCK();
			mtmsr(msr);
			ci->ci_cpl = pcpl;

			/* Perhaps some interrupts arrived above */
			int_state |= (ci->ci_ipending & ~pcpl & HWIRQ_MASK);

			/* Ensure that this interrupt is enabled */
			gc_reenable_irq(is->is_hwirq);

			uvmexp.intrs++;
			is->is_ev.ev_count++;
		}
	}

	mtmsr(msr | PSL_EE);
	splx(pcpl);	/* Process pendings. */
	mtmsr(msr);
}

void
ext_intr_openpic()
{
	struct cpu_info *ci = curcpu();
	int irq, realirq;
	int pcpl, msr, r_imen;
	struct intrsource *is;
	struct intrhand *ih;

	msr = mfmsr();

#ifdef MULTIPROCESSOR
	/* Only cpu0 can handle interrupts. */
	if (cpu_number() != 0) {
		realirq = openpic_read_irq(cpu_number());
		while (realirq == IPI_VECTOR) {
			openpic_eoi(cpu_number());
			cpuintr(NULL);
			realirq = openpic_read_irq(cpu_number());
		}
		if (realirq == 255) {
			return;
		}
		panic("non-IPI intr %d on cpu%d", realirq, cpu_number());
	}
#endif

	pcpl = ci->ci_cpl;

	realirq = openpic_read_irq(0);
	if (realirq == 255) {
		/* printf("spurious interrupt\n"); */
		return;
	}

start:
#ifdef MULTIPROCESSOR
	while (realirq == IPI_VECTOR) {
		openpic_eoi(0);
		cpuintr(NULL);

		realirq = openpic_read_irq(0);
		if (realirq == 255) {
			return;
		}
	}
#endif
	irq = virq[realirq];
	KASSERT(realirq < ICU_LEN);
	r_imen = 1 << irq;
	is = &intrsources[irq];

	if ((pcpl & r_imen) != 0) {
		ci->ci_ipending |= r_imen;	/* Masked! Mark this as pending */
		openpic_disable_irq(realirq);
	} else {
		splraise(is->is_mask);
		mtmsr(msr | PSL_EE);
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		ih = is->is_hand;
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK();
		mtmsr(msr);
		ci->ci_cpl = pcpl;

		uvmexp.intrs++;
		is->is_ev.ev_count++;
	}

	openpic_eoi(0);

	realirq = openpic_read_irq(0);
	if (realirq != 255)
		goto start;

	mtmsr(msr | PSL_EE);
	splx(pcpl);	/* Process pendings. */
	mtmsr(msr);
}

static void
do_pending_int()
{
	struct cpu_info * const ci = curcpu();
	struct intrsource *is;
	struct intrhand *ih;
	int irq;
	int pcpl;
	int hwpend;
	int emsr, dmsr;

	if (ci->ci_iactive)
		return;

	ci->ci_iactive = 1;
	emsr = mfmsr();
	KASSERT(emsr & PSL_EE);
	dmsr = emsr & ~PSL_EE;
	mtmsr(dmsr);

	pcpl = ci->ci_cpl;
again:

#ifdef MULTIPROCESSOR
	if (ci->ci_cpuid == 0) {
#endif
	/* Do now unmasked pendings */
	while ((hwpend = (ci->ci_ipending & ~pcpl & HWIRQ_MASK)) != 0) {
		irq = 31 - cntlzw(hwpend);
		is = &intrsources[irq];

		ci->ci_ipending &= ~(1 << irq);
		splraise(is->is_mask);
		mtmsr(emsr);
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		ih = is->is_hand;
		while (ih) {
#ifdef DIAGNOSTIC
			if (!ih->ih_fun) {
				printf("NULL interrupt handler!\n");
				panic("irq %02d, hwirq %02d, is %p\n",
					irq, is->is_hwirq, is);
			}
#endif
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK();
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;

		is->is_ev.ev_count++;
		if (have_openpic)
			openpic_enable_irq(is->is_hwirq, is->is_type);
		else
			gc_reenable_irq(is->is_hwirq);
	}
#ifdef MULTIPROCESSOR
	}
#endif

	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_SERIAL)) {
		ci->ci_ipending &= ~(1 << SIR_SERIAL);
		splsoftserial();
		mtmsr(emsr);
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTSERIAL);
#else
		softserial();
#endif
		KERNEL_UNLOCK();
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softserial.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_NET)) {
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		int pisr;
#endif
		ci->ci_ipending &= ~(1 << SIR_NET);
		splsoftnet();
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		pisr = netisr;
		netisr = 0;
#endif
		mtmsr(emsr);
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTNET);
#else
		softnet(pisr);
#endif
		KERNEL_UNLOCK();
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softnet.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_CLOCK)) {
		ci->ci_ipending &= ~(1 << SIR_CLOCK);
		splsoftclock();
		mtmsr(emsr);
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTCLOCK);
#else
		softclock(NULL);
#endif
		KERNEL_UNLOCK();
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softclock.ev_count++;
		goto again;
	}

	ci->ci_cpl = pcpl;	/* Don't use splx... we are here already! */
	ci->ci_iactive = 0;
	mtmsr(emsr);
}

int
splraise(ncpl)
	int ncpl;
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* don't reorder.... */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ocpl | ncpl;
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

void
splx(ncpl)
	int ncpl;
{

	struct cpu_info *ci = curcpu();
	__asm volatile("sync; eieio");	/* reorder protect */
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(ncpl)
	int ncpl;
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* reorder protect */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
void
softintr(ipl)
	int ipl;
{
	int msrsave;

	msrsave = mfmsr();
	mtmsr(msrsave & ~PSL_EE);
	curcpu()->ci_ipending |= 1 << ipl;
	mtmsr(msrsave);
}

void
openpic_init()
{
	int irq;
	u_int x;

	/* disable all interrupts */
	for (irq = 0; irq < 256; irq++)
		openpic_write(OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);

	openpic_set_priority(0, 15);

	/* we don't need 8259 pass through mode */
	x = openpic_read(OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);

	/* send all interrupts to CPU 0 */
	for (irq = 0; irq < ICU_LEN; irq++)
		openpic_write(OPENPIC_IDEST(irq), 1 << 0);

	for (irq = 0; irq < ICU_LEN; irq++) {
		x = irq;
		x |= OPENPIC_IMASK;
		x |= OPENPIC_POLARITY_POSITIVE;
		x |= OPENPIC_SENSE_LEVEL;
		x |= 8 << OPENPIC_PRIORITY_SHIFT;
		openpic_write(OPENPIC_SRC_VECTOR(irq), x);
	}

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < 256; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		openpic_disable_irq(irq);

#ifdef MULTIPROCESSOR
	x = openpic_read(OPENPIC_IPI_VECTOR(1));
	x &= ~(OPENPIC_IMASK | OPENPIC_PRIORITY_MASK | OPENPIC_VECTOR_MASK);
	x |= (15 << OPENPIC_PRIORITY_SHIFT) | IPI_VECTOR;
	openpic_write(OPENPIC_IPI_VECTOR(1), x);
#endif

	/* XXX set spurious intr vector */

	oea_install_extint(ext_intr_openpic);
}

void
legacy_int_init()
{
	int	ohare;

	out32rb(INT_ENABLE_REG_L, 0);		/* disable all intr. */
	out32rb(INT_CLEAR_REG_L, 0xffffffff);	/* clear pending intr. */
	intr_level_mask = INT_LEVEL_MASK_GC;
	if (heathrow_FCR) {
		out32rb(INT_ENABLE_REG_H, 0);
		out32rb(INT_CLEAR_REG_H, 0xffffffff);
		intr_level_mask = INT_LEVEL_MASK_OHARE;
	}
	ohare = OF_finddevice("/bandit/ohare");
	if (ohare != -1) {
		intr_level_mask = INT_LEVEL_MASK_OHARE;
	}

	oea_install_extint(ext_intr);
}

#define HEATHROW_FCR_OFFSET	0x38		/* XXX should not here */
#define GC_OBIO_BASE		0xf3000000

void
init_interrupt()
{
	int chosen;
	int mac_io, reg[5];
	int32_t ictlr;
	char type[32];

	mac_io = OF_finddevice("mac-io");
	if (mac_io == -1)
		mac_io = OF_finddevice("/pci/mac-io");

	if (mac_io == -1) {
		/*
		 * No mac-io.  Assume Grand-Central or OHare.
		 */
		obio_base = (void *)GC_OBIO_BASE;
		legacy_int_init();
		return;
	}

	if (OF_getprop(mac_io, "assigned-addresses", reg, sizeof(reg)) < 20)
		goto failed;

	obio_base = (void *)reg[2];
	heathrow_FCR = (void *)(obio_base + HEATHROW_FCR_OFFSET);

	memset(type, 0, sizeof(type));
	ictlr = -1;

	/* 
	 * Look for The interrupt controller. It used to be referenced
	 * by the "interrupt-controller" property of device "/chosen",
	 * but this is not true anymore on newer machines (e.g.: iBook G4)
	 * Device "mpic" is the interrupt controller on theses machines.
	 */
	if (((chosen = OF_finddevice("/chosen")) == -1)  ||
	    (OF_getprop(chosen, "interrupt-controller", &ictlr, 4) != 4))
		ictlr = OF_finddevice("mpic");

	OF_getprop(ictlr, "device_type", type, sizeof(type));

	if (strcmp(type, "open-pic") != 0) {
		/*
		 * Not an open-pic.  Must be a Heathrow (compatible).
		 */
		legacy_int_init();
		return;
	} else {
		/*
		 * We have an Open PIC.
		 */
		if (OF_getprop(ictlr, "reg", reg, sizeof(reg)) < 8)
			goto failed;

		openpic_base = (void *)(obio_base + reg[0]);
		openpic_init();
		return;
	}

	printf("unknown interrupt controller\n");
failed:
	panic("init_interrupt: failed to initialize interrupt controller");
}
