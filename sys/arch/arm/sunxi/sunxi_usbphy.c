/* $NetBSD: sunxi_usbphy.c,v 1.11 2018/02/19 20:15:23 jmcneill Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sunxi_usbphy.c,v 1.11 2018/02/19 20:15:23 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

/* PHY control registers */
#define	PHYCTL_ICR		0x00
#define	 PHYCTL_ICR_ID_PULLUP	__BIT(17)
#define	 PHYCTL_ICR_DPDM_PULLUP	__BIT(16)
#define	 PHYCTL_ICR_FORCE_ID	__BITS(15,14)
#define	  PHYCTL_ICR_FORCE_ID_LOW	2
#define	  PHYCTL_ICR_FORCE_ID_HIGH	3
#define	 PHYCTL_ICR_FORCE_VBUS	__BITS(13,12)
#define	  PHYCTL_ICR_FORCE_VBUS_LOW	2
#define	  PHYCTL_ICR_FORCE_VBUS_HIGH	3
#define	PHYCTL_A10		0x04
#define	PHYCTL_A33		0x10
#define	 PHYCTL_ADDR		__BITS(15,8)
#define	 PHYCTL_DATA		__BIT(7)
#define	PHYCTL_OTG_CFG		0x20
#define	 PHYCTL_OTG_ROUTE_OTG	__BIT(0)

/* PHY registers */
#define	PHY_RES45_CAL_EN	0x0c
#define	PHY_TX_AMPLITUDE_TUNE	0x20
#define	PHY_DISCON_TH_SEL	0x2a

/* PMU registers */
#define	PMU_CFG			0x00
#define	 AHB_INCR8		__BIT(10)
#define	 AHB_INCR4		__BIT(9)
#define	 AHB_INCRX_ALIGN	__BIT(8)
#define	 ULPI_BYPASS		__BIT(0)
#define	PMU_UNK_H3		0x10
#define	 PMU_UNK_H3_CLR		__BIT(1)

static int sunxi_usbphy_match(device_t, cfdata_t, void *);
static void sunxi_usbphy_attach(device_t, device_t, void *);

enum sunxi_usbphy_type {
	USBPHY_A10 = 1,
	USBPHY_A13,
	USBPHY_A20,
	USBPHY_A31,
	USBPHY_A64,
	USBPHY_A83T,
	USBPHY_H3,
	USBPHY_H6,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-usb-phy",	USBPHY_A10 },
	{ "allwinner,sun5i-a13-usb-phy",	USBPHY_A13 },
	{ "allwinner,sun6i-a31-usb-phy",	USBPHY_A31 },
	{ "allwinner,sun7i-a20-usb-phy",	USBPHY_A20 },
	{ "allwinner,sun8i-a83t-usb-phy",	USBPHY_A83T },
	{ "allwinner,sun8i-h3-usb-phy",		USBPHY_H3 },
	{ "allwinner,sun50i-a64-usb-phy",	USBPHY_A64 },
	{ "allwinner,sun50i-h6-usb-phy",	USBPHY_H6 },
	{ NULL }
};

#define	SUNXI_MAXUSBPHY		4

struct sunxi_usbphy {
	u_int			phy_index;
	bus_space_handle_t	phy_bsh;
	struct fdtbus_regulator *phy_reg;
};

struct sunxi_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_phy_ctrl;
	enum sunxi_usbphy_type	sc_type;

	struct sunxi_usbphy	sc_phys[SUNXI_MAXUSBPHY];
	u_int			sc_nphys;

	struct fdtbus_gpio_pin	*sc_gpio_id_det;
	struct fdtbus_gpio_pin	*sc_gpio_vbus_det;
};

#define	PHYCTL_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst,			\
	    (sc)->sc_bsh_phy_ctrl, (reg))
#define	PHYCTL_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst,			\
	    (sc)->sc_bsh_phy_ctrl, (reg), (val))
#define	PMU_READ(sc, id, reg)			\
	bus_space_read_4((sc)->sc_bst,			\
	    (sc)->sc_phys[(id)].phy_bsh, (reg))
#define	PMU_WRITE(sc, id, reg, val)			\
	bus_space_write_4((sc)->sc_bst,			\
	    (sc)->sc_phys[(id)].phy_bsh, (reg), (val))

CFATTACH_DECL_NEW(sunxi_usbphy, sizeof(struct sunxi_usbphy_softc),
	sunxi_usbphy_match, sunxi_usbphy_attach, NULL, NULL);

static void
sunxi_usbphy_write(struct sunxi_usbphy_softc *sc,
    struct sunxi_usbphy *phy, u_int bit_addr, u_int bits,
    u_int len)
{
	const uint32_t usbc_mask = __BIT(phy->phy_index * 2);;
	bus_size_t reg;
	uint32_t val;

	switch (sc->sc_type) {
	case USBPHY_A10:
	case USBPHY_A13:
	case USBPHY_A20:
	case USBPHY_A31:
		reg = PHYCTL_A10;
		break;
	case USBPHY_H3:
	case USBPHY_H6:
	case USBPHY_A64:
	case USBPHY_A83T:
		reg = PHYCTL_A33;
		break;
	default:
		panic("unsupported phy type");
	}

	if (reg == PHYCTL_A33)
		PHYCTL_WRITE(sc, reg, 0);

	for (; len > 0; bit_addr++, bits >>= 1, len--) {
		val = PHYCTL_READ(sc, reg);
		val &= ~PHYCTL_ADDR;
		val |= __SHIFTIN(bit_addr, PHYCTL_ADDR);
		PHYCTL_WRITE(sc, reg, val);

		val = PHYCTL_READ(sc, reg);
		val &= ~PHYCTL_DATA;
		val |= __SHIFTIN(bits & 1, PHYCTL_DATA);
		PHYCTL_WRITE(sc, reg, val);

		PHYCTL_READ(sc, reg);
		val |= usbc_mask;
		PHYCTL_WRITE(sc, reg, val);

		PHYCTL_READ(sc, reg);
		val &= ~usbc_mask;
		PHYCTL_WRITE(sc, reg, val);
	}
}

static bool
sunxi_usbphy_vbus_detect(struct sunxi_usbphy_softc *sc)
{
	if (sc->sc_gpio_vbus_det)
		return fdtbus_gpio_read(sc->sc_gpio_vbus_det);
	return 1;
}

static void *
sunxi_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct sunxi_usbphy_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	const int phy_id = be32dec(data);
	if (phy_id >= sc->sc_nphys || !sc->sc_phys[phy_id].phy_bsh)
		return NULL;

	return &sc->sc_phys[phy_id];
}

static void
sunxi_usbphy_release(device_t dev, void *priv)
{
}

static int
sunxi_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct sunxi_usbphy_softc * const sc = device_private(dev);
	struct sunxi_usbphy * const phy = priv;
	u_int disc_thresh;
	bool phy0_reroute;
	uint32_t val;

	switch (sc->sc_type) {
	case USBPHY_A13:
		disc_thresh = 0x2;
		phy0_reroute = false;
		break;
	case USBPHY_A10:
	case USBPHY_A20:
	case USBPHY_A31:
		disc_thresh = 0x3;
		phy0_reroute = false;
		break;
	case USBPHY_A64:
	case USBPHY_H3:
	case USBPHY_H6:
		disc_thresh = 0x3;
		phy0_reroute = true;
		break;
	case USBPHY_A83T:
		disc_thresh = 0x0;
		phy0_reroute = false;
		break;
	default:
		aprint_error_dev(dev, "unsupported board\n");
		return ENXIO;
	}

	if (phy->phy_bsh) {
		/* Enable/disable passby */
		const uint32_t mask =
		    ULPI_BYPASS|AHB_INCR8|AHB_INCR4|AHB_INCRX_ALIGN;
		val = PMU_READ(sc, phy->phy_index, PMU_CFG);
		if (enable)
			val |= mask;
		else
			val &= ~mask;
		PMU_WRITE(sc, phy->phy_index, PMU_CFG, val);
	}

	switch (sc->sc_type) {
	case USBPHY_H3:
	case USBPHY_A64:
		if (enable && phy->phy_bsh) {
			val = PMU_READ(sc, phy->phy_index, PMU_UNK_H3);
			val &= ~PMU_UNK_H3_CLR;
			PMU_WRITE(sc, phy->phy_index, PMU_UNK_H3, val);
		}
		break;
	default:
		break;
	}

	if (enable) {
		switch (sc->sc_type) {
		case USBPHY_A83T:
		case USBPHY_H6:
			break;
		default:
			if (phy->phy_index == 0)
				sunxi_usbphy_write(sc, phy, PHY_RES45_CAL_EN, 0x1, 1);
			sunxi_usbphy_write(sc, phy, PHY_TX_AMPLITUDE_TUNE, 0x14, 5);
			sunxi_usbphy_write(sc, phy, PHY_DISCON_TH_SEL, disc_thresh, 2);
			break;
		}
	}

	if (phy->phy_index == 0) {
		const uint32_t mask =
		    PHYCTL_ICR_ID_PULLUP|PHYCTL_ICR_DPDM_PULLUP;
		val = PHYCTL_READ(sc, PHYCTL_ICR);

		if (enable)
			val |= mask;
		else
			val &= ~mask;

		/* XXX only host mode is supported */
		val &= ~PHYCTL_ICR_FORCE_ID;
		val |= __SHIFTIN(PHYCTL_ICR_FORCE_ID_LOW, PHYCTL_ICR_FORCE_ID);
		val &= ~PHYCTL_ICR_FORCE_VBUS;
		val |= __SHIFTIN(PHYCTL_ICR_FORCE_VBUS_HIGH, PHYCTL_ICR_FORCE_VBUS);

		PHYCTL_WRITE(sc, PHYCTL_ICR, val);

		if (phy0_reroute) {
			val = PHYCTL_READ(sc, PHYCTL_OTG_CFG);
			val &= ~PHYCTL_OTG_ROUTE_OTG;
			PHYCTL_WRITE(sc, PHYCTL_OTG_CFG, val);
		}
	}

	if (phy->phy_reg == NULL)
		return 0;

	if (enable) {
		/* If an external vbus is detected, do not enable phy 0 */
		if (phy->phy_index == 0 && sunxi_usbphy_vbus_detect(sc))
			return 0;
		return fdtbus_regulator_enable(phy->phy_reg);
	} else {
		return fdtbus_regulator_disable(phy->phy_reg);
	}
}

const struct fdtbus_phy_controller_func sunxi_usbphy_funcs = {
	.acquire = sunxi_usbphy_acquire,
	.release = sunxi_usbphy_release,
	.enable = sunxi_usbphy_enable,
};

static int
sunxi_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct sunxi_usbphy *phy;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	char pname[20];
	u_int n;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;

	if (fdtbus_get_reg_byname(phandle, "phy_ctrl", &addr, &size) != 0) {
		aprint_error(": couldn't get phy ctrl registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_phy_ctrl) != 0) {
		aprint_error(": couldn't map phy ctrl registers\n");
		return;
	}

	for (sc->sc_nphys = 0; sc->sc_nphys < SUNXI_MAXUSBPHY; sc->sc_nphys++) {
		phy = &sc->sc_phys[sc->sc_nphys];
		phy->phy_index = sc->sc_nphys;
		snprintf(pname, sizeof(pname), "pmu%d", sc->sc_nphys);
		if (fdtbus_get_reg_byname(phandle, pname, &addr, &size) != 0) {
			continue;
		} else if (bus_space_map(sc->sc_bst, addr, size, 0, &phy->phy_bsh) != 0) {
			aprint_error(": failed to map reg #%d\n", sc->sc_nphys);
			return;
		}
		/* Get optional regulator */
		snprintf(pname, sizeof(pname), "usb%d_vbus-supply", sc->sc_nphys);
		phy->phy_reg = fdtbus_regulator_acquire(phandle, pname);
	}

	/* Enable clocks */
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	/* De-assert resets */
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	fdtbus_register_phy_controller(self, phandle, &sunxi_usbphy_funcs);
}
