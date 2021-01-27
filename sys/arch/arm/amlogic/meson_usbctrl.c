/* $NetBSD: meson_usbctrl.c,v 1.5 2021/01/27 03:10:18 thorpej Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_usbctrl.c,v 1.5 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

/*
 * USB Glue registers: 0xffe09000
 */

/* usb2 phy ports control registers */
#define MESONUSBCTRL_MAXPHYS				3
#define U2P_R0_REG(i)					(0x20 * (i) + 0x00)
#define  U2P_R0_DRV_VBUS				__BIT(5)
#define  U2P_R0_ID_PULLUP				__BIT(4)
#define  U2P_R0_POWER_ON_RESET				__BIT(3)
#define  U2P_R0_HAST_MODE				__BIT(2)
#define  U2P_R0_POWER_OK				__BIT(1)
#define  U2P_R0_HOST_DEVICE				__BIT(0)
#define U2P_R1_REG(i)					(0x20 * (i) + 0x04)
#define  U2P_R1_VBUS_VALID				__BIT(3)
#define  U2P_R1_OTG_SESSION_VALID			__BIT(2)
#define  U2P_R1_ID_DIG					__BIT(1)
#define  U2P_R1_PHY_READY				__BIT(0)

/* glue registers */
#define USB_R0_REG					0x80
#define  USB_R0_U2D_ACT					__BIT(31)
#define  USB_R0_U2D_SS_SCALEDOWN_MODE_MASK		__BITS(30,29)
#define  USB_R0_P30_PCS_RX_LOS_MASK_VAL_MASK		__BITS(28,19)
#define  USB_R0_P30_LANE0_EXT_PCLK_REQ			__BIT(18)
#define  USB_R0_P30_LANE0_TX2RX_LOOPBACK		__BIT(17)
#define USB_R1_REG					0x84
#define  USB_R1_P30_PCS_TX_SWING_FULL_MASK		__BITS(31,25)
#define  USB_R1_U3H_FLADJ_30MHZ_REG_MASK		__BITS(24,19)
#define  USB_R1_U3H_HOST_MSI_ENABLE			__BIT(18)
#define  USB_R1_U3H_HOST_PORT_POWER_CONTROL_PRESENT	__BIT(17)
#define  USB_R1_U3H_HOST_U3_PORT_DISABLE		__BIT(16)
#define  USB_R1_U3H_HOST_U2_PORT_DISABLE_MASK		__BITS(13,12)
#define  USB_R1_U3H_HUB_PORT_PERM_ATTACH_MASK		__BITS(9,7)
#define  USB_R1_U3H_HUB_PORT_OVERCURRENT_MASK		__BITS(4,2)
#define  USB_R1_U3H_PME_ENABLE				__BIT(1)
#define  USB_R1_U3H_BIGENDIAN_GS			__BIT(0)
#define USB_R2_REG					0x88
#define  USB_R2_P30_PCS_TX_DEEMPH_6DB_MASK		__BITS(31,26)
#define  USB_R2_P30_PCS_TX_DEEMPH_3P5DB_MASK		__BITS(25,20)
#define USB_R3_REG					0x8c
#define  USB_R3_P30_REF_SSP_EN				__BIT(13)
#define  USB_R3_P30_SSC_REF_CLK_SEL_MASK		__BITS(12,4)
#define  USB_R3_P30_SSC_RANGE_MASK			__BITS(3,1)
#define  USB_R3_P30_SSC_ENABLE				__BIT(0)
#define USB_R4_REG					0x90
#define  USB_R4_P21_ONLY				__BIT(4)
#define  USB_R4_MEM_PD_MASK				__BITS(3,2)
#define  USB_R4_P21_SLEEP_M0				__BIT(1)
#define  USB_R4_P21_PORT_RESET_0			__BIT(0)
#define USB_R5_REG					0x94
#define  USB_R5_ID_DIG_CNT_MASK				__BITS(23,16)
#define  USB_R5_ID_DIG_TH_MASK				__BITS(15,8)
#define  USB_R5_ID_DIG_IRQ				__BIT(7)
#define  USB_R5_ID_DIG_CURR				__BIT(6)
#define  USB_R5_ID_DIG_EN_1				__BIT(5)
#define  USB_R5_ID_DIG_EN_0				__BIT(4)
#define  USB_R5_ID_DIG_CFG_MASK				__BITS(3,2)
#define  USB_R5_ID_DIG_REG				__BIT(1)
#define  USB_R5_ID_DIG_SYNC				__BIT(0)

#define USBCTRL_READ_REG(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define USBCTRL_WRITE_REG(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

struct meson_usbctrl_config {
	int num_phys;
};

struct meson_usbctrl_config mesong12_conf = {
	.num_phys = 3
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-g12a-usb-ctrl", .data = &mesong12_conf },
	DEVICE_COMPAT_EOL
};

struct meson_usbctrl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	const struct meson_usbctrl_config *sc_conf;
	struct fdtbus_regulator *sc_supply;
	int sc_phandle;
};

static void
meson_usbctrl_usb2_init(struct meson_usbctrl_softc *sc)
{
	int i;
	const char *p;

	for (i = 0; i < sc->sc_conf->num_phys; i++) {
		/* setup only for usb2 phys */
		p = fdtbus_get_string_index(sc->sc_phandle, "phy-names", i);
		if (p == NULL || strstr(p, "usb2") == NULL)
			continue;

		USBCTRL_WRITE_REG(sc, U2P_R0_REG(i),
		    USBCTRL_READ_REG(sc, U2P_R0_REG(i)) |
		    U2P_R0_POWER_ON_RESET);

		/* XXX: OTG not supported. always set HOST_DEVICE mode */
		USBCTRL_WRITE_REG(sc, U2P_R0_REG(i),
		    USBCTRL_READ_REG(sc, U2P_R0_REG(i)) |
		    U2P_R0_HOST_DEVICE);

		USBCTRL_WRITE_REG(sc, U2P_R0_REG(i),
		    USBCTRL_READ_REG(sc, U2P_R0_REG(i)) &
		    ~U2P_R0_POWER_ON_RESET);
	}
}

static void
meson_usbctrl_usb_glue_init(struct meson_usbctrl_softc *sc)
{
	uint32_t val;

	val = USBCTRL_READ_REG(sc, USB_R1_REG);
	val &= ~USB_R1_U3H_FLADJ_30MHZ_REG_MASK;
	val |= __SHIFTIN(0x20, USB_R1_U3H_FLADJ_30MHZ_REG_MASK);
	USBCTRL_WRITE_REG(sc, USB_R1_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R5_REG);
	val |= USB_R5_ID_DIG_EN_0;
	USBCTRL_WRITE_REG(sc, USB_R5_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R5_REG);
	val |= USB_R5_ID_DIG_EN_1;
	USBCTRL_WRITE_REG(sc, USB_R5_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R5_REG);
	val &= ~USB_R5_ID_DIG_TH_MASK;
	val |= __SHIFTIN(0xff, USB_R5_ID_DIG_TH_MASK);
	USBCTRL_WRITE_REG(sc, USB_R5_REG, val);
}

static void
meson_usbctrl_usb3_init(struct meson_usbctrl_softc *sc)
{
	uint32_t val;

	val = USBCTRL_READ_REG(sc, USB_R3_REG);
	val &= ~USB_R3_P30_SSC_RANGE_MASK;
	val &= ~USB_R3_P30_SSC_ENABLE;
	val |= __SHIFTIN(2, USB_R3_P30_SSC_RANGE_MASK);
	val |= USB_R3_P30_REF_SSP_EN;
	USBCTRL_WRITE_REG(sc, USB_R3_REG, val);

	delay(2);

	val = USBCTRL_READ_REG(sc, USB_R2_REG);
	val &= ~USB_R2_P30_PCS_TX_DEEMPH_3P5DB_MASK;
	val |= __SHIFTIN(0x15, USB_R2_P30_PCS_TX_DEEMPH_3P5DB_MASK);
	USBCTRL_WRITE_REG(sc, USB_R2_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R2_REG);
	val &= ~USB_R2_P30_PCS_TX_DEEMPH_6DB_MASK;
	val |= __SHIFTIN(0x20, USB_R2_P30_PCS_TX_DEEMPH_6DB_MASK);
	USBCTRL_WRITE_REG(sc, USB_R2_REG, val);

	delay(2);

	val = USBCTRL_READ_REG(sc, USB_R1_REG);
	val |= USB_R1_U3H_HOST_PORT_POWER_CONTROL_PRESENT;
	USBCTRL_WRITE_REG(sc, USB_R1_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R1_REG);
	val &= ~USB_R1_P30_PCS_TX_SWING_FULL_MASK;
	val |= __SHIFTIN(127, USB_R1_P30_PCS_TX_SWING_FULL_MASK);
	USBCTRL_WRITE_REG(sc, USB_R1_REG, val);

	/* XXX: force HOST_DEVICE mode */
	val = USBCTRL_READ_REG(sc, USB_R0_REG);
	val &= ~USB_R0_U2D_ACT;
	USBCTRL_WRITE_REG(sc, USB_R0_REG, val);

	val = USBCTRL_READ_REG(sc, USB_R4_REG);
	val &= ~USB_R4_P21_SLEEP_M0;
	USBCTRL_WRITE_REG(sc, USB_R4_REG, val);
}

static void
meson_usbctrl_enable_usb3_phys(struct meson_usbctrl_softc *sc)
{
	struct fdtbus_phy *phy;
	int i;
	const char *phyname;

	/*
	 * enable only for usb3 phys.
	 * node of "snps,dwc3" decl in "amlogic,meson-g12a-usb-ctrl" have
	 * no "phys" property, so enable the phy here.
	 */
	for (i = 0; i < sc->sc_conf->num_phys; i++) {
		phyname = fdtbus_get_string_index(sc->sc_phandle,
		    "phy-names", i);
		if (strstr(phyname, "usb3") == NULL)
			continue;

		phy = fdtbus_phy_get_index(sc->sc_phandle, i);
		if (phy == NULL)
			continue;
		if (fdtbus_phy_enable(phy, true) != 0)
			aprint_error_dev(sc->sc_dev, "couldn't enable phy %s\n",
			    phyname);
	}
}

static int
meson_usbctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_usbctrl_attach(device_t parent, device_t self, void *aux)
{
	struct meson_usbctrl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int phandle, child;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle = faa->faa_phandle;
	sc->sc_conf = of_compatible_lookup(phandle, compat_data)->data;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": USB Controllers\n");

	sc->sc_supply = fdtbus_regulator_acquire(phandle, "vbus-supply");
	if (sc->sc_supply != NULL)
		fdtbus_regulator_enable(sc->sc_supply);	/* USB HOST MODE */

	meson_usbctrl_usb2_init(sc);
	meson_usbctrl_usb_glue_init(sc);
	meson_usbctrl_usb3_init(sc);
	meson_usbctrl_enable_usb3_phys(sc);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		fdt_add_child(parent, child, faa, 0);
	}
}

CFATTACH_DECL_NEW(meson_usbctrl, sizeof(struct meson_usbctrl_softc),
    meson_usbctrl_match, meson_usbctrl_attach, NULL, NULL);
