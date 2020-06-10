/*	$NetBSD: imx6_clk.c,v 1.2 2020/06/10 17:57:50 jmcneill Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx6_clk.c,v 1.2 2020/06/10 17:57:50 jmcneill Exp $");

#include "opt_fdt.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/param.h>

#include <arm/imx/imx6_ccmvar.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>

static struct clk *imx6_clk_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func imx6_ccm_fdtclock_funcs = {
	.decode = imx6_clk_decode
};

static struct clk *
imx6_clk_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct clk *clk;

	/* #clock-cells should be 1 */
	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);

	clk = imx6_get_clock_by_id(clock_id);
	if (clk)
		return clk;

	return NULL;
}

static void
imx6_clk_fixed_from_fdt(const char *name)
{
	struct imx6_clk *iclk = (struct imx6_clk *)imx6_get_clock(name);

	KASSERT(iclk != NULL);

	char *path = kmem_asprintf("/clocks/%s", name);
	int phandle = OF_finddevice(path);
	kmem_free(path, strlen(path) + 1);

	if (of_getprop_uint32(phandle, "clock-frequency", &iclk->clk.fixed.rate) != 0)
		iclk->clk.fixed.rate = 0;
}

static int imxccm_match(device_t, cfdata_t, void *);
static void imxccm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx6ccm, sizeof(struct imxccm_softc),
    imxccm_match, imxccm_attach, NULL, NULL);

static int
imxccm_match(device_t parent, cfdata_t cfdata, void *aux)
{
	const char * const compatible[] = { "fsl,imx6q-ccm", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
imxccm_attach(device_t parent, device_t self, void *aux)
{
	struct imxccm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error(": can't map ccm registers\n");
		return;
	}

	int phandle = OF_finddevice("/soc/aips-bus/anatop");
	fdtbus_get_reg(phandle, 0, &addr, &size);

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh_analog)) {
		aprint_error(": can't map anatop registers\n");
		return;
	}

	imxccm_attach_common(self);

	imx6_clk_fixed_from_fdt("ckil");
	imx6_clk_fixed_from_fdt("ckih");
	imx6_clk_fixed_from_fdt("osc");
	imx6_clk_fixed_from_fdt("anaclk1");
	imx6_clk_fixed_from_fdt("anaclk2");

	fdtbus_register_clock_controller(self, faa->faa_phandle,
	    &imx6_ccm_fdtclock_funcs);
}

