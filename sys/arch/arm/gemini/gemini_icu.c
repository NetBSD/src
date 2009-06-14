/*	$NetBSD: gemini_icu.c,v 1.3 2009/06/14 23:20:35 rjs Exp $	*/

/* adapted from:
 *	NetBSD: omap2_icu.c,v 1.4 2008/08/27 11:03:10 matt Exp
 */

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
#include "opt_gemini.h"
#include "geminiicu.h"

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_icu.c,v 1.3 2009/06/14 23:20:35 rjs Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/atomic.h>

#include <arm/pic/picvar.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>


#define	INTC_READ(sc, o)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (o))
#define	INTC_WRITE(sc, o, v)	\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (o), v)

static int geminiicu_match(device_t, cfdata_t, void *);
static void geminiicu_attach(device_t, device_t, void *);

static void geminiicu_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void geminiicu_block_irqs(struct pic_softc *, size_t, uint32_t);
static void geminiicu_establish_irq(struct pic_softc *, struct intrsource *);
static void geminiicu_source_name(struct pic_softc *, int, char *, size_t);

static const struct pic_ops geminiicu_picops = {
	.pic_unblock_irqs = geminiicu_unblock_irqs,
	.pic_block_irqs = geminiicu_block_irqs,
	.pic_establish_irq = geminiicu_establish_irq,
	.pic_source_name = geminiicu_source_name,
};

#define	PICTOSOFTC(pic)	\
	((void *)((uintptr_t)(pic) - offsetof(struct geminiicu_softc, sc_pic)))

static struct geminiicu_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	struct pic_softc sc_pic;
	uint32_t sc_enabled_mask;
	uint32_t sc_edge_mask;
	uint32_t sc_edge_rising_mask;
	uint32_t sc_edge_falling_mask;
	uint32_t sc_level_mask;
	uint32_t sc_level_hi_mask;
	uint32_t sc_level_lo_mask;
} geminiicu_softc = {
	.sc_pic = {
		.pic_ops = &geminiicu_picops,
		.pic_maxsources = 32,
		.pic_name = "geminiicu",
	},
};

static const char * const sources[32] = {
	"ipi(0)",	"gmac0(1)",	"gmac1(2)",	"wdt(3)",
	"ide0(4)",	"ide1(5)",	"raid(6)",	"crypto(7)",
	"pci(8)",	"dma(9)",	"usb0(10)",	"usb1(11)",
	"flash(12)",	"tve(13)",	"timer0(14)",	"timer1(15)",
	"timer2(16)",	"rtc(17)",	"uart(18)",	"lcd(19)",
	"lpc(20)",	"ssp(21)",	"gpio0(22)",	"gpio1(23)",
	"gpio2(24)",	"cir(25)",	"power(26)",	"irq 27",
	"irq 28",	"irq 29",	"usbc0(30)",	"usbc1(31)"
};

static void geminiicu_source_name(struct pic_softc *pic, int irq,
	char *buf, size_t len)
{
	KASSERT((unsigned int)irq < 32);
	strlcpy(buf, sources[irq], len);
}

static void
geminiicu_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct geminiicu_softc * const sc = PICTOSOFTC(pic);
	KASSERT(irqbase == 0 && (irq_mask & sc->sc_enabled_mask) == 0);
	sc->sc_enabled_mask |= irq_mask;
	INTC_WRITE(sc, GEMINI_ICU_IRQ_ENABLE, sc->sc_enabled_mask);
	/*
	 * If this is a level source, ack it now.  If it's still asserted
	 * it'll come back.
	 */
	if (irq_mask & sc->sc_level_mask)
		INTC_WRITE(sc, GEMINI_ICU_IRQ_CLEAR,
		    irq_mask & sc->sc_level_mask);
}

static void
geminiicu_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{
	struct geminiicu_softc * const sc = PICTOSOFTC(pic);
	KASSERT(irqbase == 0);

	sc->sc_enabled_mask &= ~irq_mask;
	INTC_WRITE(sc, GEMINI_ICU_IRQ_ENABLE, sc->sc_enabled_mask);
	/*
	 * If any of the source are edge triggered, ack them now so
	 * we won't lose them.
	 */
	if (irq_mask & sc->sc_edge_mask)
		INTC_WRITE(sc, GEMINI_ICU_IRQ_CLEAR,
		    irq_mask & sc->sc_edge_mask);
}

/*
 * Called with interrupts disabled
 */
static int
find_pending_irqs(struct geminiicu_softc *sc)
{
	uint32_t pending = INTC_READ(sc, GEMINI_ICU_IRQ_STATUS);

	KASSERT((sc->sc_enabled_mask & pending) == pending);

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&sc->sc_pic, 0, pending);
}

void
gemini_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct geminiicu_softc * const sc = &geminiicu_softc;
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	uvmexp.intrs++;

	KASSERT(sc->sc_enabled_mask != 0);

	ipl_mask = find_pending_irqs(sc);

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

void
geminiicu_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct geminiicu_softc * const sc = PICTOSOFTC(pic);
	const uint32_t irq_mask = __BIT(is->is_irq);

	KASSERT(is->is_irq < 32);

	sc->sc_enabled_mask &= ~irq_mask;
	/* Have to do with this interrupt disabled.  */
	INTC_WRITE(sc, GEMINI_ICU_IRQ_ENABLE, sc->sc_enabled_mask);
	INTC_WRITE(sc, GEMINI_ICU_IRQ_CLEAR, irq_mask);

	sc->sc_edge_rising_mask &= ~irq_mask;
	sc->sc_edge_falling_mask &= ~irq_mask;
	sc->sc_level_lo_mask &= ~irq_mask;
	sc->sc_level_hi_mask &= ~irq_mask;

        switch (is->is_type) {
        case IST_LEVEL_LOW: sc->sc_level_lo_mask |= irq_mask; break;
        case IST_LEVEL_HIGH: sc->sc_level_hi_mask |= irq_mask; break;
        case IST_EDGE_FALLING: sc->sc_edge_falling_mask |= irq_mask; break;
        case IST_EDGE_RISING: sc->sc_edge_rising_mask |= irq_mask; break;
        }

        sc->sc_edge_mask = sc->sc_edge_rising_mask | sc->sc_edge_falling_mask;
        sc->sc_level_mask = sc->sc_level_hi_mask|sc->sc_level_lo_mask;

	/*
	 * Set the new interrupt mode.
	 */
	INTC_WRITE(sc, GEMINI_ICU_IRQ_TRIGMODE, sc->sc_edge_mask);
	INTC_WRITE(sc, GEMINI_ICU_IRQ_TRIGLEVEL,
	    sc->sc_level_lo_mask | sc->sc_edge_falling_mask);
}

int
geminiicu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const oa = aux;

#if defined(SL3516)
	if ((oa->obio_addr == GEMINI_IC0_BASE)
	||  (oa->obio_addr == GEMINI_IC1_BASE))
		return 1;

	return 0;
#else
#error unsupported GEMINI variant
#endif
}

void
geminiicu_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args * const oa = aux;
	struct geminiicu_softc * const sc = &geminiicu_softc;
	int error;

	aprint_normal("\n");

	sc->sc_memt = oa->obio_iot;

	error = bus_space_map(sc->sc_memt, oa->obio_addr, 0x1000, 0,
	    &sc->sc_memh);
	if (error)
		panic("failed to map interrupt registers: %d", error);

	INTC_WRITE(sc, GEMINI_ICU_IRQ_ENABLE, 0);
	INTC_WRITE(sc, GEMINI_ICU_IRQ_CLEAR, 0xffffffff);
	INTC_WRITE(sc, GEMINI_ICU_IRQ_TRIGMODE, 0);
	INTC_WRITE(sc, GEMINI_ICU_IRQ_TRIGLEVEL, 0xffffffff);

	sc->sc_dev = self;
	self->dv_private = sc;

	pic_add(&sc->sc_pic, 0);
}

CFATTACH_DECL_NEW(geminiicu,
    0,
    geminiicu_match, geminiicu_attach,
    NULL, NULL);
