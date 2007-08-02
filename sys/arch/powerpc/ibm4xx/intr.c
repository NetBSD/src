/*	$NetBSD: intr.c,v 1.16.14.1 2007/08/02 05:34:14 macallan Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.16.14.1 2007/08/02 05:34:14 macallan Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/psl.h>

#include <powerpc/cpu.h>
#include <powerpc/spr.h>
#include <powerpc/softintr.h>


/*
 * Number of interrupts (hard + soft), irq number legality test,
 * mapping of irq number to mask and a way to pick irq number
 * off a mask of active intrs.
 */
#define	ICU_LEN			32
#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN)

#define	IRQ_TO_MASK(irq) 	(0x80000000UL >> (irq))
#define	IRQ_OF_MASK(mask) 	cntlzw(mask)

/*
 * Assign these to unused (reserved) interrupt bits.
 *
 * For 405GP (and 403CGX?) interrupt bits 0-18 and 25-31 are used
 * by hardware.  This leaves us bits 19-24 for software.
 */
#define	IRQ_SOFTNET	19
#define	IRQ_SOFTCLOCK	20
#define	IRQ_SOFTSERIAL	21
#define	IRQ_CLOCK       22
#define	IRQ_STATCLOCK	23

/*
 * Platform specific code may override any of the above.
 */
#ifdef PPC_IBM403

#include <powerpc/ibm4xx/dcr403cgx.h>
#define	INTR_STATUS	DCR_EXISR
#define	INTR_ACK	DCR_EXISR
#define	INTR_ENABLE	DCR_EXIER

#elif defined(__virtex__)

#include <evbppc/virtex/dev/xintcreg.h>
#define	INTR_STATUS 	XINTC_ISR
#define	INTR_ACK 	XINTC_IAR
#define	INTR_ENABLE 	XINTC_IER
#define	INTR_MASTER 	XINTC_MER
#define	INTR_MASTER_EN 	(MER_HIE|MER_ME)
#undef	IRQ_TO_MASK
#undef	IRQ_OF_MASK
#undef	IRQ_SOFTNET
#undef	IRQ_SOFTCLOCK
#undef	IRQ_SOFTSERIAL
#undef	IRQ_CLOCK
#undef	IRQ_STATCLOCK
#define	IRQ_TO_MASK(i) 	(1 << (i)) 		/* Redefine mappings */
#define	IRQ_OF_MASK(m) 	(31 - cntlzw(m))
#define	IRQ_SOFTNET	31 			/* Redefine "unused" pins */
#define	IRQ_SOFTCLOCK	30
#define	IRQ_SOFTSERIAL	29
#define	IRQ_CLOCK       28
#define	IRQ_STATCLOCK	27

#else /* Generic 405 Universal Interrupt Controller */

#include <powerpc/ibm4xx/dcr405gp.h>
#define	INTR_STATUS	DCR_UIC0_MSR
#define	INTR_ACK	DCR_UIC0_SR
#define	INTR_ENABLE	DCR_UIC0_ER

#endif

#define	MASK_SOFTNET	IRQ_TO_MASK(IRQ_SOFTNET)
#define	MASK_SOFTCLOCK	IRQ_TO_MASK(IRQ_SOFTCLOCK)
#define	MASK_SOFTSERIAL	IRQ_TO_MASK(IRQ_SOFTSERIAL)
#define	MASK_STATCLOCK 	IRQ_TO_MASK(IRQ_STATCLOCK)
#define	MASK_CLOCK	(IRQ_TO_MASK(IRQ_CLOCK) | IRQ_TO_MASK(IRQ_STATCLOCK))
#define	MASK_SOFTINTR	(MASK_SOFTCLOCK|MASK_SOFTNET|MASK_SOFTSERIAL)
#define	MASK_HARDINTR 	~(MASK_SOFTINTR|MASK_CLOCK)

static inline void disable_irq(int);
static inline void enable_irq(int);
static void intr_calculatemasks(void);
static void do_pending_int(void);
static const char *intr_typename(int);


/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int			(*ih_fun)(void *);
	void			*ih_arg;
	struct	intrhand 	*ih_next;
	int 			ih_level;
};

struct intrsrc {
	struct evcnt 		is_evcnt;
	struct intrhand 	*is_head;
	u_int 			is_mask;
	int			is_level; 	/* spls bitmask */
	int 			is_type; 	/* sensitivity */
};


volatile u_int 			imask[NIPL];
const int 			mask_clock = MASK_CLOCK;
const int 			mask_statclock = MASK_STATCLOCK;

static struct intrsrc 		intrs[ICU_LEN] = {
#define	DEFINTR(name) 		\
	{ EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "uic", name), NULL, 0, 0 }

	DEFINTR("pin0"), 	DEFINTR("pin1"), 	DEFINTR("pin2"),
	DEFINTR("pin3"), 	DEFINTR("pin4"), 	DEFINTR("pin5"),
	DEFINTR("pin6"), 	DEFINTR("pin7"), 	DEFINTR("pin8"),
	DEFINTR("pin9"), 	DEFINTR("pin10"), 	DEFINTR("pin11"),
	DEFINTR("pin12"), 	DEFINTR("pin13"), 	DEFINTR("pin14"),
	DEFINTR("pin15"), 	DEFINTR("pin16"), 	DEFINTR("pin17"),
	DEFINTR("pin18"),

	/* Reserved intrs, accounted in cpu_info */
	DEFINTR(NULL), 		/* unused "pin19", softnet */
	DEFINTR(NULL), 		/* unused "pin20", softclock */
	DEFINTR(NULL), 		/* unused "pin21", softserial */
	DEFINTR(NULL), 		/* unused "pin22", PIT hardclock */
	DEFINTR(NULL), 		/* unused "pin23", FIT statclock */

	DEFINTR("pin24"), 	DEFINTR("pin25"), 	DEFINTR("pin26"),
	DEFINTR("pin27"), 	DEFINTR("pin28"), 	DEFINTR("pin29"),
	DEFINTR("pin30"), 	DEFINTR("pin31")

#undef DEFINTR
};


/* Write External Enable Immediate */
#define	wrteei(en) 		__asm volatile ("wrteei %0" : : "K"(en))

/* Enforce In Order Execution Of I/O */
#define	eieio() 		__asm volatile ("eieio")

/*
 * Set up interrupt mapping array.
 */
void
intr_init(void)
{
	int i;

	for (i = 0; i < ICU_LEN; i++)
		switch (i) {
		case IRQ_SOFTNET:
		case IRQ_SOFTCLOCK:
		case IRQ_SOFTSERIAL:
		case IRQ_CLOCK:
		case IRQ_STATCLOCK:
			continue;
		default:
			evcnt_attach_static(&intrs[i].is_evcnt);
		}

	/* Initialized in powerpc/ibm4xx/cpu.c */
	evcnt_attach_static(&curcpu()->ci_ev_softclock);
	evcnt_attach_static(&curcpu()->ci_ev_softnet);
	evcnt_attach_static(&curcpu()->ci_ev_softserial);

	softintr__init();

	mtdcr(INTR_ENABLE, 0x00000000); 	/* mask all */
	mtdcr(INTR_ACK, 0xffffffff); 		/* acknowledge all */
#ifdef INTR_MASTER
	mtdcr(INTR_MASTER, INTR_MASTER_EN); 	/* enable controller */
#endif
}

/*
 * external interrupt handler
 */
void
ext_intr(void)
{
	struct cpu_info *ci = curcpu();
	struct intrhand *ih;
	int i, bits_to_clear;
	int r_imen, msr;
	int pcpl;
	u_long int_state;

	pcpl = ci->ci_cpl;
	msr = mfmsr();

	int_state = mfdcr(INTR_STATUS);	/* Read non-masked interrupt status */
	bits_to_clear = int_state;

	while (int_state) {
		i = IRQ_OF_MASK(int_state);

		r_imen = IRQ_TO_MASK(i);
		int_state &= ~r_imen;

		if ((pcpl & r_imen) != 0) {
			/* Masked! Mark as pending */
			ci->ci_ipending |= r_imen;
			disable_irq(i);
 		} else {
			splraise(intrs[i].is_mask);
			if (intrs[i].is_type == IST_LEVEL)
				disable_irq(i);
			wrteei(1);

			KERNEL_LOCK(1, NULL);
			ih = intrs[i].is_head;
			while (ih) {
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
			KERNEL_UNLOCK_ONE(NULL);

			mtmsr(msr);
			if (intrs[i].is_type == IST_LEVEL)
				enable_irq(i);
			ci->ci_cpl = pcpl;
			uvmexp.intrs++;
			intrs[i].is_evcnt.ev_count++;
		}
	}
	mtdcr(INTR_ACK, bits_to_clear);	/* Acknowledge all pending interrupts */

	wrteei(1);
	splx(pcpl);
	mtmsr(msr);
}

static inline void
disable_irq(int irq)
{
	int mask, omask;

	mask = omask = mfdcr(INTR_ENABLE);
	mask &= ~IRQ_TO_MASK(irq);
	if (mask == omask)
		return;
	mtdcr(INTR_ENABLE, mask);
#ifdef IRQ_DEBUG
	printf("irq_disable: irq=%d, mask=%08x\n",irq,mask);
#endif
}

static inline void
enable_irq(int irq)
{
	int mask, omask;

	mask = omask = mfdcr(INTR_ENABLE);
	mask |= IRQ_TO_MASK(irq);
	if (mask == omask)
		return;
	mtdcr(INTR_ENABLE, mask);
#ifdef IRQ_DEBUG
	printf("enable_irq: irq=%d, mask=%08x\n",irq,mask);
#endif
}

static const char *
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
		panic("intr_typename: invalid type %d", type);
	}
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(int irq, int type, int level, int (*ih_fun)(void *),
    void *ih_arg)
{
	struct intrhand *ih;
	int msr;

	if (! LEGAL_IRQ(irq))
		panic("intr_establish: bogus irq %d", irq);

	if (type == IST_NONE)
		panic("intr_establish: bogus type %d for irq %d", type, irq);

	/* No point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	switch (intrs[irq].is_type) {
	case IST_NONE:
		intrs[irq].is_type = type;
		break;

	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrs[irq].is_type)
			break;
		/* FALLTHROUGH */

	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(intrs[irq].is_type),
			    intr_typename(type));
		break;
	}

	/*
	 * We're not on critical paths, so just block intrs for a while.
	 * Note that spl*() at this point would use old (wrong) masks.
	 */
	msr = mfmsr();
	wrteei(0);

	/*
	 * Poke the real handler in now. We deliberately don't preserve order,
	 * the user is not allowed to make any assumptions about it anyway.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_level = level;
	ih->ih_next = intrs[irq].is_head;
	intrs[irq].is_head = ih;

	intr_calculatemasks();

	eieio();
	mtmsr(msr);

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
	struct intrhand **p;
	int i, msr;

	/* Lookup the handler. This is expensive, but not run often. */
	for (i = 0; i < ICU_LEN; i++)
		for (p = &intrs[i].is_head; *p != NULL; p = &(*p)->ih_next)
			if (*p == ih)
				goto out;
 out:
	if (i == ICU_LEN)
		panic("intr_disestablish: handler not registered");

	*p = ih->ih_next;
	free(ih, M_DEVBUF);

	msr = mfmsr();
	wrteei(0);
	intr_calculatemasks();
	mtmsr(msr);

	if (intrs[i].is_head == NULL)
		intrs[i].is_type = IST_NONE;
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway. We assume PSL_EE is clear when we're called.
 */
static void
intr_calculatemasks(void)
{
	struct intrhand *q;
	int irq, level;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int levels = 0;
		for (q = intrs[irq].is_head; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrs[irq].is_level = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrs[irq].is_level & (1 << level))
				irqs |= IRQ_TO_MASK(irq);
		imask[level] = irqs | MASK_SOFTINTR;
	}

	/*
	 * Not external interrupts, make them block themselves manually.
	 */
	imask[IPL_CLOCK] = MASK_CLOCK;
	imask[IPL_STATCLOCK] = MASK_STATCLOCK;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = MASK_SOFTCLOCK;
	imask[IPL_SOFTNET] = MASK_SOFTNET;
	imask[IPL_SOFTSERIAL] = MASK_SOFTSERIAL;

	/*
	 * Enforce hierarchy required by spl(9).
	 */
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_SOFTSERIAL] |= imask[IPL_SOFTCLOCK];

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
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
	 * We have separate statclock.
	 */
	imask[IPL_STATCLOCK] |= imask[IPL_CLOCK];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_STATCLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int irqs = IRQ_TO_MASK(irq);
		for (q = intrs[irq].is_head; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrs[irq].is_mask = irqs;
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		if (intrs[irq].is_head != NULL)
			enable_irq(irq);
		else
			disable_irq(irq);
}

static void
do_pending_int(void)
{
	struct cpu_info *ci = curcpu();
	struct intrhand *ih;
	int irq;
	int pcpl;
	int hwpend;
	int emsr;

	if (ci->ci_iactive)
		return;

	ci->ci_iactive = 1;
	emsr = mfmsr();
	wrteei(0);

	pcpl = ci->ci_cpl;	/* Turn off all */
  again:
	while ((hwpend = ci->ci_ipending & ~pcpl & MASK_HARDINTR) != 0) {
		irq = IRQ_OF_MASK(hwpend);
		if (intrs[irq].is_type != IST_LEVEL)
			enable_irq(irq);

		ci->ci_ipending &= ~IRQ_TO_MASK(irq);

		splraise(intrs[irq].is_mask);
		mtmsr(emsr);

		KERNEL_LOCK(1, NULL);
		ih = intrs[irq].is_head;
		while(ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK_ONE(NULL);

		wrteei(0);
		if (intrs[irq].is_type == IST_LEVEL)
			enable_irq(irq);
		ci->ci_cpl = pcpl;
		intrs[irq].is_evcnt.ev_count++;
	}

	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTSERIAL) {
		ci->ci_ipending &= ~MASK_SOFTSERIAL;
		splsoftserial();
		mtmsr(emsr);

		KERNEL_LOCK(1, NULL);
		softintr__run(IPL_SOFTSERIAL);
		KERNEL_UNLOCK_ONE(NULL);

		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softserial.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTNET) {
		ci->ci_ipending &= ~MASK_SOFTNET;
		splsoftnet();
		mtmsr(emsr);

		KERNEL_LOCK(1, NULL);
		softintr__run(IPL_SOFTNET);
		KERNEL_UNLOCK_ONE(NULL);

		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softnet.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTCLOCK) {
		ci->ci_ipending &= ~MASK_SOFTCLOCK;
		splsoftclock();
		mtmsr(emsr);

		KERNEL_LOCK(1, NULL);
		softintr__run(IPL_SOFTCLOCK);
		KERNEL_UNLOCK_ONE(NULL);

		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softclock.ev_count++;
		goto again;
	}
	ci->ci_cpl = pcpl; /* Don't use splx... we are here already! */
	mtmsr(emsr);
	ci->ci_iactive = 0;
}

void
softintr(int idx)
{
	static const int softmap[3] = {
		MASK_SOFTCLOCK, MASK_SOFTNET, MASK_SOFTSERIAL
	};
	int oldmsr;

	KASSERT(idx >= 0 && idx < 3);

	/*
	 * This could be implemented with lwarx/stwcx to avoid the
	 * disable/enable...
	 */

	oldmsr = mfmsr();
	wrteei(0);

	curcpu()->ci_ipending |= softmap[idx];

	mtmsr(oldmsr);
}

int
splraise(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int oldcpl, oldmsr;

	/*
	 * We're about to block some intrs, so make sure they don't
	 * fire while we're busy.
	 */

	oldmsr = mfmsr();
	wrteei(0);

	oldcpl = ci->ci_cpl;
	ci->ci_cpl |= newcpl;

	mtmsr(oldmsr);
	return (oldcpl);
}

void
splx(int newcpl)
{
	struct cpu_info *ci = curcpu();

	ci->ci_cpl = newcpl;
	if (ci->ci_ipending & ~newcpl)
		do_pending_int();
}

int
spllower(int newcpl)
{
	struct cpu_info *ci = curcpu();
	int oldcpl;

	oldcpl = ci->ci_cpl;
	ci->ci_cpl = newcpl;
	if (ci->ci_ipending & ~newcpl)
		do_pending_int();

	return (oldcpl);
}
