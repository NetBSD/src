/* $NetBSD: rk_tcphy.c,v 1.1 2025/06/03 19:10:26 rjs Exp $ */
/* $OpenBSD: rktcphy.c,v 1.2 2022/04/06 18:59:28 naddy Exp $ */
/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2019 Emmanuel Vadot <manu@FreeBSD.Org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Rockchip PHY TYPEC
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	GRF_USB3OTG_BASE(x)	(0x2430 + (0x10 * x))
#define	GRF_USB3OTG_CON0(x)	(GRF_USB3OTG_BASE(x) + 0x0)
#define	GRF_USB3OTG_CON1(x)	(GRF_USB3OTG_BASE(x) + 0x4)
#define	 USB3OTG_CON1_U3_DIS	(1 << 0)

#define	GRF_USB3PHY_BASE(x)	(0x0e580 + (0xc * (x)))
#define	GRF_USB3PHY_CON0(x)	(GRF_USB3PHY_BASE(x) + 0x0)
#define	 USB3PHY_CON0_USB2_ONLY	(1 << 3)
#define	GRF_USB3PHY_CON1(x)	(GRF_USB3PHY_BASE(x) + 0x4)
#define	GRF_USB3PHY_CON2(x)	(GRF_USB3PHY_BASE(x) + 0x8)
#define	GRF_USB3PHY_STATUS0	0x0e5c0
#define	GRF_USB3PHY_STATUS1	0x0e5c4

#define	CMN_PLL0_VCOCAL_INIT		(0x84 << 2)
#define	CMN_PLL0_VCOCAL_ITER		(0x85 << 2)
#define	CMN_PLL0_INTDIV			(0x94 << 2)
#define	CMN_PLL0_FRACDIV		(0x95 << 2)
#define	CMN_PLL0_HIGH_THR		(0x96 << 2)
#define	CMN_PLL0_DSM_DIAG		(0x97 << 2)
#define	CMN_PLL0_SS_CTRL1		(0x98 << 2)
#define	CMN_PLL0_SS_CTRL2		(0x99 << 2)
#define	CMN_DIAG_PLL0_FBH_OVRD		(0x1c0 << 2)
#define	CMN_DIAG_PLL0_FBL_OVRD		(0x1c1 << 2)
#define	CMN_DIAG_PLL0_OVRD		(0x1c2 << 2)
#define	CMN_DIAG_PLL0_V2I_TUNE		(0x1c5 << 2)
#define	CMN_DIAG_PLL0_CP_TUNE		(0x1c6 << 2)
#define	CMN_DIAG_PLL0_LF_PROG		(0x1c7 << 2)
#define	CMN_DIAG_HSCLK_SEL		(0x1e0 << 2)
#define	 CMN_DIAG_HSCLK_SEL_PLL_CONFIG	0x30
#define	 CMN_DIAG_HSCLK_SEL_PLL_MASK	0x33

#define	TX_TXCC_MGNFS_MULT_000(lane)	((0x4050 | ((lane) << 9)) << 2)
#define	XCVR_DIAG_BIDI_CTRL(lane)	((0x40e8 | ((lane) << 9)) << 2)
#define	XCVR_DIAG_LANE_FCM_EN_MGN(lane)	((0x40f2 | ((lane) << 9)) << 2)
#define	TX_PSC_A0(lane)			((0x4100 | ((lane) << 9)) << 2)
#define	TX_PSC_A1(lane)			((0x4101 | ((lane) << 9)) << 2)
#define	TX_PSC_A2(lane)			((0x4102 | ((lane) << 9)) << 2)
#define	TX_PSC_A3(lane)			((0x4103 | ((lane) << 9)) << 2)
#define	TX_RCVDET_EN_TMR(lane)		((0x4122 | ((lane) << 9)) << 2)
#define	TX_RCVDET_ST_TMR(lane)		((0x4123 | ((lane) << 9)) << 2)

#define	RX_PSC_A0(lane)			((0x8000 | ((lane) << 9)) << 2)
#define	RX_PSC_A1(lane)			((0x8001 | ((lane) << 9)) << 2)
#define	RX_PSC_A2(lane)			((0x8002 | ((lane) << 9)) << 2)
#define	RX_PSC_A3(lane)			((0x8003 | ((lane) << 9)) << 2)
#define	RX_PSC_CAL(lane)		((0x8006 | ((lane) << 9)) << 2)
#define	RX_PSC_RDY(lane)		((0x8007 | ((lane) << 9)) << 2)
#define	RX_SIGDET_HL_FILT_TMR(lane)	((0x8090 | ((lane) << 9)) << 2)
#define	RX_REE_CTRL_DATA_MASK(lane)	((0x81bb | ((lane) << 9)) << 2)
#define	RX_DIAG_SIGDET_TUNE(lane)	((0x81dc | ((lane) << 9)) << 2)

#define	PMA_LANE_CFG			(0xc000 << 2)
#define	PIN_ASSIGN_D_F			0x5100
#define	DP_MODE_CTL			(0xc008 << 2)
#define	DP_MODE_ENTER_A2		0xc104
#define	PMA_CMN_CTRL1			(0xc800 << 2)
#define	 PMA_CMN_CTRL1_READY		(1 << 0)

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

static int rk_typec_match(device_t, cfdata_t, void *);
static void rk_typec_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-typec-phy" },
	DEVICE_COMPAT_EOL
};

struct rk_typec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct syscon		*sc_grf;
	struct clk		*sc_core_clk;
	struct clk		*sc_phy_ref_clk;
	struct fdtbus_reset	*sc_rst;
	struct fdtbus_reset	*sc_rst_pipe;
	struct fdtbus_reset	*sc_rst_tcphy;

	int			sc_mode;
	int			sc_phy_ctrl_id;
};

CFATTACH_DECL_NEW(rk_typec, sizeof(struct rk_typec_softc),
    rk_typec_match, rk_typec_attach, NULL, NULL);

int
rk_typec_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
rk_typec_attach(device_t parent, device_t self, void *aux)
{
	struct rk_typec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	};

	/*
	 * Find out which phy we are.  There is no property for this so we need
	 * to know the address to use the correct GRF registers.
	 */
	switch (addr) {
	case 0xff7c0000:
		sc->sc_phy_ctrl_id = 0;
		break;
	case 0xff800000:
		sc->sc_phy_ctrl_id = 1;
		break;
	default:
		aprint_error(": unknown address 0x%lx\n", addr);
		return;
	}

	sc->sc_grf = fdtbus_syscon_acquire(phandle, "rockchip,grf");
	if (sc->sc_grf == NULL) {
		aprint_error(": couldn't get grf syscon\n");
		return;
	}

	sc->sc_rst = fdtbus_reset_get(phandle, "uphy");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset uphy\n");
		return;
	}
	sc->sc_rst_pipe = fdtbus_reset_get(phandle, "uphy-pipe");
	if (sc->sc_rst_pipe == NULL) {
		aprint_error(": couldn't get reset uphy-pipe\n");
		return;
	}
	sc->sc_rst_tcphy = fdtbus_reset_get(phandle, "uphy-tcphy");
	if (sc->sc_rst_tcphy == NULL) {
		aprint_error(": couldn't get reset uphy-tcphy\n");
		return;
	}	
	fdtbus_reset_assert(sc->sc_rst);
	fdtbus_reset_assert(sc->sc_rst_pipe);
	fdtbus_reset_assert(sc->sc_rst_tcphy);

	fdtbus_clock_assign(phandle);
	sc->sc_core_clk = fdtbus_clock_get(phandle, "tcpdcore");
	if (sc->sc_core_clk == NULL) {
		aprint_error(": couldn't get tcpdcore clock\n");
		return;
	}
	sc->sc_phy_ref_clk = fdtbus_clock_get(phandle, "tcpdphy-ref");
	if (sc->sc_phy_ref_clk == NULL) {
		aprint_error(": couldn't get tcpdphy-ref clock\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": USB-C PHY\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = fdtbus_get_string(child, "name");
		cfaa.faa_quiet = false;

		config_found(self, &cfaa, NULL, CFARGS_NONE);
	}
}

/*
 * USB3 phy
 */

static int rk_tcphy_match(device_t, cfdata_t, void *);
static void rk_tcphy_attach(device_t, device_t, void *);

struct rk_tcphy_softc {
	device_t	sc_dev;
	int		sc_phandle;
};

CFATTACH_DECL_NEW(rk_tcphy, sizeof(struct rk_tcphy_softc),
	rk_tcphy_match, rk_tcphy_attach, NULL, NULL);

static void *
rk_tcphy_usb3_acquire(device_t dev, const void *data, size_t len)
{
	struct rk_tcphy_softc * const sc = device_private(dev);

	return sc;
}

static void
rk_tcphy_set_usb2_only(struct rk_typec_softc *sc, int usb2only)
{
	uint32_t reg;

	/* Disable usb3tousb2 only */
	syscon_lock(sc->sc_grf);
	reg = syscon_read_4(sc->sc_grf, GRF_USB3PHY_CON0(sc->sc_phy_ctrl_id));
	if (usb2only)
		reg |= USB3PHY_CON0_USB2_ONLY;
	else
		reg &= ~USB3PHY_CON0_USB2_ONLY;
	/* Write Mask */
	reg |= (USB3PHY_CON0_USB2_ONLY) << 16;
	syscon_write_4(sc->sc_grf, GRF_USB3PHY_CON0(sc->sc_phy_ctrl_id), reg);

	/* Enable the USB3 Super Speed port */
	reg = syscon_read_4(sc->sc_grf, GRF_USB3OTG_CON1(sc->sc_phy_ctrl_id));
	if (usb2only)
		reg |= USB3OTG_CON1_U3_DIS;
	else
		reg &= ~USB3OTG_CON1_U3_DIS;
	/* Write Mask */
	reg |= (USB3OTG_CON1_U3_DIS) << 16;
	syscon_write_4(sc->sc_grf, GRF_USB3OTG_CON1(sc->sc_phy_ctrl_id), reg);
	syscon_unlock(sc->sc_grf);
}

static int
rk_tcphy_usb3_enable(device_t dev, void *priv, bool enable)
{
	struct rk_typec_softc * const sc = device_private(device_parent(dev));
	uint32_t reg;
	int i;

	aprint_normal_dev(dev, "enable %d\n", enable);
	if (enable == false)
		return 0;

	rk_tcphy_set_usb2_only(sc, false);

	clk_enable(sc->sc_core_clk);
	clk_enable(sc->sc_phy_ref_clk);

	fdtbus_reset_deassert(sc->sc_rst_tcphy);

	/* 24M configuration, magic values from rockchip */
	HWRITE4(sc, PMA_CMN_CTRL1, 0x830);
	for (i = 0; i < 4; i++) {
		HWRITE4(sc, XCVR_DIAG_LANE_FCM_EN_MGN(i), 0x90);
		HWRITE4(sc, TX_RCVDET_EN_TMR(i), 0x960);
		HWRITE4(sc, TX_RCVDET_ST_TMR(i), 0x30);
	}
	reg = HREAD4(sc, CMN_DIAG_HSCLK_SEL);
	reg &= ~CMN_DIAG_HSCLK_SEL_PLL_MASK;
	reg |= CMN_DIAG_HSCLK_SEL_PLL_CONFIG;
	HWRITE4(sc, CMN_DIAG_HSCLK_SEL, reg);

	/* PLL configuration, magic values from rockchip */
	HWRITE4(sc, CMN_PLL0_VCOCAL_INIT, 0xf0);
	HWRITE4(sc, CMN_PLL0_VCOCAL_ITER, 0x18);
	HWRITE4(sc, CMN_PLL0_INTDIV, 0xd0);
	HWRITE4(sc, CMN_PLL0_FRACDIV, 0x4a4a);
	HWRITE4(sc, CMN_PLL0_HIGH_THR, 0x34);
	HWRITE4(sc, CMN_PLL0_SS_CTRL1, 0x1ee);
	HWRITE4(sc, CMN_PLL0_SS_CTRL2, 0x7f03);
	HWRITE4(sc, CMN_PLL0_DSM_DIAG, 0x20);
	HWRITE4(sc, CMN_DIAG_PLL0_OVRD, 0);
	HWRITE4(sc, CMN_DIAG_PLL0_FBH_OVRD, 0);
	HWRITE4(sc, CMN_DIAG_PLL0_FBL_OVRD, 0);
	HWRITE4(sc, CMN_DIAG_PLL0_V2I_TUNE, 0x7);
	HWRITE4(sc, CMN_DIAG_PLL0_CP_TUNE, 0x45);
	HWRITE4(sc, CMN_DIAG_PLL0_LF_PROG, 0x8);

	/* Configure the TX and RX line, magic values from rockchip */
	HWRITE4(sc, TX_PSC_A0(0), 0x7799);
	HWRITE4(sc, TX_PSC_A1(0), 0x7798);
	HWRITE4(sc, TX_PSC_A2(0), 0x5098);
	HWRITE4(sc, TX_PSC_A3(0), 0x5098);
	HWRITE4(sc, TX_TXCC_MGNFS_MULT_000(0), 0x0);
	HWRITE4(sc, XCVR_DIAG_BIDI_CTRL(0), 0xbf);

	HWRITE4(sc, RX_PSC_A0(1), 0xa6fd);
	HWRITE4(sc, RX_PSC_A1(1), 0xa6fd);
	HWRITE4(sc, RX_PSC_A2(1), 0xa410);
	HWRITE4(sc, RX_PSC_A3(1), 0x2410);
	HWRITE4(sc, RX_PSC_CAL(1), 0x23ff);
	HWRITE4(sc, RX_SIGDET_HL_FILT_TMR(1), 0x13);
	HWRITE4(sc, RX_REE_CTRL_DATA_MASK(1), 0x03e7);
	HWRITE4(sc, RX_DIAG_SIGDET_TUNE(1), 0x1004);
	HWRITE4(sc, RX_PSC_RDY(1), 0x2010);
	HWRITE4(sc, XCVR_DIAG_BIDI_CTRL(1), 0xfb);

	HWRITE4(sc, PMA_LANE_CFG, PIN_ASSIGN_D_F);

	HWRITE4(sc, DP_MODE_CTL, DP_MODE_ENTER_A2);

	fdtbus_reset_deassert(sc->sc_rst);

	for (i = 10000; i > 0; i--) {
		reg = HREAD4(sc, PMA_CMN_CTRL1);
		if (reg & PMA_CMN_CTRL1_READY)
			break;
		delay(10);
	}
	if (i == 0) {
		aprint_error_dev(sc->sc_dev, "timeout waiting for PMA\n");
		return ENXIO;
	}

	fdtbus_reset_deassert(sc->sc_rst_pipe);

	return 0;
}

const struct fdtbus_phy_controller_func rk_tcphy_usb3_funcs = {
 	.acquire = rk_tcphy_usb3_acquire,
 	.release = (void *)voidop,
 	.enable = rk_tcphy_usb3_enable,
};

static int
rk_tcphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *name = fdtbus_get_string(phandle, "name");

	if (strcmp(name, "usb3-port") == 0)
		return 1;
#if 0
	if (strcmp(name, "dp-port") == 0)
		return 1;
#endif
	return 0;
}

static void
rk_tcphy_attach(device_t parent, device_t self, void *aux)
{
	struct rk_tcphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *name = fdtbus_get_string(phandle, "name");

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");

	if (strcmp(name, "usb3-port") == 0) {
		aprint_normal(": USB3 port\n");
		fdtbus_register_phy_controller(self, phandle, &rk_tcphy_usb3_funcs);
	}
}
