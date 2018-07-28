/* $NetBSD: exynos_usbdrdphy.c,v 1.2.2.2 2018/07/28 04:37:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: exynos_usbdrdphy.c,v 1.2.2.2 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

/*
 * PHY Registers
 */
#define	PHY_LINK_SYSTEM			0x04
#define	 PHY_LINK_SYSTEM_XHCI_VERCTL	__BIT(27)
#define	 PHY_LINK_SYSTEM_FLADJ		__BITS(6,1)
#define	PHY_UTMI			0x08
#define	 PHY_UTMI_OTGDISABLE		__BIT(6)
#define	PHY_CLK_RST			0x10
#define	 PHY_CLK_RST_SSC_REFCLKSEL	__BITS(30,23)
#define	 PHY_CLK_RST_SSC_EN		__BIT(20)
#define	 PHY_CLK_RST_REF_SSP_EN		__BIT(19)
#define	 PHY_CLK_RST_MPLL_MULT		__BITS(17,11)
#define	  PHY_CLK_RST_MPLL_MULT_24M	0x68
#define	 PHY_CLK_RST_FSEL		__BITS(10,5)
#define	  PHY_CLK_RST_FSEL_24M		0x5
#define	 PHY_CLK_RST_RETENABLEN		__BIT(4)
#define	 PHY_CLK_RST_REFCLKSEL		__BITS(3,2)
#define	  PHY_CLK_RST_REFCLKSEL_EXT	3
#define	 PHY_CLK_RST_PORTRESET		__BIT(1)
#define	 PHY_CLK_RST_COMMONONN		__BIT(0)
#define	PHY_REG0			0x14
#define	PHY_PARAM0			0x1c
#define	 PHY_PARAM0_REF_USE_PAD		__BIT(31)
#define	 PHY_PARAM0_REF_LOSLEVEL	__BITS(30,26)
#define	PHY_PARAM1			0x20
#define	 PHY_PARAM1_TXDEEMPH		__BITS(4,0)
#define	PHY_TEST			0x28
#define	 PHY_TEST_POWERDOWN_SSP		__BIT(3)
#define	 PHY_TEST_POWERDOWN_HSP		__BIT(2)
#define	PHY_BATCHG			0x30
#define	 PHY_BATCHG_UTMI_CLKSEL		__BIT(2)
#define	PHY_RESUME			0x34

/*
 * PMU Registers
 */
#define	USBDRD_PHY_CTRL(n)		(0x704 + (n) * 4)
#define	 USBDRD_PHY_CTRL_EN		__BIT(0)

static int exynos_usbdrdphy_match(device_t, cfdata_t, void *);
static void exynos_usbdrdphy_attach(device_t, device_t, void *);

enum {
	PHY_ID_UTMI_PLUS = 0,
	PHY_ID_PIPE3,
	NPHY_ID
};

static const struct of_compat_data compat_data[] = {
	{ "samsung,exynos5420-usbdrd-phy",	0 },
	{ NULL }
};

struct exynos_usbdrdphy_softc;

struct exynos_usbdrdphy {
	struct exynos_usbdrdphy_softc *phy_sc;
	u_int			phy_index;
};

struct exynos_usbdrdphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct syscon		*sc_pmureg;

	struct exynos_usbdrdphy	*sc_phy;
	u_int			sc_nphy;

	struct fdtbus_gpio_pin	*sc_gpio_id_det;
	struct fdtbus_gpio_pin	*sc_gpio_vbus_det;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(exynos_usbdrdphy, sizeof(struct exynos_usbdrdphy_softc),
	exynos_usbdrdphy_match, exynos_usbdrdphy_attach, NULL, NULL);

static void *
exynos_usbdrdphy_acquire(device_t dev, const void *data, size_t len)
{
	struct exynos_usbdrdphy_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	const u_int index = be32dec(data);
	if (index >= sc->sc_nphy)
		return NULL;

	return &sc->sc_phy[index];
}

static void
exynos_usbdrdphy_release(device_t dev, void *priv)
{
}

static int
exynos_usbdrdphy_enable(device_t dev, void *priv, bool enable)
{
	struct exynos_usbdrdphy * const phy = priv;
	struct exynos_usbdrdphy_softc * const sc = phy->phy_sc;
	uint32_t val;

	syscon_lock(sc->sc_pmureg);
	val = syscon_read_4(sc->sc_pmureg, USBDRD_PHY_CTRL(phy->phy_index));
	if (enable)
		val |= USBDRD_PHY_CTRL_EN;
	else
		val &= ~USBDRD_PHY_CTRL_EN;
	syscon_write_4(sc->sc_pmureg, USBDRD_PHY_CTRL(phy->phy_index), val);
	syscon_unlock(sc->sc_pmureg);

	return 0;
}

const struct fdtbus_phy_controller_func exynos_usbdrdphy_funcs = {
	.acquire = exynos_usbdrdphy_acquire,
	.release = exynos_usbdrdphy_release,
	.enable = exynos_usbdrdphy_enable,
};

static void
exynos_usbdrdphy_init(struct exynos_usbdrdphy_softc *sc)
{
	uint32_t val;

	PHY_WRITE(sc, PHY_REG0, 0);

	val = PHY_READ(sc, PHY_PARAM0);
	val &= ~PHY_PARAM0_REF_USE_PAD;
	val &= ~PHY_PARAM0_REF_LOSLEVEL;
	val |= __SHIFTIN(9, PHY_PARAM0_REF_LOSLEVEL);
	PHY_WRITE(sc, PHY_PARAM0, val);

	PHY_WRITE(sc, PHY_RESUME, 0);

	val = PHY_READ(sc, PHY_LINK_SYSTEM);
	val |= PHY_LINK_SYSTEM_XHCI_VERCTL;
	val &= ~PHY_LINK_SYSTEM_FLADJ;
	val |= __SHIFTIN(0x20, PHY_LINK_SYSTEM_FLADJ);
	PHY_WRITE(sc, PHY_LINK_SYSTEM, val);

	val = PHY_READ(sc, PHY_PARAM1);
	val &= ~PHY_PARAM1_TXDEEMPH;
	val |= __SHIFTIN(0x1c, PHY_PARAM1_TXDEEMPH);
	PHY_WRITE(sc, PHY_PARAM1, val);

	val = PHY_READ(sc, PHY_BATCHG);
	val |= PHY_BATCHG_UTMI_CLKSEL;
	PHY_WRITE(sc, PHY_BATCHG, val);

	val = PHY_READ(sc, PHY_TEST);
	val &= ~PHY_TEST_POWERDOWN_SSP;
	val &= ~PHY_TEST_POWERDOWN_HSP;
	PHY_WRITE(sc, PHY_TEST, val);

	PHY_WRITE(sc, PHY_UTMI, PHY_UTMI_OTGDISABLE);

	val = __SHIFTIN(PHY_CLK_RST_REFCLKSEL_EXT, PHY_CLK_RST_REFCLKSEL);
	val |= __SHIFTIN(PHY_CLK_RST_FSEL_24M, PHY_CLK_RST_FSEL);
	val |= __SHIFTIN(PHY_CLK_RST_MPLL_MULT_24M, PHY_CLK_RST_MPLL_MULT);
	val |= __SHIFTIN(0x88, PHY_CLK_RST_SSC_REFCLKSEL);
	val |= PHY_CLK_RST_PORTRESET;
	val |= PHY_CLK_RST_RETENABLEN;
	val |= PHY_CLK_RST_REF_SSP_EN;
	val |= PHY_CLK_RST_SSC_EN;
	val |= PHY_CLK_RST_COMMONONN;
	PHY_WRITE(sc, PHY_CLK_RST, val);

	delay(50000);

	val &= ~PHY_CLK_RST_PORTRESET;
	PHY_WRITE(sc, PHY_CLK_RST, val);
}

static int
exynos_usbdrdphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
exynos_usbdrdphy_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usbdrdphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get phy registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map phy registers\n");
		return;
	}

	sc->sc_nphy = NPHY_ID;
	sc->sc_phy = kmem_alloc(sizeof(*sc->sc_phy) * sc->sc_nphy, KM_SLEEP);
	for (n = 0; n < sc->sc_nphy; n++) {
		sc->sc_phy[n].phy_sc = sc;
		sc->sc_phy[n].phy_index = n;
	}

	sc->sc_pmureg = fdtbus_syscon_acquire(phandle, "samsung,pmu-syscon");
	if (sc->sc_pmureg == NULL) {
		aprint_error(": couldn't acquire pmureg syscon\n");
		return;
	}

	/* Enable clocks */
	clk = fdtbus_clock_get(phandle, "phy");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable phy clock\n");
		return;
	}
	clk = fdtbus_clock_get(phandle, "ref");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable ref clock\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": USB DRD PHY\n");

	exynos_usbdrdphy_init(sc);

	fdtbus_register_phy_controller(self, phandle, &exynos_usbdrdphy_funcs);
}
