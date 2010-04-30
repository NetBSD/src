/*	$NetBSD: intr.c,v 1.21.4.1 2010/04/30 14:39:42 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.21.4.1 2010/04/30 14:39:42 uebayasi Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/psl.h>

#include <powerpc/cpu.h>
#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>


/*
 * Number of interrupts (hard + soft), irq number legality test,
 * mapping of irq number to mask and a way to pick irq number
 * off a mask of active intrs.
 */
#define	ICU_LEN			32
#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN)

#define	IRQ_TO_MASK(irq) 	(0x80000000UL >> ((irq) & 0x1f))
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

#else /* Generic 405/440/460 Universal Interrupt Controller */

#include <powerpc/ibm4xx/dcr4xx.h>

#include "opt_uic.h"
#ifndef MULTIUIC

/* 405EP/405GP/405GPr/Virtex-4 */

#define	INTR_STATUS	(DCR_UIC0_BASE + DCR_UIC_MSR)
#define	INTR_ACK	(DCR_UIC0_BASE + DCR_UIC_SR)
#define	INTR_ENABLE	(DCR_UIC0_BASE + DCR_UIC_ER)

#else

/*
 * We has following UICs.  However can regist 32 HW-irqs.
 *   440EP/440GP/440SP has 2 UICs.
 *   405EX has 3 UICs.
 *   440SPe has 4 UICs.
 *   440GX has 4 UICs(3 UIC + UIC Base).
 */

#define NUIC	4
#define NIRQ	((NUIC) * ICU_LEN)

struct intrsrc;
static struct uic {
	char uic_name[5];
	uint32_t		uic_baddr; 	/* UICn base address */
	uint32_t		uic_birq; 	/* UICn base irq */
	struct intrsrc		*uic_intrsrc;
} uics[NUIC];
static int num_uic = 0;
int base_uic = 0;

#define	INTR_STATUS	DCR_UIC_MSR
#define	INTR_ACK	DCR_UIC_SR
#define	INTR_ENABLE	DCR_UIC_ER

#undef mtdcr
#define _mtdcr(base, reg, val) \
	__asm volatile("mtdcr %0,%1" : : "K"((base) + (reg)), "r"(val))
#define mtdcr(reg, val)							\
({									\
	switch(uic->uic_baddr) {					\
	case DCR_UIC0_BASE: _mtdcr(DCR_UIC0_BASE, (reg), (val)); break;	\
	case DCR_UIC1_BASE: _mtdcr(DCR_UIC1_BASE, (reg), (val)); break;	\
	case DCR_UIC2_BASE: _mtdcr(DCR_UIC2_BASE, (reg), (val)); break;	\
	case DCR_UIC3_BASE: _mtdcr(DCR_UIC3_BASE, (reg), (val)); break;	\
	case DCR_UICB_BASE: _mtdcr(DCR_UICB_BASE, (reg), (val)); break;	\
	case DCR_UIC2_BASE_440GX:					\
		_mtdcr(DCR_UIC2_BASE_440GX, (reg), (val)); break;	\
	default:							\
		panic("unknown UIC register 0x%x\n", uic->uic_baddr);	\
	}								\
})

#undef mfdcr
#define _mfdcr(base, reg) \
	__asm volatile("mfdcr %0,%1" : "=r"(__val) : "K"((base) + (reg)))
#define mfdcr(reg)                                              	\
({                                                              	\
	uint32_t __val;                                         	\
									\
	switch(uic->uic_baddr) {					\
	case DCR_UIC0_BASE: _mfdcr(DCR_UIC0_BASE, reg);	break;		\
	case DCR_UIC1_BASE: _mfdcr(DCR_UIC1_BASE, reg);	break;		\
	case DCR_UIC2_BASE: _mfdcr(DCR_UIC2_BASE, reg);	break;		\
	case DCR_UIC3_BASE: _mfdcr(DCR_UIC3_BASE, reg);	break;		\
	case DCR_UICB_BASE: _mfdcr(DCR_UICB_BASE, reg);	break;		\
	case DCR_UIC2_BASE_440GX:					\
		_mfdcr(DCR_UIC2_BASE_440GX, reg); break;		\
	default:							\
		panic("unknown UIC register 0x%x\n", uic->uic_baddr);	\
	}								\
	__val;                                                  	\
})

uint8_t hwirq[ICU_LEN];
uint8_t virq[NIRQ];
int virq_max = 0;

static int ext_intr_core(void *);

#endif

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
	int 			ih_level;
	struct	intrhand 	*ih_next;
	int			ih_irq;
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

static struct intrsrc 		intrs0[ICU_LEN] = {
#define	DEFINTR(name)							\
	{ EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "uic0", name),	\
	    NULL, 0, 0, 0 }

	DEFINTR("pin0"), 	DEFINTR("pin1"), 	DEFINTR("pin2"),
	DEFINTR("pin3"), 	DEFINTR("pin4"), 	DEFINTR("pin5"),
	DEFINTR("pin6"), 	DEFINTR("pin7"), 	DEFINTR("pin8"),
	DEFINTR("pin9"), 	DEFINTR("pin10"), 	DEFINTR("pin11"),
	DEFINTR("pin12"), 	DEFINTR("pin13"), 	DEFINTR("pin14"),
	DEFINTR("pin15"), 	DEFINTR("pin16"), 	DEFINTR("pin17"),
	DEFINTR("pin18"), 	DEFINTR("pin19"), 	DEFINTR("pin20"),
	DEFINTR("pin21"), 	DEFINTR("pin22"), 	DEFINTR("pin23"),
	DEFINTR("pin24"), 	DEFINTR("pin25"), 	DEFINTR("pin26"),
	DEFINTR("pin27"), 	DEFINTR("pin28"), 	DEFINTR("pin29"),
	DEFINTR("pin30"), 	DEFINTR("pin31")

#undef DEFINTR
};


/* Write External Enable Immediate */
#define	wrteei(en) 		__asm volatile ("wrteei %0" : : "K"(en))

/* Enforce In Order Execution of I/O */
#define	eieio() 		__asm volatile ("eieio")

/*
 * Set up interrupt mapping array.
 */
void
intr_init(void)
{
#ifdef MULTIUIC
	struct uic *uic = &uics[0];
#endif
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
			evcnt_attach_static(&intrs0[i].is_evcnt);
		}

	/* Initialized in powerpc/ibm4xx/cpu.c */
	evcnt_attach_static(&curcpu()->ci_ev_softclock);
	evcnt_attach_static(&curcpu()->ci_ev_softnet);
	evcnt_attach_static(&curcpu()->ci_ev_softserial);

#ifdef MULTIUIC
	strcpy(uic->uic_name, intrs0[0].is_evcnt.ev_name);;
	uic->uic_baddr = DCR_UIC0_BASE;
	uic->uic_birq = num_uic * 32;
	uic->uic_intrsrc = intrs0;
	num_uic++;
#endif
	mtdcr(INTR_ENABLE, 0x00000000); 	/* mask all */
	mtdcr(INTR_ACK, 0xffffffff); 		/* acknowledge all */
}

int
uic_add(u_int base, int irq)
{
#ifndef MULTIUIC

	return -1;
#else
	struct uic *uic = &uics[num_uic];
	struct intrsrc *intrs;
	int i;

#define UIC_BASE_TO_CHAR(b)	\
	(((b) >= DCR_UIC0_BASE && (b) <= DCR_UIC2_BASE) ?		\
	    '0' + (((b) - DCR_UIC0_BASE) >> 4) :			\
	    (((b) == DCR_UICB_BASE) ? 'b' :		/* 440GX only */\
	    (((b) == DCR_UIC2_BASE_440GX) ? '2' : '?')))/* 440GX only */

	sprintf(uic->uic_name, "uic%c", UIC_BASE_TO_CHAR(base));
	uic->uic_baddr = base;
	uic->uic_birq = num_uic * ICU_LEN;
	intrs = malloc(sizeof(struct intrsrc) * ICU_LEN, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (intrs == NULL)
		return ENOMEM;
	for (i = 0; i < ICU_LEN; i++)
		switch (i + num_uic * ICU_LEN) {
		case IRQ_SOFTNET:
		case IRQ_SOFTCLOCK:
		case IRQ_SOFTSERIAL:
		case IRQ_CLOCK:
		case IRQ_STATCLOCK:
			continue;
		default:
			evcnt_attach_dynamic(
			    &intrs[i].is_evcnt, EVCNT_TYPE_INTR, NULL,
			    uic->uic_name, intrs0[i].is_evcnt.ev_name);
		}
	uic->uic_intrsrc = intrs;
	num_uic++;

	mtdcr(INTR_ENABLE, 0x00000000); 	/* mask all */
	mtdcr(INTR_ACK, 0xffffffff); 		/* acknowledge all */

	intr_establish(irq, IST_LEVEL, IPL_NONE, ext_intr_core, uic);

	return 0;
#endif
}


/*
 * external interrupt handler
 */
#ifdef MULTIUIC
void
ext_intr(void)
{

	ext_intr_core(&uics[base_uic]);
}
#endif

#ifndef MULTIUIC
void
ext_intr(void)
#else
static int
ext_intr_core(void *arg)
#endif
{
	struct cpu_info *ci = curcpu();
#ifndef MULTIUIC
	struct intrsrc *intrs = intrs0;
#else
	struct uic *uic = arg;
	struct intrsrc *intrs = uic->uic_intrsrc;
#endif
	struct intrhand *ih;
	int irq, bits_to_clear, i;
	int r_imen, msr;
	int pcpl;
	u_long int_state;

	pcpl = ci->ci_cpl;
	msr = mfmsr();

	int_state = mfdcr(INTR_STATUS);	/* Read non-masked interrupt status */
	bits_to_clear = int_state;

	while (int_state) {
		i = IRQ_OF_MASK(int_state);

#ifndef MULTIUIC
		irq = i;
		r_imen = IRQ_TO_MASK(irq);
		int_state &= ~r_imen;
#else
		irq = uic->uic_birq + i;
		r_imen = IRQ_TO_MASK(virq[irq]);
		int_state &= ~IRQ_TO_MASK(i);
#endif

		if ((pcpl & r_imen) != 0) {
			/* Masked! Mark as pending */
			ci->ci_ipending |= r_imen;
			disable_irq(irq);
 		} else {
			ci->ci_idepth++;
			splraise(intrs[i].is_mask);
			if (intrs[i].is_type == IST_LEVEL)
				disable_irq(irq);
			wrteei(1);

			ih = intrs[i].is_head;
			while (ih) {
				if (ih->ih_level == IPL_VM)
					KERNEL_LOCK(1, NULL);
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
				if (ih->ih_level == IPL_VM)
					KERNEL_UNLOCK_ONE(NULL);
			}

			mtmsr(msr);
			if (intrs[i].is_type == IST_LEVEL)
				enable_irq(irq);
			ci->ci_cpl = pcpl;
			uvmexp.intrs++;
			intrs[i].is_evcnt.ev_count++;
			ci->ci_idepth--;
		}
	}
	mtdcr(INTR_ACK, bits_to_clear);	/* Acknowledge all pending interrupts */

	wrteei(1);
	splx(pcpl);
	mtmsr(msr);

#ifdef MULTIUIC
	return 0;
#endif
}

static inline void
disable_irq(int irq)
{
#ifdef MULTIUIC
	struct uic *uic = &uics[irq / ICU_LEN];
#endif
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
#ifdef MULTIUIC
	struct uic *uic = &uics[irq / ICU_LEN];
#endif
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
	struct intrsrc *intrs;
	struct intrhand *ih;
	int msr, i;

#ifndef MULTIUIC
	if (! LEGAL_IRQ(irq))
		panic("intr_establish: bogus IRQ %d", irq);
	intrs = intrs0;
	i = irq;
#else
	if (irq >= num_uic * ICU_LEN)
		panic("intr_establish: bogus IRQ %d, max is %d",
		    irq, num_uic * ICU_LEN - 1);
	intrs = uics[irq / ICU_LEN].uic_intrsrc;
	i = irq % ICU_LEN;
	if (intrs[i].is_head == NULL) {
		virq[irq] = ++virq_max;
		hwirq[virq[irq]] = irq;
	}
#endif

	if (type == IST_NONE)
		panic("intr_establish: bogus type %d for irq %d", type, irq);

	/* No point in sleeping unless someone can free memory. */
	ih = malloc(sizeof(*ih), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	switch (intrs[i].is_type) {
	case IST_NONE:
		intrs[i].is_type = type;
		break;

	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrs[i].is_type)
			break;
		/* FALLTHROUGH */

	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(intrs[i].is_type),
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
	ih->ih_next = intrs[i].is_head;
	ih->ih_irq = irq;
	intrs[i].is_head = ih;

	intr_calculatemasks();

	eieio();
	mtmsr(msr);

#ifdef IRQ_DEBUG
	printf("***** intr_establish: irq%d h=%p arg=%p\n",
	    irq, ih_fun, ih_arg);
#endif
	return (ih);
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(void *arg)
{
	struct intrsrc *intrs;
	struct intrhand *ih = arg;
	struct intrhand **p;
	int i, msr;

#ifndef MULTIUIC
	intrs = intrs0;
#else
	if (virq[ih->ih_irq] == 0)
		panic("intr_disestablish: bogus irq %d", ih->ih_irq);
	intrs = uics[ih->ih_irq / ICU_LEN].uic_intrsrc;
#endif

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
#ifndef MULTIUIC
	struct intrsrc *intrs = intrs0;
#else
	struct intrsrc *intrs;
	struct uic *uic;
	int n;
#endif
	struct intrhand *q;
	int irq, level;

#ifdef MULTIUIC
	for (n = 0, intrs = uics[n].uic_intrsrc; n < num_uic; n++)
#endif
	{

		/* First, figure out which levels each IRQ uses. */
		for (irq = 0; irq < ICU_LEN; irq++) {
			register int levels = 0;
			for (q = intrs[irq].is_head; q; q = q->ih_next)
				levels |= 1 << q->ih_level;
			intrs[irq].is_level = levels;
		}
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
#ifdef MULTIUIC
		for (n = 0, uic = &uics[n], intrs = uic->uic_intrsrc;
		    n < num_uic; n++)
#endif
		{
			for (irq = 0; irq < ICU_LEN; irq++)
				if (intrs[irq].is_level & (1 << level))
#ifndef MULTIUIC
					irqs |= IRQ_TO_MASK(irq);
#else
					irqs |= IRQ_TO_MASK(
					    virq[uic->uic_birq + irq]);
#endif
		}
		imask[level] = irqs | MASK_SOFTINTR;
	}

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_NONE] = 0;
	imask[IPL_SOFTCLOCK] |= MASK_SOFTCLOCK;
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK] | MASK_SOFTNET;
	imask[IPL_SOFTSERIAL] = imask[IPL_SOFTNET] | MASK_SOFTSERIAL;
	imask[IPL_VM] |= imask[IPL_SOFTSERIAL];
	imask[IPL_SCHED] = imask[IPL_VM] | MASK_CLOCK | MASK_STATCLOCK;
	imask[IPL_HIGH] |= imask[IPL_SCHED];

#ifndef MULTIUIC
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
#else
	for (n = 0; n < num_uic; n++) {
		uic = &uics[n];
		intrs = uic->uic_intrsrc;

		/* And eventually calculate the complete masks. */
		for (irq = 0; irq < ICU_LEN; irq++) {
			register int irqs =
			    IRQ_TO_MASK(virq[uic->uic_birq + irq]);

			for (q = intrs[irq].is_head; q; q = q->ih_next)
				irqs |= imask[q->ih_level];
			intrs[irq].is_mask = irqs;
		}

		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrs[irq].is_head != NULL)
				enable_irq(uic->uic_birq + irq);
			else
				disable_irq(uic->uic_birq + irq);
	}
#endif
}

static void
do_pending_int(void)
{
	struct cpu_info *ci = curcpu();
#ifndef MULTIUIC
	struct intrsrc *intrs = intrs0;
#else
	struct intrsrc *intrs;
	struct uic *uic;
#endif
	struct intrhand *ih;
	int irq, i;
	int pcpl;
	int pend, hwpend;
	int emsr;

	if (ci->ci_idepth)
		return;
#ifdef __HAVE_FAST_SOFTINTS
#error don't count soft interrupts
#else
	ci->ci_idepth++;
#endif
	emsr = mfmsr();
	wrteei(0);

	pcpl = ci->ci_cpl;	/* Turn off all */
#ifdef __HAVE_FAST_SOFTINTS
  again:
#endif
	while ((pend = ci->ci_ipending & ~pcpl & MASK_HARDINTR) != 0) {
#ifndef MULTIUIC
		irq = IRQ_OF_MASK(pend);
		i = irq;
		hwpend = pend;
#else
		irq = hwirq[IRQ_OF_MASK(pend)];
		i = irq % ICU_LEN;
		hwpend = IRQ_TO_MASK(irq);
		uic = &uics[irq / ICU_LEN];
		intrs = uic->uic_intrsrc;
#endif
		if (intrs[i].is_type != IST_LEVEL)
			enable_irq(irq);

#ifndef MULTIUIC
		ci->ci_ipending &= ~IRQ_TO_MASK(irq);
#else
		ci->ci_ipending &= ~IRQ_TO_MASK(virq[irq]);
#endif

		splraise(intrs[i].is_mask);
		mtmsr(emsr);

		ih = intrs[i].is_head;
		while(ih) {
			if (ih->ih_level == IPL_VM)
				KERNEL_LOCK(1, NULL);
			(*ih->ih_fun)(ih->ih_arg);
			if (ih->ih_level == IPL_VM)
				KERNEL_UNLOCK_ONE(NULL);
			ih = ih->ih_next;
		}
		mtdcr(INTR_ACK, hwpend);

		wrteei(0);
		if (intrs[i].is_type == IST_LEVEL)
			enable_irq(irq);
		ci->ci_cpl = pcpl;
		intrs[i].is_evcnt.ev_count++;
	}
#ifdef __HAVE_FAST_SOFTINTS
	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTSERIAL) {
		ci->ci_ipending &= ~MASK_SOFTSERIAL;
		splsoftserial();
		mtmsr(emsr);
		softintr__run(IPL_SOFTSERIAL);
		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softserial.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTNET) {
		ci->ci_ipending &= ~MASK_SOFTNET;
		splsoftnet();
		mtmsr(emsr);
		softintr__run(IPL_SOFTNET);
		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softnet.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & MASK_SOFTCLOCK) {
		ci->ci_ipending &= ~MASK_SOFTCLOCK;
		splsoftclock();
		mtmsr(emsr);
		softintr__run(IPL_SOFTCLOCK);
		wrteei(0);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softclock.ev_count++;
		goto again;
	}
#endif
	ci->ci_cpl = pcpl; /* Don't use splx... we are here already! */
	mtmsr(emsr);
	ci->ci_idepth--;
}

#ifdef __HAVE_FAST_SOFTINTS
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
#endif

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
