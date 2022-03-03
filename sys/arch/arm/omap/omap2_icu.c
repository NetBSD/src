/*	$NetBSD: omap2_icu.c,v 1.12 2022/03/03 06:26:29 riastradh Exp $	*/
/*
 * Define the SDP2430 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
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
#include "opt_omap.h"

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_icu.c,v 1.12 2022/03/03 06:26:29 riastradh Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_obiovar.h>


#define	INTC_READ(sc, g, o)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o))
#define	INTC_WRITE(sc, g, o, v)	\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o), v)

static int omap2icu_match(device_t, cfdata_t, void *);
static void omap2icu_attach(device_t, device_t, void *);

static void omap2icu_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_block_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_establish_irq(struct pic_softc *, struct intrsource *);
#if 0
static void omap2icu_source_name(struct pic_softc *, int, char *, size_t);
#endif

static const struct pic_ops omap2icu_picops = {
	.pic_unblock_irqs = omap2icu_unblock_irqs,
	.pic_block_irqs = omap2icu_block_irqs,
	.pic_establish_irq = omap2icu_establish_irq,
#if 0
	.pic_source_name = omap2icu_source_name,
#endif
};

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct omap2icu_softc, sc_pic)))

static struct omap2icu_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	struct pic_softc sc_pic;
#if defined(TI_AM335X)
	uint32_t sc_enabled_irqs[4];
#else
	uint32_t sc_enabled_irqs[3];
#endif
} omap2icu_softc = {
	.sc_pic = {
		.pic_ops = &omap2icu_picops,
#if defined(TI_AM335X)
		.pic_maxsources = 128,
#else
		.pic_maxsources = 96,
#endif
		.pic_name = "omap2icu",
	},
};

static void
omap2icu_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct omap2icu_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irqbase / 32;
	KASSERT((irq_mask & sc->sc_enabled_irqs[group]) == 0);
	sc->sc_enabled_irqs[group] |= irq_mask;
	INTC_WRITE(sc, group, INTC_MIR_CLEAR, irq_mask);

	/* Force INTC to recompute IRQ availability */
	INTC_WRITE(sc, 0, INTC_CONTROL, INTC_CONTROL_NEWIRQAGR);
}

static void
omap2icu_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct omap2icu_softc * const sc = PICTOSOFTC(pic);
	const size_t group = irqbase / 32;

	INTC_WRITE(sc, group, INTC_MIR_SET, irq_mask);
	sc->sc_enabled_irqs[group] &= ~irq_mask;
}

/*
 * Called with interrupts disabled
 */
static int
find_pending_irqs(struct omap2icu_softc *sc, size_t group)
{
	uint32_t pending = INTC_READ(sc, group, INTC_PENDING_IRQ);

	KASSERT((sc->sc_enabled_irqs[group] & pending) == pending);

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&sc->sc_pic, group * 32, pending);
}

void
omap_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct omap2icu_softc * const sc = &omap2icu_softc;
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

	/* force INTC to recomputq IRQ */
	INTC_WRITE(sc, 0, INTC_CONTROL, INTC_CONTROL_NEWIRQAGR);

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

void
omap2icu_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	KASSERT(is->is_irq < omap2icu_softc.sc_pic.pic_maxsources);
	KASSERT(is->is_type == IST_LEVEL);
}

int
omap2icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const oa = aux;

#if defined(OMAP_2430) || defined(OMAP_2420)
	return oa->obio_addr == INTC_BASE;
#elif defined(OMAP3)
	return oa->obio_addr == INTC_BASE_3530;
#else
#error unsupported OMAP variant
#endif
}

void
omap2icu_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct omap2icu_softc * const sc = &omap2icu_softc;
	int error;

	aprint_normal("\n");

	sc->sc_memt = oa->obio_iot;

	error = bus_space_map(sc->sc_memt, oa->obio_addr, 0x1000, 0,
	    &sc->sc_memh);
	if (error)
		panic("failed to map interrupt registers: %d", error);

	INTC_WRITE(sc, 0, INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(sc, 1, INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(sc, 2, INTC_MIR_SET, 0xffffffff);

	sc->sc_dev = self;
	device_set_private(self, sc);

	pic_add(&sc->sc_pic, 0);
}

CFATTACH_DECL_NEW(omap2icu,
    0,
    omap2icu_match, omap2icu_attach,
    NULL, NULL);
