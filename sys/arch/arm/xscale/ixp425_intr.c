/*	$NetBSD: ixp425_intr.c,v 1.10.16.4 2008/01/21 09:35:52 yamt Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ixp425_intr.c,v 1.10.16.4 2008/01/21 09:35:52 yamt Exp $");

#ifndef EVBARM_SPL_NOINLINE
#define	EVBARM_SPL_NOINLINE
#endif

/*
 * Interrupt support for the Intel IXP425 NetworkProcessor.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
int ixp425_imask[NIPL];

/* Current interrupt priority level. */
volatile int current_spl_level;  

/* Interrupts pending. */
volatile int ixp425_ipending;

/* Software copy of the IRQs we have enabled. */
volatile uint32_t intr_enabled;

/* Mask if interrupts steered to FIQs. */
uint32_t intr_steer;

#ifdef __HAVE_FAST_SOFTINTS
/*
 * Map a software interrupt queue index
 *
 * XXX: !NOTE! :XXX
 * We 'borrow' bits from the interrupt status register for interrupt sources
 * which are not used by the current IXP425 port. Should any of the following
 * interrupt sources be used at some future time, this must be revisited.
 *
 *  Bit#31: SW Interrupt 1
 *  Bit#30: SW Interrupt 0
 *  Bit#14: Timestamp Timer
 *  Bit#11: General-purpose Timer 1
 */
static const uint32_t si_to_irqbit[SI_NQUEUES] = {
	IXP425_INT_bit31,		/* SI_SOFT */
	IXP425_INT_bit30,		/* SI_SOFTCLOCK */
	IXP425_INT_bit14,		/* SI_SOFTNET */
	IXP425_INT_bit11,		/* SI_SOFTSERIAL */
};

#define	SI_TO_IRQBIT(si)	(1U << si_to_irqbit[(si)])

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[] = {
	[SI_SOFTCLOCK] =	IPL_SOFTCLOCK,
	[SI_SOFTBIO] =		IPL_SOFTBIO,
	[SI_SOFTNET] =		IPL_SOFTNET,
	[SI_SOFTSERIAL] =	IPL_SOFTSERIAL,
};
#endif /* __HAVE_FAST_SOFTINTS */
void	ixp425_intr_dispatch(struct clockframe *frame);

static inline uint32_t
ixp425_irq_read(void)
{
	return IXPREG(IXP425_INT_STATUS) & intr_enabled;
}

static inline void
ixp425_set_intrsteer(void)
{
	IXPREG(IXP425_INT_SELECT) = intr_steer & IXP425_INT_HWMASK;
}

static inline void
ixp425_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	ixp425_set_intrmask();
}

static inline void
ixp425_disable_irq(int irq)
{

	intr_enabled &= ~(1U << irq);
	ixp425_set_intrmask();
}

static inline u_int32_t
ixp425_irq2gpio_bit(int irq)
{

	static const u_int8_t int2gpio[32] __attribute__ ((aligned(32))) = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* INT#0 -> INT#5 */
		0x00, 0x01,				/* GPIO#0 -> GPIO#1 */
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* INT#8 -> INT#13 */
		0xff, 0xff, 0xff, 0xff, 0xff,		/* INT#14 -> INT#18 */
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07,	/* GPIO#2 -> GPIO#7 */
		0x08, 0x09, 0x0a, 0x0b, 0x0c,		/* GPIO#8 -> GPIO#12 */
		0xff, 0xff				/* INT#30 -> INT#31 */
	};

#ifdef DEBUG
	if (int2gpio[irq] == 0xff)
		panic("ixp425_irq2gpio_bit: bad GPIO irq: %d\n", irq);
#endif
	return (1U << int2gpio[irq]);
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
ixp425_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		ixp425_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		for (irq = 0; irq < NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		ixp425_imask[ipl] = irqs;
	}

	KASSERT(ixp425_imask[IPL_NONE] == 0);

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	ixp425_imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTCLOCK);
	ixp425_imask[IPL_SOFTBIO] = SI_TO_IRQBIT(SI_SOFTBIO);
	ixp425_imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTNET);
	ixp425_imask[IPL_SOFTSERIAL] = SI_TO_IRQBIT(SI_SOFTSERIAL);
#endif

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	ixp425_imask[IPL_SOFTBIO] |= ixp425_imask[IPL_SOFTCLOCK];
	ixp425_imask[IPL_SOFTNET] |= ixp425_imask[IPL_SOFTBIO];
	ixp425_imask[IPL_SOFTSERIAL] |= ixp425_imask[IPL_SOFTNET];
	ixp425_imask[IPL_VM] |= ixp425_imask[IPL_SOFTSERIAL];
	ixp425_imask[IPL_SCHED] |= ixp425_imask[IPL_VM];
	ixp425_imask[IPL_HIGH] |= ixp425_imask[IPL_SCHED];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int irqs = (1U << irq);
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			ixp425_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			irqs |= ixp425_imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

#ifdef __HAVE_FAST_SOFTINTS
void
ixp425_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int new, oldirqstate;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((ixp425_ipending & ~new) & SI_TO_IRQBIT(si)) {		\
		ixp425_ipending &= ~SI_TO_IRQBIT(si);			\
		current_spl_level |= ixp425_imask[si_to_ipl[(si)]];	\
		restore_interrupts(oldirqstate);			\
		softintr_dispatch(si);					\
		oldirqstate = disable_interrupts(I32_bit);		\
		current_spl_level = new;				\
	}

	DO_SOFTINT(SI_SOFTSERIAL);
	DO_SOFTINT(SI_SOFTNET);
	DO_SOFTINT(SI_SOFTCLOCK);
	DO_SOFTINT(SI_SOFT);

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
}
#endif

void
splx(int new)
{

	ixp425_splx(new);
}

int
_spllower(int ipl)
{

	return (ixp425_spllower(ipl));
}

int
_splraise(int ipl)
{

	return (ixp425_splraise(ipl));
}

#ifdef __HAVE_FAST_SOFTINTS
void
_setsoftintr(int si)
{
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	ixp425_ipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if ((ixp425_ipending & INT_SWMASK) & ~current_spl_level)
		ixp425_do_pending();
}
#endif /* __HAVE_FAST_SOFTINTS */

/*
 * ixp425_icu_init:
 *
 * 	Called early in bootstrap to make clear interrupt register
 */
void
ixp425_icu_init(void)
{

	intr_enabled = 0;	/* All interrupts disabled */
	ixp425_set_intrmask();

	intr_steer = 0;		/* All interrupts steered to IRQ */
	ixp425_set_intrsteer();
}

/*
 * ixp425_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
ixp425_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, "ixp425", iq->iq_name);
	}

	ixp425_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
ixp425_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("ixp425_intr_establish: IRQ %d out of range", irq);
#ifdef DEBUG
	printf("ixp425_intr_establish(irq=%d, ipl=%d, func=%08x, arg=%08x)\n",
	       irq, ipl, (u_int32_t) func, (u_int32_t) arg);
#endif

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq = &intrq[irq];

	/* All IXP425 interrupts are level-triggered. */
	iq->iq_ist = IST_LEVEL; /* XXX */

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	ixp425_intr_calculate_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
ixp425_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	ixp425_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

void
ixp425_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, pcpl, irq, ibit, hwpend;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	pcpl = current_spl_level;
	hwpend = ixp425_irq_read();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	ixp425_set_intrmask();

	while (hwpend != 0) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
			 */
			ixp425_ipending |= ibit;
			continue;
		}

		ixp425_ipending &= ~ibit;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		current_spl_level |= iq->iq_mask;

		/* Clear down non-level triggered GPIO interrupts now */
		if ((ibit & IXP425_INT_GPIOMASK) && iq->iq_ist != IST_LEVEL) {
			IXPREG(IXP425_GPIO_VBASE + IXP425_GPIO_GPISR) =
			    ixp425_irq2gpio_bit(irq);
		}

		oldirqstate = enable_interrupts(I32_bit);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		}
		restore_interrupts(oldirqstate);

		/* Clear down level triggered GPIO interrupts now */
		if ((ibit & IXP425_INT_GPIOMASK) && iq->iq_ist == IST_LEVEL) {
			IXPREG(IXP425_GPIO_VBASE + IXP425_GPIO_GPISR) =
			    ixp425_irq2gpio_bit(irq);
		}

		current_spl_level = pcpl;

		/* Re-enable this interrupt now that's it's cleared. */
		intr_enabled |= ibit;
		ixp425_set_intrmask();

		/*
		 * Don't forget to include interrupts which may have
		 * arrived in the meantime.
		 */
		hwpend |= ((ixp425_ipending & IXP425_INT_HWMASK) & ~pcpl);
	}
	ci->ci_idepth--;

#ifdef __HAVE_FAST_SOFTINTS
	/* Check for pendings soft intrs. */
	if ((ixp425_ipending & INT_SWMASK) & ~current_spl_level) {
		oldirqstate = enable_interrupts(I32_bit);
		ixp425_do_pending();
		restore_interrupts(oldirqstate);
	}
#endif
}
