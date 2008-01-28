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
__KERNEL_RCSID(0, "$NetBSD: omap2_icu.c,v 1.1.2.3 2008/01/28 18:29:07 matt Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/atomic.h>

#include <arm/omap/omap2430reg.h>
#include <arm/omap/omap2430obiovar.h>


#define	INTC_READ(sc, g, o)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o))
#define	INTC_WRITE(sc, g, o, v)	\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o), v)

static int omap2icu_match(device_t, cfdata_t, void *);
static void omap2icu_attach(device_t, device_t, void *);

static void omap2icu_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_block_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_establish_irq(struct pic_softc *, int, int, int);
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
	uint32_t sc_enabled_irqs[3];
} omap2icu_softc = {
	.sc_pic = {
		.pic_ops = &omap2icu_picops,
		.pic_maxsources = 96,
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
	if (sc->sc_enabled_irqs[group] & irq_mask) {
		INTC_WRITE(sc, group, INTC_MIR_SET, irq_mask);
		sc->sc_enabled_irqs[group] &= ~irq_mask;
	}
}

/*
 * Called with interrupts disabled
 */
static int
find_pending_irqs(struct omap2icu_softc *sc, int group, void *frame)
{
	struct intrsource ** const isbase = &sc->sc_pic.pic_sources[group * 32];
	struct intrsource *is;
	uint32_t pending;
	int ipl_mask = 0;

	pending = INTC_READ(sc, group, INTC_PENDING_IRQ);

	KASSERT((sc->sc_enabled_irqs[group] & pending) == pending);
	KASSERT((sc->sc_pic.pic_pending_irqs[group] & pending) == 0);

	while (pending != 0) {
		int n = ffs(pending);
		if (n-- == 0)
			break;
		is = isbase[n];
		KASSERT(is != NULL);
		pic_mark_pending_source(&sc->sc_pic, is);
		pending &= ~__BIT(n);
		ipl_mask |= __BIT(is->is_ipl);
	};

	return ipl_mask;
}

void
omap_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct omap2icu_softc * const sc = &omap2icu_softc;
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	uvmexp.intrs++;

	if (sc->sc_enabled_irqs[0])
		ipl_mask |= find_pending_irqs(sc, 0, frame);
	if (sc->sc_enabled_irqs[1])
		ipl_mask |= find_pending_irqs(sc, 1, frame);
	if (sc->sc_enabled_irqs[2])
		ipl_mask |= find_pending_irqs(sc, 2, frame);

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

void
omap2icu_establish_irq(struct pic_softc *pic, int irq, int ipl, int type)
{
	struct intrsource *is;

	KASSERT(irq >= 0 && irq < 96);
	KASSERT(type == IST_LEVEL);
	is = pic->pic_sources[irq & 0x1f];

	is->is_ipl = ipl;
}

int
omap2icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const oa = aux;

	return (oa->obio_addr == INTC_BASE);
}

void
omap2icu_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct omap2icu_softc * const sc = device_private(self);
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
	self->dv_private = sc;
}

CFATTACH_DECL(omap2icu,
    sizeof(struct device),
    omap2icu_match, omap2icu_attach,
    NULL, NULL);
