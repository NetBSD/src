/*	$NetBSD: extintr.c,v 1.6 1999/01/12 12:06:46 tsubai Exp $	*/

/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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
#include <sys/param.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/pio.h>

unsigned int imen = 0xffffffff;
volatile int cpl, ipending, astpending, tickspending;
int imask[NIPL];
u_char *interrupt_reg;

void intr_calculatemasks __P((void));
char *intr_typename __P((int));
int fakeintr __P((void *));

int intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

extern u_int *heathrow_FCR;

static __inline int cntlzw __P((int));
static int read_irq __P((void));
static void enable_irq __P((int));
static int mapirq __P((int));

static int hwirq[ICU_LEN], virq[64];
static int virq_max = 0;

#define HWIRQ_MAX 27
#define HWIRQ_MASK 0x0fffffff

#define INT_STATE_REG0  (interrupt_reg + 0x20)
#define INT_ENABLE_REG0 (interrupt_reg + 0x24)
#define INT_CLEAR_REG0  (interrupt_reg + 0x28)
#define INT_LEVEL_REG0  (interrupt_reg + 0x2c)
#define INT_STATE_REG1  (INT_STATE_REG0  - 0x10)
#define INT_ENABLE_REG1 (INT_ENABLE_REG0 - 0x10)
#define INT_CLEAR_REG1  (INT_CLEAR_REG0  - 0x10)
#define INT_LEVEL_REG1  (INT_LEVEL_REG0  - 0x10)

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
		register int irqs = 1 << irq;
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrhand[irq])
				irqs |= 1 << irq;
		imen = ~irqs;
		enable_irq(~imen);
	}
}

int
fakeintr(arg)
	void *arg;
{

	return 0;
}

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN)

char *
intr_typename(type)
	int type;
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
		panic("intr_typename: invalid type %d", type);
#if 1 /* XXX */
		return ("unknown");
#endif
	}
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(irq, type, level, ih_fun, ih_arg)
	int irq;
	int type;
	int level;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = {fakeintr};
	extern int cold;

	irq = mapirq(irq);

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	switch (intrtype[irq]) {
	case IST_NONE:
		intrtype[irq] = type;
		break;
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

	return (ih);
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
 * Count leading zeros.
 */
static __inline int
cntlzw(x)
	int x;
{
	int a;

	__asm __volatile ("cntlzw %0,%1" : "=r"(a) : "r"(x));

	return a;
}

/*
 * external interrupt handler
 */
void
ext_intr()
{
	int i, irq = 0;
	int o_imen, r_imen;
	int pcpl;
	struct intrhand *ih;
	volatile unsigned long int_state;

	pcpl = splhigh();	/* Turn off all */

	int_state = read_irq();
	if (int_state == 0)
		goto out;

start:
	irq = 31 - cntlzw(int_state);

	o_imen = imen;
	r_imen = 1 << irq;

	if ((pcpl & r_imen) != 0) {
		ipending |= r_imen;	/* Masked! Mark this as pending */
		imen |= r_imen;
		enable_irq(~imen);
	} else {
		ih = intrhand[irq];
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

#if defined(UVM)
		uvmexp.intrs++;
#endif
		intrcnt[hwirq[irq]]++;
	}
	int_state &= ~r_imen;
	if (int_state)
		goto start;

out:
	splx(pcpl);	/* Process pendings. */
}

void
do_pending_int()
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

	pcpl = splhigh();		/* Turn off all */
	hwpend = ipending & ~pcpl;	/* Do now unmasked pendings */
	imen &= ~hwpend;
	enable_irq(~imen);
	while (hwpend) {
		irq = 31 - cntlzw(hwpend);
		hwpend &= ~(1L << irq);
		ih = intrhand[irq];
		while(ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		intrcnt[hwirq[irq]]++;
	}

	/*out32rb(INT_ENABLE_REG, ~imen);*/

	if ((ipending & ~pcpl) & (1 << SIR_CLOCK)) {
		ipending &= ~(1 << SIR_CLOCK);
		softclock();
		intrcnt[CNT_SOFTCLOCK]++;
	}
	if ((ipending & ~pcpl) & (1 << SIR_NET)) {
		ipending &= ~(1 << SIR_NET);
		softnet();
		intrcnt[CNT_SOFTNET]++;
	}
	if ((ipending & ~pcpl) & (1 << SIR_SERIAL)) {
		ipending &= ~(1 << SIR_SERIAL);
		softserial();
		intrcnt[CNT_SOFTSERIAL]++;
	}
	ipending &= pcpl;
	cpl = pcpl;	/* Don't use splx... we are here already! */
	asm volatile("mtmsr %0" :: "r"(emsr));
	processing = 0;
}

/*
 * Map 64 irqs into 32 (bits).
 */
int
mapirq(irq)
	int irq;
{
	int v;

	if (irq < 0 || irq >= 64)
		panic("invalid irq");
	virq_max++;
	v = virq_max;
	if (v > HWIRQ_MAX)
		panic("virq overflow");

	hwirq[v] = irq;
	virq[irq] = v;

	return v;
}

int
read_irq()
{
	int rv = 0;
	int state0, state1, p;

	state0 = in32rb(INT_STATE_REG0);
	if (state0)
		out32rb(INT_CLEAR_REG0, state0);
	while (state0) {
		p = 31 - cntlzw(state0);
		rv |= 1 << virq[p];
		state0 &= ~(1 << p);
	}

	if (heathrow_FCR)			/* has heathrow? */
		state1 = in32rb(INT_STATE_REG1);
	else
		state1 = 0;

	if (state1)
		out32rb(INT_CLEAR_REG1, state1);
	while (state1) {
		p = 31 - cntlzw(state1);
		rv |= 1 << virq[p + 32];
		state1 &= ~(1 << p);
	}

	/* 1 << 0 is invalid. */
	return rv & ~1;
}

void
enable_irq(x)
	int x;
{
	int state0, state1, v;
	int irq;

	x &= HWIRQ_MASK;	/* XXX Higher bits are software interrupts. */

	state0 = state1 = 0;
	while (x) {
		v = 31 - cntlzw(x);
		irq = hwirq[v];
		if (irq < 32)
			state0 |= 1 << irq;
		else
			state1 |= 1 << (irq - 32);
		x &= ~(1 << v);
	}

	out32rb(INT_ENABLE_REG0, state0);
	if (heathrow_FCR)
		out32rb(INT_ENABLE_REG1, state1);
}
