/*	$NetBSD: imx31_icu.c,v 1.8 2022/02/12 03:24:34 riastradh Exp $	*/
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: imx31_icu.c,v 1.8 2022/02/12 03:24:34 riastradh Exp $");

#define _INTR_PRIVATE
 
#include "locators.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>
#include <sys/atomic.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>
 
#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>

static void avic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void avic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void avic_establish_irq(struct pic_softc *, struct intrsource *);
static void avic_source_name(struct pic_softc *, int, char *, size_t);

const struct pic_ops avic_pic_ops = {
	.pic_unblock_irqs = avic_unblock_irqs,
	.pic_block_irqs = avic_block_irqs,
	.pic_establish_irq = avic_establish_irq,
	.pic_source_name = avic_source_name
};

struct avic_softc {
	struct pic_softc avic_pic;
	bus_space_tag_t avic_memt;
	bus_space_handle_t avic_memh;
};

extern struct cfdriver avic_cd;

#define	INTC_READ(avic, reg) \
	bus_space_read_4((avic)->avic_memt, (avic)->avic_memh, (reg))
#define	INTC_WRITE(avic, reg, val) \
	bus_space_write_4((avic)->avic_memt, (avic)->avic_memh, (reg), (val))
#define	HW_TO_SW_IPL(ipl)	((ipl) + 1)
#define	SW_TO_HW_IPL(ipl)	((ipl) - 1)

void
avic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct avic_softc * const avic = (void *) pic;
#if 0
	if (irq_base == 0)
		INTC_WRITE(avic, IMX31_INTENABLEL, irq_mask);
	else
		INTC_WRITE(avic, IMX31_INTENABLEH, irq_mask);
#else
	uint32_t irq;
	while ((irq = ffs(irq_mask)) != 0) {
		irq--;
		irq_base += irq;
		irq_mask >>= irq;
		INTC_WRITE(avic, IMX31_INTENNUM, irq_base);
	}
#endif
}

void
avic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct avic_softc * const avic = (void *) pic;
#if 0
	if (irq_base == 0)
		INTC_WRITE(avic, IMX31_INTDISABLEL, irq_mask);
	else
		INTC_WRITE(avic, IMX31_INTDISABLEH, irq_mask);
#else
	uint32_t irq;
	while ((irq = ffs(irq_mask)) != 0) {
		irq--;
		irq_base += irq;
		irq_mask >>= irq;
		INTC_WRITE(avic, IMX31_INTDISNUM, irq_base);
	}
#endif
}

void
avic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct avic_softc * const avic = (void *) pic;
	bus_addr_t priority_reg;
	int priority_shift;
	uint32_t v;

	KASSERT(is->is_irq < 64);
	KASSERT(is->is_ipl < 16);

	priority_reg = IMX31_NIPRIORITY0 - (is->is_irq >> 3);
	priority_shift = (is->is_irq & 7) * 4; 
	v = INTC_READ(avic, priority_reg);
	v &= ~(0x0f << priority_shift);
	v |= SW_TO_HW_IPL(is->is_ipl) << priority_shift;
	INTC_WRITE(avic, priority_reg, v);

	KASSERT(is->is_type == IST_LEVEL);
}

static const char * const avic_intr_source_names[] = AVIC_INTR_SOURCE_NAMES;

void
avic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	strlcpy(buf, avic_intr_source_names[irq], len);
}

void
imx31_irq_handler(void *frame)
{
	struct avic_softc * const avic = device_lookup_private(&avic_cd, 0);
	struct pic_softc * const pic = &avic->avic_pic;
	int32_t saved_nimask;
	int32_t irq;
	int ipl, newipl, oldipl;

	saved_nimask = INTC_READ(avic, IMX31_NIMASK);
	for (;;) {
		irq = INTC_READ(avic, IMX31_NIVECSR);
		if (irq < 0)
			break;
		ipl = (int16_t) irq;
		KASSERT(ipl >= 0);
		irq >>= 16;
		KASSERT(irq < 64);
		KASSERT(pic->pic_sources[irq] != NULL);

		/*
		 * If this interrupt is not above the current spl,
		 * mark it as pending and try again.
		 */
		newipl = HW_TO_SW_IPL(ipl);
		if (newipl <= curcpu()->ci_cpl) {
			pic_mark_pending(pic, irq);
			continue;
		}

		/*
		 * Before enabling interrupts, mask out lower priority
		 * interrupts and raise SPL to its equivalent.
		 */

		INTC_WRITE(avic, IMX31_NIMASK, ipl);
		oldipl = _splraise(newipl);
		cpsie(I32_bit);

		pic_dispatch(pic->pic_sources[irq], frame);

		/*
		 * Disable interrupts again.  Drop SPL.  Restore saved
		 * HW interrupt level.
		 */
		cpsid(I32_bit);
		splx(oldipl);
		INTC_WRITE(avic, IMX31_NIMASK, saved_nimask);
	}
}

static int avic_match(device_t, cfdata_t, void *);
static void avic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(avic, sizeof(struct avic_softc),
    avic_match, avic_attach, NULL, NULL);

int
avic_match(device_t parent, cfdata_t self, void *aux)
{
	struct ahb_attach_args * const ahba = aux;

	if (ahba->ahba_addr != INTC_BASE)
		return 0;

	return 1;
}

void
avic_attach(device_t parent, device_t self, void *aux)
{
	struct avic_softc * const avic = device_private(self);
	struct ahb_attach_args * const ahba = aux;
	int error;

	KASSERT(ahba->ahba_irqbase != AHBCF_IRQBASE_DEFAULT);
	KASSERT(device_unit(self) == 0);

	if (ahba->ahba_size == AHBCF_SIZE_DEFAULT)
		ahba->ahba_size = INTC_SIZE;

	avic->avic_memt = ahba->ahba_memt;
	error = bus_space_map(avic->avic_memt, ahba->ahba_addr, ahba->ahba_size,
	    0, &avic->avic_memh);
	if (error)
		panic("avic_attach: failed to map register %#lx-%#lx: %d",
		    ahba->ahba_addr, ahba->ahba_addr + ahba->ahba_size - 1,
		    error);

	avic->avic_pic.pic_ops = &avic_pic_ops;
	avic->avic_pic.pic_maxsources = 64;
	strlcpy(avic->avic_pic.pic_name, device_xname(self),
	    sizeof(avic->avic_pic.pic_name));

	pic_add(&avic->avic_pic, ahba->ahba_irqbase);
	aprint_normal(": interrupts %d..%d\n",
	    ahba->ahba_irqbase, ahba->ahba_irqbase + 63);
#if 0
	softintr_init();
#endif
}
