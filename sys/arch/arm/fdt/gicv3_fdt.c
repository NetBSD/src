/* $NetBSD: gicv3_fdt.c,v 1.2 2018/08/12 21:44:17 jmcneill Exp $ */

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

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gicv3_fdt.c,v 1.2 2018/08/12 21:44:17 jmcneill Exp $");

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

#define	GICV3_MAXIRQ	1020

#define	IRQ_PPI(n)	((n) + 16)
#define	IRQ_SPI(n)	((n) + 32)

struct gicv3_fdt_softc;
struct gicv3_fdt_irq;

static int	gicv3_fdt_match(device_t, cfdata_t, void *);
static void	gicv3_fdt_attach(device_t, device_t, void *);

static int	gicv3_fdt_map_registers(struct gicv3_fdt_softc *);

static int	gicv3_fdt_intr(void *);

static void *	gicv3_fdt_establish(device_t, u_int *, int, int,
		    int (*)(void *), void *);
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

CFATTACH_DECL_NEW(gicv3_fdt, sizeof(struct gicv3_fdt_softc),
	gicv3_fdt_match, gicv3_fdt_attach, NULL, NULL);

static int
gicv3_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,gic-v3",
		NULL
	};
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	return of_match_compatible(phandle, compatible);
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

	error = gicv3_fdt_map_registers(sc);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	aprint_debug_dev(self, "%d redistributors\n", sc->sc_gic.sc_bsh_r_count);

	error = gicv3_init(&sc->sc_gic);
	if (error) {
		aprint_error_dev(self, "failed to initialize GIC: %d\n", error);
		return;
	}

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
	int n, r;

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
	for (reg_off = 1, n = 0; n < redistributor_regions; n++, reg_off++) {
		if (fdtbus_get_reg(phandle, reg_off, NULL, &size) != 0) {
			aprint_error_dev(gic->sc_dev, "couldn't get redistributor registers\n");
			return ENXIO;
		}
		gic->sc_bsh_r_count += howmany(size, redistributor_stride);
	}
	gic->sc_bsh_r = kmem_alloc(sizeof(bus_space_handle_t) * gic->sc_bsh_r_count, KM_SLEEP);
	for (reg_off = 1, n = 0; n < redistributor_regions; n++, reg_off++) {
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
			if (bus_space_subregion(sc->sc_gic.sc_bst, bsh, region_off, redistributor_stride, &gic->sc_bsh_r[r]) != 0) {
				aprint_error_dev(gic->sc_dev, "couldn't subregion redistributor registers\n");
				return ENXIO;
			}
		}
	}

	return 0;
}

static void *
gicv3_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg)
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
	const u_int level = (trig & 0x3) ? IST_EDGE : IST_LEVEL;

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
			firq->intr_ih = intr_establish(irq, ipl, level | mpsafe,
			    func, NULL);
		} else {
			firq->intr_ih = intr_establish(irq, ipl, level | mpsafe,
			    gicv3_fdt_intr, firq);
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
		if (firq->intr_ih != ih)
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
