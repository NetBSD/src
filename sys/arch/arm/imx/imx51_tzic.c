/*	$NetBSD: imx51_tzic.c,v 1.8 2022/02/12 03:24:34 riastradh Exp $	*/

/*-
 * Copyright (c) 2010 SHIMIZU Ryo <ryo@nerv.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx51_tzic.c,v 1.8 2022/02/12 03:24:34 riastradh Exp $");

#define	_INTR_PRIVATE	/* for arm/pic/picvar.h */

#include "opt_imx.h"
#include "locators.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>
#include <sys/atomic.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <machine/autoconf.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_tzicreg.h>

static int tzic_match(device_t, cfdata_t, void *);
static void tzic_attach(device_t, device_t, void *);

/* for arm/pic */
static void tzic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void tzic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void tzic_establish_irq(struct pic_softc *, struct intrsource *);
static void tzic_source_name(struct pic_softc *, int, char *, size_t);

struct tzic_softc {
	device_t sc_dev;
	struct pic_softc sc_pic;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	uint32_t sc_enabled_mask[4];
};

const struct pic_ops tzic_pic_ops = {
	.pic_unblock_irqs = tzic_unblock_irqs,
	.pic_block_irqs = tzic_block_irqs,
	.pic_establish_irq = tzic_establish_irq,
	.pic_source_name = tzic_source_name
};

static void tzic_intr_init(struct tzic_softc * const);

static const char * const tzic_intr_source_names[] = TZIC_INTR_SOURCE_NAMES;

extern struct cfdriver tzic_cd;

#define	PIC_TO_SOFTC(pic) \
	((struct tzic_softc *)((char *)(pic) - \
		offsetof(struct tzic_softc, sc_pic)))

#define	INTC_READ(tzic, reg) \
	bus_space_read_4((tzic)->sc_iot, (tzic)->sc_ioh, (reg))
#define	INTC_WRITE(tzic, reg, val) \
	bus_space_write_4((tzic)->sc_iot, (tzic)->sc_ioh, (reg), (val))

/* use [7:4] of interrupt priority.
 * 0 is the highest priority.
 */
#define	HW_TO_SW_IPL(ipl)	(IPL_HIGH - ((ipl) >> 3))
#define	SW_TO_HW_IPL(ipl)	((IPL_HIGH - (ipl)) << 3)

CFATTACH_DECL_NEW(tzic, sizeof(struct tzic_softc),
    tzic_match, tzic_attach, NULL, NULL);

struct tzic_softc *tzic_softc;

int
tzic_match(device_t parent, cfdata_t self, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	if (aa->aa_addr != TZIC_BASE)
		return 0;

	return 1;
}

void
tzic_attach(device_t parent, device_t self, void *aux)
{
	struct tzic_softc *tzic = device_private(self);
	struct axi_attach_args * const aa = aux;
	int error;

	KASSERT(aa->aa_irqbase != AXICF_IRQBASE_DEFAULT);
	KASSERT(device_unit(self) == 0);

	aprint_normal(": TrustZone Interrupt Controller\n");
	aprint_naive("\n");

	tzic->sc_dev = self;
	tzic->sc_iot = aa->aa_iot;

	tzic_softc = tzic;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = TZIC_SIZE;

	error = bus_space_map(tzic->sc_iot, aa->aa_addr, aa->aa_size, 0, &tzic->sc_ioh);

	if (error) {
		panic("tzic_attach: failed to map register %#x-%#x: %d",
		    (uint32_t)aa->aa_addr,
		    (uint32_t)(aa->aa_addr + aa->aa_size - 1),
		    error);
	}

	tzic_intr_init(tzic);

	tzic->sc_pic.pic_ops = &tzic_pic_ops;
	tzic->sc_pic.pic_maxsources = TZIC_INTNUM;
	strlcpy(tzic->sc_pic.pic_name, device_xname(self),
	    sizeof(tzic->sc_pic.pic_name));

	pic_add(&tzic->sc_pic, aa->aa_irqbase);

	aprint_normal_dev(tzic->sc_dev, "interrupts %d..%d  register VA:%p\n",
	    aa->aa_irqbase, aa->aa_irqbase + TZIC_INTNUM,
	    (void *)tzic->sc_ioh);

	/* Everything is all set.  Enable the interrupts. */
	enable_interrupts(I32_bit|F32_bit);
}


void
tzic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct tzic_softc * const tzic = PIC_TO_SOFTC(pic);
	const size_t group = irq_base / 32;

	KASSERT((irq_mask & tzic->sc_enabled_mask[group]) == 0);

	tzic->sc_enabled_mask[group] |= irq_mask;
	INTC_WRITE(tzic, TZIC_ENSET(group), irq_mask);
}

void
tzic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
	struct tzic_softc * const tzic = PIC_TO_SOFTC(pic);
	const size_t group = irq_base / 32;

	tzic->sc_enabled_mask[group] &= ~irq_mask;

	INTC_WRITE(tzic, TZIC_ENCLEAR(group), irq_mask);
}

/*
 * Called with interrupts disabled
 */
static int
find_pending_irqs(struct tzic_softc *tzic, size_t group)
{
	uint32_t pending = 0;

	KASSERT( group <= 3 );

	pending = INTC_READ(tzic, TZIC_PND(group));

	KASSERT((tzic->sc_enabled_mask[group] & pending) == pending);

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&tzic->sc_pic, group * 32, pending);
}

void
tzic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct tzic_softc * const tzic = PIC_TO_SOFTC(pic);
	int priority_shift;
	int priority_offset;
	uint32_t reg;

	KASSERT(is->is_irq < 128);
	KASSERT(is->is_ipl < 16);
	KASSERT(is->is_type == IST_LEVEL);

	priority_shift = (is->is_irq % 4) * 8;
	priority_offset = (is->is_irq / 4);
	reg = INTC_READ(tzic, TZIC_PRIORITY(priority_offset));
	reg &= ~(0xff << priority_shift);
	reg |= SW_TO_HW_IPL(is->is_ipl) << priority_shift;
	INTC_WRITE(tzic, TZIC_PRIORITY(priority_offset), reg);
}

void
tzic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	strlcpy(buf, tzic_intr_source_names[irq], len);
}

void
imx51_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	if (tzic_softc->sc_enabled_mask[0])
		ipl_mask |= find_pending_irqs(tzic_softc, 0);
	if (tzic_softc->sc_enabled_mask[1])
		ipl_mask |= find_pending_irqs(tzic_softc, 1);
	if (tzic_softc->sc_enabled_mask[2])
		ipl_mask |= find_pending_irqs(tzic_softc, 2);
	if (tzic_softc->sc_enabled_mask[3])
		ipl_mask |= find_pending_irqs(tzic_softc, 3);

	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

static void
tzic_intr_init(struct tzic_softc * const tzic)
{
	int i;

	disable_interrupts(I32_bit|F32_bit);

	(void) INTC_READ(tzic, TZIC_INTCNTL);
	INTC_WRITE(tzic, TZIC_INTCNTL, INTCNTL_NSEN_MASK|INTCNTL_NSEN|INTCNTL_EN);
	(void) INTC_READ(tzic, TZIC_INTCNTL);
	INTC_WRITE(tzic, TZIC_PRIOMASK, SW_TO_HW_IPL(IPL_NONE));
	(void) INTC_READ(tzic, TZIC_PRIOMASK);

	INTC_WRITE(tzic, TZIC_SYNCCTRL, 0x00);
	(void) INTC_READ(tzic, TZIC_SYNCCTRL);

	/* route all interrupts to IRQ.  secure interrupts are for FIQ */
	for (i = 0; i < 4; i++)
		INTC_WRITE(tzic, TZIC_INTSEC(i), 0xffffffff);

	/* disable all interrupts */
	for (i = 0; i < 4; i++)
		INTC_WRITE(tzic, TZIC_ENCLEAR(i), 0xffffffff);

}
