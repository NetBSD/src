/* $NetBSD: mesong12_usb2phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mesong12_usb2phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

/*
 * USB PHY 20: 0xff636000
 * USB PHY 21: 0xff63a000
 */
#define USB2PHY_R00_REG				0x00
#define USB2PHY_R01_REG				0x04
#define USB2PHY_R02_REG				0x08
#define USB2PHY_R03_REG				0x0c
#define  USB2PHY_R03_DISC_THRESH			__BITS(7,4)
#define  USB2PHY_R03_HSDIC_REF				__BITS(3,2)
#define  USB2PHY_R03_SQUELCH_REF			__BITS(1,0)
#define USB2PHY_R04_REG				0x10
#define  USB2PHY_R04_I_C2L_BIAS_TRIM			__BITS(31,28)
#define  USB2PHY_R04_TEST_BYPASS_MODE_EN		__BIT(27)
#define  USB2PHY_R04_I_C2L_CAL_DONE			__BIT(26)
#define  USB2PHY_R04_I_C2L_CAL_RESET_N			__BIT(25)
#define  USB2PHY_R04_I_C2L_CAL_EN			__BIT(24)
#define  USB2PHY_R04_CALIBRATION_CODE_VALUE		__BITS(23,0)
#define USB2PHY_R05_REG				0x14
#define USB2PHY_R06_REG				0x18
#define USB2PHY_R07_REG				0x1c
#define USB2PHY_R08_REG				0x20
#define USB2PHY_R09_REG				0x24
#define USB2PHY_R10_REG				0x28
#define USB2PHY_R11_REG				0x2c
#define USB2PHY_R12_REG				0x30
#define USB2PHY_R13_REG				0x34
#define  USB2PHY_R13_I_C2L_FSLS_RX_EN			__BIT(30)
#define  USB2PHY_R13_I_C2L_HS_RX_EN			__BIT(29)
#define  USB2PHY_R13_I_C2L_FS_OE			__BIT(28)
#define  USB2PHY_R13_I_C2L_HS_OE			__BIT(27)
#define  USB2PHY_R13_I_C2L_LS_EN			__BIT(26)
#define  USB2PHY_R13_I_C2L_FS_EN			__BIT(25)
#define  USB2PHY_R13_I_C2L_HS_EN			__BIT(24)
#define  USB2PHY_R13_BYPASS_HOST_DISCONNECT_ENABLE	__BIT(23)
#define  USB2PHY_R13_BYPASS_HOST_DISCONNECT_VALUE	__BIT(22)
#define  USB2PHY_R13_CLEAR_HOLD_HS_DISCONNECT		__BIT(21)
#define  USB2PHY_R13_MINIMUM_COUNT_FOR_SYNC_DETECTION	__BITS(20,16)
#define  USB2PHY_R13_UPDATE_PMA_SIGNALS			__BIT(15)
#define  USB2PHY_R13_LOAD_STAT				__BIT(14)
#define  USB2PHY_R13_CUSTOM_PATTERN_19			__BITS(7,0)
#define USB2PHY_R14_REG				0x38
#define  USB2PHY_R14_BYPASS_CTRL			__BITS(23,8)
#define  USB2PHY_R14_I_C2L_ASSERT_SINGLE_ENABLE_ZERO	__BIT(6)
#define  USB2PHY_R14_I_C2L_DATA_16_8			__BIT(5)
#define  USB2PHY_R14_PG_RSTN				__BIT(4)
#define  USB2PHY_R14_I_RPU_SW2_EN			__BITS(3,2)
#define  USB2PHY_R14_I_RPU_SW1_EN			__BIT(1)
#define  USB2PHY_R14_I_RDP_EN				__BIT(0)
#define USB2PHY_R15_REG				0x3c
#define USB2PHY_R16_REG				0x40
#define  USB2PHY_R16_USB2_MPLL_LOCK_DIG			__BIT(31)
#define  USB2PHY_R16_USB2_MPLL_LOCK			__BIT(30)
#define  USB2PHY_R16_USB2_MPLL_RESET			__BIT(29)
#define  USB2PHY_R16_USB2_MPLL_EN			__BIT(28)
#define  USB2PHY_R16_USB2_MPLL_FAST_LOCK		__BIT(27)
#define  USB2PHY_R16_USB2_MPLL_LOCK_F			__BIT(26)
#define  USB2PHY_R16_USB2_MPLL_LOCK_LONG		__BITS(25,24)
#define  USB2PHY_R16_USB2_MPLL_DCO_SDM_EN		__BIT(23)
#define  USB2PHY_R16_USB2_MPLL_LOAD			__BIT(22)
#define  USB2PHY_R16_USB2_MPLL_SDM_EN			__BIT(21)
#define  USB2PHY_R16_USB2_MPLL_TDC_MODE			__BIT(20)
#define  USB2PHY_R16_USB2_MPLL_N			__BITS(14,10)
#define  USB2PHY_R16_USB2_MPLL_M			__BITS(8,0)
#define USB2PHY_R17_REG				0x44
#define  USB2PHY_R17_USB2_MPLL_FILTER_PVT1		__BITS(31,28)
#define  USB2PHY_R17_USB2_MPLL_FILTER_PVT2		__BITS(27,24)
#define  USB2PHY_R17_USB2_MPLL_FILTER_MODE		__BIT(23)
#define  USB2PHY_R17_USB2_MPLL_LAMBDA0			__BITS(22,20)
#define  USB2PHY_R17_USB2_MPLL_LAMBDA1			__BITS(19,17)
#define  USB2PHY_R17_USB2_MPLL_FIX_EN			__BIT(16)
#define  USB2PHY_R17_USB2_MPLL_FRAC_IN			__BITS(13,0)
#define USB2PHY_R18_REG				0x48
#define  USB2PHY_R18_USB2_MPLL_ACG_RANGE		__BIT(31)
#define  USB2PHY_R18_USB2_MPLL_ADJ_LDO			__BITS(30,29)
#define  USB2PHY_R18_USB2_MPLL_ALPHA			__BITS(28,26)
#define  USB2PHY_R18_USB2_MPLL_BB_MODE			__BITS(25,24)
#define  USB2PHY_R18_USB2_MPLL_BIAS_ADJ			__BITS(23,22)
#define  USB2PHY_R18_USB2_MPLL_DATA_SEL			__BITS(21,19)
#define  USB2PHY_R18_USB2_MPLL_ROU			__BITS(18,16)
#define  USB2PHY_R18_USB2_MPLL_PFD_GAIN			__BITS(15,14)
#define  USB2PHY_R18_USB2_MPLL_DCO_CLK_SEL		__BIT(13)
#define  USB2PHY_R18_USB2_MPLL_DCO_M_EN			__BIT(12)
#define  USB2PHY_R18_USB2_MPLL_LK_S			__BITS(11,6)
#define  USB2PHY_R18_USB2_MPLL_LK_W			__BITS(5,2)
#define  USB2PHY_R18_USB2_MPLL_LKW_SEL			__BITS(1,0)
#define USB2PHY_R19_REG				0x4c
#define USB2PHY_R20_REG				0x50
#define  USB2PHY_R20_BYPASS_CAL_DONE_R5			__BIT(31)
#define  USB2PHY_R20_USB2_BGR_DBG_1_0			__BITS(30,29)
#define  USB2PHY_R20_USB2_BGR_VREF_4_0			__BITS(28,24)
#define  USB2PHY_R20_USB2_BGR_START			__BIT(21)
#define  USB2PHY_R20_USB2_BGR_ADJ_4_0			__BITS(20,16)
#define  USB2PHY_R20_USB2_EDGE_DRV_TRIM_1_0		__BITS(15,14)
#define  USB2PHY_R20_USB2_EDGE_DRV_EN			__BIT(13)
#define  USB2PHY_R20_USB2_DMON_SEL_3_0			__BITS(12,9)
#define  USB2PHY_R20_USB2_DMON_EN			__BIT(8)
#define  USB2PHY_R20_BYPASS_OTG_DET			__BIT(7)
#define  USB2PHY_R20_USB2_CAL_CODE_R5			__BIT(6)
#define  USB2PHY_R20_USB2_AMON_EN			__BIT(5)
#define  USB2PHY_R20_USB2_OTG_VBUSDET_EN		__BIT(4)
#define  USB2PHY_R20_USB2_OTG_VBUS_TRIM_2_0		__BITS(3,1)
#define  USB2PHY_R20_USB2_IDDET_EN			__BIT(0)
#define USB2PHY_R21_REG				0x54
#define  USB2PHY_R21_BYPASS_UTMI_REG			__BITS(25,20)
#define  USB2PHY_R21_BYPASS_UTMI_CNTR			__BITS(15,6)
#define  USB2PHY_R21_USB2_OTG_ACA_TRIM_1_0		__BITS(5,4)
#define  USB2PHY_R21_USB2_TX_STRG_PD			__BIT(3)
#define  USB2PHY_R21_USB2_OTG_ACA_EN			__BIT(2)
#define  USB2PHY_R21_USB2_CAL_ACK_EN			__BIT(1)
#define  USB2PHY_R21_USB2_BGR_FORCE			__BIT(0)
#define USB2PHY_R22_REG				0x58
#define USB2PHY_R23_REG				0x5c

#define USB2PHY_READ_REG(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define USB2PHY_WRITE_REG(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

struct mesong12_usb2phy_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct clk *sc_clk;
	struct fdtbus_reset *sc_reset;
	struct fdtbus_regulator *sc_supply;
	int sc_phandle;
};

static void *
mesong12_usb2phy_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 0)
		return NULL;

	return (void *)(uintptr_t)1;
}

static void
mesong12_usb2phy_release(device_t dev, void *priv)
{
	__nothing;
}

static int
mesong12_usb2phy_enable(device_t dev, void *priv, bool enable)
{
	struct mesong12_usb2phy_softc * const sc = device_private(dev);

	if (sc->sc_reset != NULL) {
		fdtbus_reset_assert(sc->sc_reset);
		delay(10);
		fdtbus_reset_deassert(sc->sc_reset);
		delay(1000);
	}

	if (sc->sc_supply != NULL)
		fdtbus_regulator_enable(sc->sc_supply);

	if (!enable)
		return 0;

	USB2PHY_WRITE_REG(sc, USB2PHY_R21_REG,
	    USB2PHY_READ_REG(sc, USB2PHY_R21_REG) |
	    USB2PHY_R21_USB2_OTG_ACA_EN);

	/* set PLL to 480MHz */
	USB2PHY_WRITE_REG(sc, USB2PHY_R16_REG,
	    USB2PHY_R16_USB2_MPLL_RESET |
	    USB2PHY_R16_USB2_MPLL_EN |
	    USB2PHY_R16_USB2_MPLL_FAST_LOCK |
	    __SHIFTIN(1, USB2PHY_R16_USB2_MPLL_LOCK_LONG) |
	    USB2PHY_R16_USB2_MPLL_LOAD |
	    __SHIFTIN(1, USB2PHY_R16_USB2_MPLL_N) |
	    __SHIFTIN(20, USB2PHY_R16_USB2_MPLL_M));
	USB2PHY_WRITE_REG(sc, USB2PHY_R17_REG,
	    __SHIFTIN(0, USB2PHY_R17_USB2_MPLL_FILTER_PVT1) |
	    __SHIFTIN(7, USB2PHY_R17_USB2_MPLL_FILTER_PVT2) |
	    __SHIFTIN(7, USB2PHY_R17_USB2_MPLL_LAMBDA0) |
	    __SHIFTIN(2, USB2PHY_R17_USB2_MPLL_LAMBDA1) |
	    __SHIFTIN(9, USB2PHY_R17_USB2_MPLL_FRAC_IN));
	USB2PHY_WRITE_REG(sc, USB2PHY_R18_REG,
	    USB2PHY_R18_USB2_MPLL_ACG_RANGE |
	    __SHIFTIN(1, USB2PHY_R18_USB2_MPLL_ADJ_LDO) |
	    __SHIFTIN(3, USB2PHY_R18_USB2_MPLL_ALPHA) |
	    __SHIFTIN(0, USB2PHY_R18_USB2_MPLL_BB_MODE) |
	    __SHIFTIN(1, USB2PHY_R18_USB2_MPLL_BIAS_ADJ) |
	    __SHIFTIN(3, USB2PHY_R18_USB2_MPLL_DATA_SEL) |
	    __SHIFTIN(7, USB2PHY_R18_USB2_MPLL_ROU) |
	    __SHIFTIN(1, USB2PHY_R18_USB2_MPLL_PFD_GAIN) |
	    __SHIFTIN(39, USB2PHY_R18_USB2_MPLL_LK_S) |
	    __SHIFTIN(9, USB2PHY_R18_USB2_MPLL_LK_W) |
	    __SHIFTIN(1, USB2PHY_R18_USB2_MPLL_LKW_SEL));
	delay(100);
	USB2PHY_WRITE_REG(sc, USB2PHY_R16_REG,
	    USB2PHY_R16_USB2_MPLL_EN |
	    USB2PHY_R16_USB2_MPLL_FAST_LOCK |
	    __SHIFTIN(1, USB2PHY_R16_USB2_MPLL_LOCK_LONG) |
	    USB2PHY_R16_USB2_MPLL_LOAD |
	    __SHIFTIN(1, USB2PHY_R16_USB2_MPLL_N) |
	    __SHIFTIN(20, USB2PHY_R16_USB2_MPLL_M));

	/* tune PHY */
	USB2PHY_WRITE_REG(sc, USB2PHY_R20_REG,
	    __SHIFTIN(0, USB2PHY_R20_USB2_BGR_DBG_1_0) |
	    __SHIFTIN(0, USB2PHY_R20_USB2_BGR_VREF_4_0) |
	    __SHIFTIN(0, USB2PHY_R20_USB2_BGR_ADJ_4_0) |
	    __SHIFTIN(3, USB2PHY_R20_USB2_EDGE_DRV_TRIM_1_0) |
	    USB2PHY_R20_USB2_EDGE_DRV_EN |
	    __SHIFTIN(15, USB2PHY_R20_USB2_DMON_SEL_3_0) |
	    USB2PHY_R20_USB2_OTG_VBUSDET_EN |
	    __SHIFTIN(4, USB2PHY_R20_USB2_OTG_VBUS_TRIM_2_0));

	USB2PHY_WRITE_REG(sc, USB2PHY_R04_REG,
	    __SHIFTIN(0, USB2PHY_R04_I_C2L_BIAS_TRIM) |
	    USB2PHY_R04_TEST_BYPASS_MODE_EN |
	    __SHIFTIN(0xfff, USB2PHY_R04_CALIBRATION_CODE_VALUE));

	/* tune disconnect threshold */
	USB2PHY_WRITE_REG(sc, USB2PHY_R03_REG,
	    __SHIFTIN(3, USB2PHY_R03_DISC_THRESH) |
	    __SHIFTIN(1, USB2PHY_R03_HSDIC_REF) |
	    __SHIFTIN(0, USB2PHY_R03_SQUELCH_REF));

	/* analog settings */
	USB2PHY_WRITE_REG(sc, USB2PHY_R14_REG,
	    __SHIFTIN(0, USB2PHY_R14_BYPASS_CTRL) |
	    __SHIFTIN(0, USB2PHY_R14_I_RPU_SW2_EN));
	USB2PHY_WRITE_REG(sc, USB2PHY_R13_REG,
	    __SHIFTIN(7, USB2PHY_R13_MINIMUM_COUNT_FOR_SYNC_DETECTION) |
	    USB2PHY_R13_UPDATE_PMA_SIGNALS);

	return 0;
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,g12a-usb2-phy" },
	DEVICE_COMPAT_EOL
};

static int
mesong12_usb2phy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static const struct fdtbus_phy_controller_func mesong12_usb2phy_funcs = {
	.acquire = mesong12_usb2phy_acquire,
	.release = mesong12_usb2phy_release,
	.enable = mesong12_usb2phy_enable
};

static void
mesong12_usb2phy_attach(device_t parent, device_t self, void *aux)
{
	struct mesong12_usb2phy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		goto attach_failure;
	}
	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		goto attach_failure;
	}

	sc->sc_reset = fdtbus_reset_get_index(phandle, 0);
	sc->sc_supply = fdtbus_regulator_acquire(phandle, "phy-supply");

	aprint_naive("\n");
	aprint_normal(": USB2 PHY\n");

	fdtbus_register_phy_controller(self, phandle, &mesong12_usb2phy_funcs);
	return;

 attach_failure:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, size);
	return;
}

CFATTACH_DECL_NEW(mesong12_usb2phy, sizeof(struct mesong12_usb2phy_softc),
    mesong12_usb2phy_match, mesong12_usb2phy_attach, NULL, NULL);
