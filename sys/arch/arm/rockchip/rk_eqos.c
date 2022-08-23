/*	$NetBSD: rk_eqos.c,v 1.1 2022/08/23 05:40:46 ryo Exp $	*/

/*-
 * Copyright (c) 2022 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "opt_net_mpsafe.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk_eqos.c,v 1.1 2022/08/23 05:40:46 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/mii/miivar.h>
#include <dev/ic/dwc_eqos_var.h>

struct rk_eqos_softc {
	struct eqos_softc sc_base;

	struct syscon *sc_grf;
	struct syscon *sc_php_grf;
	int sc_id;	/* ethernet0 or 1? */
};

static int rk_eqos_match(device_t, cfdata_t, void *);
static void rk_eqos_attach(device_t, device_t, void *);

struct rk_eqos_ops {
	void (*set_mode_rgmii)(struct rk_eqos_softc *, int, int);
	void (*set_speed_rgmii)(struct rk_eqos_softc *, int);
	void (*clock_selection)(struct rk_eqos_softc *, int);
	int (*get_unit)(struct rk_eqos_softc *, int);
};

CFATTACH_DECL_NEW(rk_eqos, sizeof(struct rk_eqos_softc),
    rk_eqos_match, rk_eqos_attach, NULL, NULL);

/*
 * RK3588 specific
 */
#define RK3588_ETHERNET1_ADDR			0xfe1c0000

/* grf */
#define RK3588_GRF_GMAC_TXRXCLK_DELAY_EN_REG	(0x0300 + (4 * 7))
#define  RK3588_GMAC_RXCLK_DELAY_EN(id)		__BIT(3 + 2 * (id))
#define   RK3588_GMAC_RXCLK_DELAY_DISABLE	0
#define   RK3588_GMAC_RXCLK_DELAY_ENABLE	1
#define  RK3588_GMAC_TXCLK_DELAY_EN(id)		__BIT(2 + 2 * (id))
#define   RK3588_GMAC_TXCLK_DELAY_DISABLE	0
#define   RK3588_GMAC_TXCLK_DELAY_ENABLE	1
#define RK3588_GRF_GMAC_TXRX_DELAY_CFG_REG(id)	(0x0300 + (4 * 8) + (4 * (id)))
#define  RK3588_GMAC_RXCLK_DELAY_CFG		__BITS(15,8)
#define  RK3588_GMAC_TXCLK_DELAY_CFG		__BITS(7,0)

/* grf_php */
#define RK3588_GRF_GMAC_PHY_REG			0x0008
#define  RK3588_GMAC_PHY_IFACE_SEL(id)		(__BITS(5,3) << ((id) * 6))
#define   RK3588_GMAC_PHY_IFACE_SEL_RGMII	1
#define   RK3588_GMAC_PHY_IFACE_SEL_RMII	4
#define RK3588_GRF_CLK_CON1			0x0070
#define RK3588_GRF_GMAC_CLK_REG			0x0070
#define  RK3588_GMAC_CLK_SELECT(id)		__BIT(4 + 5 * (id))
#define   RK3588_GMAC_CLK_SELECT_IO		0
#define   RK3588_GMAC_CLK_SELECT_CRU		1
#define  RK3588_GMAC_CLK_RMII_DIV(id)		__BIT(2 + 5 * (id))
#define   RK3588_GMA_CLK_RMII_DIV_DIV20		0
#define   RK3588_GMA_CLK_RMII_DIV_DIV2		1
#define  RK3588_GMAC_CLK_RGMII_DIV(id)		(__BITS(3,2) << ((id) * 5))
#define   RK3588_GMAC_CLK_RGMII_DIV_DIV1	1
#define   RK3588_GMAC_CLK_RGMII_DIV_DIV50	2
#define   RK3588_GMAC_CLK_RGMII_DIV_DIV5	3
#define  RK3588_GMAC_CLK_RMII_GATE_EN(id)	__BIT(1 + (id) * 5)
#define   RK3588_GMAC_CLK_RMII_GATE_DISABLE	0
#define   RK3588_GMAC_CLK_RMII_GATE_ENABLE	1
#define  RK3588_GMAC_CLK_MODE(id)		__BIT(0 + (id) * 5)
#define   RK3588_GMAC_CLK_MODE_RGMII		0
#define   RK3588_GMAC_CLK_MODE_RMII		1

static void
rk3588_eqos_set_mode_rgmii(struct rk_eqos_softc *rk_sc,
    int tx_delay, int rx_delay)
{
	const int id = rk_sc->sc_id;
	uint32_t txen, rxen;

	if (tx_delay >= 0) {
		txen = RK3588_GMAC_TXCLK_DELAY_ENABLE;
	} else {
		txen = RK3588_GMAC_TXCLK_DELAY_DISABLE;
		tx_delay = 0;
	}
	if (rx_delay >= 0) {
		rxen = RK3588_GMAC_RXCLK_DELAY_ENABLE;
	} else {
		rxen = RK3588_GMAC_RXCLK_DELAY_DISABLE;
		rx_delay = 0;
	}

	syscon_lock(rk_sc->sc_grf);
	syscon_write_4(rk_sc->sc_grf, RK3588_GRF_GMAC_TXRXCLK_DELAY_EN_REG,
	    RK3588_GMAC_TXCLK_DELAY_EN(id) << 16 |		/* masks */
	    RK3588_GMAC_RXCLK_DELAY_EN(id) << 16 |
	    __SHIFTIN(txen, RK3588_GMAC_TXCLK_DELAY_EN(id)) |	/* values */
	    __SHIFTIN(rxen, RK3588_GMAC_RXCLK_DELAY_EN(id)));
	syscon_write_4(rk_sc->sc_grf, RK3588_GRF_GMAC_TXRX_DELAY_CFG_REG(id),
	    RK3588_GMAC_TXCLK_DELAY_CFG << 16 |			/* masks */
	    RK3588_GMAC_RXCLK_DELAY_CFG << 16 |
	    __SHIFTIN(tx_delay, RK3588_GMAC_TXCLK_DELAY_CFG) |	/* values */
	    __SHIFTIN(rx_delay, RK3588_GMAC_RXCLK_DELAY_CFG));
	syscon_unlock(rk_sc->sc_grf);

	syscon_lock(rk_sc->sc_php_grf);
	syscon_write_4(rk_sc->sc_php_grf, RK3588_GRF_GMAC_PHY_REG,
	    RK3588_GMAC_PHY_IFACE_SEL(id) << 16 |		/* mask */
	    __SHIFTIN(RK3588_GMAC_PHY_IFACE_SEL_RGMII,		/* value */
	    RK3588_GMAC_PHY_IFACE_SEL(id)));
	syscon_write_4(rk_sc->sc_php_grf, RK3588_GRF_GMAC_CLK_REG,
	    RK3588_GMAC_CLK_MODE(id) << 16 |			/* mask */
	    __SHIFTIN(RK3588_GMAC_CLK_MODE_RGMII,		/* value */
	    RK3588_GMAC_CLK_MODE(id)));
	syscon_unlock(rk_sc->sc_php_grf);
}

static void
rk3588_eqos_set_speed_rgmii(struct rk_eqos_softc *rk_sc, int speed)
{
	const int id = rk_sc->sc_id;
	u_int clksel;

	switch (speed) {
	case IFM_10_T:
		clksel = RK3588_GMAC_CLK_RGMII_DIV_DIV50;
		break;
	case IFM_100_TX:
		clksel = RK3588_GMAC_CLK_RGMII_DIV_DIV5;
		break;
	case IFM_1000_T:
	default:
		clksel = RK3588_GMAC_CLK_RGMII_DIV_DIV1;
		break;
	}

	syscon_lock(rk_sc->sc_php_grf);
	syscon_write_4(rk_sc->sc_php_grf, RK3588_GRF_GMAC_CLK_REG,
	    RK3588_GMAC_CLK_RGMII_DIV(id) << 16 |		/* mask */
	    __SHIFTIN(clksel, RK3588_GMAC_CLK_RGMII_DIV(id)));	/* value */
	syscon_unlock(rk_sc->sc_php_grf);
}

static void
rk3588_eqos_clock_selection(struct rk_eqos_softc *rk_sc, int phandle)
{
	const int id = rk_sc->sc_id;
	const char *clock_in_out;

	clock_in_out = fdtbus_get_string(phandle, "clock_in_out");
	if (clock_in_out != NULL) {
		bool input = (strcmp(clock_in_out, "input") == 0) ?
		    true : false;
		uint32_t clksel, gate;

		if (input) {
			clksel = RK3588_GMAC_CLK_SELECT_IO;
			gate = RK3588_GMAC_CLK_RMII_GATE_DISABLE;
		} else {
			clksel = RK3588_GMAC_CLK_SELECT_CRU;
			gate = RK3588_GMAC_CLK_RMII_GATE_ENABLE;
		}

		syscon_lock(rk_sc->sc_php_grf);
		syscon_write_4(rk_sc->sc_php_grf, RK3588_GRF_GMAC_CLK_REG,
		    /* masks */
		    RK3588_GMAC_CLK_SELECT(id) << 16 |
		    RK3588_GMAC_CLK_RMII_GATE_EN(id) << 16 |
		    /* values */
		    __SHIFTIN(clksel, RK3588_GMAC_CLK_SELECT(id)) |
		    __SHIFTIN(gate, RK3588_GMAC_CLK_RMII_GATE_EN(id)));
		syscon_unlock(rk_sc->sc_php_grf);
	}
}

static int
rk3588_eqos_get_unit(struct rk_eqos_softc *rk_sc, int phandle)
{
	bus_addr_t addr;
	bus_size_t size;

	fdtbus_get_reg(phandle, 0, &addr, &size);
	if (addr == RK3588_ETHERNET1_ADDR)
		return 1;
	return 0;
}

static const struct rk_eqos_ops rk3588_ops = {
	.set_mode_rgmii = rk3588_eqos_set_mode_rgmii,
	.set_speed_rgmii = rk3588_eqos_set_speed_rgmii,
	.clock_selection = rk3588_eqos_clock_selection,
	.get_unit = rk3588_eqos_get_unit
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3588-gmac", .value = (uintptr_t)&rk3588_ops },
	DEVICE_COMPAT_EOL
};

static int
rk_eqos_reset_gpio(const int phandle)
{
	struct fdtbus_gpio_pin *pin_reset;
	const u_int *reset_delay_us;
	bool reset_active_low;
	int len;

	if (!of_hasprop(phandle, "snps,reset-gpio"))
		return 0;

	pin_reset = fdtbus_gpio_acquire(phandle, "snps,reset-gpio",
	    GPIO_PIN_OUTPUT);
	if (pin_reset == NULL)
		return ENOENT;

	reset_delay_us = fdtbus_get_prop(phandle, "snps,reset-delays-us", &len);
	if (reset_delay_us == NULL || len != 12)
		return ENXIO;

	reset_active_low = of_hasprop(phandle, "snps,reset-active-low");

	fdtbus_gpio_write_raw(pin_reset, reset_active_low ? 1 : 0);
	delay(be32toh(reset_delay_us[0]));
	fdtbus_gpio_write_raw(pin_reset, reset_active_low ? 0 : 1);
	delay(be32toh(reset_delay_us[1]));
	fdtbus_gpio_write_raw(pin_reset, reset_active_low ? 1 : 0);
	delay(be32toh(reset_delay_us[2]));

	return 0;
}

static void
rk_eqos_init_props(struct eqos_softc *sc, int phandle)
{
	prop_dictionary_t prop = device_properties(sc->sc_dev);

	/* Defaults */
	prop_dictionary_set_uint(prop, "snps,wr_osr_lmt", 4);
	prop_dictionary_set_uint(prop, "snps,rd_osr_lmt", 8);

	if (of_hasprop(phandle, "snps,mixed-burst"))
		prop_dictionary_set_bool(prop, "snps,mixed-burst", true);
	if (of_hasprop(phandle, "snps,tso"))
		prop_dictionary_set_bool(prop, "snps,tso", true);
}

static int
rk_eqos_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_eqos_attach(device_t parent, device_t self, void *aux)
{
	struct rk_eqos_softc * const rk_sc = device_private(self);
	struct eqos_softc * const sc = &rk_sc->sc_base;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *phy_mode;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int tx_delay, rx_delay;
	int n;

	struct rk_eqos_ops *ops = (struct rk_eqos_ops *)
	    of_compatible_lookup(phandle, compat_data)->value;

	/* multiple ethernet? */
	if (ops->get_unit != NULL)
		rk_sc->sc_id = ops->get_unit(rk_sc, phandle);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	rk_sc->sc_grf = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (rk_sc->sc_grf == NULL) {
		aprint_error(": couldn't get grf syscon\n");
		return;
	}
	rk_sc->sc_php_grf = fdtbus_syscon_acquire(phandle, "rockchip,php_grf");
	if (rk_sc->sc_php_grf == NULL) {
		aprint_error(": couldn't get php_grf syscon\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_dmat = faa->faa_dmat;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	/* enable clocks */
	struct clk *clk;
	fdtbus_clock_assign(phandle);
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	}
	/* de-assert resets */
	struct fdtbus_reset *rst;
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}
	}
	if (rk_eqos_reset_gpio(phandle) != 0)
		aprint_error(": GPIO reset failed\n");	/* ignore */

	if (ops->clock_selection != NULL)
		ops->clock_selection(rk_sc, phandle);

	if (of_getprop_uint32(phandle, "tx_delay", &tx_delay) != 0)
		tx_delay = -1;
	if (of_getprop_uint32(phandle, "rx_delay", &rx_delay) != 0)
		rx_delay = -1;

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL)
		phy_mode = "rgmii";	/* default: RGMII */

	if (strncmp(phy_mode, "rgmii", 5) == 0) {
		ops->set_mode_rgmii(rk_sc, tx_delay, rx_delay);
		if (ops->set_speed_rgmii != NULL) {
			/*
			 * XXX: should be called back from
			 *  sys/dev/ic/dwc_eqos.c:eqos_update_link() ?
			 */
			ops->set_speed_rgmii(rk_sc, IFM_1000_T);
		}
	} else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		return;
	}

	rk_eqos_init_props(sc, phandle);
	sc->sc_phy_id = MII_PHY_ANY;
#define CSR_RATE_RGMII	125000000	/* default */
	sc->sc_csr_clock = CSR_RATE_RGMII;

	if (eqos_attach(sc) != 0)
		return;

#ifdef NET_MPSAFE
#define FDT_INTR_FLAGS	FDT_INTR_MPSAFE
#else
#define FDT_INTR_FLAGS	0
#endif
	if (fdtbus_intr_establish_xname(phandle, 0, IPL_NET, FDT_INTR_FLAGS,
	    eqos_intr, sc, device_xname(self)) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}
