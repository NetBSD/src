/* $NetBSD: jh71x0_eth.c,v 1.1 2024/10/26 15:49:43 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: jh71x0_eth.c,v 1.1 2024/10/26 15:49:43 skrll Exp $");

#include <sys/param.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/mii/miivar.h>

//#include <prop/proplib.h>

#include <riscv/starfive/jh71x0_eth.h>


/* Register definitions */

#define	STARFIVE_GMAC_PHY_INFT_RGMII		0x1
#define	STARFIVE_GMAC_PHY_INFT_RMII		0x4
#define	STARFIVE_GMAC_PHY_INFT_MASK		__BITS(2, 0)

#define JH7100_SYSMAIN_REGISTER49_DLYCHAIN	0xc8

static void
jh71x0_eth_set_phy_rgmii_id(struct jh71x0_eth_softc *jh_sc)
{
	const uint32_t reg = jh_sc->sc_phy_syscon_reg;
	const uint32_t shift = jh_sc->sc_phy_syscon_shift;

	syscon_lock(jh_sc->sc_syscon);
	uint32_t val = syscon_read_4(jh_sc->sc_syscon, reg);
	val &= ~(STARFIVE_GMAC_PHY_INFT_MASK << shift);
	val |= STARFIVE_GMAC_PHY_INFT_RGMII << shift;
	syscon_write_4(jh_sc->sc_syscon, reg, val);

	if (jh_sc->sc_type == JH71X0ETH_GMAC) {
		syscon_write_4(jh_sc->sc_syscon,
		    JH7100_SYSMAIN_REGISTER49_DLYCHAIN, 4);
	}

	syscon_unlock(jh_sc->sc_syscon);
}


int
jh71x0_eth_attach(struct jh71x0_eth_softc *jh_sc, struct fdt_attach_args *faa,
    bus_space_handle_t *bshp)
{
	const int phandle = faa->faa_phandle;
	int phandle_phy;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	int n;

	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}

	int len;
	const u_int * const syscon_data =
	    fdtbus_get_prop(phandle, "starfive,syscon", &len);
	if (syscon_data == NULL) {
		aprint_error(": couldn't get 'starfive,syscon' property\n");
		return ENXIO;
	}
	if (len != 3 * sizeof(uint32_t)) {
		aprint_error(": incorrect syscon data (len = %u)\n",
		    len);
		return ENXIO;
	}

	const int syscon_phandle =
	    fdtbus_get_phandle_from_native(be32dec(&syscon_data[0]));

	jh_sc->sc_syscon = fdtbus_syscon_lookup(syscon_phandle);
	if (jh_sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon\n");
		return ENXIO;
	}

	jh_sc->sc_phy_syscon_reg = be32dec(&syscon_data[1]);
	jh_sc->sc_phy_syscon_shift = be32dec(&syscon_data[2]);

	if (bus_space_map(faa->faa_bst, addr, size, 0, bshp) != 0) {
		aprint_error(": couldn't map registers\n");
		return ENXIO;
	}

	/* enable clocks */
	struct clk *clk;
	fdtbus_clock_assign(phandle);
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return ENXIO;
		}
	}
	/* de-assert resets */
	struct fdtbus_reset *rst;
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return ENXIO;
		}
	}

	const char *phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL)
		phy_mode = "rgmii-id";	/* default: RGMII */

	phandle_phy = fdtbus_get_phandle(phandle, "phy-handle");
	if (phandle_phy > 0) {
		of_getprop_uint32(phandle_phy, "reg", &jh_sc->sc_phy);
	}
	jh_sc->sc_phandle_phy = phandle_phy;

	if (strncmp(phy_mode, "rgmii-id", 8) == 0) {
		jh71x0_eth_set_phy_rgmii_id(jh_sc);
	} else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		return ENXIO;
	}

	if (!of_hasprop(phandle, "starfive,tx-use-rgmii-clk")) {
		jh_sc->sc_clk_tx = fdtbus_clock_get(phandle, "tx");
	}

	const uint8_t *macaddr = fdtbus_get_prop(phandle, "local-mac-address",
	   &len);
	if (macaddr != NULL && len == ETHER_ADDR_LEN) {
		prop_dictionary_t prop = device_properties(jh_sc->sc_dev);
		prop_dictionary_set_data(prop, "mac-address", macaddr, len);
	}

	return 0;
}

