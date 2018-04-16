/* $NetBSD: sunxi_tcon.c,v 1.5.2.3 2018/04/16 01:59:53 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Manuel Bouyer <bouyer@antioche.eu.org>
 * All rights reserved.
 *
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_tcon.c,v 1.5.2.3 2018/04/16 01:59:53 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>
#include <dev/fdt/panel_fdt.h>

#include <dev/videomode/videomode.h>

#include <arm/sunxi/sunxi_tconreg.h>
#include <arm/sunxi/sunxi_display.h>

#define DIVIDE(x,y)     (((x) + ((y) / 2)) / (y))

enum sunxi_tcon_type {
	TCON_A10 = 1,
};

struct sunxi_tcon_softc {
	device_t sc_dev;
	enum sunxi_tcon_type sc_type;
	int sc_phandle;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct clk *sc_clk_ahb;
	struct clk *sc_clk_ch0;
	struct clk *sc_clk_ch1;
	unsigned int sc_output_type;
#define OUTPUT_HDMI 0
#define OUTPUT_LVDS 1
#define OUTPUT_VGA 2
	struct fdt_device_ports sc_ports;
	int sc_unit; /* tcon0 or tcon1 */
	struct fdt_endpoint *sc_in_ep;
	struct fdt_endpoint *sc_in_rep;
	struct fdt_endpoint *sc_out_ep;
};

static bus_space_handle_t tcon_mux_bsh;
static bool tcon_mux_inited = false;

static void sunxi_tcon_ep_connect(device_t, struct fdt_endpoint *, bool);
static int  sunxi_tcon_ep_activate(device_t, struct fdt_endpoint *, bool);
static int  sunxi_tcon_ep_enable(device_t, struct fdt_endpoint *, bool);
static int  sunxi_tcon0_set_video(struct sunxi_tcon_softc *);
static int  sunxi_tcon0_enable(struct sunxi_tcon_softc *, bool);
static int  sunxi_tcon1_enable(struct sunxi_tcon_softc *, bool);
void sunxi_tcon_dump_regs(int);

#define TCON_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TCON_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun4i-a10-tcon", TCON_A10},
	{"allwinner,sun7i-a20-tcon", TCON_A10},
	{NULL}
};

static int	sunxi_tcon_match(device_t, cfdata_t, void *);
static void	sunxi_tcon_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sunxi_tcon, sizeof(struct sunxi_tcon_softc),
	sunxi_tcon_match, sunxi_tcon_attach, NULL, NULL);

static int
sunxi_tcon_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_tcon_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_tcon_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	struct fdtbus_reset *rst, *lvds_rst;


	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "ahb");
	sc->sc_clk_ch0 = fdtbus_clock_get(phandle, "tcon-ch0");
	sc->sc_clk_ch1 = fdtbus_clock_get(phandle, "tcon-ch1");

	if (sc->sc_clk_ahb == NULL || sc->sc_clk_ch0 == NULL
	    || sc->sc_clk_ch1 == NULL) {
		aprint_error(": couldn't get clocks\n");
		aprint_debug_dev(self, "clk ahb %s tcon-ch0 %s tcon-ch1 %s\n",
		    sc->sc_clk_ahb == NULL ? "missing" : "present",
		    sc->sc_clk_ch0 == NULL ? "missing" : "present",
		    sc->sc_clk_ch1 == NULL ? "missing" : "present");
		return;
	}

	rst = fdtbus_reset_get(phandle, "lcd");
	if (rst == NULL) {
		aprint_error(": couldn't get lcd reset\n");
		return;
	}

	lvds_rst = fdtbus_reset_get(phandle, "lvds");

	if (clk_disable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't disable ahb clock\n");
		return;
	}
	if (clk_disable(sc->sc_clk_ch0) != 0) {
		aprint_error(": couldn't disable ch0 clock\n");
		return;
	}

	if (clk_disable(sc->sc_clk_ch1) != 0) {
		aprint_error(": couldn't disable ch1 clock\n");
		return;
	}

	if (fdtbus_reset_assert(rst) != 0) {
		aprint_error(": couldn't assert lcd reset\n");
		return;
	}
	if (lvds_rst != NULL) {
		if (fdtbus_reset_assert(lvds_rst) != 0) {
			aprint_error(": couldn't assert lvds reset\n");
			return;
		}
	}
	delay(1);
	if (fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert lcd reset\n");
		return;
	}
	if (lvds_rst != NULL) {
		if (fdtbus_reset_deassert(lvds_rst) != 0) {
			aprint_error(": couldn't de-assert lvds reset\n");
			return;
		}
	}

	if (clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't enable ahb clock\n");
		return;
	}

	sc->sc_type = of_search_compatible(faa->faa_phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": LCD/TV timing controller (%s)\n", 
	    fdtbus_get_string(phandle, "name"));

	sc->sc_unit = -1;
	sc->sc_ports.dp_ep_connect = sunxi_tcon_ep_connect;
	sc->sc_ports.dp_ep_activate = sunxi_tcon_ep_activate;
	sc->sc_ports.dp_ep_enable = sunxi_tcon_ep_enable;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_OTHER);

	TCON_WRITE(sc, SUNXI_TCON_GINT0_REG, 0);
	TCON_WRITE(sc, SUNXI_TCON_GINT1_REG,
	    __SHIFTIN(0x20, SUNXI_TCON_GINT1_TCON0_LINENO));
	TCON_WRITE(sc, SUNXI_TCON0_DCLK_REG, 0xf0000000);
	TCON_WRITE(sc, SUNXI_TCON0_LVDS_IF_REG, 0x0);
	TCON_WRITE(sc, SUNXI_TCON0_CTL_REG, 0);
	TCON_WRITE(sc, SUNXI_TCON0_IO_TRI_REG, 0xffffffff);
	TCON_WRITE(sc, SUNXI_TCON1_CTL_REG, 0);
	TCON_WRITE(sc, SUNXI_TCON1_IO_TRI_REG, 0xffffffff);
	TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, 0);

	/* clock needed for the mux in unit 0 */
	if (sc->sc_unit != 0) {
		if (clk_disable(sc->sc_clk_ahb) != 0) {
			aprint_error(": couldn't disable ahb clock\n");
			return;
		}
	}
}

static void
sunxi_tcon_ep_connect(device_t self, struct fdt_endpoint *ep, bool connect)
{
	struct sunxi_tcon_softc *sc = device_private(self);
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);
	int rep_idx = fdt_endpoint_index(rep);

	KASSERT(device_is_a(self, "sunxitcon"));
	if (!connect) {
		aprint_error_dev(self, "endpoint disconnect not supported\n");
		return;
	}

	if (fdt_endpoint_port_index(ep) == 0) {
		bool do_print = (sc->sc_unit == -1);
		/*
		 * one of our input endpoints has been connected.
		 * the remote id is our unit number
		 */
		if (sc->sc_unit != -1 && rep_idx != -1 &&
		    sc->sc_unit != rep_idx) {
			aprint_error_dev(self, ": remote id %d doens't match"
			    " discovered unit number %d\n",
			    rep_idx, sc->sc_unit);
			return;
		}
		if (!device_is_a(fdt_endpoint_device(rep), "sunxidebe")) {
			aprint_error_dev(self,
			    ": input %d connected to unknown device\n",
			    fdt_endpoint_index(ep));
			return;
		}

		if (rep_idx != -1)
			sc->sc_unit = rep_idx;
		else {
			/* assume only one tcon */
			sc->sc_unit = 0;
		}
		if (do_print)
			aprint_verbose_dev(self, "tcon unit %d\n", sc->sc_unit);
		if (!tcon_mux_inited && sc->sc_unit == 0) {
			/* the mux register is only in LCD0 */
			if (clk_enable(sc->sc_clk_ahb) != 0) {
				aprint_error_dev(self,
				    "couldn't enable ahb clock\n");
				return;
			}
			bus_space_subregion(sc->sc_bst, sc->sc_bsh,
			    SUNXI_TCON_MUX_CTL_REG, 4, &tcon_mux_bsh);
			tcon_mux_inited = true;
			bus_space_write_4(sc->sc_bst, tcon_mux_bsh, 0,
			    __SHIFTIN(SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC_CLOSE,
			    SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC));
		}
	} else if (fdt_endpoint_port_index(ep) == 1) {
		device_t rep_dev = fdt_endpoint_device(rep);
		switch(fdt_endpoint_index(ep)) {
		case 0:
			break;
		case 1:
			if (!device_is_a(rep_dev, "sunxihdmi")) {
				aprint_error_dev(self,
				    ": output 1 connected to unknown device\n");
				return;
			}
			break;
		default:
			break;
		}
	}
}

static int
sunxi_tcon_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_tcon_softc *sc = device_private(dev);
	struct fdt_endpoint *in_ep, *out_ep;
	int outi;
	int error = ENODEV;

	KASSERT(device_is_a(dev, "sunxitcon"));
	/* our input is activated by debe, we activate our output */
	if (fdt_endpoint_port_index(ep) != SUNXI_PORT_INPUT) {
		panic("sunxi_tcon_ep_activate: port %d",
		    fdt_endpoint_port_index(ep));
	}

	if (!activate)
		return EOPNOTSUPP;

	if (clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error_dev(dev, "couldn't enable ahb clock\n");
		return EIO;
	}
	sc->sc_in_ep = ep;
	sc->sc_in_rep = fdt_endpoint_remote(ep);
	/* check that our other input is not active */
	switch (fdt_endpoint_index(ep)) {
	case 0:
		in_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    SUNXI_PORT_INPUT, 1);
		break;
	case 1:
		in_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    SUNXI_PORT_INPUT, 0);
		break;
	default:
		in_ep = NULL;
		panic("sunxi_tcon_ep_activate: input index %d",
		    fdt_endpoint_index(ep));
	}
	if (in_ep != NULL) {
		if (fdt_endpoint_is_active(in_ep))
			return EBUSY;
	}
	/* try output 0 (RGB/LVDS) first, then ouput 1 (HDMI) if it fails */
	for (outi = 0; outi < 2; outi++) {
		out_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    SUNXI_PORT_OUTPUT, outi);
		if (out_ep == NULL)
			continue;
		error = fdt_endpoint_activate(out_ep, activate);
		if (error == 0) {
			struct fdt_endpoint *rep = fdt_endpoint_remote(out_ep);
			aprint_verbose_dev(dev, "output to %s\n",
			    device_xname(fdt_endpoint_device(rep)));
			sc->sc_out_ep = out_ep;
			if (outi == 0)
				return sunxi_tcon0_set_video(sc);
			/* XXX should check VGA here */
			sc->sc_output_type = OUTPUT_HDMI;
			return 0;
		}
	}
	if (out_ep == NULL) {
		aprint_error_dev(dev, "no output endpoint\n");
		return ENODEV;
	}
	return error;
}

static int
sunxi_tcon_ep_enable(device_t dev, struct fdt_endpoint *ep, bool enable)
{
	struct sunxi_tcon_softc *sc = device_private(dev);
	int error;
	KASSERT(device_is_a(dev, "sunxitcon"));
	switch (fdt_endpoint_port_index(ep)) {
	case SUNXI_PORT_INPUT:
		KASSERT(ep == sc->sc_in_ep);
		if (fdt_endpoint_index(sc->sc_out_ep) == 0) {
			/* tcon0 active */
			return sunxi_tcon0_enable(sc, enable);
		}
		/* propagate to our output, it will get back to us */
		return fdt_endpoint_enable(sc->sc_out_ep, enable);
	case SUNXI_PORT_OUTPUT:
		KASSERT(ep == sc->sc_out_ep);
		switch (fdt_endpoint_index(ep)) {
		case 0:
			panic("sunxi_tcon0_ep_enable");
		case 1:
			error = sunxi_tcon1_enable(sc, enable);
			break;
		default:
			panic("sunxi_tcon_ep_enable ep %d",
			    fdt_endpoint_index(ep));
			
		}
		break;
	default:
		panic("sunxi_tcon_ep_enable port %d", fdt_endpoint_port_index(ep));
	}
#if defined(SUNXI_TCON_DEBUG)
	sunxi_tcon_dump_regs(device_unit(dev));
#endif
	return error;
}

static int
sunxi_tcon0_set_video(struct sunxi_tcon_softc *sc)
{
	const struct fdt_panel * panel;
	int32_t lcd_x, lcd_y;
	int32_t lcd_hbp, lcd_ht, lcd_vbp, lcd_vt;
	int32_t lcd_hspw, lcd_vspw, lcd_io_cfg0;
	uint32_t vblk, start_delay;
	uint32_t val;
	uint32_t best_div;
	int best_diff, best_clk_freq, clk_freq, lcd_dclk_freq;
	bool dualchan = false;
	static struct videomode mode;
	int error;

	panel = fdt_endpoint_get_data(fdt_endpoint_remote(sc->sc_out_ep));
	KASSERT(panel != NULL);
	KASSERT(panel->panel_type == PANEL_DUAL_LVDS ||
	    panel->panel_type == PANEL_LVDS);
	sc->sc_output_type = OUTPUT_LVDS;

	lcd_x = panel->panel_timing.hactive;
	lcd_y = panel->panel_timing.vactive;

	lcd_dclk_freq = panel->panel_timing.clock_freq;

	lcd_hbp = panel->panel_timing.hback_porch;
	lcd_hspw = panel->panel_timing.hsync_len;
	lcd_ht = panel->panel_timing.hfront_porch + lcd_hspw + lcd_x + lcd_hbp;

	lcd_vbp = panel->panel_timing.vback_porch;
	lcd_vspw = panel->panel_timing.vsync_len;
	lcd_vt = panel->panel_timing.vfront_porch + lcd_vspw + lcd_y + lcd_vbp;
	
	lcd_io_cfg0 = 0x10000000; /* XXX */

	if (panel->panel_type == PANEL_DUAL_LVDS)
		dualchan = true;

	vblk = lcd_vt - lcd_y;
	start_delay = (vblk >= 32) ? 30 : (vblk - 2);

	if (lcd_dclk_freq > 150000000) /* hardware limit ? */
		lcd_dclk_freq = 150000000;

	best_diff = INT_MAX;
	best_div = 0;
	best_clk_freq = 0;
	for (u_int div = 7; div <= 15; div++) {
		int dot_freq, diff;
		clk_freq = clk_round_rate(sc->sc_clk_ch0, lcd_dclk_freq * div);
		if (clk_freq == 0)
			continue;
		dot_freq = clk_freq / div;
		diff = abs(lcd_dclk_freq - dot_freq);
		if (best_diff > diff) {
			best_diff = diff;
			best_div = div;
			best_clk_freq = clk_freq;
			if (diff == 0)
				break;
		}
	}
	if (best_clk_freq == 0) {
		device_printf(sc->sc_dev,
		    ": failed to find params for dot clock %d\n",
		    lcd_dclk_freq);
		return EINVAL;
	}

	error = clk_set_rate(sc->sc_clk_ch0, best_clk_freq);
	if (error) {
		device_printf(sc->sc_dev,
		    ": failed to set ch0 clock to %d for %d: %d\n",
		    best_clk_freq, lcd_dclk_freq, error);
		panic("tcon0 set clk");
	}
	error = clk_enable(sc->sc_clk_ch0);
	if (error) {
		device_printf(sc->sc_dev,
		    ": failed to enable ch0 clock: %d\n", error);
		return EIO;
	}

	val = __SHIFTIN(start_delay, SUNXI_TCONx_CTL_START_DELAY);
	/*
	 * the DE selector selects the primary DEBE for this tcon:
	 * 0 selects debe0 for tcon0 and debe1 for tcon1
	 */
	val |= __SHIFTIN(SUNXI_TCONx_CTL_SRC_SEL_DE0,
			 SUNXI_TCONx_CTL_SRC_SEL);
	TCON_WRITE(sc, SUNXI_TCON0_CTL_REG, val);

	val =  (lcd_x - 1) << 16 |  (lcd_y - 1);
	TCON_WRITE(sc, SUNXI_TCON0_BASIC0_REG, val);
	val = (lcd_ht - 1) << 16 | (lcd_hbp - 1);
	TCON_WRITE(sc, SUNXI_TCON0_BASIC1_REG, val);
	val = (lcd_vt * 2) << 16 | (lcd_vbp - 1);
	TCON_WRITE(sc, SUNXI_TCON0_BASIC2_REG, val);
	val =  ((lcd_hspw > 0) ? (lcd_hspw - 1) : 0) << 16;
	val |= ((lcd_vspw > 0) ? (lcd_vspw - 1) : 0);
	TCON_WRITE(sc, SUNXI_TCON0_BASIC3_REG, val);

	val = 0;
	if (dualchan)
		val |= SUNXI_TCON0_LVDS_IF_DUALCHAN;
	if (panel->panel_lvds_format == LVDS_JEIDA_24)
		val |= SUNXI_TCON0_LVDS_IF_MODE_JEIDA;
	if (panel->panel_lvds_format == LVDS_JEIDA_18)
		val |= SUNXI_TCON0_LVDS_IF_18BITS;
	TCON_WRITE(sc, SUNXI_TCON0_LVDS_IF_REG, val);

	TCON_WRITE(sc, SUNXI_TCON0_IO_POL_REG, lcd_io_cfg0);
	TCON_WRITE(sc, SUNXI_TCON0_IO_TRI_REG, 0);
	TCON_WRITE(sc, SUNXI_TCON_GINT1_REG,
	    __SHIFTIN(start_delay + 2, SUNXI_TCON_GINT1_TCON0_LINENO));

	val = 0xf0000000;
	val &= ~SUNXI_TCON0_DCLK_DIV;
	val |= __SHIFTIN(best_div, SUNXI_TCON0_DCLK_DIV);
	TCON_WRITE(sc, SUNXI_TCON0_DCLK_REG, val);

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

	sunxi_debe_set_videomode(fdt_endpoint_device(sc->sc_in_rep), &mode);

	/* XXX
	 * magic values here from linux. these are not documented
	 * in the A20 user manual, and other Allwiner LVDS-capable SoC
	 * documentation don't make sense with these values
	 */
	val = TCON_READ(sc, SUNXI_TCON_LVDS_ANA0);
	val |= 0x3F310000;
	TCON_WRITE(sc, SUNXI_TCON_LVDS_ANA0, val);
	val = TCON_READ(sc, SUNXI_TCON_LVDS_ANA0);
	val |= 1 << 22;
	TCON_WRITE(sc, SUNXI_TCON_LVDS_ANA0, val);
	delay(2);
	val = TCON_READ(sc, SUNXI_TCON_LVDS_ANA1);
	val |= (0x1f << 26 | 0x1f << 10);
	TCON_WRITE(sc, SUNXI_TCON_LVDS_ANA1, val);
	delay(2);
	val = TCON_READ(sc, SUNXI_TCON_LVDS_ANA1);
	val |= (0x1f << 16 | 0x1f << 0);
	TCON_WRITE(sc, SUNXI_TCON_LVDS_ANA1, val);
	val = TCON_READ(sc, SUNXI_TCON_LVDS_ANA0);
	val |= 1 << 22;
	TCON_WRITE(sc, SUNXI_TCON_LVDS_ANA0, val);
	return 0;
}

static int
sunxi_tcon0_enable(struct sunxi_tcon_softc *sc, bool enable)
{
	uint32_t val;
	int error;

	/* turn on/off backlight and lcd  */
	error = fdt_endpoint_enable(sc->sc_out_ep, enable);
	if (error)
		return error;

	/* and finally disable or enable the tcon */
	error = fdt_endpoint_enable(sc->sc_in_ep, enable);
	if (error)
		return error;
	delay(20000);
	if (enable) {
		if ((error = clk_enable(sc->sc_clk_ch0)) != 0) {
			device_printf(sc->sc_dev,
			    ": couldn't enable ch0 clock\n");
			return error;
		}
		val = TCON_READ(sc, SUNXI_TCON_GCTL_REG);
		val |= SUNXI_TCON_GCTL_EN;
		TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, val);
		val = TCON_READ(sc, SUNXI_TCON0_CTL_REG);
		val |= SUNXI_TCONx_CTL_EN;
		TCON_WRITE(sc, SUNXI_TCON0_CTL_REG, val);
		val = TCON_READ(sc, SUNXI_TCON0_LVDS_IF_REG);
		val |= SUNXI_TCON0_LVDS_IF_EN;
		TCON_WRITE(sc, SUNXI_TCON0_LVDS_IF_REG, val);
		TCON_WRITE(sc, SUNXI_TCON0_IO_TRI_REG, 0);
	} else {
		TCON_WRITE(sc, SUNXI_TCON0_IO_TRI_REG, 0xffffffff);
		val = TCON_READ(sc, SUNXI_TCON0_LVDS_IF_REG);
		val &= ~SUNXI_TCON0_LVDS_IF_EN;
		TCON_WRITE(sc, SUNXI_TCON0_LVDS_IF_REG, val);
		val = TCON_READ(sc, SUNXI_TCON0_CTL_REG);
		val &= ~SUNXI_TCONx_CTL_EN;
		TCON_WRITE(sc, SUNXI_TCON0_CTL_REG, val);
		val = TCON_READ(sc, SUNXI_TCON_GCTL_REG);
		val &= ~SUNXI_TCON_GCTL_EN;
		TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, val);
		if ((error = clk_disable(sc->sc_clk_ch0)) != 0) {
			device_printf(sc->sc_dev,
			    ": couldn't disable ch0 clock\n");
			return error;
		}
	}
#ifdef SUNXI_TCON_DEBUG
	sunxi_tcon_dump_regs(device_unit(sc->sc_dev));
#endif
	return 0;
}

static int
sunxi_tcon1_enable(struct sunxi_tcon_softc *sc, bool enable)
{
	uint32_t val;
	int error;

	KASSERT((sc->sc_output_type == OUTPUT_HDMI) || 
		    (sc->sc_output_type == OUTPUT_VGA));

	fdt_endpoint_enable(sc->sc_in_ep, enable);
	delay(20000);
	if (enable) {
		if ((error = clk_enable(sc->sc_clk_ch1)) != 0) {
			device_printf(sc->sc_dev,
			    ": couldn't enable ch1 clock\n");
			return error;
		}
		val = TCON_READ(sc, SUNXI_TCON_GCTL_REG);
		val |= SUNXI_TCON_GCTL_EN;
		TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, val);
		val = TCON_READ(sc, SUNXI_TCON1_CTL_REG);
		val |= SUNXI_TCONx_CTL_EN;
		TCON_WRITE(sc, SUNXI_TCON1_CTL_REG, val);
		if (sc->sc_output_type == OUTPUT_VGA) {
			TCON_WRITE(sc, SUNXI_TCON1_IO_TRI_REG, 0x0cffffff);
		} else
			TCON_WRITE(sc, SUNXI_TCON1_IO_TRI_REG, 0);
	} else {
		TCON_WRITE(sc, SUNXI_TCON1_IO_TRI_REG, 0xffffffff);
		val = TCON_READ(sc, SUNXI_TCON1_CTL_REG);
		val &= ~SUNXI_TCONx_CTL_EN;
		TCON_WRITE(sc, SUNXI_TCON1_CTL_REG, val);
		val = TCON_READ(sc, SUNXI_TCON_GCTL_REG);
		val &= ~SUNXI_TCON_GCTL_EN;
		TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, val);
		if ((error = clk_disable(sc->sc_clk_ch1)) != 0) {
			device_printf(sc->sc_dev,
			    ": couldn't disable ch1 clock\n");
			return error;
		}
	}

	KASSERT(tcon_mux_inited);
	val = bus_space_read_4(sc->sc_bst, tcon_mux_bsh, 0);
#ifdef SUNXI_TCON_DEBUG
	printf("sunxi_tcon1_enable(%d) %d val 0x%x", sc->sc_unit, enable, val);
#endif
	val &= ~ SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC;
	switch(sc->sc_unit) {
	case 0:
		val |= __SHIFTIN(SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC_LCDC0_TCON1,
		    SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC);
		break;
	case 1:
		val |= __SHIFTIN(SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC_LCDC1_TCON1,
		    SUNXI_TCON_MUX_CTL_HDMI_OUTPUT_SRC);
		break;
	default:
		panic("tcon: invalid unid %d\n", sc->sc_unit);
	}
#ifdef SUNXI_TCON_DEBUG
	printf(" -> 0x%x", val);
#endif
	bus_space_write_4(sc->sc_bst, tcon_mux_bsh, 0, val);
#ifdef SUNXI_TCON_DEBUG
	printf(": 0x%" PRIxBSH " 0x%" PRIxBSH " 0x%x 0x%x\n", sc->sc_bsh,
	    tcon_mux_bsh, bus_space_read_4(sc->sc_bst, tcon_mux_bsh, 0),
	    TCON_READ(sc, SUNXI_TCON_MUX_CTL_REG));
#endif
	return 0;
}

void
sunxi_tcon1_set_videomode(device_t dev, const struct videomode *mode)
{
	struct sunxi_tcon_softc *sc = device_private(dev);
	uint32_t val;
	int error;

	KASSERT(device_is_a(dev, "sunxitcon"));
	KASSERT((sc->sc_output_type == OUTPUT_HDMI) || 
		    (sc->sc_output_type == OUTPUT_VGA));

	sunxi_debe_set_videomode(fdt_endpoint_device(sc->sc_in_rep), mode);
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

		val = TCON_READ(sc, SUNXI_TCON_GCTL_REG);
		val |= SUNXI_TCON_GCTL_IO_MAP_SEL;
		TCON_WRITE(sc, SUNXI_TCON_GCTL_REG, val);

		/* enable */
		val = SUNXI_TCONx_CTL_EN;
		if (interlace_p)
			val |= SUNXI_TCONx_CTL_INTERLACE_EN;
		val |= __SHIFTIN(start_delay, SUNXI_TCONx_CTL_START_DELAY);
#ifdef SUNXI_TCON1_BLUEDATA
		val |= __SHIFTIN(SUNXI_TCONx_CTL_SRC_SEL_BLUEDATA,
				 SUNXI_TCONx_CTL_SRC_SEL);
#else
		/*
		 * the DE selector selects the primary DEBE for this tcon:
		 * 0 selects debe0 for tcon0 and debe1 for tcon1
		 */
		val |= __SHIFTIN(SUNXI_TCONx_CTL_SRC_SEL_DE0,
				 SUNXI_TCONx_CTL_SRC_SEL);
#endif
		TCON_WRITE(sc, SUNXI_TCON1_CTL_REG, val);

		/* Source width/height */
		TCON_WRITE(sc, SUNXI_TCON1_BASIC0_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Scaler width/height */
		TCON_WRITE(sc, SUNXI_TCON1_BASIC1_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Output width/height */
		TCON_WRITE(sc, SUNXI_TCON1_BASIC2_REG,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
		/* Horizontal total + back porch */
		TCON_WRITE(sc, SUNXI_TCON1_BASIC3_REG,
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
		TCON_WRITE(sc, SUNXI_TCON1_BASIC4_REG,
		    (vtotal << 16) | (vbp - 1));

		/* Sync */
		TCON_WRITE(sc, SUNXI_TCON1_BASIC5_REG,
		    ((hspw - 1) << 16) | (vspw - 1));
		/* Polarity */
		val = SUNXI_TCON_IO_POL_IO2_INV;
		if (phsync_p)
			val |= SUNXI_TCON_IO_POL_PHSYNC;
		if (pvsync_p)
			val |= SUNXI_TCON_IO_POL_PVSYNC;
		TCON_WRITE(sc, SUNXI_TCON1_IO_POL_REG, val);

		TCON_WRITE(sc, SUNXI_TCON_GINT1_REG,
		    __SHIFTIN(start_delay + 2, SUNXI_TCON_GINT1_TCON1_LINENO));

		/* Setup LCDx CH1 PLL */
		error = clk_set_rate(sc->sc_clk_ch1, mode->dot_clock * 1000);
		if (error) {
			device_printf(sc->sc_dev,
			    ": failed to set ch1 clock to %d: %d\n",
			    mode->dot_clock, error);
		}
		error = clk_enable(sc->sc_clk_ch1);
		if (error) {
			device_printf(sc->sc_dev,
			    ": failed to enable ch1 clock: %d\n",
			    error);
		}
	} else {
		/* disable */
		val = TCON_READ(sc, SUNXI_TCON1_CTL_REG);
		val &= ~SUNXI_TCONx_CTL_EN;
		TCON_WRITE(sc, SUNXI_TCON1_CTL_REG, val);
		error = clk_disable(sc->sc_clk_ch1);
		if (error) {
			device_printf(sc->sc_dev,
			    ": failed to disable ch1 clock: %d\n",
			    error);
		}
	}
}

/* check if this tcon is the console chosen by firmare */
bool
sunxi_tcon_is_console(device_t dev, const char *pipeline)
{
	struct sunxi_tcon_softc *sc = device_private(dev);
	char p[64];
	char *e, *n = p;
	bool is_console = false;

	KASSERT(device_is_a(dev, "sunxitcon"));
	strncpy(p, pipeline, sizeof(p) - 1);
	p[sizeof(p) - 1] = '\0';

	/*
	 * pipeline is like "de_be0-lcd0-hdmi"
	 * of "de_be0-lcd1".
	 * In the first case check output type
	 * In the second check tcon unit number
	 */
	 n = p;
	 e = strsep(&n, "-");
	 if (e == NULL || memcmp(e, "de_be", 5) != 0)
		goto bad;
	 e = strsep(&n, "-");
	 if (e == NULL)
		goto bad;
	 if (n == NULL) {
		/* second case */
		if (strcmp(e, "lcd0") == 0) {
			if (sc->sc_unit == 0)
				is_console = true;
		 } else if (strcmp(e, "lcd1") == 0) {
			if (sc->sc_unit == 1)
				is_console = true;
		} else
			goto bad;
		return is_console;
	}
	/* first case */
	if (strcmp(n, "hdmi") == 0) {
		if (sc->sc_output_type == OUTPUT_HDMI)
			is_console = true;
		return is_console;
	}
bad:
	aprint_error("warning: can't parse pipeline %s\n", pipeline);
	return is_console;
}

#if defined(DDB) || defined(SUNXI_TCON_DEBUG)
void
sunxi_tcon_dump_regs(int u)
{
	static const struct {
		const char *name;
		uint16_t reg;
	} regs[] = {
		{ "TCON0_BASIC0_REG", SUNXI_TCON0_BASIC0_REG },
		{ "TCON0_BASIC1_REG", SUNXI_TCON0_BASIC1_REG },
		{ "TCON0_BASIC2_REG", SUNXI_TCON0_BASIC2_REG },
		{ "TCON0_BASIC3_REG", SUNXI_TCON0_BASIC3_REG },
		{ "TCON0_CTL_REG", SUNXI_TCON0_CTL_REG },
		{ "TCON0_DCLK_REG", SUNXI_TCON0_DCLK_REG },
		{ "TCON0_IO_POL_REG", SUNXI_TCON0_IO_POL_REG },
		{ "TCON0_IO_TRI_REG", SUNXI_TCON0_IO_TRI_REG },
		{ "TCON0_LVDS_IF_REG", SUNXI_TCON0_LVDS_IF_REG },
		{ "TCON1_BASIC0_REG", SUNXI_TCON1_BASIC0_REG },
		{ "TCON1_BASIC1_REG", SUNXI_TCON1_BASIC1_REG },
		{ "TCON1_BASIC2_REG", SUNXI_TCON1_BASIC2_REG },
		{ "TCON1_BASIC3_REG", SUNXI_TCON1_BASIC3_REG },
		{ "TCON1_BASIC4_REG", SUNXI_TCON1_BASIC4_REG },
		{ "TCON1_BASIC5_REG", SUNXI_TCON1_BASIC5_REG },
		{ "TCON1_CTL_REG", SUNXI_TCON1_CTL_REG },
		{ "TCON1_IO_POL_REG", SUNXI_TCON1_IO_POL_REG },
		{ "TCON1_IO_TRI_REG", SUNXI_TCON1_IO_TRI_REG },
		{ "TCON_GCTL_REG", SUNXI_TCON_GCTL_REG },
		{ "TCON_GINT0_REG", SUNXI_TCON_GINT0_REG },
		{ "TCON_GINT1_REG", SUNXI_TCON_GINT1_REG },
		{ "TCON_MUX_CTL_REG", SUNXI_TCON_MUX_CTL_REG },
	};
	struct sunxi_tcon_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("sunxitcon", u);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (int i = 0; i < __arraycount(regs); i++) {
		printf("%s: 0x%08x\n", regs[i].name,
		    TCON_READ(sc, regs[i].reg));
	}
}
#endif
