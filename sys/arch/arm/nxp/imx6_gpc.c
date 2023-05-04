/*	$NetBSD: imx6_gpc.c,v 1.4 2023/05/04 13:29:33 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx6_gpc.c,v 1.4 2023/05/04 13:29:33 bouyer Exp $");

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cortex/gic_intr.h>
#include <arm/nxp/imx6_gpcreg.h>

struct imxgpc_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int imxgpc_match(device_t, struct cfdata *, void *);
static void imxgpc_attach(device_t, device_t, void *);

static void *imxgpc_establish(device_t, u_int *, int, int,
    int (*)(void *), void *, const char *);
static void imxgpc_disestablish(device_t, void *);
static bool imxgpc_intrstr(device_t, u_int *, char *, size_t);

struct fdtbus_interrupt_controller_func imxgpc_funcs = {
	.establish = imxgpc_establish,
	.disestablish = imxgpc_disestablish,
	.intrstr = imxgpc_intrstr
};

CFATTACH_DECL_NEW(imxgpc, sizeof(struct imxgpc_softc),
    imxgpc_match, imxgpc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-gpc" },
	{ .compat = "fsl,imx6sx-gpc" },
	DEVICE_COMPAT_EOL
};

static int
imxgpc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imxgpc_attach(device_t parent, device_t self, void *aux)
{
	struct imxgpc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t gpc_addr;
	bus_size_t gpc_size;
	int error;

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
	    &imxgpc_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": General Power Controller\n");

	return;
}

static void *
imxgpc_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	/* 1st cell is the interrupt type; 0 is SPI, 1 is PPI */
	/* 2nd cell is the interrupt number */
	/* 3rd cell is flags */

	const u_int type = be32toh(specifier[0]);
	const u_int intr = be32toh(specifier[1]);
	const u_int irq = type == 0 ? IRQ_SPI(intr) : IRQ_PPI(intr);
	const u_int trig = be32toh(specifier[2]) & 0xf;
	const u_int level = (trig & 0x3) ? IST_EDGE : IST_LEVEL;

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	aprint_debug_dev(dev, "intr establish irq %d, level %d\n", irq, level);
	return intr_establish_xname(irq, ipl, level | mpsafe, func, arg,
	  xname);
}

static void
imxgpc_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
imxgpc_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
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
