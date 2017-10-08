/* $NetBSD: sun9i_a80_mmcclk.c,v 1.1 2017/10/08 18:00:36 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sun9i_a80_mmcclk.c,v 1.1 2017/10/08 18:00:36 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>

#define	SDC_COMM(port)	(0x04 * (port))

static int sun9i_a80_mmcclk_match(device_t, cfdata_t, void *);
static void sun9i_a80_mmcclk_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"allwinner,sun9i-a80-mmc-config-clk",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a80_mmcclk, sizeof(struct sunxi_ccu_softc),
	sun9i_a80_mmcclk_match, sun9i_a80_mmcclk_attach, NULL, NULL);

static struct sunxi_ccu_reset sun9i_a80_mmcclk_resets[] = {
	SUNXI_CCU_RESET(0, SDC_COMM(0), 18),
	SUNXI_CCU_RESET(1, SDC_COMM(1), 18),
	SUNXI_CCU_RESET(2, SDC_COMM(2), 18),
	SUNXI_CCU_RESET(3, SDC_COMM(3), 18),
};

static struct sunxi_ccu_clk sun9i_a80_mmcclk_clks[] = {
	SUNXI_CCU_GATE(0, "mmc0_config", "ahb", SDC_COMM(0), 16),
	SUNXI_CCU_GATE(1, "mmc1_config", "ahb", SDC_COMM(1), 16),
	SUNXI_CCU_GATE(2, "mmc2_config", "ahb", SDC_COMM(2), 16),
	SUNXI_CCU_GATE(3, "mmc3_config", "ahb", SDC_COMM(3), 16),
};

static int
sun9i_a80_mmcclk_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun9i_a80_mmcclk_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun9i_a80_mmcclk_resets;
	sc->sc_nresets = __arraycount(sun9i_a80_mmcclk_resets);

	sc->sc_clks = sun9i_a80_mmcclk_clks;
	sc->sc_nclks = __arraycount(sun9i_a80_mmcclk_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A80 SD/MMC-COMM\n");

	sunxi_ccu_print(sc);
}
