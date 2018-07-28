/* $NetBSD: exynos_usbphy.c,v 1.1.20.1 2018/07/28 04:37:29 pgoyette Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: exynos_usbphy.c,v 1.1.20.1 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos5_reg.h>

/*
 * System Registers
 */
#define	USB20PHY_CFG			0x230
#define	 USB20PHY_CFG_HOST_LINK_EN	__BIT(0)

/*
 * PMU Registers
 */
#define	USBHOST_PHY_CTRL		0x708
#define	 USBHOST_PHY_CTRL_EN		__BIT(0)

enum {
	PHY_ID_DEVICE = 0,
	PHY_ID_HOST,
	PHY_ID_HSIC0,
	PHY_ID_HSIC1,
	NPHY_ID
};

static int exynos_usbphy_match(device_t, cfdata_t, void *);
static void exynos_usbphy_attach(device_t, device_t, void *);

static const struct of_compat_data compat_data[] = {
	{ "samsung,exynos5250-usb2-phy",	0 },
	{ NULL }
};

struct exynos_usbphy_softc;

struct exynos_usbphy {
	struct exynos_usbphy_softc *phy_sc;
	u_int			phy_index;
};

struct exynos_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct syscon		*sc_sysreg;
	struct syscon		*sc_pmureg;

	u_int			sc_refcnt;

	struct exynos_usbphy	*sc_phy;
	u_int			sc_nphy;

	struct fdtbus_gpio_pin	*sc_gpio_id_det;
	struct fdtbus_gpio_pin	*sc_gpio_vbus_det;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(exynos_usbphy, sizeof(struct exynos_usbphy_softc),
	exynos_usbphy_match, exynos_usbphy_attach, NULL, NULL);

static void *
exynos_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct exynos_usbphy_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	const u_int index = be32dec(data);
	if (index >= sc->sc_nphy)
		return NULL;

	return &sc->sc_phy[index];
}

static void
exynos_usbphy_release(device_t dev, void *priv)
{
}

static int
exynos_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct exynos_usbphy * const phy = priv;
	struct exynos_usbphy_softc * const sc = phy->phy_sc;
	bool do_common;
	uint32_t val;

	if (enable) {
		sc->sc_refcnt++;
	} else {
		KASSERT(sc->sc_refcnt > 0);
		sc->sc_refcnt--;
	}
	do_common = sc->sc_refcnt == enable;

	if (do_common) {
		syscon_lock(sc->sc_sysreg);
		val = syscon_read_4(sc->sc_sysreg, USB20PHY_CFG);
		if (enable)
			val |= USB20PHY_CFG_HOST_LINK_EN;
		else
			val &= ~USB20PHY_CFG_HOST_LINK_EN;
		syscon_write_4(sc->sc_sysreg, USB20PHY_CFG, val);
		syscon_unlock(sc->sc_sysreg);

		syscon_lock(sc->sc_pmureg);
		val = syscon_read_4(sc->sc_pmureg, USBHOST_PHY_CTRL);
		if (enable)
			val |= USBHOST_PHY_CTRL_EN;
		else
			val &= ~USBHOST_PHY_CTRL_EN;
		syscon_write_4(sc->sc_pmureg, USBHOST_PHY_CTRL, val);
		syscon_unlock(sc->sc_pmureg);

		if (enable) {
			val = PHY_READ(sc, USB_PHY_HOST_CTRL0);
			val &= ~HOST_CTRL0_COMMONON_N;
			val &= ~HOST_CTRL0_PHY_SWRST;
			val &= ~HOST_CTRL0_PHY_SWRST_ALL;
			val &= ~HOST_CTRL0_SIDDQ;
			val &= ~HOST_CTRL0_FORCESUSPEND;
			val &= ~HOST_CTRL0_FORCESLEEP;
			val &= ~HOST_CTRL0_FSEL_MASK;
			val |= __SHIFTIN(FSEL_CLKSEL_24M, HOST_CTRL0_FSEL_MASK);
			val |= HOST_CTRL0_LINK_SWRST;
			val |= HOST_CTRL0_UTMI_SWRST;
			PHY_WRITE(sc, USB_PHY_HOST_CTRL0, val);

			delay(10000);

			val &= ~HOST_CTRL0_LINK_SWRST;
			val &= ~HOST_CTRL0_UTMI_SWRST;
			PHY_WRITE(sc, USB_PHY_HOST_CTRL0, val);

			delay(10000);
		}
	}

	switch (phy->phy_index) {
	case PHY_ID_HSIC0:
	case PHY_ID_HSIC1:
		if (enable) {
			const bus_size_t reg = phy->phy_index == PHY_ID_HSIC0 ?
			    USB_PHY_HSIC_CTRL1 : USB_PHY_HSIC_CTRL2;

			val = HSIC_CTRL_PHY_SWRST;
			val |= __SHIFTIN(HSIC_CTRL_REFCLKDIV_12, HSIC_CTRL_REFCLKDIV_MASK);
			val |= __SHIFTIN(HSIC_CTRL_REFCLKSEL_DEFAULT, HSIC_CTRL_REFCLKSEL_MASK);
			PHY_WRITE(sc, reg, val);

			delay(10000);

			val &= ~HSIC_CTRL_PHY_SWRST;
			PHY_WRITE(sc, reg, val);

			delay(10000);
		}
		break;
	}

	if (do_common) {
		if (enable) {
			val = PHY_READ(sc, USB_PHY_HOST_EHCICTRL);
			val |= HOST_EHCICTRL_ENA_INCRXALIGN;
			val |= HOST_EHCICTRL_ENA_INCR4;
			val |= HOST_EHCICTRL_ENA_INCR8;
			val |= HOST_EHCICTRL_ENA_INCR16;
			PHY_WRITE(sc, USB_PHY_HOST_EHCICTRL, val);
		}
	}

	return 0;
}

const struct fdtbus_phy_controller_func exynos_usbphy_funcs = {
	.acquire = exynos_usbphy_acquire,
	.release = exynos_usbphy_release,
	.enable = exynos_usbphy_enable,
};

static int
exynos_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
exynos_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usbphy_softc * const sc = device_private(self);
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

	sc->sc_sysreg = fdtbus_syscon_acquire(phandle, "samsung,sysreg-phandle");
	if (sc->sc_sysreg == NULL) {
		aprint_error(": couldn't acquire sysreg syscon\n");
		return;
	}
	sc->sc_pmureg = fdtbus_syscon_acquire(phandle, "samsung,pmureg-phandle");
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
	aprint_normal(": USB2 PHY\n");

	fdtbus_register_phy_controller(self, phandle, &exynos_usbphy_funcs);
}
