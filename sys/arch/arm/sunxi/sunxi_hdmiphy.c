/* $NetBSD: sunxi_hdmiphy.c,v 1.8 2021/01/27 03:10:20 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sunxi_hdmiphy.c,v 1.8 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_hdmiphy.h>

#define	DBG_CTRL	0x000
#define	 DBG_CTRL_POL			__BITS(15,8)
#define	  DBG_CTRL_POL_NVSYNC		1
#define	  DBG_CTRL_POL_NHSYNC		2

#define	READ_EN		0x010
#define	 READ_EN_MAGIC			0x54524545	/* "TREE" */

#define	UNSCRAMBLE	0x014
#define	 UNSCRAMBLE_MAGIC		0x42494E47	/* "BING" */

#define	ANA_CFG1	0x020
#define	 ANA_CFG1_ENRCAL		__BIT(19)
#define	 ANA_CFG1_ENCALOG		__BIT(18)
#define	 ANA_CFG1_TMDSCLK_EN		__BIT(16)
#define	 ANA_CFG1_TXEN			__BITS(15,12)
#define	 ANA_CFG1_BIASEN		__BITS(11,8)
#define	 ANA_CFG1_ENP2S			__BITS(7,4)
#define	 ANA_CFG1_CKEN			__BIT(3)
#define	 ANA_CFG1_LDOEN			__BIT(2)
#define	 ANA_CFG1_ENVBS			__BIT(1)
#define	 ANA_CFG1_ENBI			__BIT(0)

#define	ANA_CFG2	0x024
#define	 ANA_CFG2_REG_RESDI		__BITS(5,0)

#define	ANA_CFG3	0x028
#define	 ANA_CFG3_REG_SDAEN		__BIT(2)
#define	 ANA_CFG3_REG_SCLEN		__BIT(0)

#define	PLL_CFG1	0x02c
#define	 PLL_CFG1_REG_OD1		__BIT(31)
#define	 PLL_CFG1_REG_OD0		__BIT(30)
#define	 PLL_CFG1_CKIN_SEL		__BIT(26)
#define	 PLL_CFG1_PLLEN			__BIT(25)
#define	 PLL_CFG1_B_IN			__BITS(5,0)

#define	PLL_CFG2	0x030
#define	 PLL_CFG2_PREDIV		__BITS(3,0)

#define	PLL_CFG3	0x034

#define	ANA_STS		0x038
#define	 ANA_STS_HPDO			__BIT(19)
#define	 ANA_STS_B_OUT			__BITS(16,11)
#define	 ANA_STS_RCALEND2D		__BIT(7)
#define	 ANA_STS_RESDO2D		__BITS(5,0)

#define	CEC		0x03c
#define	 CEC_CONTROL_SEL		__BIT(7)
#define	 CEC_INPUT_DATA			__BIT(1)
#define	 CEC_OUTPUT_DATA		__BIT(0)

#define	CONTROLLER_VER	0xff8

#define	PHY_VER		0xffc

struct sunxi_hdmiphy_softc;

static int sunxi_hdmiphy_match(device_t, cfdata_t, void *);
static void sunxi_hdmiphy_attach(device_t, device_t, void *);

static void sun8i_h3_hdmiphy_init(struct sunxi_hdmiphy_softc *);
static int sun8i_h3_hdmiphy_config(struct sunxi_hdmiphy_softc *, u_int);

struct sunxi_hdmiphy_data {
	void	(*init)(struct sunxi_hdmiphy_softc *);
	int	(*config)(struct sunxi_hdmiphy_softc *, u_int);
};

static const struct sunxi_hdmiphy_data sun8i_h3_hdmiphy_data = {
	.init = sun8i_h3_hdmiphy_init,
	.config = sun8i_h3_hdmiphy_config,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-h3-hdmi-phy",
	  .data = &sun8i_h3_hdmiphy_data },
	{ .compat = "allwinner,sun50i-a64-hdmi-phy",
	  .data = &sun8i_h3_hdmiphy_data },

	DEVICE_COMPAT_EOL
};

struct sunxi_hdmiphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	const struct sunxi_hdmiphy_data *sc_data;

	struct fdtbus_reset	*sc_rst;
	struct clk		*sc_clk_bus;
	struct clk		*sc_clk_mod;
	struct clk		*sc_clk_pll0;

	u_int			sc_rcalib;
};

#define	PHY_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	PHY_SET_CLEAR(sc, reg, set, clr)				\
	do {								\
		uint32_t _tval = PHY_READ((sc), (reg));			\
		_tval &= ~(clr);					\
		_tval |= (set);						\
		PHY_WRITE((sc), (reg), _tval);				\
	} while (0)
#define	PHY_SET(sc, reg, set)						\
	PHY_SET_CLEAR(sc, reg, set, 0)
#define	PHY_CLEAR(sc, reg, clr)						\
	PHY_SET_CLEAR(sc, reg, 0, clr)

CFATTACH_DECL_NEW(sunxi_hdmiphy, sizeof(struct sunxi_hdmiphy_softc),
	sunxi_hdmiphy_match, sunxi_hdmiphy_attach, NULL, NULL);

static void *
sunxi_hdmiphy_acquire(device_t dev, const void *data, size_t len)
{
	struct sunxi_hdmiphy_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return sc;
}

static void
sunxi_hdmiphy_release(device_t dev, void *priv)
{
}

static int
sunxi_hdmiphy_enable(device_t dev, void *priv, bool enable)
{
	return 0;
}

static const struct fdtbus_phy_controller_func sunxi_hdmiphy_funcs = {
	.acquire = sunxi_hdmiphy_acquire,
	.release = sunxi_hdmiphy_release,
	.enable = sunxi_hdmiphy_enable,
};

#ifdef SUNXI_HDMIPHY_DEBUG
static void
sunxi_hdmiphy_dump(struct sunxi_hdmiphy_softc *sc)
{
	device_printf(sc->sc_dev, "ANA_CFG1: %#x\tANA_CFG2: %#x\tANA_CFG3: %#x\n",
	    PHY_READ(sc, ANA_CFG1), PHY_READ(sc, ANA_CFG2), PHY_READ(sc, ANA_CFG3));
	device_printf(sc->sc_dev, "PLL_CFG1: %#x\tPLL_CFG2: %#x\tPLL_CFG3: %#x\n",
	    PHY_READ(sc, PLL_CFG1), PHY_READ(sc, PLL_CFG2), PHY_READ(sc, PLL_CFG3));
	device_printf(sc->sc_dev, "DBG_CTRL: %#x\tANA_STS: %#x\n",
	    PHY_READ(sc, DBG_CTRL), PHY_READ(sc, ANA_STS));
}
#endif

static void
sun8i_h3_hdmiphy_init(struct sunxi_hdmiphy_softc *sc)
{
	uint32_t val;
	int retry;

	PHY_WRITE(sc, ANA_CFG1, 0);

	PHY_SET(sc, ANA_CFG1, ANA_CFG1_ENBI);
	delay(5);

	/* Enable TMDS clock */
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_TMDSCLK_EN);

	/* Enable common voltage reference bias module */
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_ENVBS);
	delay(20);

	/* Enable internal LDO */
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_LDOEN);
	delay(5);

	/* Enable common clock module */
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_CKEN);
	delay(100);

	/* Enable resistance calibration analog and digital modules */
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_ENRCAL);
	delay(200);
	PHY_SET(sc, ANA_CFG1, ANA_CFG1_ENCALOG);

	/* P2S module enable for TMDS data lane */
	PHY_SET_CLEAR(sc, ANA_CFG1, __SHIFTIN(0x7, ANA_CFG1_ENP2S), ANA_CFG1_ENP2S);

	/* Wait for resistance calibration to finish */
	for (retry = 2000; retry > 0; retry--) {
		if ((PHY_READ(sc, ANA_STS) & ANA_STS_RCALEND2D) != 0)
			break;
		delay(1);
	}
	if (retry == 0)
		aprint_error_dev(sc->sc_dev, "HDMI PHY resistance calibration timed out\n");

	/* Enable current and voltage module */
	PHY_SET_CLEAR(sc, ANA_CFG1, __SHIFTIN(0xf, ANA_CFG1_BIASEN), ANA_CFG1_BIASEN);

	/* P2S module enable for TMDS clock lane */
	PHY_SET_CLEAR(sc, ANA_CFG1, __SHIFTIN(0xf, ANA_CFG1_ENP2S), ANA_CFG1_ENP2S);

	/* Enable DDC */
	PHY_SET(sc, ANA_CFG3, ANA_CFG3_REG_SDAEN | ANA_CFG3_REG_SCLEN);

	/* Set parent clock to videopll0 */
	PHY_CLEAR(sc, PLL_CFG1, PLL_CFG1_CKIN_SEL);

	/* Clear software control of CEC pins */
	PHY_CLEAR(sc, CEC, CEC_CONTROL_SEL);

	/* Read calibration value for source termination resistors */
	val = PHY_READ(sc, ANA_STS);
	sc->sc_rcalib = __SHIFTOUT(val, ANA_STS_RESDO2D);
}

/*
 * The following table is based on data from the "HDMI TX PHY S40 Specification".
 */
static const struct sun8i_h3_hdmiphy_init {
	/* PLL Recommended Configuration */
	uint32_t pll_cfg1;
	uint32_t pll_cfg2;
	uint32_t pll_cfg3;
	/* TMDS Characteristics Recommended Configuration */
	uint32_t ana_cfg1;
	uint32_t ana_cfg2;
	uint32_t ana_cfg3;
	bool ana_cfg2_rcal_200;
	u_int b_offset;
} sun8i_h3_hdmiphy_inittab[] = {
	/* 27 MHz */
	[0] = {
		.pll_cfg1 = 0x3ddc5040,	.pll_cfg2 = 0x8008430a,	.pll_cfg3 = 0x1,
		.ana_cfg1 = 0x11ffff7f,	.ana_cfg2 = 0x80623000,	.ana_cfg3 = 0x0f80c285,
		.ana_cfg2_rcal_200 = true,
	},
	/* 74.25 MHz */
	[1] = {
		.pll_cfg1 = 0x3ddc5040,	.pll_cfg2 = 0x80084343,	.pll_cfg3 = 0x1,
		.ana_cfg1 = 0x11ffff7f,	.ana_cfg2 = 0x80623000, .ana_cfg3 = 0x0f814385,
		.ana_cfg2_rcal_200 = true,
	},
	/* 148.5 MHz */
	[2] = {
		.pll_cfg1 = 0x3ddc5040,	.pll_cfg2 = 0x80084381,	.pll_cfg3 = 0x1,
		.ana_cfg1 = 0x01ffff7f,	.ana_cfg2 = 0x8063a800,	.ana_cfg3 = 0x0f81c485,
	},
	/* 297 MHz */
	[3] = {
		.pll_cfg1 = 0x35dc5fc0,	.pll_cfg2 = 0x800863c0,	.pll_cfg3 = 0x1,
		.ana_cfg1 = 0x01ffff7f,	.ana_cfg2 = 0x8063b000,	.ana_cfg3 = 0x0f8246b5,
		.b_offset = 2,
	},
};

static int
sun8i_h3_hdmiphy_config(struct sunxi_hdmiphy_softc *sc, u_int rate)
{
	const struct sun8i_h3_hdmiphy_init *inittab;
	u_int init_index, b_out, prediv;
	uint32_t val, rcalib;

	if (rate == 0) {
		/* Disable the PHY */
		PHY_WRITE(sc, ANA_CFG1, ANA_CFG1_LDOEN | ANA_CFG1_ENVBS | ANA_CFG1_ENBI);
		PHY_WRITE(sc, PLL_CFG1, 0);
		return 0;
	}

	init_index = 0;
	if (rate > 27000000)
		init_index++;
	if (rate > 74250000)
		init_index++;
	if (rate > 148500000)
		init_index++;
	inittab = &sun8i_h3_hdmiphy_inittab[init_index];

	val = PHY_READ(sc, PLL_CFG2);
	prediv = val & PLL_CFG2_PREDIV;

	/* Config PLL */
	PHY_WRITE(sc, PLL_CFG1, inittab->pll_cfg1 & ~PLL_CFG1_CKIN_SEL);
	PHY_WRITE(sc, PLL_CFG2, (inittab->pll_cfg2 & ~PLL_CFG2_PREDIV) | prediv);
	delay(15000);
	PHY_WRITE(sc, PLL_CFG3, inittab->pll_cfg3);

	/* Enable PLL */
	PHY_SET(sc, PLL_CFG1, PLL_CFG1_PLLEN);
	delay(100000);

	/* Config PLL */
	val = PHY_READ(sc, ANA_STS);
	b_out = __SHIFTOUT(val, ANA_STS_B_OUT);
	b_out = MIN(b_out + inittab->b_offset, __SHIFTOUT_MASK(ANA_STS_B_OUT));

	PHY_SET(sc, PLL_CFG1, PLL_CFG1_REG_OD1 | PLL_CFG1_REG_OD0);
	PHY_SET(sc, PLL_CFG1, __SHIFTIN(b_out, PLL_CFG1_B_IN));
	delay(100000);

	/* Config TMDS characteristics */
	if (inittab->ana_cfg2_rcal_200)
		rcalib = sc->sc_rcalib >> 2;
	else
		rcalib = 0;
	PHY_WRITE(sc, ANA_CFG1, inittab->ana_cfg1);
	PHY_WRITE(sc, ANA_CFG2, inittab->ana_cfg2 | rcalib);
	PHY_WRITE(sc, ANA_CFG3, inittab->ana_cfg3);

#ifdef SUNXI_HDMIPHY_DEBUG
	sunxi_hdmiphy_dump(sc);
#endif

	return 0;
}

static int
sunxi_hdmiphy_set_rate(struct sunxi_hdmiphy_softc *sc, u_int new_rate)
{
	u_int prediv, best_prediv, best_rate;

	if (sc->sc_clk_pll0 == NULL)
		return 0;

	const u_int parent_rate = clk_get_rate(sc->sc_clk_pll0);

	best_rate = 0;

	for (prediv = 0; prediv <= __SHIFTOUT_MASK(PLL_CFG2_PREDIV); prediv++) {
		const u_int tmp_rate = parent_rate / (prediv + 1);
		const int diff = new_rate - tmp_rate;
		if (diff >= 0 && tmp_rate > best_rate) {
			best_rate = tmp_rate;
			best_prediv = prediv;
		}
	}

	if (best_rate == 0)
		return ERANGE;

	PHY_SET_CLEAR(sc, PLL_CFG2, __SHIFTIN(best_prediv, PLL_CFG2_PREDIV), PLL_CFG2_PREDIV);

	return 0;
}

static int
sunxi_hdmiphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_hdmiphy_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_hdmiphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk_bus, *clk_mod, *clk_pll0;
	struct fdtbus_reset *rst;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	rst = fdtbus_reset_get(phandle, "phy");
	if (rst == NULL) {
		aprint_error(": couldn't get reset\n");
		return;
	}
	clk_bus = fdtbus_clock_get(phandle, "bus");
	clk_mod = fdtbus_clock_get(phandle, "mod");
	clk_pll0 = fdtbus_clock_get(phandle, "pll-0");
	if (clk_bus == NULL || clk_mod == NULL || clk_pll0 == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_data = of_compatible_lookup(phandle, compat_data)->data;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_rst = rst;
	sc->sc_clk_bus = clk_bus;
	sc->sc_clk_mod = clk_mod;
	sc->sc_clk_pll0 = clk_pll0;

	aprint_naive("\n");
	aprint_normal(": HDMI PHY\n");

	fdtbus_register_phy_controller(self, phandle, &sunxi_hdmiphy_funcs);
}

void
sunxi_hdmiphy_init(struct fdtbus_phy *phy)
{
	device_t dev = fdtbus_phy_device(phy);
	struct sunxi_hdmiphy_softc * const sc = device_private(dev);

	clk_enable(sc->sc_clk_bus);
	clk_enable(sc->sc_clk_mod);
	clk_enable(sc->sc_clk_pll0);

	fdtbus_reset_deassert(sc->sc_rst);

	sc->sc_data->init(sc);

	PHY_WRITE(sc, READ_EN, READ_EN_MAGIC);
	PHY_WRITE(sc, UNSCRAMBLE, UNSCRAMBLE_MAGIC);

#ifdef SUNXI_HDMIPHY_DEBUG
	sunxi_hdmiphy_dump(sc);
#endif
}

int
sunxi_hdmiphy_config(struct fdtbus_phy *phy, struct drm_display_mode *mode)
{
	device_t dev = fdtbus_phy_device(phy);
	struct sunxi_hdmiphy_softc * const sc = device_private(dev);
	u_int pol;
	int error;

	pol = 0;
	if ((mode->flags & DRM_MODE_FLAG_NHSYNC) != 0)
		pol |= __SHIFTIN(DBG_CTRL_POL_NHSYNC, DBG_CTRL_POL);
	if ((mode->flags & DRM_MODE_FLAG_NVSYNC) != 0)
		pol |= __SHIFTIN(DBG_CTRL_POL_NVSYNC, DBG_CTRL_POL);

	PHY_SET_CLEAR(sc, DBG_CTRL, pol, DBG_CTRL_POL);

	error = sunxi_hdmiphy_set_rate(sc, mode->crtc_clock * 1000);
	if (error != 0) {
		aprint_error_dev(dev, "failed to set HDMI PHY clock: %d\n", error);
		return error;
	}

	return sc->sc_data->config(sc, mode->crtc_clock * 1000);
}

bool
sunxi_hdmiphy_detect(struct fdtbus_phy *phy, bool force)
{
	device_t dev = fdtbus_phy_device(phy);
	struct sunxi_hdmiphy_softc * const sc = device_private(dev);
	uint32_t val;

	val = PHY_READ(sc, ANA_STS);

	return ISSET(val, ANA_STS_HPDO);
}
