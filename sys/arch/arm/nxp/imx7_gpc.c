/*	$NetBSD: imx7_gpc.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $	*/
/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_gpc.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $");

#include "opt_fdt.h"

#define	_INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/nxp/imx6var.h>
#include <arm/nxp/imx6_reg.h>
#include <arm/nxp/imx6_gpcreg.h>

#include <arm/cortex/gic_intr.h>

#include <dev/fdt/fdtvar.h>

#define	GPC_PCG_CPU_0_1_MAPPING		0xec
#define	 OTG2_A53_DOMAIN		__BIT(5)
#define	 OTG1_A53_DOMAIN		__BIT(4)

#define	GPC_PU_PGC_SW_PUP_REQ		0xf8
#define	 USB_OTG2_SW_PUP_REQ		__BIT(3)
#define	 USB_OTG1_SW_PUP_REQ		__BIT(2)

#define	IMXGPC_MAXCPUS	4

/* Mapping of CPU number to GPC_IMR1_COREx base offset */
static const bus_size_t imx7gpc_imr_base[IMXGPC_MAXCPUS] = {
	0x30,
	0x40,
	0x1c0,
	0x1d0,
};

#define	GPC_IMRn_COREx(n,x)	(imx7gpc_imr_base[(x)] + (n) * 0x4)

struct imx7gpc_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int imx7gpc_match(device_t, struct cfdata *, void *);
static void imx7gpc_attach(device_t, device_t, void *);

static void imx7gpc_powerup(struct imx7gpc_softc *, uint32_t, uint32_t);
static void imx7gpc_mask(struct imx7gpc_softc *, u_int, bool);
static void imx7gpc_unmask(struct imx7gpc_softc *, u_int, bool);

static void *imx7gpc_establish(device_t, u_int *, int, int,
    int (*)(void *), void *, const char *);
static void imx7gpc_disestablish(device_t, void *);
static bool imx7gpc_intrstr(device_t, u_int *, char *, size_t);

struct fdtbus_interrupt_controller_func imx7gpc_funcs = {
	.establish = imx7gpc_establish,
	.disestablish = imx7gpc_disestablish,
	.intrstr = imx7gpc_intrstr
};

CFATTACH_DECL_NEW(imx7gpc, sizeof(struct imx7gpc_softc),
    imx7gpc_match, imx7gpc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx7d-gpc" },
	{ .compat = "fsl,imx8mq-gpc" },
	DEVICE_COMPAT_EOL
};

static int
imx7gpc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx7gpc_attach(device_t parent, device_t self, void *aux)
{
	struct imx7gpc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t gpc_addr;
	bus_size_t gpc_size;
	int error;

	KASSERT(ncpu <= IMXGPC_MAXCPUS);

	if (fdtbus_get_reg(phandle, 0, &gpc_addr, &gpc_size) != 0) {
		aprint_error(": couldn't get gpc registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	error = bus_space_map(sc->sc_iot, gpc_addr, gpc_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error(": couldn't map gpc registers: %d\n", error);
		return;
	}

	error = fdtbus_register_interrupt_controller(self, faa->faa_phandle,
	    &imx7gpc_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": General Power Controller\n");

	/* XXX enable OTG power domains */
	imx7gpc_powerup(sc, USB_OTG2_SW_PUP_REQ | USB_OTG1_SW_PUP_REQ,
	    OTG2_A53_DOMAIN | OTG1_A53_DOMAIN);
}

static void
imx7gpc_powerup(struct imx7gpc_softc *sc, uint32_t req, uint32_t map)
{
	uint32_t val;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GPC_PCG_CPU_0_1_MAPPING);
	val |= map;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GPC_PCG_CPU_0_1_MAPPING, val);

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GPC_PU_PGC_SW_PUP_REQ);
	val |= req;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GPC_PU_PGC_SW_PUP_REQ, val);

	delay(5000);

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GPC_PCG_CPU_0_1_MAPPING);
	val &= ~map;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GPC_PCG_CPU_0_1_MAPPING, val);
}

static void
imx7gpc_mask(struct imx7gpc_softc *sc, u_int irq, bool mpsafe)
{
	const u_int reg = irq / 32;
	const u_int bit = irq % 32;
	uint32_t val;

	for (u_int cpu = 0; cpu < (mpsafe ? ncpu : 1); cpu++) {
		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    GPC_IMRn_COREx(reg, cpu));
		val |= __BIT(bit);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    GPC_IMRn_COREx(reg, cpu), val);
	}
}

static void
imx7gpc_unmask(struct imx7gpc_softc *sc, u_int irq, bool mpsafe)
{
	const u_int reg = irq / 32;
	const u_int bit = irq % 32;
	uint32_t val;

	for (u_int cpu = 0; cpu < (mpsafe ? ncpu : 1); cpu++) {
		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    GPC_IMRn_COREx(reg, cpu));
		val &= ~__BIT(bit);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    GPC_IMRn_COREx(reg, cpu), val);
	}
}


static void *
imx7gpc_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct imx7gpc_softc * const sc = device_private(dev);
	void *ih;

	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);
	const u_int trig = be32toh(specifier[2]) & 0xf;
	const u_int level = (trig & 0x3) ? IST_EDGE : IST_LEVEL;

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	if (type != 0)
		return NULL;	/* Only SPIs are supported */

	KASSERT(irq >= 32);

	aprint_debug_dev(dev, "intr establish irq %d, level %d\n", irq, level);

	ih = intr_establish_xname(irq, ipl, level | mpsafe, func, arg, xname);
	if (ih != NULL)
		imx7gpc_unmask(sc, irq - 32, mpsafe == IST_MPSAFE);

	return ih;
}

static void
imx7gpc_disestablish(device_t dev, void *ih)
{
	struct imx7gpc_softc * const sc = device_private(dev);
	struct intrsource *is = ih;
	const u_int irq = is->is_irq;
	const bool mpsafe = is->is_mpsafe;

	intr_disestablish(ih);
	imx7gpc_mask(sc, irq - 32, mpsafe);
}

static bool
imx7gpc_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	if (!specifier)
		return false;

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);

	snprintf(buf, buflen, "irq %d", irq);

	return true;
}
