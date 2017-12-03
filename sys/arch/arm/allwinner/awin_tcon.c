/* $NetBSD: awin_tcon.c,v 1.13.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: awin_tcon.c,v 1.13.16.2 2017/12/03 11:35:51 jdolecek Exp $");

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

struct awin_tcon_gpio {
	const char *value;
	const char *name;
};


struct awin_tcon_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ch0clk_bsh;
	bus_space_handle_t sc_ch1clk_bsh;
	unsigned int sc_port;
	unsigned int sc_clk_pll;
	unsigned int sc_clk_div;
	bool sc_clk_dbl;
	unsigned int sc_debe_unit;
	unsigned int sc_output_type;
#define OUTPUT_HDMI 0
#define OUTPUT_LVDS 1
#define OUTPUT_VGA 2
	const char *sc_lcdpwr_pin_name;
	struct awin_gpio_pindata sc_lcdpwr_pin;
	const char *sc_lcdblk_pin_name;
	struct awin_gpio_pindata sc_lcdblk_pin;
};

static const struct awin_gpio_pinset awin_lvds0_pinset = 
	{ 'D', AWIN_PIO_PD_LVDS0_FUNC, AWIN_PIO_PD_LVDS0_PINS};
static const struct awin_gpio_pinset awin_lvds1_pinset = 
	{ 'D', AWIN_PIO_PD_LVDS1_FUNC, AWIN_PIO_PD_LVDS1_PINS};

static bus_space_handle_t tcon_mux_bsh;
static bool tcon_mux_inited = false;

#define TCON_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TCON_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_tcon_match(device_t, cfdata_t, void *);
static void	awin_tcon_attach(device_t, device_t, void *);

static void	awin_tcon_set_pll(struct awin_tcon_softc *, int, int);
static void	awin_tcon0_set_video(struct awin_tcon_softc *);
static void 	awin_tcon0_enable(struct awin_tcon_softc *, bool);

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
	const char *output;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_port = loc->loc_port;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh,
	    AWIN_LCD0_CH0_CLK_REG + (loc->loc_port * 4), 4, &sc->sc_ch0clk_bsh);
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

	if (prop_dictionary_get_cstring_nocopy(cfg, "output", &output)) {
		if (strcmp(output, "hdmi") == 0) {
			sc->sc_output_type = OUTPUT_HDMI;
		} else if (strcmp(output, "lvds") == 0) {
			sc->sc_output_type = OUTPUT_LVDS;
		} else if (strcmp(output, "vga") == 0) {
			sc->sc_output_type = OUTPUT_VGA;
		} else {
			panic("tcon: wrong mode %s", output);
		}
	} else {
		sc->sc_output_type = OUTPUT_HDMI; /* default */
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

	aprint_verbose_dev(self, ": using DEBE%d, pll%d\n",
		    device_unit(self), sc->sc_clk_pll);

	awin_tcon_clear_reset(aio, sc->sc_port);
	
	TCON_WRITE(sc, AWIN_TCON_GCTL_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT0_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT1_REG,
	    __SHIFTIN(0x20, AWIN_TCON_GINT1_TCON0_LINENO));
	TCON_WRITE(sc, AWIN_TCON0_DCLK_REG, 0xf0000000);
	TCON_WRITE(sc, AWIN_TCON0_CTL_REG, 0);
	TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0xffffffff);
	TCON_WRITE(sc, AWIN_TCON1_CTL_REG, 0);
	TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0xffffffff);
	if (sc->sc_output_type == OUTPUT_LVDS) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_LVDS_CLK_REG, AWIN_LVDS_CLK_ENABLE, 0);
		awin_tcon0_set_video(sc);
	}
}

static void
awin_tcon_calc_pll(int f_ref, int f_out, int min_m, int *pm, int *pn)
{
	int best = 1000000;
	KASSERT(min_m > 0);
	for (int m = min_m; m <= 15; m++) {
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
awin_tcon_set_pll(struct awin_tcon_softc *sc, int dclk, int min_div)
{
	int n = 0, m = 0, n2 = 0, m2 = 0;
	bool dbl = false;

	awin_tcon_calc_pll(3000, dclk, min_div, &m, &n);
	awin_tcon_calc_pll(6000, dclk, min_div, &m2, &n2);

	int f_single = m ? (n * 3000) / m : 0;
	int f_double = m2 ? (n2 * 6000) / m2 : 0;

	if (f_double > f_single) {
		dbl = true;
		n = n2;
		m = m2;
	}

	if (n == 0 || m == 0) {
		device_printf(sc->sc_dev, "couldn't set pll to %d Hz\n",
		    dclk * 1000);
		sc->sc_clk_div = 0;
		return;
	}

#ifdef AWIN_TCON_DEBUG
	device_printf(sc->sc_dev, "ch%d pll%d n=%d m=%d dbl=%c freq=%d\n",
	    (sc->sc_output_type == OUTPUT_HDMI) ? 1 : 0,
	    sc->sc_clk_pll, n, m, dbl ? 'Y' : 'N', n * 3000000);
#endif

	switch(sc->sc_clk_pll) {
	case 3:
		awin_pll3_set_rate(n * 3000000);
		if ((sc->sc_output_type == OUTPUT_HDMI) ||
		    (sc->sc_output_type == OUTPUT_VGA)) {
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
		} else {
			awin_reg_set_clear(sc->sc_bst, sc->sc_ch0clk_bsh, 0,
			    AWIN_CLK_OUT_ENABLE | AWIN_LCDx_CH0_CLK_LCDx_RST |
			    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3_2X :
					    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3,
				      AWIN_LCDx_CHx_CLK_SRC_SEL),
			    AWIN_LCDx_CHx_CLK_SRC_SEL);
			awin_reg_set_clear(sc->sc_bst, sc->sc_ch1clk_bsh, 0,
			    AWIN_CLK_OUT_ENABLE |
			    AWIN_LCDx_CH1_SCLK1_GATING |
			    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3_2X :
					    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3,
				      AWIN_LCDx_CHx_CLK_SRC_SEL) |
			    __SHIFTIN(10, AWIN_LCDx_CH1_CLK_DIV_RATIO_M),
			    AWIN_LCDx_CH1_CLK_DIV_RATIO_M |
			    AWIN_LCDx_CHx_CLK_SRC_SEL |
			    AWIN_LCDx_CH1_SCLK1_SRC_SEL);
		}
		break;
	case 7:
		awin_pll7_set_rate(n * 3000000);
		if ((sc->sc_output_type == OUTPUT_HDMI) || 
		    (sc->sc_output_type == OUTPUT_VGA)) {
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
		} else {
			/* pll7x2 not available for lcd0ch0 */
			KASSERT(dbl == false || sc->sc_port != 0);
			awin_reg_set_clear(sc->sc_bst, sc->sc_ch0clk_bsh, 0,
			    AWIN_CLK_OUT_ENABLE | AWIN_LCDx_CH0_CLK_LCDx_RST |
			    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7_2X :
					    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7,
				      AWIN_LCDx_CHx_CLK_SRC_SEL),
			    AWIN_LCDx_CHx_CLK_SRC_SEL);
			awin_reg_set_clear(sc->sc_bst, sc->sc_ch1clk_bsh, 0,
			    AWIN_CLK_OUT_ENABLE |
			    AWIN_LCDx_CH1_SCLK1_GATING |
			    __SHIFTIN(dbl ? AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7_2X :
					    AWIN_LCDx_CHx_CLK_SRC_SEL_PLL7,
				      AWIN_LCDx_CHx_CLK_SRC_SEL) |
			    __SHIFTIN(10, AWIN_LCDx_CH1_CLK_DIV_RATIO_M),
			    AWIN_LCDx_CH1_CLK_DIV_RATIO_M |
			    AWIN_LCDx_CHx_CLK_SRC_SEL |
			    AWIN_LCDx_CH1_SCLK1_SRC_SEL);
		}
		break;
	default:
		panic("awin_tcon pll");
	}

	sc->sc_clk_div = m;
	sc->sc_clk_dbl = dbl;
}

static void
awin_tcon0_set_video(struct awin_tcon_softc *sc)
{
	int32_t lcd_x, lcd_y, lcd_dclk_freq;
	int32_t lcd_hbp, lcd_ht, lcd_vbp, lcd_vt;
	int32_t lcd_hspw, lcd_vspw, lcd_io_cfg0;
	uint32_t vblk, start_delay;
	prop_dictionary_t cfg = device_properties(sc->sc_dev);
	uint32_t val;
	bool propb;
	bool dualchan = false;
	static struct videomode mode;

	if (!prop_dictionary_get_int32(cfg, "lcd_x", &lcd_x)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_x\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_y", &lcd_y)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_y\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_dclk_freq", &lcd_dclk_freq)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_dclk_freq\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_hbp", &lcd_hbp)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_hbp\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_ht", &lcd_ht)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_ht\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_vbp", &lcd_vbp)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_vbp\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_vt", &lcd_vt)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_vt\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_hspw", &lcd_hspw)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_hspw\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_vspw", &lcd_vspw)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_vspw\n");
		return;
	}
	if (!prop_dictionary_get_int32(cfg, "lcd_io_cfg0", &lcd_io_cfg0)) {
		aprint_error_dev(sc->sc_dev, ": can't read lcd_io_cfg0\n");
		return;
	}

	if (prop_dictionary_get_bool(cfg, "lvds_dual", &propb) && propb)
		dualchan = true;
	if (!awin_gpio_pinset_available(&awin_lvds0_pinset)) {
		aprint_error_dev(sc->sc_dev, "lvds0 pins not available\n");
		return;
	}
	if (dualchan && !awin_gpio_pinset_available(&awin_lvds1_pinset)) {
		aprint_error_dev(sc->sc_dev, "lvds1 pins not available\n");
		return;
	}
	awin_gpio_pinset_acquire(&awin_lvds0_pinset);
	if (dualchan) {
		awin_gpio_pinset_acquire(&awin_lvds1_pinset);
	}
	prop_dictionary_get_cstring_nocopy(cfg, "lcd_power_en",
	    &sc->sc_lcdpwr_pin_name);
	if (sc->sc_lcdpwr_pin_name != NULL) {
		if (!awin_gpio_pin_reserve(
		    sc->sc_lcdpwr_pin_name, &sc->sc_lcdpwr_pin)) {
			aprint_error_dev(sc->sc_dev,
			    "failed to reserve GPIO \"%s\" for LCD power\n",
			    sc->sc_lcdpwr_pin_name);
			sc->sc_lcdpwr_pin_name = NULL;
		} else {
			aprint_verbose_dev(sc->sc_dev,
			    ": using GPIO \"%s\" for LCD power\n", 
			    sc->sc_lcdpwr_pin_name);
		}
	}
	prop_dictionary_get_cstring_nocopy(cfg, "lcd_bl_en",
	    &sc->sc_lcdblk_pin_name);
	if (sc->sc_lcdblk_pin_name != NULL) {
		if (!awin_gpio_pin_reserve(
		    sc->sc_lcdblk_pin_name, &sc->sc_lcdblk_pin)) {
			aprint_error_dev(sc->sc_dev,
			    "failed to reserve GPIO \"%s\" for backlight\n",
			    sc->sc_lcdblk_pin_name);
			sc->sc_lcdblk_pin_name = NULL;
		} else {
			if (sc->sc_lcdpwr_pin_name == NULL) {
				aprint_verbose_dev(sc->sc_dev,
				    ": using GPIO \"%s\" for backlight\n", 
				    sc->sc_lcdblk_pin_name);
			} else {
				aprint_verbose(
				    ", GPIO \"%s\" for backlight\n", 
				    sc->sc_lcdblk_pin_name);
			}
		}
	}

	if (sc->sc_lcdpwr_pin_name != NULL) {
		awin_gpio_pindata_write(&sc->sc_lcdpwr_pin, 1);
	}

	vblk = (lcd_vt / 2) - lcd_y;
	start_delay = (vblk >= 32) ? 30 : (vblk - 2);

	if (lcd_dclk_freq > 150) /* hardware limit ? */
		lcd_dclk_freq = 150;
	awin_tcon_set_pll(sc, lcd_dclk_freq * 1000, 7);

	val = AWIN_TCONx_CTL_EN;
	val |= __SHIFTIN(start_delay, AWIN_TCONx_CTL_START_DELAY);
	/*
	 * the DE selector selects the primary DEBE for this tcon:
	 * 0 selects debe0 for tcon0 and debe1 for tcon1
	 */
	val |= __SHIFTIN(AWIN_TCONx_CTL_SRC_SEL_DE0,
			 AWIN_TCONx_CTL_SRC_SEL);
	TCON_WRITE(sc, AWIN_TCON0_CTL_REG, val);

	val =  (lcd_x - 1) << 16 |  (lcd_y - 1);
	TCON_WRITE(sc, AWIN_TCON0_BASIC0_REG, val);
	val = (lcd_ht - 1) << 16 | (lcd_hbp - 1);
	TCON_WRITE(sc, AWIN_TCON0_BASIC1_REG, val);
	val = (lcd_vt) << 16 | (lcd_vbp - 1);
	TCON_WRITE(sc, AWIN_TCON0_BASIC2_REG, val);
	val =  ((lcd_hspw > 0) ? (lcd_hspw - 1) : 0) << 16;
	val |= ((lcd_vspw > 0) ? (lcd_vspw - 1) : 0);
	TCON_WRITE(sc, AWIN_TCON0_BASIC3_REG, val);

	val = 0;
	if (dualchan)
		val |= AWIN_TCON0_LVDS_IF_DUALCHAN;
	if (prop_dictionary_get_bool(cfg, "lvds_mode_jeida", &propb) && propb)
		val |= AWIN_TCON0_LVDS_IF_MODE_JEIDA;
	if (prop_dictionary_get_bool(cfg, "lvds_18bits", &propb) && propb)
		val |= AWIN_TCON0_LVDS_IF_18BITS;
	TCON_WRITE(sc, AWIN_TCON0_LVDS_IF_REG, val);


	TCON_WRITE(sc, AWIN_TCON0_IO_POL_REG, lcd_io_cfg0);
	TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT1_REG,
	    __SHIFTIN(start_delay + 2, AWIN_TCON_GINT1_TCON0_LINENO));

	val = 0xf0000000;
	val &= ~AWIN_TCON0_DCLK_DIV;
	val |= __SHIFTIN(sc->sc_clk_div, AWIN_TCON0_DCLK_DIV);
	TCON_WRITE(sc, AWIN_TCON0_DCLK_REG, val);

	mode.dot_clock = lcd_dclk_freq;
	mode.hdisplay = lcd_x;
	mode.hsync_start = lcd_ht - lcd_hbp;
	mode.hsync_end = lcd_hspw + mode.hsync_start;
	mode.htotal = lcd_ht;
	mode.vdisplay = lcd_y;
	mode.vsync_start = lcd_vt - lcd_vbp;
	mode.vsync_end = lcd_vspw + mode.vsync_start;
	mode.vtotal = lcd_vt;
	mode.flags = 0;
	mode.name = NULL;

	awin_debe_set_videomode(sc->sc_debe_unit, &mode);

	/* and finally, enable it */
	awin_debe_enable(sc->sc_debe_unit, true);
	delay(20000);

	val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
	val |= AWIN_TCON_GCTL_EN;
	TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);
	delay(20000);


	val = TCON_READ(sc, AWIN_TCON0_LVDS_IF_REG);
	val |= AWIN_TCON0_LVDS_IF_EN;
	TCON_WRITE(sc, AWIN_TCON0_LVDS_IF_REG, val);

	/* XXX
	 * magic values here from linux. these are not documented
	 * in the A20 user manual, and other Allwiner LVDS-capable SoC
	 * documentation don't make sense with these values
	 */
	val = TCON_READ(sc, AWIN_TCON_LVDS_ANA0);
	val |= 0x3F310000;
	TCON_WRITE(sc, AWIN_TCON_LVDS_ANA0, val);
	val = TCON_READ(sc, AWIN_TCON_LVDS_ANA0);
	val |= 1 << 22;
	TCON_WRITE(sc, AWIN_TCON_LVDS_ANA0, val);
	delay(2);
	val = TCON_READ(sc, AWIN_TCON_LVDS_ANA1);
	val |= (0x1f << 26 | 0x1f << 10);
	TCON_WRITE(sc, AWIN_TCON_LVDS_ANA1, val);
	delay(2);
	val = TCON_READ(sc, AWIN_TCON_LVDS_ANA1);
	val |= (0x1f << 16 | 0x1f << 0);
	TCON_WRITE(sc, AWIN_TCON_LVDS_ANA1, val);
	val = TCON_READ(sc, AWIN_TCON_LVDS_ANA0);
	val |= 1 << 22;
	TCON_WRITE(sc, AWIN_TCON_LVDS_ANA0, val);

	if (sc->sc_lcdblk_pin_name != NULL) {
		awin_gpio_pindata_write(&sc->sc_lcdblk_pin, 1);
	}
}

static void 
awin_tcon0_enable(struct awin_tcon_softc *sc, bool enable) {
	uint32_t val;

	/* turn on/off backlight */
	if (sc->sc_lcdblk_pin_name != NULL) {
		awin_gpio_pindata_write(&sc->sc_lcdblk_pin, enable ? 1 : 0);
	}
	/* turn on/off LCD */
	if (sc->sc_lcdpwr_pin_name != NULL) {
		awin_gpio_pindata_write(&sc->sc_lcdpwr_pin, enable ? 1 : 0);
	}
	/* and finally disable of enable the tcon */
	KASSERT(sc->sc_output_type != OUTPUT_HDMI);

	awin_debe_enable(device_unit(sc->sc_dev), enable);
	delay(20000);
	if (enable) {
		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val |= AWIN_TCON_GCTL_EN;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);
		val = TCON_READ(sc, AWIN_TCON0_CTL_REG);
		val |= AWIN_TCONx_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON0_CTL_REG, val);
		val = TCON_READ(sc, AWIN_TCON0_LVDS_IF_REG);
		val |= AWIN_TCON0_LVDS_IF_EN;
		TCON_WRITE(sc, AWIN_TCON0_LVDS_IF_REG, val);
		TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0);
	} else {
		TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0xffffffff);
		val = TCON_READ(sc, AWIN_TCON0_LVDS_IF_REG);
		val &= ~AWIN_TCON0_LVDS_IF_EN;
		TCON_WRITE(sc, AWIN_TCON0_LVDS_IF_REG, val);
		val = TCON_READ(sc, AWIN_TCON0_CTL_REG);
		val &= ~AWIN_TCONx_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON0_CTL_REG, val);
		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val &= ~AWIN_TCON_GCTL_EN;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);
	}
}

void
awin_tcon1_enable(int unit, bool enable)
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
	KASSERT((sc->sc_output_type == OUTPUT_HDMI) || 
		    (sc->sc_output_type == OUTPUT_VGA));

	awin_debe_enable(device_unit(sc->sc_dev), enable);
	delay(20000);
	if (enable) {
		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val |= AWIN_TCON_GCTL_EN;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);
		val = TCON_READ(sc, AWIN_TCON1_CTL_REG);
		val |= AWIN_TCONx_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON1_CTL_REG, val);
		if (sc->sc_output_type == OUTPUT_VGA) {
			TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0x0cffffff);
		} else
			TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0);
	} else {
		TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0xffffffff);
		val = TCON_READ(sc, AWIN_TCON1_CTL_REG);
		val &= ~AWIN_TCONx_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON1_CTL_REG, val);
		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val &= ~AWIN_TCON_GCTL_EN;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);
	}

	KASSERT(tcon_mux_inited);
	val = bus_space_read_4(sc->sc_bst, tcon_mux_bsh, 0);
#ifdef AWIN_TCON_DEBUG
	printf("awin_tcon1_enable(%d) %d val 0x%x", unit, enable, val);
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
awin_tcon1_set_videomode(int unit, const struct videomode *mode)
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
	KASSERT((sc->sc_output_type == OUTPUT_HDMI) || 
		    (sc->sc_output_type == OUTPUT_VGA));

	awin_debe_set_videomode(device_unit(sc->sc_dev), mode);
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

		/* enable */
		val = AWIN_TCONx_CTL_EN;
		if (interlace_p)
			val |= AWIN_TCONx_CTL_INTERLACE_EN;
		val |= __SHIFTIN(start_delay, AWIN_TCONx_CTL_START_DELAY);
#ifdef AWIN_TCON1_BLUEDATA
		val |= __SHIFTIN(AWIN_TCONx_CTL_SRC_SEL_BLUEDATA,
				 AWIN_TCONx_CTL_SRC_SEL);
#else
		/*
		 * the DE selector selects the primary DEBE for this tcon:
		 * 0 selects debe0 for tcon0 and debe1 for tcon1
		 */
		val |= __SHIFTIN(AWIN_TCONx_CTL_SRC_SEL_DE0,
				 AWIN_TCONx_CTL_SRC_SEL);
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
		awin_tcon_set_pll(sc, mode->dot_clock, 1);
	} else {
		/* disable */
		val = TCON_READ(sc, AWIN_TCON1_CTL_REG);
		val &= ~AWIN_TCONx_CTL_EN;
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

void
awin_tcon_setvideo(int unit, bool enable)
{
	struct awin_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awintcon", unit);
	if (dev == NULL) {
		printf("TCON%d: no driver found\n", unit);
		return;
	}
	sc = device_private(dev);

	switch (sc->sc_output_type) {
		case OUTPUT_HDMI:
			awin_hdmi_poweron(enable);
			break;
		case OUTPUT_LVDS:
			awin_tcon0_enable(sc, enable);
			break;
		case OUTPUT_VGA:
			awin_tcon1_enable(unit, enable);
			break;
	}
}
