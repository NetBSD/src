/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	from: @(#)isa.c	7.2 (Berkeley) 5/13/91
 *	$Id: intr.c,v 1.21 1994/05/05 16:05:57 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <machine/cpufunc.h>

#include <i386/isa/isareg.h>
#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>

#define	IDTVEC(name)	__CONCAT(X,name)
/* default interrupt vector table entries */
extern IDTVEC(wild), IDTVEC(intr)[], IDTVEC(fast)[];

/*
 * Fill in default interrupt table (in case of spuruious interrupt
 * during configuration of kernel, setup interrupt control unit
 */
void
isa_defaultirq()
{
	int i;

	imask[IPL_BIO] |= SIR_CLOCKMASK;
	imask[IPL_NET] |= SIR_NETMASK;
	imask[IPL_TTY] |= SIR_TTYMASK;
	imask[IPL_CLOCK] |= SIR_CLOCKMASK;

	/* out of range vectors */
	for (i = NRSVIDT; i < NIDT; i++)
		setidt(i, &IDTVEC(wild), SDT_SYS386IGT, SEL_KPL);

	/* icu vectors */
	for (i = 0; i < ICU_LEN; i++)
		setidt(ICU_OFFSET + i, IDTVEC(intr)[i],  SDT_SYS386IGT, SEL_KPL);
  
	/* initialize 8259's */
	outb(IO_ICU1, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU1+1, ICU_OFFSET);	/* starting at this vector index */
	outb(IO_ICU1+1, IRQ_SLAVE);	/* slave on line 2 */
#ifdef AUTO_EOI_1
	outb(IO_ICU1+1, 2 | 1);		/* auto EOI, 8086 mode */
#else
	outb(IO_ICU1+1, 1);		/* 8086 mode */
#endif
	outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU1, 0x68);		/* special mask mode (if available) */
	outb(IO_ICU1, 0x0a);		/* Read IRR by default. */
#ifdef REORDER_IRQ
	outb(IO_ICU1, 0xc0 | (3 - 1));	/* pri order 3-7, 0-2 (com2 first) */
#endif

	outb(IO_ICU2, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU2+1, ICU_OFFSET+8);	/* staring at this vector index */
	outb(IO_ICU2+1, ffs(IRQ_SLAVE)-1);
#ifdef AUTO_EOI_2
	outb(IO_ICU2+1, 2 | 1);		/* auto EOI, 8086 mode */
#else
	outb(IO_ICU2+1, 1);		/* 8086 mode */
#endif
	outb(IO_ICU2+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU2, 0x68);		/* special mask mode (if available) */
	outb(IO_ICU2, 0x0a);		/* Read IRR by default. */

	splhigh();
	enable_intr();
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
int
isa_nmi()
{

	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return(0);
}

/*
 * Caught a stray interrupt, notify
 */
void
isa_strayintr(irq)
	int irq;
{
	static u_long strays, wilds;

        /*
         * Stray interrupts on irq 7 occur when an interrupt line is raised
         * and then lowered before the CPU acknowledges it.  This generally
         * means either the device is screwed or something is cli'ing too
         * long and it's timing out.
         *
         * -1 is passed by the generic handler for out of range exceptions,
         * since we don't really want 208 little vectors just to get the
	 * message right.  (It wouldn't be all that much code, but why bother?)
         */
	if (irq == -1) {
		++wilds;
		log(LOG_ERR, "wild interrupt\n");
	} else {
		if (++strays <= 5)
			log(LOG_ERR, "stray interrupt %d%s\n", irq,
			    strays >= 5 ? "; stopped logging" : "");
	}
}

int fastvec;
int intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

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
			if (q->ih_level != IPL_NONE)
				levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < 4; level++) {
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		/* Preserve any softintr dependencies we set up earlier. */
		imask[level] = (imask[level] & -(1 << ICU_LEN)) | irqs;
	}

#include "sl.h"
#include "ppp.h"
#if NSL > 0 || NPPP > 0
	/* In the presence of SLIP or PPP, splimp > spltty. */
	imask[IPL_NET] |= imask[IPL_TTY];
#endif

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int irqs = 1 << irq;
		for (q = intrhand[irq]; q; q = q->ih_next)
			if (q->ih_level != IPL_NONE)
				irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrhand[irq])
				irqs |= 1 << irq;
		if (irqs >= 0x100) /* any IRQs >= 8 in use */
			irqs |= IRQ_SLAVE;
		imen = ~irqs;
		SET_ICUS();
	}
}

int
fakeintr(arg)
	void *arg;
{

	return 0;
}

/*
 * Set up an interrupt handler to start being called.
 */
void
intr_establish(mask, ih)
	int mask;
	struct intrhand *ih;
{
	int irq;
	struct intrhand **p, *q;
	static struct intrhand fakehand = {fakeintr};

	irq = ffs(mask) - 1;

	if (irq < 0 || irq > ICU_LEN)
		panic("intr_establish: bogus irq");
	if (fastvec & mask)
		panic("intr_establish: irq is already fast vector");

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
	fakehand.ih_level = ih->ih_level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_count = 0;
	ih->ih_next = NULL;
	*p = ih;
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(mask, ih)
	int mask;
	struct intrhand *ih;
{
	int irq;
	struct intrhand **p, *q;

	irq = ffs(mask) - 1;

	if (irq < 0 || irq > ICU_LEN)
		panic("intr_disestablish: bogus irq");
	if (fastvec & mask)
		fastvec &= ~mask;

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

	intr_calculatemasks();
}
