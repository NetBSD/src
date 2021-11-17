/* $NetBSD: gicv3_fdt.c,v 1.16 2021/11/17 21:46:12 jmcneill Exp $ */

/*-
 * Copyright (c) 2015-2018 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "pci.h"

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gicv3_fdt.c,v 1.16 2021/11/17 21:46:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cortex/gicv3.h>
#include <arm/cortex/gicv3_its.h>
#include <arm/cortex/gic_reg.h>
#include <arm/cortex/gic_v2m.h>

#define	GICV3_MAXIRQ	1020

#define	IRQ_PPI(n)	((n) + 16)
#define	IRQ_SPI(n)	((n) + 32)

struct gicv3_fdt_softc;
struct gicv3_fdt_irq;

static int	gicv3_fdt_match(device_t, cfdata_t, void *);
static void	gicv3_fdt_attach(device_t, device_t, void *);

static int	gicv3_fdt_map_registers(struct gicv3_fdt_softc *);
#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
static void	gicv3_fdt_attach_mbi(struct gicv3_fdt_softc *);
static void	gicv3_fdt_attach_its(struct gicv3_fdt_softc *, bus_space_tag_t, int);
#endif

static int	gicv3_fdt_intr(void *);

static void *	gicv3_fdt_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *, const char *);
static void	gicv3_fdt_disestablish(device_t, void *);
static bool	gicv3_fdt_intrstr(device_t, u_int *, char *, size_t);

struct fdtbus_interrupt_controller_func gicv3_fdt_funcs = {
	.establish = gicv3_fdt_establish,
	.disestablish = gicv3_fdt_disestablish,
	.intrstr = gicv3_fdt_intrstr
};

struct gicv3_fdt_irqhandler {
	struct gicv3_fdt_irq	*ih_irq;
	int			(*ih_fn)(void *);
	void			*ih_arg;
	bool			ih_mpsafe;
	TAILQ_ENTRY(gicv3_fdt_irqhandler) ih_next;
};

struct gicv3_fdt_irq {
	struct gicv3_fdt_softc	*intr_sc;
	void			*intr_ih;
	void			*intr_arg;
	int			intr_refcnt;
	int			intr_ipl;
	int			intr_level;
	int			intr_mpsafe;
	TAILQ_HEAD(, gicv3_fdt_irqhandler) intr_handlers;
	int			intr_irq;
};

struct gicv3_fdt_softc {
	struct gicv3_softc	sc_gic;
	int			sc_phandle;

	struct gicv3_fdt_irq	*sc_irq[GICV3_MAXIRQ];
};

static const struct device_compatible_entry gicv3_fdt_quirks[] = {
	{ .compat = "rockchip,rk3399",		.value = GICV3_QUIRK_RK3399 },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(gicv3_fdt, sizeof(struct gicv3_fdt_softc),
	gicv3_fdt_match, gicv3_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,gic-v3" },
	DEVICE_COMPAT_EOL
};

static int
gicv3_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	return of_compatible_match(phandle, compat_data);
}

static void
gicv3_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct gicv3_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &gicv3_fdt_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GICv3\n");

	sc->sc_phandle = phandle;
	sc->sc_gic.sc_dev = self;
	sc->sc_gic.sc_bst = faa->faa_bst;
	sc->sc_gic.sc_dmat = faa->faa_dmat;

	error = gicv3_fdt_map_registers(sc);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	aprint_debug_dev(self, "%d redistributors\n", sc->sc_gic.sc_bsh_r_count);

	/* Apply quirks */
	const struct device_compatible_entry *dce =
	    of_compatible_lookup(OF_finddevice("/"), gicv3_fdt_quirks);
	if (dce != NULL) {
		sc->sc_gic.sc_quirks |= dce->value;
	}

	error = gicv3_init(&sc->sc_gic);
	if (error) {
		aprint_error_dev(self, "failed to initialize GIC: %d\n", error);
		return;
	}

#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
	if (of_hasprop(phandle, "msi-controller")) {
		/* Message Based Interrupts */
		gicv3_fdt_attach_mbi(sc);
	} else {
		/* Interrupt Translation Services */
		static const struct device_compatible_entry its_compat[] = {
			{ .compat = "arm,gic-v3-its" },
			DEVICE_COMPAT_EOL
		};

		for (int child = OF_child(phandle); child;
		     child = OF_peer(child)) {
			if (!fdtbus_status_okay(child))
				continue;
			if (of_compatible_match(child, its_compat))
				gicv3_fdt_attach_its(sc, faa->faa_bst, child);
		}
	}
#endif

	arm_fdt_irq_set_handler(gicv3_irq_handler);
}

static int
gicv3_fdt_map_registers(struct gicv3_fdt_softc *sc)
{
	struct gicv3_softc *gic = &sc->sc_gic;
	const int phandle = sc->sc_phandle;
	u_int redistributor_regions, redistributor_stride;
	bus_space_handle_t bsh;
	bus_size_t size, region_off;
	bus_addr_t addr;
	size_t reg_off;
	int n, r, max_redist, redist;

	if (of_getprop_uint32(phandle, "#redistributor-regions", &redistributor_regions))
		redistributor_regions = 1;
	if (of_getprop_uint32(phandle, "redistributor-stride", &redistributor_stride))
		redistributor_stride = 0x20000;

	/*
	 * Map GIC Distributor interface (GICD)
	 */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(gic->sc_dev, "couldn't get distributor registers\n");
		return ENXIO;
	}
	if (bus_space_map(sc->sc_gic.sc_bst, addr, size, 0, &sc->sc_gic.sc_bsh_d) != 0) {
		aprint_error_dev(gic->sc_dev, "couldn't map distributor registers\n");
		return ENXIO;
	}

	/*
	 * GIC Redistributors (GICR)
	 */
	for (reg_off = 1, max_redist = 0, n = 0; n < redistributor_regions; n++, reg_off++) {
		if (fdtbus_get_reg(phandle, reg_off, NULL, &size) != 0) {
			aprint_error_dev(gic->sc_dev, "couldn't get redistributor registers\n");
			return ENXIO;
		}
		max_redist += howmany(size, redistributor_stride);
	}
	gic->sc_bsh_r = kmem_alloc(sizeof(bus_space_handle_t) * max_redist, KM_SLEEP);
	for (reg_off = 1, redist = 0, n = 0; n < redistributor_regions; n++, reg_off++) {
		if (fdtbus_get_reg(phandle, reg_off, &addr, &size) != 0) {
			aprint_error_dev(gic->sc_dev, "couldn't get redistributor registers\n");
			return ENXIO;
		}
		if (bus_space_map(sc->sc_gic.sc_bst, addr, size, 0, &bsh) != 0) {
			aprint_error_dev(gic->sc_dev, "couldn't map redistributor registers\n");
			return ENXIO;
		}
		const int count = howmany(size, redistributor_stride);
		for (r = 0, region_off = 0; r < count; r++, region_off += redistributor_stride) {
			if (bus_space_subregion(sc->sc_gic.sc_bst, bsh, region_off, redistributor_stride, &gic->sc_bsh_r[redist++]) != 0) {
				aprint_error_dev(gic->sc_dev, "couldn't subregion redistributor registers\n");
				return ENXIO;
			}

			/* If this is the last redist in this region, skip to the next one */
			const uint32_t typer = bus_space_read_4(sc->sc_gic.sc_bst, gic->sc_bsh_r[redist - 1], GICR_TYPER);
			if (typer & GICR_TYPER_Last)
				break;
		}
	}
	gic->sc_bsh_r_count = redist;

	return 0;
}

#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
static void
gicv3_fdt_attach_mbi(struct gicv3_fdt_softc *sc)
{
	struct gic_v2m_frame *frame;
	const u_int *ranges;
	bus_addr_t addr;
	int len, frame_count;

	/*
	 * If a GICD alias frame containing only the SET/CLRSPI registers
	 * exists, the base address will be reported by the 'mbi-alias'
	 * property. If this doesn't exist, use the GICD register frame
	 * instead.
	 */
	if (of_getprop_uint64(sc->sc_phandle, "mbi-alias", &addr) != 0 &&
	    fdtbus_get_reg(sc->sc_phandle, 0, &addr, NULL) != 0) {
		aprint_error_dev(sc->sc_gic.sc_dev, "couldn't find MBI register frame\n");
		return;
	}

	ranges = fdtbus_get_prop(sc->sc_phandle, "mbi-ranges", &len);
	if (ranges == NULL) {
		aprint_error_dev(sc->sc_gic.sc_dev, "missing 'mbi-ranges' property\n");
		return;
	}

	frame_count = 0;
	while (len >= 8) {
		const u_int base_spi = be32dec(&ranges[0]);
		const u_int num_spis = be32dec(&ranges[1]);

		frame = kmem_zalloc(sizeof(*frame), KM_SLEEP);
		frame->frame_reg = addr;
		frame->frame_pic = pic_list[0];
		frame->frame_base = base_spi;
		frame->frame_count = num_spis;

		if (gic_v2m_init(frame, sc->sc_gic.sc_dev, frame_count++) != 0) {
			aprint_error_dev(sc->sc_gic.sc_dev, "failed to initialize MBI frame\n");
		} else {
			aprint_normal_dev(sc->sc_gic.sc_dev, "MBI frame @ %#" PRIx64
			    ", SPIs %u-%u\n", frame->frame_reg,
			    frame->frame_base, frame->frame_base + frame->frame_count - 1);
		}

		ranges += 2;
		len -= 8;
	}
}

static void
gicv3_fdt_attach_its(struct gicv3_fdt_softc *sc, bus_space_tag_t bst, int phandle)
{
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(sc->sc_gic.sc_dev, "couldn't get ITS address\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_gic.sc_dev, "couldn't map ITS\n");
		return;
	}

	gicv3_its_init(&sc->sc_gic, bsh, addr, 0);

	aprint_verbose_dev(sc->sc_gic.sc_dev, "ITS @ %#" PRIxBUSADDR "\n",
	    addr);
}
#endif

static void *
gicv3_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct gicv3_fdt_softc * const sc = device_private(dev);
	struct gicv3_fdt_irq *firq;
	struct gicv3_fdt_irqhandler *firqh;

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */
	/* 4th cell is affinity */

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);
	const u_int trig = be32toh(specifier[2]) & 0xf;
	const u_int level = (trig & FDT_INTR_TYPE_DOUBLE_EDGE)
	    ? IST_EDGE : IST_LEVEL;

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	firq = sc->sc_irq[irq];
	if (firq == NULL) {
		firq = kmem_alloc(sizeof(*firq), KM_SLEEP);
		firq->intr_sc = sc;
		firq->intr_refcnt = 0;
		firq->intr_arg = arg;
		firq->intr_ipl = ipl;
		firq->intr_level = level;
		firq->intr_mpsafe = mpsafe;
		TAILQ_INIT(&firq->intr_handlers);
		firq->intr_irq = irq;
		if (arg == NULL) {
			firq->intr_ih = intr_establish_xname(irq, ipl,
			    level | mpsafe, func, NULL, xname);
		} else {
			firq->intr_ih = intr_establish_xname(irq, ipl,
			    level | mpsafe, gicv3_fdt_intr, firq, xname);
		}
		if (firq->intr_ih == NULL) {
			kmem_free(firq, sizeof(*firq));
			return NULL;
		}
		sc->sc_irq[irq] = firq;
	} else {
		if (firq->intr_arg == NULL && arg != NULL) {
			device_printf(dev, "cannot share irq with NULL arg\n");
			return NULL;
		}
		if (firq->intr_ipl != ipl) {
			device_printf(dev, "cannot share irq with different "
			    "ipl\n");
			return NULL;
		}
		if (firq->intr_level != level) {
			device_printf(dev, "cannot share edge and level "
			    "interrupts\n");
			return NULL;
		}
		if (firq->intr_mpsafe != mpsafe) {
			device_printf(dev, "cannot share between "
			    "mpsafe/non-mpsafe\n");
			return NULL;
		}
	}

	firq->intr_refcnt++;

	firqh = kmem_alloc(sizeof(*firqh), KM_SLEEP);
	firqh->ih_mpsafe = (flags & FDT_INTR_MPSAFE) != 0;
	firqh->ih_irq = firq;
	firqh->ih_fn = func;
	firqh->ih_arg = arg;
	TAILQ_INSERT_TAIL(&firq->intr_handlers, firqh, ih_next);

	return firq->intr_ih;
}

static void
gicv3_fdt_disestablish(device_t dev, void *ih)
{
	struct gicv3_fdt_softc * const sc = device_private(dev);
	struct gicv3_fdt_irqhandler *firqh;
	struct gicv3_fdt_irq *firq;
	u_int n;

	for (n = 0; n < GICV3_MAXIRQ; n++) {
		firq = sc->sc_irq[n];
		if (firq == NULL || firq->intr_ih != ih)
			continue;

		KASSERT(firq->intr_refcnt > 0);

		if (firq->intr_refcnt > 1)
			panic("%s: cannot disestablish shared irq", __func__);

		firqh = TAILQ_FIRST(&firq->intr_handlers);
		kmem_free(firqh, sizeof(*firqh));
		intr_disestablish(firq->intr_ih);
		kmem_free(firq, sizeof(*firq));
		sc->sc_irq[n] = NULL;
		return;
	}

	panic("%s: interrupt not established", __func__);
}

static int
gicv3_fdt_intr(void *priv)
{
	struct gicv3_fdt_irq *firq = priv;
	struct gicv3_fdt_irqhandler *firqh;
	int handled = 0;

	TAILQ_FOREACH(firqh, &firq->intr_handlers, ih_next)
		handled += firqh->ih_fn(firqh->ih_arg);

	return handled;
}

static bool
gicv3_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */
	/* 4th cell is affinity */

	if (!specifier)
		return false;
	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	snprintf(buf, buflen, "GICv3 irq %d", irq);

	return true;
}
