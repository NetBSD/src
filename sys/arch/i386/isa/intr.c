/*-
 * Copyright (c) 1993 Charles Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: intr.c,v 1.2 1993/10/22 19:33:13 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isa.h>
#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/timerreg.h>

void isa_defaultirq __P((void));
void isa_flushintrs __P((void));
void isa_intrmaskwickedness __P((void));

int	intrmask[NIRQ];
struct	intrhand *intrhand[NIRQ];
int	fastvec;

#define	IDTVEC(name)	__CONCAT(X,name)
/* default interrupt vector table entries */
extern	*IDTVEC(intr)[];
/* fast interrupt vector table */
extern	*IDTVEC(fast)[];
/* out of range default interrupt vector gate entry */
extern	IDTVEC(wild);

/*
 * Register a fast vector.
 */
void
intr_fasttrap(intr, ih)
	int intr;
	struct intrhand *ih;
{
	int irqnum = ffs(intr) - 1;

#ifdef DIAGNOSTIC
	if (intr == IRQUNK || intr == IRQNONE ||
	    (intr &~ (1 << irqnum)) != IRQNONE)
		panic("intr_fasttrap: weird irq");
#endif

	if (fastvec & intr)
		panic("intr_fasttrap: irq is already fast vector");
	if (intrhand[irqnum])
		panic("intr_fasttrap: irq is already slow vector");
	fastvec |= intr;

	ih->ih_count = 0;
	ih->ih_next = NULL;

	intrhand[irqnum] = ih;

	setidt(irqnum, IDTVEC(fast)[irqnum],  SDT_SYS386IGT, SEL_KPL);
}

/*
 * Register an interrupt handler.
 */
void
intr_establish(intr, ih, class)
	int intr;
	struct intrhand *ih;
	enum devclass class;
{
	int irqnum = ffs(intr) - 1;
	register struct intrhand **p, *q;

#ifdef DIAGNOSTIC
	if (intr == IRQUNK || intr == IRQNONE ||
	    (intr &~ (1 << irqnum)) != IRQNONE)
		panic("intr_establish: weird irq");
#endif

	if (fastvec & intr)
		panic("intr_establish: irq is already fast vector");

	switch (class) {
	    case DV_DULL:
		break;
	    case DV_DISK:
	    case DV_TAPE:
		biomask |= intr;
		break;
	    case DV_IFNET:
		netmask |= intr;
		break;
	    case DV_TTY:
		ttymask |= intr;
		break;
	    case DV_CPU:
	    default:
		panic("intr_establish: weird devclass");
	}

	ih->ih_count = 0;
	ih->ih_next = NULL;

	/*
	 * This is O(N^2), but we want to preserve the order, and N is
	 * always small.
	 */
	for (p = &intrhand[intr]; (q = *p) != NULL; p = &q->ih_next)
		;
	*p = ih;

	intr_enable(intr);
}

/*
 * Set up the masks for each interrupt handler based on the information
 * recorded by intr_establish().
 */
void
isa_intrmaskwickedness()
{
	int irq, mask;

	for (irq = 0; irq < NIRQ; irq++) {
		mask = (1 << irq) | astmask;
		if (biomask & (1 << irq))
			mask |= biomask;
		if (ttymask & (1 << irq))
			mask |= ttymask;
		if (netmask & (1 << irq))
			mask |= netmask;
		intrmask[irq] = mask;
	}

	impmask = netmask | ttymask;
	printf("biomask %x ttymask %x netmask %x impmask %x astmask %s\n",
	       biomask, ttymask, netmask, impmask, astmask);
}

/*
 * Fill in default interrupt table, and mask all interrupts.
 */
void
isa_defaultirq()
{
	int i;

	/* icu vectors */
	for (i = ICU_OFFSET; i < ICU_OFFSET + ICU_LEN ; i++)
		setidt(i, IDTVEC(intr)[i],  SDT_SYS386IGT, SEL_KPL);
  
	/* out of range vectors */
	for (; i < NIDT; i++)
		setidt(i, &IDTVEC(wild), SDT_SYS386IGT, SEL_KPL);

	/* initialize 8259's */
	outb(IO_ICU1, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU1+1, ICU_OFFSET);	/* starting at this vector index */
	outb(IO_ICU1+1, IRQ_SLAVE);
#ifdef AUTO_EOI_1
	outb(IO_ICU1+1, 2 | 1);		/* auto EOI, 8086 mode */
#else
	outb(IO_ICU1+1, 1);		/* 8086 mode */
#endif
	outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU1, 0x0a);		/* default to IRR on read */ 
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
	outb(IO_ICU2, 0x0a);		/* default to IRR on read */
}

/*
 * Flush any pending interrupts.
 */
void
isa_flushintrs()
{
	register int i;

	/* clear any pending interrupts */
	for (i = 0; i < 8; i++) {
		outb(IO_ICU1, ICU_EOI);
		outb(IO_ICU2, ICU_EOI);
	}
}

/*
 * Determine what IRQ a device is using by trying to force it to generate an
 * interrupt and seeing which IRQ line goes high.  It is not safe to call
 * this function after autoconfig.
 */
u_short
isa_discoverintr(force, aux)
	void (*force)();
{
	int time = TIMER_FREQ * 1;	/* wait up to 1 second */
	u_int last, now;
	u_short iobase = IO_TIMER1;

	isa_flushintrs();
	/* attempt to force interrupt */
	force(aux);
	/* set timer 2 to a known state */
	outb(iobase + TIMER_MODE, TIMER_SEL2|TIMER_RATEGEN|TIMER_16BIT);
	outb(iobase + TIMER_CNTR2, 0xff);
	outb(iobase + TIMER_CNTR2, 0xff);
	last = 0xffff;
	while (time > 0) {
		register u_char irr, lo, hi;
		irr = inb(IO_ICU1) & ~IRQ_SLAVE;
		if (irr)
			return 1 << (ffs(irr) - 1);
		irr = inb(IO_ICU2);
		if (irr)
			return 1 << (ffs(irr) + 7);
		outb(iobase + TIMER_MODE, TIMER_SEL2|TIMER_LATCH);
		lo = inb(iobase + TIMER_CNTR2);
		hi = inb(iobase + TIMER_CNTR2);
		now = (hi << 8) | lo;
		if (now <= last)
			time -= (last - now);
		else
			time -= 0x10000 - (now - last);
		last = now;
	}
	return IRQNONE;
}

/*
 * Caught a stray interrupt, notify
 */
void
isa_strayintr(d)
	int d;
{
	extern u_long intrcnt_stray[],
		      intrcnt_wild;

	/*
	 * Stray interrupts on irq 7 occur when an interrupt line is raised
	 * and then lowered before the CPU acknowledges it.  This generally
	 * means either the device is screwed or something is cli'ing too
	 * long and it's timing out.
	 *
	 * -1 is passed by the generic handler for out of range exceptions,
	 * since we don't really want 208 little vectors.  (It wouldn't be
	 * all that much code, but why bother?)
	 */
	if (d == -1) {
		intrcnt_wild++;
		log(LOG_ERR, "wild interrupt\n");
	} else {
		if (intrcnt_stray[d]++ < 5)
			log(LOG_ERR, "stray interrupt %d%s\n", d,
			    intrcnt_stray[d] == 5 ? "; stopped logging" : "");
	}
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 *
 * This implementation is hist[oe]rical.
 */
int
isa_nmi()
{

	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return 0;
}
