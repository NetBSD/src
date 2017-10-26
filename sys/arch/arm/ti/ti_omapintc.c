/*	$NetBSD: ti_omapintc.c,v 1.1 2017/10/26 01:16:32 jakllsch Exp $	*/
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

#define _INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_omapintc.c,v 1.1 2017/10/26 01:16:32 jakllsch Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/atomic.h>

#include <dev/fdt/fdtvar.h>

#define INTC_CONTROL		0x048
#define INTC_CONTROL_NEWIRQAGR	__BIT(0)
#define INTC_ITR		0x080
#define INTC_MIR		0x084
#define INTC_MIR_CLEAR		0x088
#define INTC_MIR_SET		0x08c
#define INTC_PENDING_IRQ	0x098

#define INTC_MAX_SOURCES	128


#define	INTC_READ(sc, g, o)		\
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o))
#define	INTC_WRITE(sc, g, o, v)	\
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (g) * 0x20 + (o), v)

static int omap2icu_match(device_t, cfdata_t, void *);
static void omap2icu_attach(device_t, device_t, void *);

static void omap2icu_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_block_irqs(struct pic_softc *, size_t, uint32_t);
static void omap2icu_establish_irq(struct pic_softc *, struct intrsource *);
static void omap2icu_set_priority(struct pic_softc *, int);
#if 0
static void omap2icu_source_name(struct pic_softc *, int, char *, size_t);
#endif

static const struct pic_ops omap2icu_picops = {
	.pic_unblock_irqs = omap2icu_unblock_irqs,
	.pic_block_irqs = omap2icu_block_irqs,
	.pic_establish_irq = omap2icu_establish_irq,
	.pic_set_priority = omap2icu_set_priority,
#if 0
	.pic_source_name = omap2icu_source_name,
#endif
};

#define	PICTOSOFTC(pic)	\
	((struct omap2icu_softc *)((uintptr_t)(pic) - offsetof(struct omap2icu_softc, sc_pic)))

struct omap2icu_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	struct pic_softc sc_pic;
	uint32_t sc_enabled_irqs[howmany(INTC_MAX_SOURCES, 32)];
};

static struct omap2icu_softc *intc_softc;

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

static void
omap_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	struct omap2icu_softc * const sc = intc_softc;
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
	KASSERT(is->is_irq < PICTOSOFTC(pic)->sc_pic.pic_maxsources);
	KASSERT(is->is_type == IST_LEVEL);
}

static void
omap2icu_set_priority(struct pic_softc *pic, int ipl)
{
	curcpu()->ci_cpl = ipl;
}

static void *
omapintc_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	const u_int irq = be32toh(specifier[0]);
	if (irq >= INTC_MAX_SOURCES) {
		device_printf(dev, "IRQ %u is invalid\n", irq);
		return NULL;
	}

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;
	return intr_establish(irq, ipl, IST_LEVEL | mpsafe, func, arg);
}

static void
omapintc_fdt_disestablish(device_t dev, void *ih)
{
        intr_disestablish(ih);
}

static bool
omapintc_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	if (!specifier)
		return false;

	const u_int irq = be32toh(specifier[0]);
	snprintf(buf, buflen, "INTC irq %d", irq);
	return true;
}

static const struct fdtbus_interrupt_controller_func omapintc_fdt_funcs = {
	.establish = omapintc_fdt_establish,
	.disestablish = omapintc_fdt_disestablish,
	.intrstr = omapintc_fdt_intrstr,
};

int
omap2icu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	static const char * const compatible[] = {
		"ti,am33xx-intc",
		NULL
	};

	return of_match_compatible(faa->faa_phandle, compatible);
}

void
omap2icu_attach(device_t parent, device_t self, void *aux)
{
	struct omap2icu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_memt = faa->faa_bst;
	if (bus_space_map(sc->sc_memt, addr, size, 0, &sc->sc_memh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	INTC_WRITE(sc, 0, INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(sc, 1, INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(sc, 2, INTC_MIR_SET, 0xffffffff);

	sc->sc_dev = self;
	self->dv_private = sc;

	sc->sc_pic.pic_ops = &omap2icu_picops;
	sc->sc_pic.pic_maxsources = INTC_MAX_SOURCES;
	snprintf(sc->sc_pic.pic_name, sizeof(sc->sc_pic.pic_name), "intc");
	pic_add(&sc->sc_pic, 0);
	error = fdtbus_register_interrupt_controller(self, phandle,
		&omapintc_fdt_funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register with fdtbus: %d\n",
		    error);
		return;
	}

	KASSERT(intc_softc == NULL);
	intc_softc = sc;
	arm_fdt_irq_set_handler(omap_irq_handler);
}

CFATTACH_DECL_NEW(omapintc,
    sizeof(struct omap2icu_softc),
    omap2icu_match, omap2icu_attach,
    NULL, NULL);
