/* $NetBSD: awin_tcon.c,v 1.6 2015/10/25 20:54:19 bouyer Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_allwinner.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_tcon.c,v 1.6 2015/10/25 20:54:19 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/videomode/videomode.h>

#define DIVIDE(x,y)     (((x) + ((y) / 2)) / (y))

struct awin_tcon_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ch1clk_bsh;
	unsigned int sc_port;
	unsigned int sc_clk_pll;
	unsigned int sc_clk_div;
	bool sc_clk_dbl;
	unsigned int sc_debe_unit;
};

static bus_space_handle_t tcon_mux_bsh;
static bool tcon_mux_inited = false;

#define TCON_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TCON_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_tcon_match(device_t, cfdata_t, void *);
static void	awin_tcon_attach(device_t, device_t, void *);

static void	awin_tcon_set_pll(struct awin_tcon_softc *,
				  const struct videomode *);

static void
awin_tcon_clear_reset(struct awinio_attach_args * const aio, int unit)
{
	KASSERT(unit == 0 || unit == 1);
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET1_REG,
		    AWIN_A31_AHB_RESET1_LCD0_RST << unit,
		    0);
	} else {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_LCD0_CH0_CLK_REG + (unit * 4),
		    AWIN_LCDx_CH0_CLK_LCDx_RST, 0);
	}

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_LCD0 << unit, 0);
}

CFATTACH_DECL_NEW(awin_tcon, sizeof(struct awin_tcon_softc),
	awin_tcon_match, awin_tcon_attach, NULL, NULL);

static int
awin_tcon_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_tcon_attach(device_t parent, device_t self, void *aux)
{
	struct awin_tcon_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	int8_t debe_unit = -1;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_port = loc->loc_port;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh,
	    AWIN_LCD0_CH1_CLK_REG + (loc->loc_port * 4), 4, &sc->sc_ch1clk_bsh);
	if (!tcon_mux_inited) {
		/* the mux register is only in LCD0 */
		bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
		    AWIN_LCD0_OFFSET + AWIN_TCON_MUX_CTL_REG, 4, &tcon_mux_bsh);
		tcon_mux_inited = true;
		/* always enable tcon0, the mux is there */
		awin_tcon_clear_reset(aio, 0);
	}

	if (prop_dictionary_get_int8(cfg, "debe_unit", &debe_unit)) {
		sc->sc_debe_unit = debe_unit;
	} else {
		sc->sc_debe_unit = 0; /* default value */
	}

		

	aprint_naive("\n");
	aprint_normal(": LCD/TV timing controller (TCON%d)\n", loc->loc_port);
	switch (sc->sc_port) {
	case 0:
		awin_pll3_enable();
		sc->sc_clk_pll = 3;
		break;
	case 1:
		awin_pll7_enable();
		sc->sc_clk_pll = 7;
		break;
	default:
		panic("awin_tcon port\n");
	}
	if (debe_unit >= 0) {
		aprint_verbose_dev(self, ": using DEBE%d, pll%d\n",
		    sc->sc_debe_unit, sc->sc_clk_pll);
	}

	awin_tcon_clear_reset(aio, sc->sc_port);
	
	TCON_WRITE(sc, AWIN_TCON_GCTL_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT0_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT1_REG,
	    __SHIFTIN(0x20, AWIN_TCON_GINT1_TCON1_LINENO));
	TCON_WRITE(sc, AWIN_TCON0_DCLK_REG, 0xf0000000);
	TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0xffffffff);
	TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0xffffffff);
}

static void
awin_tcon_calc_pll(int f_ref, int f_out, int *pm, int *pn)
{
	int best = 1000000;
	for (int m = 1; m <= 15; m++) {
		for (int n = 9; n <= 127; n++) {
			int f_cur = (n * f_ref) / m;
			int diff = f_out - f_cur;
			if (diff > 0 && diff < best) {
				best = diff;
				*pm = m;
				*pn = n;
			}
		}
	}
}

static void
awin_tcon_set_pll(struct awin_tcon_softc *sc, const struct videomode *mode)
{
	int n = 0, m = 0, n2 = 0, m2 = 0;
	bool dbl = false;

	awin_tcon_calc_pll(3000, mode->dot_clock, &m, &n);
	awin_tcon_calc_pll(6000, mode->dot_clock, &m2, &n2);

	int f_single = m ? (n * 3000) / m : 0;
	int f_double = m2 ? (n2 * 6000) / m2 : 0;

	if (f_double > f_single) {
		dbl = true;
		n = n2;
		m = m2;
	}

	if (n == 0 || m == 0) {
		device_printf(sc->sc_dev, "couldn't set pll to %d Hz\n",
		    mode->dot_clock * 1000);
		sc->sc_clk_div = 0;
		return;
	}

#ifdef AWIN_TCON_DEBUG
	device_printf(sc->sc_dev, "pll n=%d m=%d dbl=%c freq=%d\n", n, m,
	    dbl ? 'Y' : 'N', n * 3000000);
#endif

	switch(sc->sc_clk_pll) {
	case 3:
		awin_pll3_set_rate(n * 3000000);
		awin_reg_set_clear(sc->sc_bst, sc->sc_ch1clk_bsh, 0,
		    AWIN_CLK_OUT_ENABLE |
		    AWIN_LCDx_CH1_SCLK1_GATING |
		    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3_2X :
				    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3,
			      AWIN_LCDx_CHx_CLK_SRC_SEL) |
		    __SHIFTIN(m - 1, AWIN_LCDx_CH1_CLK_DIV_RATIO_M),
		    AWIN_LCDx_CH1_CLK_DIV_RATIO_M |
		    AWIN_LCDx_CHx_CLK_SRC_SEL |
		    AWIN_LCDx_CH1_SCLK1_SRC_SEL);
		break;
	case 7:
		awin_pll7_set_rate(n * 3000000);
		awin_reg_set_clear(sc->sc_bst, sc->sc_ch1clk_bsh, 0,
		    AWIN_CLK_OUT_ENABLE |
		    AWIN_LCDx_CH1_SCLK1_GATING |
		    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7_2X :
				    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7,
			      AWIN_LCDx_CHx_CLK_SRC_SEL) |
		    __SHIFTIN(m - 1, AWIN_LCDx_CH1_CLK_DIV_RATIO_M),
		    AWIN_LCDx_CH1_CLK_DIV_RATIO_M |
		    AWIN_LCDx_CHx_CLK_SRC_SEL |
		    AWIN_LCDx_CH1_SCLK1_SRC_SEL);
		break;
	default:
		panic("awin_tcon pll");
	}

	sc->sc_clk_div = m;
	sc->sc_clk_dbl = dbl;
}

void
awin_tcon_enable(int unit, bool enable)
{
	struct awin_tcon_softc *sc;
	device_t dev;
	uint32_t val;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return;
	}
	sc = device_private(dev);

	awin_debe_enable(sc->sc_debe_unit, enable);
	delay(20000);
	val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
	if (enable) {
		val |= AWIN_TCON_GCTL_EN;
	} else {
		val &= ~AWIN_TCON_GCTL_EN;
	}
	TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);

	val = TCON_READ(sc, AWIN_TCON1_IO_TRI_REG);
	if (enable) {
		val &= ~0x03000000;
	} else {
		val |= 0x03000000;
	}
	TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, val);

	KASSERT(tcon_mux_inited);
	val = bus_space_read_4(sc->sc_bst, tcon_mux_bsh, 0);
#ifdef AWIN_TCON_DEBUG
	printf("awin_tcon_enable(%d) val 0x%x", unit, val);
#endif
	val &= ~ AWIN_TCON_MUX_CTL_HDMI_OUTPUT_SRC;
	if (unit == 0) {
		val |= __SHIFTIN(AWIN_TCON_MUX_CTL_HDMI_OUTPUT_SRC_LCDC0_TCON1,
		    AWIN_TCON_MUX_CTL_HDMI_OUTPUT_SRC);
	} else if (unit == 1) {
		val |= __SHIFTIN(AWIN_TCON_MUX_CTL_HDMI_OUTPUT_SRC_LCDC1_TCON1,
		    AWIN_TCON_MUX_CTL_HDMI_OUTPUT_SRC);
	} 
#ifdef AWIN_TCON_DEBUG
	printf(" -> 0x%x", val);
#endif
	bus_space_write_4(sc->sc_bst, tcon_mux_bsh, 0, val);
#ifdef AWIN_TCON_DEBUG
	printf(": 0x%" PRIxBSH " 0x%" PRIxBSH " 0x%x 0x%x\n", sc->sc_bsh,
	    tcon_mux_bsh, bus_space_read_4(sc->sc_bst, tcon_mux_bsh, 0),
	    TCON_READ(sc, AWIN_TCON_MUX_CTL_REG));
#endif
}

void
awin_tcon_set_videomode(int unit, const struct videomode *mode)
{
	struct awin_tcon_softc *sc;
	device_t dev;
	uint32_t val;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return;
	}
	sc = device_private(dev);

	awin_debe_set_videomode(sc->sc_debe_unit, mode);
	if (mode) {
		const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
		const u_int phsync_p = !!(mode->flags & VID_PHSYNC);
		const u_int pvsync_p = !!(mode->flags & VID_PVSYNC);
		const u_int hspw = mode->hsync_end - mode->hsync_start;
		const u_int hbp = mode->htotal - mode->hsync_start;
		const u_int vspw = mode->vsync_end - mode->vsync_start;
		const u_int vbp = mode->vtotal - mode->vsync_start;
		const u_int vblank_len =
		    ((mode->vtotal << interlace_p) >> 1) - mode->vdisplay - 2;
		const u_int start_delay =
		    vblank_len >= 32 ? 30 : vblank_len - 2;

		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val |= AWIN_TCON_GCTL_IO_MAP_SEL;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);

		val = TCON_READ(sc, AWIN_TCON0_CTL_REG);
		val &= ~0x00400003;
		if (sc->sc_debe_unit == 0) {
			val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_DE0,
					 AWIN_TCON_CTL_SRC_SEL);
		} else {
			val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_DE1,
					 AWIN_TCON_CTL_SRC_SEL);
		}
		TCON_WRITE(sc, AWIN_TCON0_CTL_REG, val);

		/* enable */
		val = AWIN_TCON_CTL_EN;
		if (interlace_p)
			val |= AWIN_TCON_CTL_INTERLACE_EN;
		val |= __SHIFTIN(start_delay, AWIN_TCON_CTL_START_DELAY);
#ifdef AWIN_TCON1_BLUEDATA
		val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_BLUEDATA,
				 AWIN_TCON_CTL_SRC_SEL);
#else
		if (sc->sc_debe_unit == 0) {
			val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_DE0,
					 AWIN_TCON_CTL_SRC_SEL);
		} else {
			val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_DE1,
					 AWIN_TCON_CTL_SRC_SEL);
		}
#endif
		TCON_WRITE(sc, AWIN_TCON1_CTL_REG, val);

		/* Source width/height */
		TCON_WRITE(sc, AWIN_TCON1_BASIC0_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Scaler width/height */
		TCON_WRITE(sc, AWIN_TCON1_BASIC1_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Output width/height */
		TCON_WRITE(sc, AWIN_TCON1_BASIC2_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Horizontal total + back porch */
		TCON_WRITE(sc, AWIN_TCON1_BASIC3_REG,
		    ((mode->htotal - 1) << 16) | (hbp - 1));
		/* Vertical total + back porch */
		u_int vtotal = mode->vtotal * 2;
		if (interlace_p) {
			u_int framerate =
			    DIVIDE(DIVIDE(mode->dot_clock * 1000, mode->htotal),
			    mode->vtotal);
			u_int clk = mode->htotal * (mode->vtotal * 2 + 1) *
			    framerate;
			if ((clk / 2) == mode->dot_clock * 1000)
				vtotal += 1;
		}
		TCON_WRITE(sc, AWIN_TCON1_BASIC4_REG,
		    (vtotal << 16) | (vbp - 1));

		/* Sync */
		TCON_WRITE(sc, AWIN_TCON1_BASIC5_REG,
		    ((hspw - 1) << 16) | (vspw - 1));
		/* Polarity */
		val = AWIN_TCON_IO_POL_IO2_INV;
		if (phsync_p)
			val |= AWIN_TCON_IO_POL_PHSYNC;
		if (pvsync_p)
			val |= AWIN_TCON_IO_POL_PVSYNC;
		TCON_WRITE(sc, AWIN_TCON1_IO_POL_REG, val);

		TCON_WRITE(sc, AWIN_TCON_GINT1_REG,
		    __SHIFTIN(start_delay + 2, AWIN_TCON_GINT1_TCON1_LINENO));

		/* Setup LCDx CH1 PLL */
		awin_tcon_set_pll(sc, mode);
#if 0
{
	int i;
	for (i = 0; i < 0x800; i += 4) {
		printf("TCON 0x%04x: 0x%08x\n", i, TCON_READ(sc, i));
	}
}
#endif
	} else {
		/* disable */
		val = TCON_READ(sc, AWIN_TCON1_CTL_REG);
		val &= ~AWIN_TCON_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON1_CTL_REG, val);
	}
}

unsigned int
awin_tcon_get_clk_pll(int unit)
{
	struct awin_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return 0;
	}
	sc = device_private(dev);

	return sc->sc_clk_pll;
}

unsigned int
awin_tcon_get_clk_div(int unit)
{
	struct awin_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return 0;
	}
	sc = device_private(dev);

	return sc->sc_clk_div;
}

bool
awin_tcon_get_clk_dbl(int unit)
{
	struct awin_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return 0;
	}
	sc = device_private(dev);

	return sc->sc_clk_dbl;
}
