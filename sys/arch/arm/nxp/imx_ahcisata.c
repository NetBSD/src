/*	$NetBSD: imx_ahcisata.c,v 1.4 2022/02/06 20:20:19 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx_ahcisata.c,v 1.4 2022/02/06 20:20:19 andvar Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <arm/nxp/imx_ahcisatareg.h>
#include <arm/nxp/imx6_iomuxreg.h>
#include <arm/nxp/imx6_ccmreg.h>
#include <arm/nxp/imx6_ccmvar.h>

#include <dev/fdt/fdtvar.h>

static int imx_ahcisata_match(device_t, cfdata_t, void *);
static void imx_ahcisata_attach(device_t, device_t, void *);

struct imx_ahcisata_softc {
	struct ahci_softc sc;

	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_gpr_ioh;
	void *sc_ih;

	u_int sc_tx_level;
	u_int sc_tx_boost;
	u_int sc_tx_atten;
	u_int sc_rx_eq;
	u_int sc_ss;

	struct clk *sc_clk_sata;
	struct clk *sc_clk_sata_ref;
	struct clk *sc_clk_ahb;
};

static int imx_ahcisata_init(struct imx_ahcisata_softc *);
static int imx_ahcisata_phy_ctrl(struct imx_ahcisata_softc *, uint32_t, int);
static int imx_ahcisata_phy_addr(struct imx_ahcisata_softc *, uint32_t);
static int imx_ahcisata_phy_write(struct imx_ahcisata_softc *, uint32_t, uint16_t);
static int imx_ahcisata_phy_read(struct imx_ahcisata_softc *, uint32_t);
static int imx_ahcisata_init_clocks(struct imx_ahcisata_softc *);

CFATTACH_DECL_NEW(imx_ahcisata, sizeof(struct imx_ahcisata_softc),
	imx_ahcisata_match, imx_ahcisata_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-ahci" },
	DEVICE_COMPAT_EOL
};

static int
imx_ahcisata_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct imx_ahcisata_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t ahci_addr;
	bus_size_t ahci_size;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	int error;

	if (fdtbus_get_reg(phandle, 0, &ahci_addr, &ahci_size) != 0) {
		aprint_error(": couldn't get ahci registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "fsl,transmit-level-mV", &sc->sc_tx_level) != 0)
		sc->sc_tx_level = 1104;
	if (of_getprop_uint32(phandle, "fsl,transmit-boost-mdB", &sc->sc_tx_boost) != 0)
		sc->sc_tx_boost = 3330;
	if (of_getprop_uint32(phandle, "fsl,transmit-atten-16ths", &sc->sc_tx_atten) != 0)
		sc->sc_tx_atten = 9;
	if (of_getprop_uint32(phandle, "fsl,receive-eq-mdB", &sc->sc_rx_eq) != 0)
		sc->sc_rx_eq = 3000;
	if (of_getprop_bool(phandle, "fsl,no-spread-spectrum") == false)
		sc->sc_ss = 1;
	else
		sc->sc_ss = 0;

	sc->sc_clk_sata = fdtbus_clock_get(phandle, "sata");
	if (sc->sc_clk_sata == NULL) {
		aprint_error(": couldn't get clock sata\n");
		return;
	}
	sc->sc_clk_sata_ref = fdtbus_clock_get(phandle, "sata_ref");
	if (sc->sc_clk_sata_ref == NULL) {
		aprint_error(": couldn't get clock sata_ref\n");
		return;
	}
	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "ahb");
	if (sc->sc_clk_ahb == NULL) {
		aprint_error(": couldn't get clock ahb\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": AHCI Controller\n");

	aprint_debug_dev(self, "tx level %d [mV]\n", sc->sc_tx_level);
	aprint_debug_dev(self, "tx boost %d [mdB]\n", sc->sc_tx_boost);
	aprint_debug_dev(self, "tx atten %d [16ths]\n", sc->sc_tx_atten);
	aprint_debug_dev(self, "rx eq    %d [mdB]\n", sc->sc_rx_eq);
	aprint_debug_dev(self, "ss       %d\n", sc->sc_ss);

	sc->sc_dev = self;

	sc->sc.sc_atac.atac_dev = self;
	sc->sc.sc_ahci_ports = 1;
	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_ahcit = faa->faa_bst;
	sc->sc.sc_ahcis = ahci_size;
	error = bus_space_map(sc->sc.sc_ahcit, ahci_addr, ahci_size, 0,
	    &sc->sc.sc_ahcih);
	if (error) {
		aprint_error(": couldn't map ahci registers: %d\n", error);
		return;
	}

	sc->sc_iot = sc->sc.sc_ahcit;
	sc->sc_ioh = sc->sc.sc_ahcih;

	const int gpr_phandle = OF_finddevice("/soc/aips-bus/iomuxc-gpr");
	fdtbus_get_reg(gpr_phandle, 0, &addr, &size);
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_gpr_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	if (imx_ahcisata_init_clocks(sc) != 0) {
		aprint_error_dev(self, "couldn't init clocks\n");
		return;
	}

	if (imx_ahcisata_init(sc) != 0) {
		aprint_error_dev(self, "couldn't init ahci\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_BIO, 0,
	    ahci_intr, &sc->sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	ahci_attach(&sc->sc);
}

static int
imx_ahcisata_phy_ctrl(struct imx_ahcisata_softc *sc, uint32_t bitmask, int on)
{
	uint32_t v;
	int timeout;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR);
	if (on)
		v |= bitmask;
	else
		v &= ~bitmask;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, v);

	for (timeout = 5000; timeout > 0; --timeout) {
		v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYSR);
		if (!!(v & SATA_P0PHYSR_CR_ACK) == !!on)
			break;
		delay(100);
	}

	if (timeout > 0)
		return 0;

	return -1;
}

static int
imx_ahcisata_phy_addr(struct imx_ahcisata_softc *sc, uint32_t addr)
{
	delay(100);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, addr);

	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_ADDR, 1) != 0)
		return -1;
	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_ADDR, 0) != 0)
		return -1;

	return 0;
}

static int
imx_ahcisata_phy_write(struct imx_ahcisata_softc *sc, uint32_t addr,
                        uint16_t data)
{
	if (imx_ahcisata_phy_addr(sc, addr) != 0)
		return -1;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, data);

	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_DATA, 1) != 0)
		return -1;
	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_DATA, 0) != 0)
		return -1;

	if ((addr == SATA_PHY_CLOCK_RESET) && data) {
		/* we can't check ACK after RESET */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR,
		    data | SATA_P0PHYCR_CR_WRITE);
		return 0;
	}

	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_WRITE, 1) != 0)
		return -1;
	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_WRITE, 0) != 0)
		return -1;

	return 0;
}

static int
imx_ahcisata_phy_read(struct imx_ahcisata_softc *sc, uint32_t addr)
{
	uint32_t v;

	if (imx_ahcisata_phy_addr(sc, addr) != 0)
		return -1;

	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_READ, 1) != 0)
		return -1;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYSR);

	if (imx_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_READ, 0) != 0)
		return -1;

	return SATA_P0PHYSR_CR_DATA_OUT(v);
}

const static int tx_level[] = {
	 937,
	 947,
	 957,
	 966,
	 976,
	 986,
	 996,
	1005,
	1015,
	1025,
	1035,
	1045,
	1054,
	1064,
	1074,
	1084,
	1094,
	1104,
	1113,
	1123,
	1133,
	1143,
	1152,
	1162,
	1172,
	1182,
	1191,
	1201,
	1211,
	1221,
	1230,
	1240,
};

const static int tx_boots[] = {
	   0,
	 370,
	 740,
	1110,
	1480,
	1850,
	2220,
	2590,
	2960,
	3330,
	3700,
	4070,
	4440,
	4810,
	5280,
	5750,
};

const static int tx_atten[] = {
	  16,
	  14,
	  12,
	  10,
	   9,
	   8,
};

const static int rx_eq[] = {
	 500,
	1000,
	1500,
	2000,
	2500,
	3000,
	3500,
	4000,
};

static int
imx_ahcisata_search_regval(const int *values, int count, int val)
{
	for (int i = 0; i < count; i++)
		if (values[i] == val)
			return i;

	return -1;
}

static int
imx_ahcisata_init(struct imx_ahcisata_softc *sc)
{
	uint32_t v;
	int timeout;
	int pllstat;

	v = bus_space_read_4(sc->sc_iot, sc->sc_gpr_ioh, IOMUX_GPR13);
	/* clear */
	v &= ~(IOMUX_GPR13_SATA_PHY_8 |
	    IOMUX_GPR13_SATA_PHY_7 |
	    IOMUX_GPR13_SATA_PHY_6 |
	    IOMUX_GPR13_SATA_SPEED |
	    IOMUX_GPR13_SATA_PHY_5 |
	    IOMUX_GPR13_SATA_PHY_4 |
	    IOMUX_GPR13_SATA_PHY_3 |
	    IOMUX_GPR13_SATA_PHY_2 |
	    IOMUX_GPR13_SATA_PHY_1 |
	    IOMUX_GPR13_SATA_PHY_0);
	/* setting */
	struct {
		const int *array;
		int count;
		int val;
		int def_val;
		int mask;
	} gpr13_sata_phy_settings[] = {
		{ tx_level, __arraycount(tx_level), sc->sc_tx_level,
		  0x11, IOMUX_GPR13_SATA_PHY_2 },
		{ tx_boots, __arraycount(tx_boots), sc->sc_tx_boost,
		  0x09, IOMUX_GPR13_SATA_PHY_3 },
		{ tx_atten, __arraycount(tx_atten), sc->sc_tx_atten,
		  0x04, IOMUX_GPR13_SATA_PHY_4 },
		{ rx_eq, __arraycount(rx_eq), sc->sc_rx_eq,
		  0x05, IOMUX_GPR13_SATA_PHY_8 }
	};
	for (int i = 0; i < __arraycount(gpr13_sata_phy_settings); i++) {
		int val;
		val = imx_ahcisata_search_regval(
			gpr13_sata_phy_settings[i].array,
			gpr13_sata_phy_settings[i].count,
			gpr13_sata_phy_settings[i].val);
		if (val == -1)
			val = gpr13_sata_phy_settings[i].def_val;
		v |= __SHIFTIN(val, gpr13_sata_phy_settings[i].mask);
	}
	v |= __SHIFTIN(0x12, IOMUX_GPR13_SATA_PHY_7);	/* Rx SATA2m */
	v |= __SHIFTIN(3, IOMUX_GPR13_SATA_PHY_6);	/* Rx DPLL mode */
	v |= __SHIFTIN(1, IOMUX_GPR13_SATA_SPEED);	/* 3.0GHz */
	v |= __SHIFTIN(sc->sc_ss, IOMUX_GPR13_SATA_PHY_5);
	v |= __SHIFTIN(1, IOMUX_GPR13_SATA_PHY_1);	/* PLL clock enable */
	bus_space_write_4(sc->sc_iot, sc->sc_gpr_ioh, IOMUX_GPR13, v);

	/* phy reset */
	if (imx_ahcisata_phy_write(sc, SATA_PHY_CLOCK_RESET,
	    SATA_PHY_CLOCK_RESET_RST) < 0) {
		aprint_error_dev(sc->sc_dev, "cannot reset PHY\n");
		return -1;
	}

	for (timeout = 50; timeout > 0; --timeout) {
		delay(100);
		pllstat = imx_ahcisata_phy_read(sc, SATA_PHY_LANE0_OUT_STAT);
		if (pllstat < 0) {
			aprint_error_dev(sc->sc_dev,
			    "cannot read LANE0 status\n");
			break;
		}
		if (pllstat & SATA_PHY_LANE0_OUT_STAT_RX_PLL_STATE)
			break;
	}
	if (timeout <= 0)
		return -1;

	/* Support Staggered Spin-up */
	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_CAP);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_CAP, v | SATA_CAP_SSS);

	/* Ports Implemented. must set 1 */
	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_PI);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_PI, v | SATA_PI_PI);

	/* set 1ms-timer = AHB clock / 1000 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_TIMER1MS,
	    clk_get_rate(sc->sc_clk_ahb) / 1000);

	return 0;
}

static int
imx_ahcisata_init_clocks(struct imx_ahcisata_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk_sata);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable sata: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_sata_ref);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable sata-ref: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_ahb);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable anb: %d\n", error);
		return error;
	}

	return 0;
}
