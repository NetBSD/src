/*	$NetBSD: extintr.c,v 1.10.2.7 2001/03/27 15:31:10 bouyer Exp $	*/

/*-
 * Copyright (c) 2000, 2001 Tsubai Masanari.
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
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/pio.h>

#include <powerpc/openpic.h>

#define NIRQ 32
#define HWIRQ_MAX (NIRQ - 4 - 1)
#define HWIRQ_MASK 0x0fffffff

void intr_calculatemasks __P((void));
char *intr_typename __P((int));
int fakeintr __P((void *));

static inline int cntlzw __P((int));
static inline int gc_read_irq __P((void));
static inline int mapirq __P((int));
static void gc_enable_irq __P((int));
static void gc_disable_irq __P((int));

static void do_pending_int __P((void));

int imask[NIPL];

int intrtype[NIRQ], intrmask[NIRQ], intrlevel[NIRQ];
struct intrhand *intrhand[NIRQ];

static u_char hwirq[NIRQ], virq[ICU_LEN];
static int virq_max = 0;

static u_char *obio_base;

extern u_int *heathrow_FCR;

#ifdef MULTIPROCESSOR
#define ipending	(curcpu()->ci_ipending)
#define cpl		(curcpu()->ci_cpl)
#else
volatile int cpl, ipending;
#endif

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

	hwirq[v] = irq;
	virq[irq] = v;

	return v;
}

/*
 * Count leading zeros.
 */
static __inline int
cntlzw(x)
	int x;
{
	int a;

	asm volatile ("cntlzw %0,%1" : "=r"(a) : "r"(x));

	return a;
}

int
gc_read_irq()
{
	int rv = 0;
	int lo, hi, p;

	lo = in32rb(INT_STATE_REG_L);
	if (lo)
		out32rb(INT_CLEAR_REG_L, lo);
	while (lo) {
		p = 31 - cntlzw(lo);
		rv |= 1 << virq[p];
		lo &= ~(1 << p);
	}

	if (heathrow_FCR)			/* has heathrow? */
		hi = in32rb(INT_STATE_REG_H);
	else
		hi = 0;

	if (hi)
		out32rb(INT_CLEAR_REG_H, hi);
	while (hi) {
		p = 31 - cntlzw(hi);
		rv |= 1 << virq[p + 32];
		hi &= ~(1 << p);
	}

	/* 1 << 0 is invalid. */
	return rv & ~1;
}

void
gc_enable_irq(irq)
	int irq;
{
	u_int x;

	if (irq < 32) {
		x = in32rb(INT_ENABLE_REG_L);
		x |= 1 << irq;
		out32rb(INT_ENABLE_REG_L, x);
	} else {
		x = in32rb(INT_ENABLE_REG_H);
		x |= 1 << (irq - 32);
		out32rb(INT_ENABLE_REG_H, x);
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
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < NIRQ; irq++) {
		register int levels = 0;
		for (q = intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0; irq < NIRQ; irq++)
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
	for (irq = 0; irq < NIRQ; irq++) {
		register int irqs = 1 << irq;
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	/* Lastly, enable IRQs actually in use. */
	if (have_openpic) {
		for (irq = 0; irq < ICU_LEN; irq++)
			openpic_disable_irq(irq);
		for (irq = 0; irq < NIRQ; irq++) {
			if (intrhand[irq])
				openpic_enable_irq(hwirq[irq], intrtype[irq]);
		}
	} else {
		out32rb(INT_ENABLE_REG_L, 0);
		if (heathrow_FCR)
			out32rb(INT_ENABLE_REG_H, 0);
		for (irq = 0; irq < NIRQ; irq++) {
			if (intrhand[irq])
				gc_enable_irq(hwirq[irq]);
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

char *
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
#if 1 /* XXX */
		return "unknown";
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

	irq = mapirq(irq);

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq (%d) or type (%d), irq, type");

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
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq %d", irq);

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
	struct intrhand *ih;
	u_long int_state;

#ifdef MULTIPROCESSOR
	/* Only cpu0 can handle external interrupts. */
	if (cpu_number() != 0) {
		/* XXX IPI should be maskable */
		out32(HH_INTR_SECONDARY, ~0);
		cpuintr(NULL);
		return;
	}
#endif

	pcpl = cpl;
	asm volatile ("mfmsr %0" : "=r"(msr));

	int_state = gc_read_irq();
#ifdef MULTIPROCESSOR
	r_imen = 1 << virq[GC_IPI_IRQ];
	if (int_state & r_imen) {
		/* XXX IPI should be maskable */
		int_state &= ~r_imen;
		cpuintr(NULL);
	}
#endif
	if (int_state == 0)
		return;

start:
	irq = 31 - cntlzw(int_state);
	r_imen = 1 << irq;

	if ((pcpl & r_imen) != 0) {
		ipending |= r_imen;	/* Masked! Mark this as pending */
		gc_disable_irq(hwirq[irq]);
	} else {
		splraise(intrmask[irq]);
		asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		ih = intrhand[irq];
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK();
		asm volatile ("mtmsr %0" :: "r"(msr));
		cpl = pcpl;

		uvmexp.intrs++;
		intrcnt[hwirq[irq]]++;
	}

	int_state &= ~r_imen;
	if (int_state)
		goto start;

	asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
	splx(pcpl);	/* Process pendings. */
	asm volatile ("mtmsr %0" :: "r"(msr));
}

void
ext_intr_openpic()
{
	int irq, realirq;
	int pcpl, msr, r_imen;
	struct intrhand *ih;

#ifdef MULTIPROCESSOR
	/* Only cpu0 can handle interrupts. */
	if (cpu_number() != 0)
		return;
#endif

	pcpl = cpl;
	asm volatile ("mfmsr %0" : "=r"(msr));

	realirq = openpic_read_irq(0);
	if (realirq == 255) {
		printf("spurious interrupt\n");
		return;
	}

start:
	irq = virq[realirq];		/* XXX check range */
	r_imen = 1 << irq;

	if ((pcpl & r_imen) != 0) {
		ipending |= r_imen;	/* Masked! Mark this as pending */
		openpic_disable_irq(realirq);
	} else {
		splraise(intrmask[irq]);
		asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		ih = intrhand[irq];
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK();
		asm volatile ("mtmsr %0" :: "r"(msr));
		cpl = pcpl;

		uvmexp.intrs++;
		intrcnt[hwirq[irq]]++;
	}

	openpic_eoi(0);

	realirq = openpic_read_irq(0);
	if (realirq != 255)
		goto start;

	asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
	splx(pcpl);	/* Process pendings. */
	asm volatile ("mtmsr %0" :: "r"(msr));
}

static void
do_pending_int()
{
	struct intrhand *ih;
	int irq;
	int pcpl;
	int hwpend;
	int emsr, dmsr;
	const int cpu_id = cpu_number();
	static int processing[2];	/* XXX */

	if (processing[cpu_id])
		return;

	processing[cpu_id] = 1;
	asm volatile("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile("mtmsr %0" :: "r"(dmsr));

	pcpl = cpl;
again:

#ifdef MULTIPROCESSOR
	if (cpu_id == 0) {
#endif
	/* Do now unmasked pendings */
	while ((hwpend = ipending & ~pcpl & HWIRQ_MASK) != 0) {
		irq = 31 - cntlzw(hwpend);
		if (!have_openpic)
			gc_enable_irq(hwirq[irq]);

		ipending &= ~(1 << irq);
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

		intrcnt[hwirq[irq]]++;
		if (have_openpic)
			openpic_enable_irq(hwirq[irq], intrtype[irq]);
	}
#ifdef MULTIPROCESSOR
	}
#endif

	if ((ipending & ~pcpl) & (1 << SIR_SERIAL)) {
		ipending &= ~(1 << SIR_SERIAL);
		splsoftserial();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softserial();
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SOFTSERIAL]++;
		goto again;
	}
	if ((ipending & ~pcpl) & (1 << SIR_NET)) {
		ipending &= ~(1 << SIR_NET);
		splsoftnet();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softnet();
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SOFTNET]++;
		goto again;
	}
	if ((ipending & ~pcpl) & (1 << SIR_CLOCK)) {
		ipending &= ~(1 << SIR_CLOCK);
		splsoftclock();
		asm volatile("mtmsr %0" :: "r"(emsr));
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		softclock(NULL);
		KERNEL_UNLOCK();
		asm volatile("mtmsr %0" :: "r"(dmsr));
		cpl = pcpl;
		intrcnt[CNT_SOFTCLOCK]++;
		goto again;
	}

#if 0
	if (ipending & ~pcpl) {
		printf("do_pending_int (again) 0x%x\n", ipending & ~pcpl);
		goto again;
	}
#endif
	cpl = pcpl;	/* Don't use splx... we are here already! */
	processing[cpu_id] = 0;
	asm volatile("mtmsr %0" :: "r"(emsr));
}

int
splraise(ncpl)
	int ncpl;
{
	int ocpl;

	asm volatile("sync; eieio");	/* don't reorder.... */
	ocpl = cpl;
	cpl = ocpl | ncpl;
	asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

void
splx(ncpl)
	int ncpl;
{

	asm volatile("sync; eieio");	/* reorder protect */
	cpl = ncpl;
	if (ipending & ~ncpl)
		do_pending_int();
	asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(ncpl)
	int ncpl;
{
	int ocpl;

	asm volatile("sync; eieio");	/* reorder protect */
	ocpl = cpl;
	cpl = ncpl;
	if (ipending & ~ncpl)
		do_pending_int();
	asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
void
softintr(ipl)
	int ipl;
{
	int msrsave;

	asm volatile("mfmsr %0" : "=r"(msrsave));
	asm volatile("mtmsr %0" :: "r"(msrsave & ~PSL_EE));
	ipending |= 1 << ipl;
	asm volatile("mtmsr %0" :: "r"(msrsave));
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

	/* send all interrupts to cpu 0 */
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

	/* XXX IPI */
	/* XXX set spurious intr vector */

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < 256; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		openpic_disable_irq(irq);

	install_extint(ext_intr_openpic);
}

void
legacy_int_init()
{
	out32rb(INT_ENABLE_REG_L, 0);		/* disable all intr. */
	out32rb(INT_CLEAR_REG_L, 0xffffffff);	/* clear pending intr. */
	if (heathrow_FCR) {
		out32rb(INT_ENABLE_REG_H, 0);
		out32rb(INT_CLEAR_REG_H, 0xffffffff);
	}

	install_extint(ext_intr);
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

	bzero(type, sizeof(type));
	chosen = OF_finddevice("/chosen");
	if (OF_getprop(chosen, "interrupt-controller", &ictlr, 4) == 4)
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
