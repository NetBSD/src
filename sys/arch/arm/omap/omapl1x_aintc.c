
/*
 * Copyright (c) 2013 Linu Cherian
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: omapl1x_aintc.c,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $");

#include "opt_omapl1x.h"

#define _INTR_PRIVATE  

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/atomic.h>

#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omap_tipb.h>

static int omapl1xaintc_match(device_t, cfdata_t, void *);
static void omapl1xaintc_attach(device_t, device_t, void *);

static void omapl1xaintc_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void omapl1xaintc_block_irqs(struct pic_softc *, size_t, uint32_t);
static void omapl1xaintc_establish_irq(struct pic_softc *, struct intrsource *);

#define	INTC_READ(sc, o)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (o))
#define	INTC_WRITE(sc, o, v)	\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (o), v)

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct omapl1xaintc_softc, sc_pic)))

static const struct pic_ops omapl1xaintc_picops = {
	.pic_unblock_irqs = omapl1xaintc_unblock_irqs,
	.pic_block_irqs = omapl1xaintc_block_irqs,
	.pic_establish_irq = omapl1xaintc_establish_irq,
};

static struct omapl1xaintc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	struct pic_softc sc_pic;
	uint32_t sc_enabled_irqs[4];
} omapl1xaintc_softc = {
	.sc_pic = {
		.pic_ops = &omapl1xaintc_picops,
		.pic_maxsources = 101,
		.pic_name = "omapl1xaintc",
	},
};

/* Host Side Interrupt Numbers */
#define HOST_IRQ 1
#define HOST_FIQ 0

CFATTACH_DECL_NEW(omapl1xaintc, 0, omapl1xaintc_match, omapl1xaintc_attach,
		  NULL, NULL);

static void
omapl1xaintc_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct omapl1xaintc_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irqbase / 32;

	KASSERT((irq_mask & sc->sc_enabled_irqs[group]) == 0);
	sc->sc_enabled_irqs[group] |= irq_mask;
	INTC_WRITE(sc, AINTC_ESR1 + group * 4, irq_mask);
}

static void
omapl1xaintc_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct omapl1xaintc_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irqbase / 32;

	INTC_WRITE(sc, AINTC_ECR1 + group * 4, irq_mask);
	sc->sc_enabled_irqs[group] &= ~irq_mask;
}

/*
 * Called with interrupts disabled
 */
static int
find_pending_irqs(struct omapl1xaintc_softc *sc, size_t group)
{
	uint32_t pending = INTC_READ(sc, AINTC_SECR1 + group * 4);

	KASSERT((sc->sc_enabled_irqs[group] & pending) == pending);

	if (pending == 0)
		return 0;

	/* Clear what we have read */
	INTC_WRITE(sc, AINTC_SECR1 + group * 4, pending);

	return pic_mark_pending_sources(&sc->sc_pic, group * 32, pending);
}

void
omapl1xaintc_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct omapl1xaintc_softc * const sc = &omapl1xaintc_softc;
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	if (sc->sc_enabled_irqs[0])
		ipl_mask |= find_pending_irqs(sc, 0);
	if (sc->sc_enabled_irqs[1])
		ipl_mask |= find_pending_irqs(sc, 1);
	if (sc->sc_enabled_irqs[2])
		ipl_mask |= find_pending_irqs(sc, 2);
	if (sc->sc_enabled_irqs[3])
		ipl_mask |= find_pending_irqs(sc, 3);

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

void
omapl1xaintc_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	KASSERT(is->is_irq < 101);
}

int
omapl1xaintc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

void
omapl1xaintc_attach(device_t parent, device_t self, void *aux)
{
	int i, error;
	uint32_t group, num_irqs, num_regs;
	struct tipb_attach_args * const ta = aux;
	struct omapl1xaintc_softc * const sc = &omapl1xaintc_softc;

	aprint_normal("\n");

	num_irqs = PIC_MAXSOURCES;

	sc->sc_memt = ta->tipb_iot;

	error = bus_space_map(sc->sc_memt, ta->tipb_addr, ta->tipb_size, 0,
			      &sc->sc_memh);
	if (error)
		panic("failed to map interrupt registers: %d", error);

	/* Clear global interrupt */
	INTC_WRITE(sc, AINTC_GER, 0);

	/* Clear all host interrupts */
	INTC_WRITE(sc, AINTC_HIER, 0);
	
	/* Disable all system interrupts */
	for (i = 0, group = 0; i < num_irqs; i++, group = i/32)
		INTC_WRITE(sc, AINTC_ECR1 + group * 4, ~0);

	/* Clear all system interrupts status */
	for (i = 0, group = 0; i < num_irqs; i++, group = i/32)
		INTC_WRITE(sc, AINTC_SECR1 + group * 4, ~0);

	/* 
	 * Map all system interrupts to channel 7
	 * XXX Not sure about why channel 7. I'm just
	 * following what linux does here. 
	*/
	num_regs = (num_irqs + 3) >> 2;
	for (i = 0; i < num_regs; i++)
		INTC_WRITE(sc, AINTC_CMR0 + i * 4, 0x07070707);

	/* Enable Host side IRQ line */
	INTC_WRITE(sc, AINTC_HIEISR, (unsigned long)HOST_IRQ); 

	/* Enable Global interrupt */
	INTC_WRITE(sc, AINTC_GER, 0x1); 

	pic_add(&sc->sc_pic, 0);

	enable_interrupts(I32_bit);
}
