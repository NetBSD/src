/* $NetBSD: jh7110_eqos.c,v 1.1 2024/10/26 15:49:43 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: jh7110_eqos.c,v 1.1 2024/10/26 15:49:43 skrll Exp $");

#include <sys/param.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>	// maybe not

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_eqos_var.h>

#include <prop/proplib.h>

#include <riscv/starfive/jh71x0_eth.h>




struct jh7110_eqos_softc {
	struct eqos_softc	sc_eqos;
	struct jh71x0_eth_softc	sc_jh71x0eth;
};

static void
jh7110_parse_phyprops(struct jh7110_eqos_softc *jh_sc, int phandle)
{
	struct eqos_softc * const sc = &jh_sc->sc_eqos;
	prop_dictionary_t dict = device_properties(sc->sc_dev);

	if (of_hasprop(phandle, "motorcomm,tx-clk-adj-enabled")) {
		prop_dictionary_set_bool(dict, "motorcomm,tx-clk-adj-enabled",
		    true);
	}
	if (of_hasprop(phandle, "motorcomm,tx-clk-100-inverted")) {
		prop_dictionary_set_bool(dict, "motorcomm,tx-clk-100-inverted",
		    true);
	}
	if (of_hasprop(phandle, "motorcomm,tx-clk-1000-inverted")) {
		prop_dictionary_set_bool(dict, "motorcomm,tx-clk-1000-inverted",
		    true);
	}

	uint32_t val;
	if (of_getprop_uint32(phandle,
	    "motorcomm,rx-clk-drv-microamp", &val) == 0) {
		prop_dictionary_set_uint32(dict,
		    "motorcomm,rx-clk-drv-microamp", val);
	}
	if (of_getprop_uint32(phandle,
	    "motorcomm,rx-data-drv-microamp", &val) == 0) {
		prop_dictionary_set_uint32(dict,
		    "motorcomm,rx-data-drv-microamp", val);
	}
	if (of_getprop_uint32(phandle,
	    "rx-internal-delay-ps", &val) == 0) {
		prop_dictionary_set_uint32(dict,
		    "rx-internal-delay-ps", val);
	}
	if (of_getprop_uint32(phandle,
	    "tx-internal-delay-ps", &val) == 0) {
		prop_dictionary_set_uint32(dict,
		    "tx-internal-delay-ps", val);
	}
}

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-dwmac" },
	DEVICE_COMPAT_EOL
};

static int
jh7110_eqos_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}
static void
jh7110_eqos_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_eqos_softc * const sc = device_private(self);
	struct jh71x0_eth_softc * const jheth_sc = &sc->sc_jh71x0eth;
	struct eqos_softc * const eqos_sc = &sc->sc_eqos;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	int error;

	sc->sc_eqos.sc_dev = self;
	sc->sc_eqos.sc_bst = faa->faa_bst;
	sc->sc_eqos.sc_dmat = faa->faa_dmat;
	sc->sc_jh71x0eth.sc_dev = self;
	sc->sc_jh71x0eth.sc_phy = MII_PHY_ANY;
	sc->sc_jh71x0eth.sc_type = JH71X0ETH_EQOS;

	error = jh71x0_eth_attach(jheth_sc, faa, &sc->sc_eqos.sc_bsh);
	if (error)
		return;

	jh7110_parse_phyprops(sc, sc->sc_jh71x0eth.sc_phandle_phy);

//	aprint_naive("\n");
//	aprint_normal(": Gigabit Ethernet Controller\n");

#define CSR_RATE_RGMII	125000000	/* default */
	eqos_sc->sc_csr_clock = CSR_RATE_RGMII;
	error = eqos_attach(eqos_sc);
	if (error)
		return;
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_NET,
	    FDT_INTR_MPSAFE, eqos_intr, eqos_sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		// XXXNH unwind
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

CFATTACH_DECL_NEW(jh7110_eqos, sizeof(struct jh7110_eqos_softc),
    jh7110_eqos_match, jh7110_eqos_attach, NULL, NULL);
