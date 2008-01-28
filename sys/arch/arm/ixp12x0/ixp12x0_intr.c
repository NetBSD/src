/* $NetBSD: ixp12x0_intr.c,v 1.15.30.3 2008/01/28 18:29:06 matt Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_intr.c,v 1.15.30.3 2008/01/28 18:29:06 matt Exp $");

/*
 * Interrupt support for the Intel ixp12x0
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/simplelock.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ixp12x0/ixp12x0reg.h> 
#include <arm/ixp12x0/ixp12x0var.h> 
#include <arm/ixp12x0/ixp12x0_comreg.h> 
#include <arm/ixp12x0/ixp12x0_comvar.h> 
#include <arm/ixp12x0/ixp12x0_pcireg.h> 


extern u_int32_t	ixpcom_cr;	/* current cr from *_com.c */
extern u_int32_t	ixpcom_imask;	/* tell mask to *_com.c */

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static u_int32_t imask[NIPL];
static u_int32_t pci_imask[NIPL];

/* Current interrupt priority level. */
volatile int hardware_spl_level;

/* Software copy of the IRQs we have enabled. */
volatile u_int32_t intr_enabled;
volatile u_int32_t pci_intr_enabled;

/* Interrupts pending. */
static volatile int ipending;

void	ixp12x0_intr_dispatch(struct irqframe *frame);

#define IXPREG(reg)	*((volatile u_int32_t*) (reg))

static inline u_int32_t
ixp12x0_irq_read(void)
{
	return IXPREG(IXP12X0_IRQ_VBASE) & IXP12X0_INTR_MASK;
}

static inline u_int32_t
ixp12x0_pci_irq_read(void)
{
	return IXPREG(IXPPCI_IRQ_STATUS);
}

static void
ixp12x0_enable_uart_irq(void)
{
	ixpcom_imask = 0;
	if (ixpcom_sc)
		bus_space_write_4(ixpcom_sc->sc_iot, ixpcom_sc->sc_ioh,
				  IXPCOM_CR, ixpcom_cr & ~ixpcom_imask);
}

static void
ixp12x0_disable_uart_irq(void)
{
	ixpcom_imask = CR_RIE | CR_XIE;
	if (ixpcom_sc)
		bus_space_write_4(ixpcom_sc->sc_iot, ixpcom_sc->sc_ioh,
				  IXPCOM_CR, ixpcom_cr & ~ixpcom_imask);
}

static void
ixp12x0_set_intrmask(u_int32_t irqs, u_int32_t pci_irqs)
{
	if (irqs & (1U << IXP12X0_INTR_UART)) {
		ixp12x0_disable_uart_irq();
	} else {
		ixp12x0_enable_uart_irq();
	}
	IXPREG(IXPPCI_IRQ_ENABLE_CLEAR) = pci_irqs;
	IXPREG(IXPPCI_IRQ_ENABLE_SET) = pci_intr_enabled & ~pci_irqs;
}

static void
ixp12x0_enable_irq(int irq)
{
	if (irq < SYS_NIRQ) {
		intr_enabled |= (1U << irq);
		switch (irq) {
		case IXP12X0_INTR_UART:
			ixp12x0_enable_uart_irq();
			break;

		case IXP12X0_INTR_PCI:
			/* nothing to do */
			break;
		default:
			panic("enable_irq:bad IRQ %d", irq);
		}
	} else {
		pci_intr_enabled |= (1U << (irq - SYS_NIRQ));
		IXPREG(IXPPCI_IRQ_ENABLE_SET) = (1U << (irq - SYS_NIRQ));
	}
}

static inline void
ixp12x0_disable_irq(int irq)
{
	if (irq < SYS_NIRQ) {
		intr_enabled ^= ~(1U << irq);
		switch (irq) {
		case IXP12X0_INTR_UART:
			ixp12x0_disable_uart_irq();
			break;

		case IXP12X0_INTR_PCI:
			/* nothing to do */
			break;
		default:
			/* nothing to do */
			break;
		}
	} else {
		pci_intr_enabled &= ~(1U << (irq - SYS_NIRQ));
		IXPREG(IXPPCI_IRQ_ENABLE_CLEAR) = (1U << (irq - SYS_NIRQ));
	}
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
ixp12x0_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		ixp12x0_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		int pci_irqs = 0;
		for (irq = 0; irq < SYS_NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		imask[ipl] = irqs;
		for (irq = 0; irq < SYS_NIRQ; irq++) {
			if (intrq[irq + SYS_NIRQ].iq_levels & (1U << ipl))
				pci_irqs |= (1U << irq);
		}
		pci_imask[ipl] = pci_irqs;
	}

	KASSERT(imask[IPL_NONE] == 0);
	KASSERT(pci_imask[IPL_NONE] == 0);

	KASSERT(imask[IPL_VM] != 0);
	KASSERT(pci_imask[IPL_VM] != 0);

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	imask[IPL_CLOCK] |= imask[IPL_VM];
	pci_imask[IPL_CLOCK] |= pci_imask[IPL_VM];

	/*
	 * splhigh() must block "everything".
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];
	pci_imask[IPL_HIGH] |= pci_imask[IPL_CLOCK];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int	irqs;
		int	pci_irqs;

		if (irq < SYS_NIRQ) {
			irqs = (1U << irq);
			pci_irqs = 0;
		} else {
			irqs = 0;
			pci_irqs = (1U << (irq - SYS_NIRQ));
		}
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			ixp12x0_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			irqs |= imask[ih->ih_ipl];
			pci_irqs |= pci_imask[ih->ih_ipl];
		}
		iq->iq_mask = irqs;
		iq->iq_pci_mask = pci_irqs;
	}
}

inline void
splx(int new)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = curcpl();
	set_curcpl(new);
	if (new != hardware_spl_level) {
		hardware_spl_level = new;
		ixp12x0_set_intrmask(imask[new], pci_imask[new]);
	}
	restore_interrupts(oldirqstate);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

int
_splraise(int ipl)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = curcpl();
	set_curcpl(ipl);
	restore_interrupts(oldirqstate);
	return (old);
}

int
_spllower(int ipl)
{
	int	old = curcpl();

	if (old <= ipl)
		return (old);
	splx(ipl);
	return (old);
}

/*
 * ixp12x0_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
ixp12x0_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;
	pci_intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "ipl %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, "ixpintr", iq->iq_name);
	}
	curcpu()->ci_intr_depth = 0;
	curcpu()->ci_cpl = 0;
	hardware_spl_level = 0;

	ixp12x0_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
ixp12x0_intr_establish(int irq, int ipl, int (*ih_func)(void *), void *arg)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
#ifdef DEBUG
	printf("ixp12x0_intr_establish(irq=%d, ipl=%d, ih_func=%08x, arg=%08x)\n",
	       irq, ipl, (u_int32_t) ih_func, (u_int32_t) arg);
#endif
	if (irq < 0 || irq > NIRQ)
		panic("ixp12x0_intr_establish: IRQ %d out of range", ipl);
	if (ipl < 0 || ipl > NIPL)
		panic("ixp12x0_intr_establish: IPL %d out of range", ipl);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = ih_func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	iq = &intrq[irq];
	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);
	ixp12x0_intr_calculate_masks();
	restore_interrupts(oldirqstate);

	return (ih);
}

void
ixp12x0_intr_disestablish(void *cookie)
{
	struct intrhand*	ih = cookie;
	struct intrq*		iq = &intrq[ih->ih_ipl];
	u_int			oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	ixp12x0_intr_calculate_masks();
	restore_interrupts(oldirqstate);
}

void
ixp12x0_intr_dispatch(struct irqframe *frame)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	struct cpu_info* const	ci = curcpu();
	const int		ppl = ci->ci_cpl;
	u_int			oldirqstate;
	u_int32_t		hwpend;
	u_int32_t		pci_hwpend;
	int			irq;
	u_int32_t		ibit;


	hwpend = ixp12x0_irq_read();
	pci_hwpend = ixp12x0_pci_irq_read();

	hardware_spl_level = ppl;
	ixp12x0_set_intrmask(imask[ppl] | hwpend, pci_imask[ppl] | pci_hwpend);

	hwpend &= ~imask[ppl];
	pci_hwpend &= ~pci_imask[ppl];

	while (hwpend) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			ci->ci_cpl = ih->ih_ipl;
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
			hwpend &= ~ibit;
		}
	}
	while (pci_hwpend) {
		irq = ffs(pci_hwpend) - 1;
		ibit = (1U << irq);

		iq = &intrq[irq + SYS_NIRQ];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			ci->ci_cpl = ih->ih_ipl;
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
		}
		pci_hwpend &= ~ibit;
	}

	ci->ci_cpl = ppl;
	hardware_spl_level = ppl;
	ixp12x0_set_intrmask(imask[ppl], pci_imask[ppl]);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}
