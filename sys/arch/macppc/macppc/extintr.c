/*	$NetBSD: extintr.c,v 1.62 2006/10/13 19:48:41 macallan Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: extintr.c,v 1.62 2006/10/13 19:48:41 macallan Exp $");

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
#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < NIRQ)

#if 0
#define LPIC_DEBUG
#endif

static void		intr_calculatemasks(void);
static int		fakeintr(void *);

static inline uint32_t	gc_read_irq(void);
static inline int	mapirq(uint32_t);
static void		gc_enable_irq(uint32_t);
static void		gc_reenable_irq(uint32_t);
static void		gc_disable_irq(uint32_t);

static int	lpic_add(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
static uint32_t	lpic_read_events(int);
static void	lpic_enable_irq(int, uint32_t);
static void	lpic_disable_irq(int, uint32_t);
static void	lpic_disable_all(void);
static void	lpic_clear_all(void);
#ifdef DIAGNOSTIC
static void	lpic_dump(int);
#endif

static void	do_pending_int(void);
static void	legacy_int_init(void);

static void	ext_intr_openpic(void);

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

extern u_int *heathrow_FCR;

typedef struct __lpic_regs {
	uint32_t	lpic_state;
	uint32_t	lpic_enable;
	uint32_t	lpic_clear;
	uint32_t	lpic_level;
	uint32_t	lpic_level_mask;
	uint32_t	lpic_enable_mask;
	uint32_t	lpic_cascade_mask;
} lpic_regs;

static lpic_regs lpics[4]; 

#define IPI_VECTOR 64
#define have_openpic	(openpic_base != NULL)

#define INT_STATE_REG_H		0x10
#define INT_ENABLE_REG_H	0x14
#define INT_CLEAR_REG_H		0x18
#define INT_LEVEL_REG_H		0x1c
#define INT_STATE_REG_L		0x20
#define INT_ENABLE_REG_L	0x24
#define INT_CLEAR_REG_L		0x28
#define INT_LEVEL_REG_L		0x2c

#define HEATHROW_FCR_OFFSET	0x38		/* XXX should not here */

#define HH_INTR_SECONDARY	0xf80000c0
#define GC_OBIO_BASE		0xf3000000
#define GC_IPI_IRQ		30

static uint32_t		num_hw_irqs = 0;
static uint32_t		num_lpics = 0;

#define INT_LEVEL_MASK_GC	0x3ff00000
#define INT_LEVEL_MASK_OHARE	0x1ff00000	/* also for Heathrow */

static int
lpic_add(uint32_t state, uint32_t enable, uint32_t clear, uint32_t level, 
    uint32_t mask)
{
	lpic_regs *this;
	
	if (num_lpics >= 4)
		return -1;
	this = &lpics[num_lpics];
	this->lpic_state = state;
	this->lpic_enable = enable;
	this->lpic_clear = clear;
	this->lpic_level = level;
	this->lpic_level_mask = mask;
	this->lpic_cascade_mask = 0;
	this->lpic_enable_mask = 0;
	num_lpics++;
	num_hw_irqs += 32;
	return 0;
}

static inline uint32_t
lpic_read_events(int num)
{
	lpic_regs *this = &lpics[num];
	uint32_t irqs, levels, events;
	
	irqs = in32rb(this->lpic_state);
	events = irqs & ~this->lpic_level_mask;

	levels = in32rb(this->lpic_level) & this->lpic_enable_mask;
	events |= levels & this->lpic_level_mask;
	
	/* Clear any interrupts that we've read */
	out32rb(this->lpic_clear, events | irqs);

	/* don't return cascade interrupts */
	events &= ~this->lpic_cascade_mask;
#ifdef LPIC_DEBUG
	printf("lpic %d: %08x %08x %08x ", num, irqs, levels, events);
#endif
	return events;
}

static inline void
lpic_enable_irq(int num, uint32_t irq)
{
	lpic_regs *this = &lpics[num];

#ifdef DIAGNOSTIC	
	if (irq > 31)
		printf("bogus irq %d\n", irq);
#endif
	this->lpic_enable_mask |= (1 << irq) | this->lpic_cascade_mask;
	out32rb(this->lpic_enable, this->lpic_enable_mask);	/* unmask */
#ifdef LPIC_DEBUG
	printf("enabling %d %d %08x ", num, irq, mask);
#endif
}

static void
lpic_disable_irq(int num, uint32_t irq)
{
	lpic_regs *this = &lpics[num];

#ifdef DIAGNOSTIC
	if (irq > 31)
		printf("bogus irq %d\n", irq);
#endif
	this->lpic_enable_mask &= ~(1 << irq);
	out32rb(this->lpic_enable, this->lpic_enable_mask);	/* unmask */
}

#ifdef DIAGNOSTIC
static void
lpic_dump(int num)
{
	lpic_regs *this = &lpics[num];

	printf("lpic %d:\n", num);
	printf("state:      %08x\n", in32rb(this->lpic_state));
	printf("enable:     %08x\n", in32rb(this->lpic_enable));
	printf("clear:      %08x\n", in32rb(this->lpic_clear));
	printf("level:      %08x\n", in32rb(this->lpic_level));
	printf("level_mask: %08x\n", this->lpic_level_mask);
	printf("cascade:    %08x\n", this->lpic_cascade_mask);
}
#endif

static void
lpic_disable_all(void)
{
	int i;
	
	for (i = 0; i < num_lpics; i++) {
		lpics[i].lpic_enable_mask = 0;
		out32rb(lpics[i].lpic_enable, 0);
	}
}

static void
lpic_clear_all(void)
{
	int i;
	
	for (i = 0; i < num_lpics; i++)
		out32rb(lpics[i].lpic_clear, 0xffffffff);
}
	
/*
 * Map 64 irqs into 32 (bits).
 */
static int
mapirq(uint32_t irq)
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
#ifdef LPIC_DEBUG
	printf("mapping irq %d to virq %d\n", irq, v);
#endif
	return v;
}

static uint32_t
gc_read_irq(void)
{
	uint32_t rv = 0;
	uint32_t e;
	uint32_t events;
	int i;

	for (i = 0; i < num_lpics; i++) {
		events = lpic_read_events(i);
		while (events) {
			e = 31 - cntlzw(events);
			rv |= 1 << virq[e + (i << 5)];
			events &= ~(1 << e);
		}
	}

#ifdef LPIC_DEBUG
	printf("rv: %08x ", rv);
#endif	
	/* 1 << 0 is invalid because virq will always be at least 1. */
	return rv & ~1;
}

static void
gc_reenable_irq(uint32_t irq)
{
	lpic_regs *this;
	struct cpu_info *ci = curcpu();
	uint32_t levels, irqbit, vi;
	uint32_t pic;

	pic = irq >> 5;
	this = &lpics[pic];
	
	irqbit = 1 << (irq & 0x1f);

	/* Already enabled? */
	if (this->lpic_enable_mask & irqbit)
		return;

	lpic_enable_irq(pic, irq & 0x1f);
	
	/* look for lost level interrupts */
	levels = in32rb(this->lpic_level);
	if (levels & irqbit) {
		vi = virq[irq];	/* map to virtual irq */
		ci->ci_ipending |= (1 << vi);	
		out32rb(this->lpic_clear, irqbit);
	}
}

static void
gc_enable_irq(uint32_t irq)
{
	uint32_t pic;
	
	pic = (irq >> 5);
	lpic_enable_irq(pic, irq & 0x1f);
}

static void
gc_disable_irq(uint32_t irq)
{
	uint32_t pic;
	
	pic = (irq >> 5);
	lpic_disable_irq(pic, irq & 0x1f);
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
		lpic_disable_all();
		for (irq = 0, is = intrsources; irq < NIRQ; irq++, is++) {
			if (is->is_hand)
				gc_enable_irq(is->is_hwirq);
		}
	}
}

static int
fakeintr(void *arg)
{

	return 0;
}

const char *
intr_typename(int type)
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
intr_establish(int hwirq, int type, int level, int (*ih_fun)(void *),
    void *ih_arg)
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
intr_disestablish(void *arg)
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

extern int cpuintr(void *);

/*
 * external interrupt handler
 */
void
ext_intr(void)
{
	int irq;
	int pcpl, msr, r_imen;
	struct cpu_info *ci = curcpu();
	struct intrsource *is;
	struct intrhand *ih;
	uint32_t int_state;
#ifdef DIAGNOSTIC
	int intr_spin = 0;
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

	while ((int_state = gc_read_irq()) != 0) {

#ifdef DIAGNOSTIC
		if (intr_spin > 100) {
			int i;
			printf("spinning in ext_intr! %08x\n", int_state);
			for (i = 0; i < num_lpics; i++) {
				lpic_dump(i);
			}
			panic("stuck interrupt");
			continue;
		}
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
#ifdef DIAGNOSTIC
		intr_spin++;
#endif
	}

	mtmsr(msr | PSL_EE);
	splx(pcpl);	/* Process pendings. */
	mtmsr(msr);
}

void
ext_intr_openpic(void)
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
		ci->ci_ipending |= r_imen; /* Masked! Mark this as pending */
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
do_pending_int(void)
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
splraise(int ncpl)
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
splx(int ncpl)
{

	struct cpu_info *ci = curcpu();
	__asm volatile("sync; eieio");	/* reorder protect */
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(int ncpl)
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
softintr(int ipl)
{
	int msrsave;

	msrsave = mfmsr();
	mtmsr(msrsave & ~PSL_EE);
	curcpu()->ci_ipending |= 1 << ipl;
	mtmsr(msrsave);
}

void
openpic_init(void)
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
legacy_int_init(void)
{
	uint32_t reg[5];
	uint32_t obio_base;
	int	ohare, ohare2;

	ohare = OF_finddevice("/bandit/ohare");
	if (ohare != -1) {
		if (OF_getprop(ohare, "assigned-addresses", reg, sizeof(reg))
		    == 20) {
			obio_base = reg[2];
			printf("found OHare at 0x%08x\n", obio_base);
			lpic_add(obio_base + INT_STATE_REG_L,
			    obio_base + INT_ENABLE_REG_L,
			    obio_base + INT_CLEAR_REG_L,
			    obio_base + INT_LEVEL_REG_L,
			    INT_LEVEL_MASK_OHARE);
		}
	} else {
		/* must be Grand Central */
		printf("found Grand Central at 0x%08x\n", GC_OBIO_BASE);
		lpic_add(GC_OBIO_BASE + INT_STATE_REG_L,
		    GC_OBIO_BASE + INT_ENABLE_REG_L,
		    GC_OBIO_BASE + INT_CLEAR_REG_L,
		    GC_OBIO_BASE + INT_LEVEL_REG_L,
		    INT_LEVEL_MASK_GC);
		return;
	}

	ohare2 = OF_finddevice("/bandit/pci106b,7");
	if (ohare2 != -1) {
		uint32_t irq;
		
		if (OF_getprop(ohare2, "assigned-addresses", reg, sizeof(reg))
		    < 20)
		    	goto next;
		if (OF_getprop(ohare2, "AAPL,interrupts", &irq, sizeof(irq))
		    < 4)
		    	goto next;
		obio_base = reg[2];			
		printf("found OHare2 at 0x%08x, irq %d\n", obio_base, irq);
		lpic_add(obio_base + INT_STATE_REG_L,
		    obio_base + INT_ENABLE_REG_L,
		    obio_base + INT_CLEAR_REG_L,
		    obio_base + INT_LEVEL_REG_L,
		    INT_LEVEL_MASK_OHARE);
		lpics[0].lpic_cascade_mask = 1 << irq;
	}
next:	
	lpic_disable_all();
	lpic_clear_all();
	
	printf("Handling %d interrupts\n", num_hw_irqs);
	
	oea_install_extint(ext_intr);
}

void
init_interrupt(void)
{
	uint32_t obio_base;
	int chosen __attribute__((unused));
	int mac_io, reg[5];
	int32_t ictlr;
	char type[32];

	mac_io = OF_finddevice("mac-io");
	if (mac_io == -1)
		mac_io = OF_finddevice("/pci/mac-io");

	if (mac_io == -1)
		mac_io = OF_finddevice("/ht/pci/mac-io");

#if !defined (MAMBO)
	if (mac_io == -1) {
		/*
		 * No mac-io.  Assume Grand-Central or OHare.
		 */
		legacy_int_init();
		return;
	}

	if (OF_getprop(mac_io, "assigned-addresses", reg, sizeof(reg)) < 20)
		goto failed;

	obio_base = reg[2];

	heathrow_FCR = (void *)(obio_base + HEATHROW_FCR_OFFSET);
		
#endif /* MAMBO */

	memset(type, 0, sizeof(type));
	ictlr = -1;

	/* 
	 * Look for The interrupt controller. It used to be referenced
	 * by the "interrupt-controller" property of device "/chosen",
	 * but this is not true anymore on newer machines (e.g.: iBook G4)
	 * Device "mpic" is the interrupt controller on theses machines.
	 */
#if defined(MAMBO)
	if ((ictlr = OF_finddevice("/interrupt-controller")) == -1)
		goto failed;
#elif defined (PMAC_G5)
	if ((ictlr = OF_finddevice("macio-mpic")) == -1)
		goto failed;
#else
	if (((chosen = OF_finddevice("/chosen")) == -1)  ||
	 	(OF_getprop(chosen, "interrupt-controller", &ictlr, 4) != 4)) {
			ictlr = OF_finddevice("mpic");
	}
#endif
	OF_getprop(ictlr, "device_type", type, sizeof(type));

	if (strcmp(type, "open-pic") != 0) {
		/*
		 * Not an open-pic.  Must be a Heathrow (compatible).
		 */
		printf("Found Heathrow at 0x%08x\n", obio_base);
		lpic_add(obio_base + INT_STATE_REG_L,
			obio_base + INT_ENABLE_REG_L,
			obio_base + INT_CLEAR_REG_L,
			obio_base + INT_LEVEL_REG_L,
			INT_LEVEL_MASK_OHARE);

		lpic_add(obio_base + INT_STATE_REG_H,
			obio_base + INT_ENABLE_REG_H,
			obio_base + INT_CLEAR_REG_H,
			obio_base + INT_LEVEL_REG_H,
			0);

		lpic_disable_all();
		lpic_clear_all();
		oea_install_extint(ext_intr);
		printf("Handling %d interrupts\n", num_hw_irqs);

		return;
	} else {
		/*
		 * We have an Open PIC.
		 */
		if (OF_getprop(ictlr, "reg", reg, sizeof(reg)) < 8) {
#ifdef MAMBO
			reg[0] = 0xF8040000;
#else
			goto failed;
#endif
		}
#if defined (PPC_OEA)
		openpic_base = (void *)(obio_base + reg[0]);
#elif defined (PPC_OEA64_BRIDGE)
		/* There is no BAT mapping the OBIO region, use PTEs */
		openpic_base = (void *)mapiodev((u_int32_t)obio_base + 
		    (u_int32_t)reg[0], 0x40000);
#endif
		printf("%s: found OpenPIC @ pa 0x%08x, %p\n", __FUNCTION__, 
		    (u_int32_t)obio_base + (u_int32_t)reg[0], openpic_base);
		openpic_init();
		return;
	}
	printf("unknown interrupt controller\n");
failed:
	panic("init_interrupt: failed to initialize interrupt controller");
}
