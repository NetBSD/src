/* $NetBSD: awin_tcon.c,v 1.2 2014/11/09 14:30:55 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_tcon.c,v 1.2 2014/11/09 14:30:55 jmcneill Exp $");

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

struct awin_tcon_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ccm_bsh;
	unsigned int sc_port;
	unsigned int sc_clk_div;
};

#define TCON_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TCON_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_tcon_match(device_t, cfdata_t, void *);
static void	awin_tcon_attach(device_t, device_t, void *);

static void	awin_tcon_set_pll(struct awin_tcon_softc *,
				  const struct videomode *);

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

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_port = loc->loc_port;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh, 0, 0x1000,
	    &sc->sc_ccm_bsh);

	aprint_naive("\n");
	aprint_normal(": LCD/TV timing controller\n");

	awin_pll3_enable();

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET1_REG,
		    AWIN_A31_AHB_RESET1_LCD0_RST << loc->loc_port,
		    0);
	}

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_LCD0 << loc->loc_port, 0);

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_LCD0_CH0_CLK_REG + (loc->loc_port * 4),
	    __SHIFTIN(AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3,
		      AWIN_LCDx_CHx_CLK_SRC_SEL) |
	    AWIN_CLK_OUT_ENABLE,
	    AWIN_LCDx_CHx_CLK_SRC_SEL);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_LCD0_CH1_CLK_REG + (loc->loc_port * 4),
	    __SHIFTIN(AWIN_LCDx_CHx_CLK_SRC_SEL_PLL3,
		      AWIN_LCDx_CHx_CLK_SRC_SEL) |
	    AWIN_CLK_OUT_ENABLE,
	    AWIN_LCDx_CHx_CLK_SRC_SEL | AWIN_LCDx_CH1_SCLK1_SRC_SEL);

	TCON_WRITE(sc, AWIN_TCON_GCTL_REG,
	    __SHIFTIN(AWIN_TCON_GCTL_IO_MAP_SEL_TCON1,
		      AWIN_TCON_GCTL_IO_MAP_SEL) |
	    AWIN_TCON_GCTL_EN);
	TCON_WRITE(sc, AWIN_TCON_GINT0_REG, 0);
	TCON_WRITE(sc, AWIN_TCON_GINT1_REG,
	    __SHIFTIN(0x20, AWIN_TCON_GINT1_TCON1_LINENO));
	TCON_WRITE(sc, AWIN_TCON0_DCLK_REG, 0xf0000000);
	TCON_WRITE(sc, AWIN_TCON0_IO_TRI_REG, 0xffffffff);
	TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, 0xffffffff);
}

static void
awin_tcon_set_pll(struct awin_tcon_softc *sc, const struct videomode *mode)
{
	unsigned int n, m, freq;
	unsigned int m1 = ~0, n1 = ~0;

	for (m = 1; m <= 15; m++) {
		n = (m * mode->dot_clock) / 3000;
		freq = (n * 3000) / m;

		if (freq == mode->dot_clock && m < m1) {
			m1 = m;
			n1 = n;
		}
	}

	if (m1 == ~0) {
		device_printf(sc->sc_dev, "couldn't set rate %u Hz\n",
		    mode->dot_clock * 1000);
		sc->sc_clk_div = 0;
		return;
	}

	awin_pll3_set_rate(n1 * 3000000);

	awin_reg_set_clear(sc->sc_bst, sc->sc_ccm_bsh,
	    AWIN_LCD0_CH1_CLK_REG + (sc->sc_port * 4),
	    AWIN_CLK_OUT_ENABLE |
	    AWIN_LCDx_CH1_SCLK1_GATING |
	    __SHIFTIN(m1 - 1, AWIN_LCDx_CH1_CLK_DIV_RATIO_M),
	    AWIN_LCDx_CH1_CLK_DIV_RATIO_M);

	sc->sc_clk_div = m1;
}

void
awin_tcon_set_videomode(const struct videomode *mode)
{
	struct awin_tcon_softc *sc;
	device_t dev;
	uint32_t val;

	dev = device_find_by_driver_unit("awintcon", 0);
	if (dev == NULL) {
		printf("TCON: no driver found\n");
		return;
	}
	sc = device_private(dev);

	val = TCON_READ(sc, AWIN_TCON1_CTL_REG);
	if (mode) {
		const u_int hbp = mode->htotal - mode->hsync_start;
		const u_int hspw = mode->hsync_end - mode->hsync_start;
		const u_int vbp = mode->vtotal - mode->vsync_start;
		const u_int vspw = mode->vsync_end - mode->vsync_start;

		/* enable */
		val |= AWIN_TCON_CTL_EN;
		val &= ~AWIN_TCON_CTL_START_DELAY;
		val |= __SHIFTIN(0x1e, AWIN_TCON_CTL_START_DELAY);
		val |= __SHIFTIN(AWIN_TCON_CTL_SRC_SEL_DE0,
				 AWIN_TCON_CTL_SRC_SEL);
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
		TCON_WRITE(sc, AWIN_TCON1_BASIC4_REG,
		    ((mode->vtotal * 2) << 16) | (vbp - 1));
		/* Sync */
		TCON_WRITE(sc, AWIN_TCON1_BASIC5_REG,
		    ((hspw - 1) << 16) | (vspw - 1));

		/* Setup LCDx CH1 PLL */
		awin_tcon_set_pll(sc, mode);

		val = TCON_READ(sc, AWIN_TCON_GCTL_REG);
		val |= AWIN_TCON_GCTL_EN;
		TCON_WRITE(sc, AWIN_TCON_GCTL_REG, val);

		val = TCON_READ(sc, AWIN_TCON1_IO_TRI_REG);
		val &= ~0x03000000;
		TCON_WRITE(sc, AWIN_TCON1_IO_TRI_REG, val);
	} else {
		/* disable */
		val &= ~AWIN_TCON_CTL_EN;
		TCON_WRITE(sc, AWIN_TCON1_CTL_REG, val);
	}
}

unsigned int
awin_tcon_get_clk_div(void)
{
	struct awin_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awintcon", 0);
	if (dev == NULL) {
		printf("TCON: no driver found\n");
		return 0;
	}
	sc = device_private(dev);

	return sc->sc_clk_div;
}
