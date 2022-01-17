/* $NetBSD: arasan_sdhc_fdt.c,v 1.7 2022/01/17 14:00:47 skrll Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: arasan_sdhc_fdt.c,v 1.7 2022/01/17 14:00:47 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	RK3399_GRF_EMMCCORE_CON0	0xf000
#define	 RK3399_CORECFG_BASECLKFREQ		__BITS(15,8)
#define	 RK3399_CORECFG_TIMEOUTCLKUNIT		__BIT(7)
#define	 RK3399_CORECFG_TUNINGCOUNT		__BITS(5,0)
#define	RK3399_GRF_EMMCCORE_CON11	0xf02c
#define	 RK3399_CORECFG_CLOCKMULTIPLIER		__BITS(7,0)

enum arasan_sdhc_type {
	AS_TYPE_RK3399 = 1,
};

struct arasan_sdhc_softc {
	struct sdhc_softc	sc_base;
	struct sdhc_host	*sc_host[1];
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_size_t		sc_bsz;
	int			sc_phandle;
	struct fdtbus_phy	*sc_phy;
	struct syscon		*sc_syscon;
	struct clk		*sc_clk_xin;
	struct clk		*sc_clk_ahb;
	enum arasan_sdhc_type	sc_type;
	struct clk_domain	sc_clkdom;
	struct clk		sc_clk_card;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-sdhci-5.1",
	  .value = AS_TYPE_RK3399 },

	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry sdhci_5_1_compat[] = {
	{ .compat = "arasan,sdhci-5.1" },
	DEVICE_COMPAT_EOL
};

static struct clk *
arasan_sdhc_clk_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct arasan_sdhc_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return &sc->sc_clk_card;
}

static const struct fdtbus_clock_controller_func arasan_sdhc_fdt_clk_funcs = {
	.decode = arasan_sdhc_clk_decode,
};

static struct clk *
arasan_sdhc_clk_get(void *priv, const char *name)
{
	struct arasan_sdhc_softc * const sc = priv;

	if (strcmp(name, sc->sc_clk_card.name) != 0)
		return NULL;

	return &sc->sc_clk_card;
}

static u_int
arasan_sdhc_clk_get_rate(void *priv, struct clk *clk)
{
	struct arasan_sdhc_softc * const sc = priv;

	return clk_get_rate(sc->sc_clk_xin);
}

static const struct clk_funcs arasan_sdhc_clk_funcs = {
	.get = arasan_sdhc_clk_get,
	.get_rate = arasan_sdhc_clk_get_rate,
};

static int
arasan_sdhc_signal_voltage(struct sdhc_softc *sdhc, int signal_voltage)
{
	if (signal_voltage == SDMMC_SIGNAL_VOLTAGE_180)
		return 0;

	return EINVAL;
}

static int
arasan_sdhc_bus_clock_pre(struct sdhc_softc *sdhc, int freq)
{
	struct arasan_sdhc_softc * const sc = device_private(sdhc->sc_dev);
	int error;

	if (sc->sc_phy != NULL) {
		error = fdtbus_phy_enable(sc->sc_phy, false);
		if (error != 0)
			return error;
	}

	return 0;
}

static int
arasan_sdhc_bus_clock_post(struct sdhc_softc *sdhc, int freq)
{
	struct arasan_sdhc_softc * const sc = device_private(sdhc->sc_dev);
	int error;

	if (sc->sc_phy != NULL) {
		error = fdtbus_phy_enable(sc->sc_phy, true);
		if (error != 0)
			return error;
	}

	return 0;
}

static void
arasan_sdhc_init_rk3399(struct arasan_sdhc_softc *sc)
{
	uint32_t mask, val;

	if (sc->sc_syscon == NULL)
		return;

	syscon_lock(sc->sc_syscon);

	/* Disable clock multiplier */
	mask = RK3399_CORECFG_CLOCKMULTIPLIER;
	val = 0;
	syscon_write_4(sc->sc_syscon, RK3399_GRF_EMMCCORE_CON11, (mask << 16) | val);

	/* Set base clock frequency */
	const u_int xin_rate = clk_get_rate(sc->sc_clk_xin);
	mask = RK3399_CORECFG_BASECLKFREQ;
	val = __SHIFTIN((xin_rate + (1000000 / 2)) / 1000000, RK3399_CORECFG_BASECLKFREQ);
	syscon_write_4(sc->sc_syscon, RK3399_GRF_EMMCCORE_CON0, (mask << 16) | val);

	syscon_unlock(sc->sc_syscon);
}

static void
arasan_sdhc_init(device_t dev)
{
	struct arasan_sdhc_softc * const sc = device_private(dev);
	int error;

	if (sc->sc_type == AS_TYPE_RK3399)
		arasan_sdhc_init_rk3399(sc);

	if (of_compatible_match(sc->sc_phandle, sdhci_5_1_compat)) {
		sc->sc_phy = fdtbus_phy_get(sc->sc_phandle, "phy_arasan");
		if (sc->sc_phy == NULL) {
			aprint_error_dev(dev, "couldn't get PHY\n");
			return;
		}
		sc->sc_base.sc_vendor_signal_voltage = arasan_sdhc_signal_voltage;
	}

	error = sdhc_host_found(&sc->sc_base, sc->sc_bst, sc->sc_bsh, sc->sc_bsz);
	if (error != 0) {
		aprint_error_dev(dev, "couldn't initialize host, error = %d\n", error);
		return;
	}
}

static int
arasan_sdhc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
arasan_sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct arasan_sdhc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	const char *clkname;
	bus_addr_t addr;
	bus_size_t size;
	u_int bus_width;
	int error;
	void *ih;

	fdtbus_clock_assign(phandle);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	sc->sc_clk_xin = fdtbus_clock_get(phandle, "clk_xin");
	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "clk_ahb");
	if (sc->sc_clk_xin == NULL || sc->sc_clk_ahb == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}
	if (clk_enable(sc->sc_clk_xin) != 0 || clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	sc->sc_syscon = fdtbus_syscon_acquire(phandle, "arasan,soc-ctl-syscon");

	if (of_getprop_uint32(phandle, "bus-width", &bus_width) != 0)
		bus_width = 4;

	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_bsz = size;
	sc->sc_type = of_compatible_lookup(phandle, compat_data)->value;

	const uint32_t caps = bus_space_read_4(sc->sc_bst, sc->sc_bsh, SDHC_CAPABILITIES);
	if ((caps & (SDHC_ADMA2_SUPP|SDHC_64BIT_SYS_BUS)) == SDHC_ADMA2_SUPP) {
		error = bus_dmatag_subregion(faa->faa_dmat, 0, UINT32_MAX,
		    &sc->sc_base.sc_dmat, BUS_DMA_WAITOK);
		if (error != 0) {
			aprint_error(": couldn't create DMA tag: %d\n", error);
			return;
		}
	} else {
		sc->sc_base.sc_dmat = faa->faa_dmat;
	}

	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_host = sc->sc_host;
	sc->sc_base.sc_flags = SDHC_FLAG_NO_CLKBASE |
			       SDHC_FLAG_SINGLE_POWER_WRITE |
			       SDHC_FLAG_32BIT_ACCESS |
			       SDHC_FLAG_USE_DMA |
			       SDHC_FLAG_USE_ADMA2 |
			       SDHC_FLAG_STOP_WITH_TC;
	if (bus_width == 8)
		sc->sc_base.sc_flags |= SDHC_FLAG_8BIT_MODE;
	sc->sc_base.sc_clkbase = clk_get_rate(sc->sc_clk_xin) / 1000;
	sc->sc_base.sc_vendor_bus_clock = arasan_sdhc_bus_clock_pre;
	sc->sc_base.sc_vendor_bus_clock_post = arasan_sdhc_bus_clock_post;

	aprint_naive("\n");
	aprint_normal(": Arasan SDHCI controller\n");

	clkname = fdtbus_get_string(phandle, "clock-output-names");
	if (clkname == NULL)
		clkname = faa->faa_name;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &arasan_sdhc_clk_funcs;
	sc->sc_clkdom.priv = sc;
	sc->sc_clk_card.domain = &sc->sc_clkdom;
	sc->sc_clk_card.name = kmem_asprintf("%s", clkname);
	clk_attach(&sc->sc_clk_card);

	fdtbus_register_clock_controller(self, phandle, &arasan_sdhc_fdt_clk_funcs);

	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_SDMMC, 0,
	    sdhc_intr, &sc->sc_base, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	arasan_sdhc_init(self);
}

CFATTACH_DECL_NEW(arasan_sdhc_fdt, sizeof(struct arasan_sdhc_softc),
	arasan_sdhc_match, arasan_sdhc_attach, NULL, NULL);
