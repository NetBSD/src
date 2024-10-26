/* $NetBSD: jh7100_gmac.c,v 1.1 2024/10/26 15:49:43 skrll Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jh7100_gmac.c,v 1.1 2024/10/26 15:49:43 skrll Exp $");

#include <sys/param.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/fdt/fdtvar.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

#include <prop/proplib.h>

#include <riscv/starfive/jh71x0_eth.h>

struct jh7100_gmac_softc {
	struct dwc_gmac_softc	sc_gmac;
	struct jh71x0_eth_softc	sc_jh71x0eth;
};

static void
jh7100_gmac_set_speed(struct dwc_gmac_softc *gmac_sc, int speed)
{
	struct jh7100_gmac_softc * const sc =
	    container_of(gmac_sc, struct jh7100_gmac_softc, sc_gmac);
	struct jh71x0_eth_softc * const jheth_sc = &sc->sc_jh71x0eth;

	u_int rate = clk_get_rate(jheth_sc->sc_clk_tx);
	switch (speed) {
	case IFM_10_T:
		rate = 2500000;
		break;
	case IFM_100_TX:
		rate = 25000000;
		break;
	case IFM_1000_T:
		rate = 125000000;
	default:
		// error unsupported
		break;
	}

	int error = clk_set_rate(jheth_sc->sc_clk_tx, rate);
	if (error)
		aprint_error_dev(jheth_sc->sc_dev, "failed to set TX rate %u",
		    rate);
}

static int
jh7100_gmac_intr(void *arg)
{
	return dwc_gmac_intr(arg);
}

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7100-dwmac" },
	DEVICE_COMPAT_EOL
};

static int
jh7100_gmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7100_gmac_attach(device_t parent, device_t self, void *aux)
{
	struct jh7100_gmac_softc * const sc = device_private(self);
	struct jh71x0_eth_softc * const jheth_sc = &sc->sc_jh71x0eth;
	struct dwc_gmac_softc * const gmac_sc = &sc->sc_gmac;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	int error;

	sc->sc_gmac.sc_dev = self;
	sc->sc_gmac.sc_bst = faa->faa_bst;
	sc->sc_gmac.sc_dmat = faa->faa_dmat;
	sc->sc_jh71x0eth.sc_dev = self;
	sc->sc_jh71x0eth.sc_phy = MII_PHY_ANY;
	sc->sc_jh71x0eth.sc_type = JH71X0ETH_GMAC;

	error = jh71x0_eth_attach(jheth_sc, faa, &sc->sc_gmac.sc_bsh);
	if (error)
		return;

	if (sc->sc_jh71x0eth.sc_clk_tx) {
		sc->sc_gmac.sc_set_speed = jh7100_gmac_set_speed;
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	error = dwc_gmac_attach(gmac_sc, sc->sc_jh71x0eth.sc_phy,
	    GMAC_MII_CLK_150_250M_DIV102);
	if (error)
		return;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_NET,
	    FDT_INTR_MPSAFE, jh7100_gmac_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		// XXXNH unwind
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}


CFATTACH_DECL_NEW(jh7100_gmac, sizeof(struct jh7100_gmac_softc),
    jh7100_gmac_match, jh7100_gmac_attach, NULL, NULL);
