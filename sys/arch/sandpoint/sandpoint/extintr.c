/*	$NetBSD: extintr.c,v 1.2.6.2 2002/06/23 17:40:01 jdolecek Exp $	*/

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

#include "opt_openpic.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>

#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/psl.h>

#include <powerpc/openpic.h>

#include "isa.h"
#if (NISA > 0)
#include <machine/isa_machdep.h>
#endif
#include "com.h"
#if NCOM > 0
extern void comsoft __P((void));
#endif

unsigned int imen = 0xffffffff;
volatile int cpl, ipending, tickspending;
int imask[NIPL];
int intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

void intr_calculatemasks __P((void));
int fakeintr __P((void *));
void ext_intr __P((void));
char *intr_typename __P((int));

int
fakeintr(arg)
	void *arg;
{
	return 0;
}

int sandpoint_icpl = 0;

/*
 * external interrupt handler
 */
void
ext_intr()
{
	int intvec, intbit, pcpl;
	struct intrhand *ih;
	extern long intrcnt[];

	pcpl = splhigh();	/* Turn off all */
	intvec = openpic_read_irq(0);	/* Get interrupt vector from EPIC */
	if (intvec == 255)
		goto out;
	do {
		intbit = 1 << intvec;

		if (pcpl & intbit) {
			/* Interrupt is masked -- mark it as pending */
			ipending |= intbit;
			openpic_eoi(0);
			openpic_disable_irq(intvec);
		} else {
			ih = intrhand[intvec];
			while (ih) {
				sandpoint_icpl = pcpl;
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}

			uvmexp.intrs++;
			intrcnt[intvec]++;

			openpic_eoi(0);
		}

		intvec = openpic_read_irq(0);

	} while (intvec != 255);

out:
	splx(pcpl);/* Will also process pendings if necessary */
}

char *
intr_typename(type)
	int	type;
{
	switch (type) {
	case IST_NONE:
		return "none";
	case IST_PULSE:
		return "pulsed";
	case IST_EDGE:
		return "edge-triggered";
	case IST_LEVEL:
		return "level-triggered";
	default:
		panic("intr_typename: invalid type %d", type);
		return "unknown";
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

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("isa_intr_establish: can't malloc handler info");

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
		imask[level] = irqs | SINT_MASK;
	}

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = SINT_SERIAL;
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
	imask[IPL_CLOCK] |= SPL_CLOCK;		/* block the clock */
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

	{
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrhand[irq])
				irqs |= 1 << irq;

		for (irq = 0; irq < ICU_LEN ; irq++) {
			if (irqs & (1 << irq)) {
				openpic_enable_irq(irq, intrtype[irq]);
			} else {
				openpic_disable_irq(irq);
			}
		}
	}
#if (NISA > 0)
	isa_intr_calculate_masks(0);
#endif
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
	extern long intrcnt[];

	if (processing != 0) {
		return;
	}

	processing = 1;
	asm volatile("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile("mtmsr %0" :: "r"(dmsr));

	pcpl = splhigh();		/* Turn off all */
	hwpend = ipending & ~pcpl;	/* Do now unmasked pendings */
	hwpend &= ICU_MASK;		/* Except soft ints */
	ipending &= ~hwpend;
	imen &= ~hwpend;
	while (hwpend) {
		irq = ffs(hwpend) - 1;
		hwpend &= ~(1L << irq);
		ih = intrhand[irq];
		while(ih) {
			sandpoint_icpl = pcpl;
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		intrcnt[irq]++;
		openpic_enable_irq(irq, intrtype[irq]);
	}

	if ((ipending & ~pcpl) & SINT_CLOCK) {
		ipending &= ~SINT_CLOCK;
		softclock(NULL);
		intrcnt[CNT_SINT_CLOCK]++;
	}
	if ((ipending & ~pcpl) & SINT_NET) {
		extern int netisr;
		void softnet(int);
		int pisr = netisr;
		netisr = 0;
		ipending &= ~SINT_NET;
		softnet(pisr);
		intrcnt[CNT_SINT_NET]++;
	}
	if ((ipending & ~pcpl) & SINT_SERIAL) {
		ipending &= ~SINT_SERIAL;
#if NCOM > 0
		comsoft();
#endif
		intrcnt[CNT_SINT_SERIAL]++;
	}
	if ((ipending & ~pcpl) & SINT_ISA) {
		ipending &= ~SINT_ISA;
#if NISA > 0
		isa_do_pending_int(pcpl);
#endif
		intrcnt[CNT_SINT_ISA]++;
	}
	cpl = pcpl;	/* Don't use splx... we are here already! */
	processing = 0;
	asm volatile("mtmsr %0" :: "r"(emsr));
}

void
openpic_init(unsigned char *base)
{
	int irq, maxirq;
	u_int x;

	openpic_base = (volatile unsigned char *) base;

	x = openpic_read(OPENPIC_FEATURE);
	maxirq = (x >> 16) & 0x7ff;

	/* disable all interrupts */
	for (irq = 0; irq < maxirq; irq++)
		openpic_write(OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);

	openpic_set_priority(0, 15);

	/* we don't need 8259 pass through mode */
	x = openpic_read(OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);

#ifdef OPENPIC_SERIAL_MODE
	x = openpic_read(OPENPIC_ICR);
	x &= ~OPENPIC_ICR_SERIAL_RATIO_MASK;
	x |= 4 << OPENPIC_ICR_SERIAL_RATIO_SHIFT;
	x |= OPENPIC_ICR_SERIAL_MODE;
	openpic_write(OPENPIC_ICR, x);
#endif

	/* send all interrupts to cpu 0 */
	for (irq = 0; irq < ICU_LEN; irq++)
		openpic_write(OPENPIC_IDEST(irq), 1 << 0);

	for (irq = 0; irq < ICU_LEN; irq++) {
		x = OPENPIC_INIT_SRC(irq);
		openpic_write(OPENPIC_SRC_VECTOR(irq), x);
	}

	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < maxirq; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		openpic_disable_irq(irq);
}
