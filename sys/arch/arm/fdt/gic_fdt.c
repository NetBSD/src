/* $NetBSD: gic_fdt.c,v 1.25 2022/08/11 13:04:35 riastradh Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gic_fdt.c,v 1.25 2022/08/11 13:04:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <dev/pci/pcivar.h>

#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gic_reg.h>
#include <arm/cortex/gic_v2m.h>
#include <arm/cortex/mpcore_var.h>

#include <dev/fdt/fdtvar.h>

#define	GIC_MAXIRQ	1020

extern struct pic_softc *pic_list[];

struct gic_fdt_softc;
struct gic_fdt_irq;

static int	gic_fdt_match(device_t, cfdata_t, void *);
static void	gic_fdt_attach(device_t, device_t, void *);
#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
static void	gic_fdt_attach_v2m(struct gic_fdt_softc *, bus_space_tag_t, int);
#endif

static int	gic_fdt_intr(void *);

static void *	gic_fdt_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *, const char *);
static void	gic_fdt_disestablish(device_t, void *);
static bool	gic_fdt_intrstr(device_t, u_int *, char *, size_t);

struct fdtbus_interrupt_controller_func gic_fdt_funcs = {
	.establish = gic_fdt_establish,
	.disestablish = gic_fdt_disestablish,
	.intrstr = gic_fdt_intrstr
};

struct gic_fdt_irqhandler {
	struct gic_fdt_irq	*ih_irq;
	int			(*ih_fn)(void *);
	void			*ih_arg;
	bool			ih_mpsafe;
	TAILQ_ENTRY(gic_fdt_irqhandler) ih_next;
};

struct gic_fdt_irq {
	struct gic_fdt_softc	*intr_sc;
	void			*intr_ih;
	void			*intr_arg;
	int			intr_refcnt;
	int			intr_ipl;
	int			intr_level;
	int			intr_mpsafe;
	TAILQ_HEAD(, gic_fdt_irqhandler) intr_handlers;
	int			intr_irq;
};

struct gic_fdt_softc {
	device_t		sc_dev;
	device_t		sc_gicdev;
	int			sc_phandle;

	int			sc_v2m_count;

	struct gic_fdt_irq	*sc_irq[GIC_MAXIRQ];
};

CFATTACH_DECL_NEW(gic_fdt, sizeof(struct gic_fdt_softc),
	gic_fdt_match, gic_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,gic-400" },
	{ .compat = "arm,cortex-a15-gic" },
	{ .compat = "arm,cortex-a9-gic" },
	{ .compat = "arm,cortex-a7-gic" },
	DEVICE_COMPAT_EOL
};

#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
static const struct device_compatible_entry v2m_compat_data[] = {
	{ .compat = "arm,gic-v2m-frame" },
	DEVICE_COMPAT_EOL
};
#endif

static int
gic_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
gic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct gic_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr_d, addr_c;
	bus_size_t size_d, size_c;
	bus_space_handle_t bsh;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &gic_fdt_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GIC\n");

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr_d, &size_d) != 0) {
		aprint_error(": couldn't get distributor address\n");
		return;
	}
	if (fdtbus_get_reg(sc->sc_phandle, 1, &addr_c, &size_c) != 0) {
		aprint_error(": couldn't get cpu interface address\n");
		return;
	}

	const bus_addr_t addr = uimin(addr_d, addr_c);
	const bus_size_t end = uimax(addr_d + size_d, addr_c + size_c);
	const bus_size_t size = end - addr;

	error = bus_space_map(faa->faa_bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map registers: %d\n", error);
		return;
	}

	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "armgic",
		.mpcaa_memt = faa->faa_bst,
		.mpcaa_memh = bsh,
		.mpcaa_off1 = addr_d - addr,
		.mpcaa_off2 = addr_c - addr,
	};

	sc->sc_gicdev = config_found(self, &mpcaa, NULL, CFARGS_NONE);

	arm_fdt_irq_set_handler(armgic_irq_handler);

#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		if (of_compatible_match(child, v2m_compat_data))
			gic_fdt_attach_v2m(sc, faa->faa_bst, child);
	}
#endif
}

#if NPCI > 0 && defined(__HAVE_PCI_MSI_MSIX)
static void
gic_fdt_attach_v2m(struct gic_fdt_softc *sc, bus_space_tag_t bst, int phandle)
{
	struct gic_v2m_frame *frame;
	u_int base_spi, num_spis;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(sc->sc_gicdev, "couldn't get V2M address\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_gicdev, "couldn't map V2M frame\n");
		return;
	}
	const uint32_t typer = bus_space_read_4(bst, bsh, GIC_MSI_TYPER);
	bus_space_unmap(bst, bsh, size);

	if (of_getprop_uint32(phandle, "arm,msi-base-spi", &base_spi))
		base_spi = __SHIFTOUT(typer, GIC_MSI_TYPER_BASE);
	if (of_getprop_uint32(phandle, "arm,msi-num-spis", &num_spis))
		num_spis = __SHIFTOUT(typer, GIC_MSI_TYPER_NUMBER);

	frame = kmem_zalloc(sizeof(*frame), KM_SLEEP);
	frame->frame_reg = addr;
	frame->frame_pic = pic_list[0];
	frame->frame_base = base_spi;
	frame->frame_count = num_spis;

	if (gic_v2m_init(frame, sc->sc_gicdev, sc->sc_v2m_count++) != 0) {
		aprint_error_dev(sc->sc_gicdev, "failed to initialize GICv2m\n");
	} else {
		aprint_normal_dev(sc->sc_gicdev, "GICv2m @ %#" PRIx64
		    ", SPIs %u-%u\n", frame->frame_reg, frame->frame_base,
		    frame->frame_base + frame->frame_count - 1);
	}
}
#endif

static void *
gic_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct gic_fdt_softc * const sc = device_private(dev);
	struct gic_fdt_irq *firq;
	struct gic_fdt_irqhandler *firqh;

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

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
			    level | mpsafe, gic_fdt_intr, firq, xname);
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
gic_fdt_disestablish(device_t dev, void *ih)
{
	struct gic_fdt_softc * const sc = device_private(dev);
	struct gic_fdt_irqhandler *firqh;
	struct gic_fdt_irq *firq;
	u_int n;

	for (n = 0; n < GIC_MAXIRQ; n++) {
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
gic_fdt_intr(void *priv)
{
	struct gic_fdt_irq *firq = priv;
	struct gic_fdt_irqhandler *firqh;
	int handled = 0;

	TAILQ_FOREACH(firqh, &firq->intr_handlers, ih_next)
		handled += firqh->ih_fn(firqh->ih_arg);

	return handled;
}

static bool
gic_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	if (!specifier)
		return false;
	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	snprintf(buf, buflen, "GIC irq %d", irq);

	return true;
}
