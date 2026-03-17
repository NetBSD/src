/*	$NetBSD: mgafb.c,v 1.2 2026/03/17 12:51:37 macallan Exp $	*/

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the Matrox Millennium (MGA-2064W).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mgafb.c,v 1.2 2026/03/17 12:51:37 macallan Exp $");

#include "opt_mgafb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <dev/pci/mgafbreg.h>
#include <dev/pci/mgafbvar.h>

#include <dev/pci/wsdisplay_pci.h>

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>

#include "opt_wsemul.h"

/* #define MGAFB_ACCEL can be the default - it works */

static inline void
MGA_WRITE4(struct mgafb_softc *sc, bus_size_t reg, uint32_t v)
{
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, v);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, reg, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
MGA_WRITE1(struct mgafb_softc *sc, bus_size_t reg, uint8_t v)
{
	bus_space_write_1(sc->sc_regt, sc->sc_regh, reg, v);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, reg, 1,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline uint32_t
MGA_READ4(struct mgafb_softc *sc, bus_size_t reg)
{
	bus_space_barrier(sc->sc_regt, sc->sc_regh, reg, 4,
	    BUS_SPACE_BARRIER_READ);
	return bus_space_read_4(sc->sc_regt, sc->sc_regh, reg);
}

static inline uint8_t
MGA_READ1(struct mgafb_softc *sc, bus_size_t reg)
{
	bus_space_barrier(sc->sc_regt, sc->sc_regh, reg, 1,
	    BUS_SPACE_BARRIER_READ);
	return bus_space_read_1(sc->sc_regt, sc->sc_regh, reg);
}


static int	mgafb_match(device_t, cfdata_t, void *);
static void	mgafb_attach(device_t, device_t, void *);

#ifdef MGAFB_PINS
static void	mgafb_read_pins(struct mgafb_softc *, const struct pci_attach_args *);
static void	mgafb_dump_pins(struct mgafb_softc *);
#endif /* MGAFB_PINS */

#ifndef MGAFB_NO_HW_INIT
static void	mgafb_preinit_wram(struct mgafb_softc *);
static void	mgafb_set_mclk(struct mgafb_softc *, int);
static void	mgafb_preinit_1064sg(struct mgafb_softc *);
#endif

static void	mgafb_detect_vram(struct mgafb_softc *);

static void	mgafb_calc_pll(int, uint8_t *, uint8_t *, uint8_t *);
static void	mgafb_calc_pll_1064sg(int, uint8_t *, uint8_t *, uint8_t *);
static void	mgafb_calc_crtc(const struct videomode *, int, bool,
		    uint8_t [25], uint8_t [6], uint8_t *, uint8_t *);

static bool	mgafb_mode_fits(struct mgafb_softc *, const struct videomode *);
static const struct videomode *mgafb_pick_mode(struct mgafb_softc *);

static void	mgafb_resolve_bars(struct mgafb_softc *,
		    const struct pci_attach_args *, int *, int *);
static uint8_t	mgafb_crtcext3_scale(struct mgafb_softc *);

static void	mgafb_set_mode(struct mgafb_softc *);
static void	mgafb_tvp3026_setup_dac(struct mgafb_softc *, bool);
static void	mgafb_tvp3026_set_pclk(struct mgafb_softc *, int, bool);
static void	mgafb_idac_setup_dac(struct mgafb_softc *);
static void	mgafb_idac_set_pclk(struct mgafb_softc *, int);

static void	mgafb_dac_write(struct mgafb_softc *, uint8_t, uint8_t);
static uint8_t	mgafb_dac_read(struct mgafb_softc *, uint8_t);
static void	mgafb_dac_write_ind(struct mgafb_softc *, uint8_t, uint8_t);
static uint8_t	mgafb_dac_read_ind(struct mgafb_softc *, uint8_t);

static void	mgafb_ddc_read(struct mgafb_softc *);
static void	mgafb_i2cbb_set_bits(void *, uint32_t);
static void	mgafb_i2cbb_set_dir(void *, uint32_t);
static uint32_t	mgafb_i2cbb_read_bits(void *);
static int	mgafb_i2c_send_start(void *, int);
static int	mgafb_i2c_send_stop(void *, int);
static int	mgafb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int	mgafb_i2c_read_byte(void *, uint8_t *, int);
static int	mgafb_i2c_write_byte(void *, uint8_t, int);

static const struct i2c_bitbang_ops mgafb_i2cbb_ops = {
	mgafb_i2cbb_set_bits,
	mgafb_i2cbb_set_dir,
	mgafb_i2cbb_read_bits,
	{
		MGA_DDC_SDA,	/* [I2C_BIT_SDA]    = bit 2 */
		MGA_DDC_SCL,	/* [I2C_BIT_SCL]    = bit 4 */
		0,
		0
	}
};

static void	mgafb_load_cmap(struct mgafb_softc *, u_int, u_int);
static void	mgafb_init_default_cmap(struct mgafb_softc *);
static int	mgafb_putcmap(struct mgafb_softc *, struct wsdisplay_cmap *);
static int	mgafb_getcmap(struct mgafb_softc *, struct wsdisplay_cmap *);

static void	mgafb_cursor_init(struct mgafb_softc *);
static void	mgafb_cursor_enable(struct mgafb_softc *, bool);
static void	mgafb_cursor_setpos(struct mgafb_softc *, int, int);
static void	mgafb_cursor_setcmap(struct mgafb_softc *);
static void	mgafb_cursor_setshape(struct mgafb_softc *, int, int);

static void	mgafb_init_screen(void *, struct vcons_screen *, int, long *);
static int	mgafb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	mgafb_mmap(void *, void *, off_t, int);

#ifdef MGAFB_ACCEL
static void	mgafb_wait_fifo(struct mgafb_softc *, int);
static void	mgafb_wait_idle(struct mgafb_softc *);
static uint32_t	mgafb_color_replicate(struct mgafb_softc *, uint32_t);
static void	mgafb_fill_rect(struct mgafb_softc *, int, int, int, int,
		    uint32_t);
static void	mgafb_blit_rect(struct mgafb_softc *, int, int, int, int,
		    int, int);
static void	mgafb_gc_bitblt(void *, int, int, int, int, int, int, int);
static void	mgafb_putchar(void *, int, int, u_int, long);
static void	mgafb_putchar_aa(void *, int, int, u_int, long);
static void	mgafb_copyrows(void *, int, int, int);
static void	mgafb_eraserows(void *, int, int, long);
static void	mgafb_copycols(void *, int, int, int, int);
static void	mgafb_erasecols(void *, int, int, int, long);
#endif /* MGAFB_ACCEL */

CFATTACH_DECL_NEW(mgafb, sizeof(struct mgafb_softc),
	mgafb_match, mgafb_attach, NULL, NULL);

static const struct mgafb_chip_info mgafb_chips[] = {
	[MGAFB_CHIP_2064W] = {
		.ci_name	= "2064W",
		.ci_max_pclk	= 175000,
		.ci_vram_default = 0,
		.ci_has_tvp3026	= true,
		.ci_has_wram	= true,
		.ci_probe_vram	= true,
	},
	[MGAFB_CHIP_2164W] = {
		.ci_name	= "2164W",
		.ci_max_pclk	= 220000,
		.ci_vram_default = 4*1024*1024,
		.ci_has_tvp3026	= true,
		.ci_has_wram	= true,
		.ci_probe_vram	= false,
	},
	[MGAFB_CHIP_1064SG] = {
		.ci_name	= "1064SG",
		.ci_max_pclk	= 135000,
		.ci_vram_default = 0,
		.ci_has_tvp3026	= false,
		.ci_has_wram	= false,
		.ci_probe_vram	= true,
	},
};

static struct wsdisplay_accessops mgafb_accessops = {
	mgafb_ioctl,
	mgafb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* pollc */
	NULL,	/* scroll */
};

static int
mgafb_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_MATROX)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_MATROX_MILLENNIUM:
/*
	case PCI_PRODUCT_MATROX_MILLENNIUM2:
	case PCI_PRODUCT_MATROX_MILLENNIUM2_AGP:
	case PCI_PRODUCT_MATROX_MYSTIQUE:
*/
	return 100;
	}

	return 0;
}

static void
mgafb_attach(device_t parent, device_t self, void *aux)
{
	struct mgafb_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args ws_aa;
	struct rasops_info *ri;
	const struct pci_attach_args *pa = aux;
	pcireg_t screg;
	bool console;
	long defattr;
	int regbar;
	int fbbar;

#ifdef MGAFB_CONSOLE
	console = true;
#else
	prop_dictionary_get_bool(device_properties(self), "is_console",
	    &console);
#endif

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_MATROX_MILLENNIUM2:
	case PCI_PRODUCT_MATROX_MILLENNIUM2_AGP:
		sc->sc_chip = MGAFB_CHIP_2164W;
		break;
	case PCI_PRODUCT_MATROX_MYSTIQUE:
		sc->sc_chip = MGAFB_CHIP_1064SG;
		break;
	default:
		sc->sc_chip = MGAFB_CHIP_2064W;
		break;
	}
	sc->sc_ci = &mgafb_chips[sc->sc_chip];

	/* Enable PCI memory decoding. */
	screg = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PCI_COMMAND_STATUS_REG);
	screg |= PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG,
	    screg);

	pci_aprint_devinfo(pa, NULL);

	mgafb_resolve_bars(sc, pa, &regbar, &fbbar);

	if (pci_mapreg_map(pa, regbar,
	    PCI_MAPREG_TYPE_MEM, BUS_SPACE_MAP_LINEAR,
	    &sc->sc_regt, &sc->sc_regh,
	    &sc->sc_reg_pa, &sc->sc_reg_size) != 0) {
		aprint_error_dev(self,
		    "unable to map control aperture\n");
		return;
	}
	if (pci_mapreg_map(pa, fbbar,
	    PCI_MAPREG_TYPE_MEM, BUS_SPACE_MAP_LINEAR,
	    &sc->sc_fbt, &sc->sc_fbh,
	    &sc->sc_fb_pa, &sc->sc_fb_size) != 0) {
		aprint_error_dev(self,
		    "unable to map framebuffer\n");
		return;
	}

	aprint_normal_dev(self,
	    "control at 0x%08" PRIxPADDR ", fb at 0x%08" PRIxPADDR "\n",
	    (paddr_t)sc->sc_reg_pa, (paddr_t)sc->sc_fb_pa);

#ifdef MGAFB_PINS 
	mgafb_read_pins(sc, pa);
	mgafb_dump_pins(sc);
#endif /* MGAFB_PINS */

#ifdef MGAFB_8BPP
	sc->sc_depth = 8;
#else
	sc->sc_depth = 16;
#endif

#ifndef MGAFB_NO_HW_INIT
	if (sc->sc_ci->ci_has_wram) {
		mgafb_preinit_wram(sc);
		mgafb_set_mclk(sc, MGA_MCLK_KHZ);
	} else {
		mgafb_preinit_1064sg(sc);
	}
#else
	aprint_normal_dev(self,
	    "hardware init skipped\n");
#endif

	mgafb_detect_vram(sc);

	mgafb_ddc_read(sc);

	sc->sc_videomode = mgafb_pick_mode(sc);
	sc->sc_width  = sc->sc_videomode->hdisplay;
	sc->sc_height = sc->sc_videomode->vdisplay;
	sc->sc_stride = sc->sc_width * (sc->sc_depth / 8);
	aprint_normal_dev(self, "videomode: %dx%d, %d kHz dot clock\n",
	    sc->sc_width, sc->sc_height, sc->sc_videomode->dot_clock);

	mgafb_set_mode(sc);

	if (sc->sc_ci->ci_has_tvp3026)
		mgafb_cursor_init(sc);

	aprint_normal_dev(self, "setting %dx%d %dbpp\n",
	    sc->sc_width, sc->sc_height, sc->sc_depth);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_video = WSDISPLAYIO_VIDEO_ON;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &mgafb_accessops);
	sc->vd.init_screen = mgafb_init_screen;

#ifdef MGAFB_ACCEL
	sc->sc_gc.gc_bitblt     = mgafb_gc_bitblt;
	sc->sc_gc.gc_rectfill   = NULL;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop        = 0; /* copy; mgafb_gc_bitblt ignores rop */
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb     = glyphcache_adapt;
#endif

	ri = &sc->sc_console_screen.scr_ri;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

#ifdef MGAFB_ACCEL
	/* Initialize the glyph cache BEFORE vcons_redraw_screen.*/
	glyphcache_init(&sc->sc_gc,
	    sc->sc_height,
	    (int)(sc->sc_vram_size / (bus_size_t)sc->sc_stride) - sc->sc_height,
	    sc->sc_width,
	    ri->ri_font->fontwidth,
	    ri->ri_font->fontheight,
	    defattr);
#endif

	if (sc->sc_depth == 8)
		mgafb_init_default_cmap(sc);
	vcons_redraw_screen(&sc->sc_console_screen);

	if (console) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);

		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

#ifdef MGAFB_ACCEL
	aprint_normal_dev(sc->sc_dev,
	    "glyph cache at VRAM offset 0x%x (%d lines, %d cells)\n",
	    sc->sc_height * sc->sc_stride,
	    sc->sc_gc.gc_lines,
	    sc->sc_gc.gc_numcells);
#endif

	if (!console && sc->sc_depth == 8)
		mgafb_init_default_cmap(sc);

	ws_aa.console = console;
	ws_aa.scrdata = &sc->sc_screenlist;
	ws_aa.accessops = &mgafb_accessops;
	ws_aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &ws_aa, wsemuldisplaydevprint, CFARGS_NONE);
}

static void
mgafb_dac_write(struct mgafb_softc *sc, uint8_t reg, uint8_t val)
{
	MGA_WRITE1(sc, MGA_DAC_BASE + reg, val);
}

static uint8_t
mgafb_dac_read(struct mgafb_softc *sc, uint8_t reg)
{
	return MGA_READ1(sc, MGA_DAC_BASE + reg);
}

static void
mgafb_dac_write_ind(struct mgafb_softc *sc, uint8_t idx, uint8_t val)
{
	mgafb_dac_write(sc, MGA_DAC_IND_INDEX, idx);
	mgafb_dac_write(sc, MGA_DAC_IND_DATA, val);
}

static uint8_t
mgafb_dac_read_ind(struct mgafb_softc *sc, uint8_t idx)
{
	mgafb_dac_write(sc, MGA_DAC_IND_INDEX, idx);
	return mgafb_dac_read(sc, MGA_DAC_IND_DATA);
}

static void
mgafb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct mgafb_softc *sc = cookie;

	/* CTL: 1 = drive LOW (assert), 0 = release HIGH. */
	sc->sc_ddc_ctl &= ~(MGA_DDC_SDA | MGA_DDC_SCL);
	sc->sc_ddc_ctl |= (~bits) & (MGA_DDC_SDA | MGA_DDC_SCL);
	mgafb_dac_write_ind(sc, MGA_TVP_GEN_IO_CTL, sc->sc_ddc_ctl);

	/* DATA: mirror desired state (1 = HIGH, 0 = LOW). */
	mgafb_dac_write_ind(sc, MGA_TVP_GEN_IO_DATA,
	    (uint8_t)bits & (MGA_DDC_SDA | MGA_DDC_SCL));
}

static void
mgafb_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	/* Nothing to do. */
}

static uint32_t
mgafb_i2cbb_read_bits(void *cookie)
{
	struct mgafb_softc *sc = cookie;

	return mgafb_dac_read_ind(sc, MGA_TVP_GEN_IO_DATA);
}

static int
mgafb_i2c_send_start(void *cookie, int flags)
{
	return i2c_bitbang_send_start(cookie, flags, &mgafb_i2cbb_ops);
}

static int
mgafb_i2c_send_stop(void *cookie, int flags)
{
	return i2c_bitbang_send_stop(cookie, flags, &mgafb_i2cbb_ops);
}

static int
mgafb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return i2c_bitbang_initiate_xfer(cookie, addr, flags, &mgafb_i2cbb_ops);
}

static int
mgafb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return i2c_bitbang_read_byte(cookie, valp, flags, &mgafb_i2cbb_ops);
}

static int
mgafb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return i2c_bitbang_write_byte(cookie, val, flags, &mgafb_i2cbb_ops);
}

static void
mgafb_ddc_read(struct mgafb_softc *sc)
{
	int i;

	/* Release both lines to idle-HIGH before starting the controller. */
	sc->sc_ddc_ctl = mgafb_dac_read_ind(sc, MGA_TVP_GEN_IO_CTL);
	sc->sc_ddc_ctl &= ~(MGA_DDC_SDA | MGA_DDC_SCL);
	mgafb_dac_write_ind(sc, MGA_TVP_GEN_IO_CTL, sc->sc_ddc_ctl);

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie        = sc;
	sc->sc_i2c.ic_send_start    = mgafb_i2c_send_start;
	sc->sc_i2c.ic_send_stop     = mgafb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = mgafb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte     = mgafb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte    = mgafb_i2c_write_byte;

	/* Some monitors don't respond on the first attempt. */
	sc->sc_edid_valid = false;
	memset(sc->sc_edid, 0, sizeof(sc->sc_edid));
	for (i = 0; i < 3; i++) {
		if (ddc_read_edid(&sc->sc_i2c, sc->sc_edid, 128) == 0 &&
		    sc->sc_edid[1] != 0)
			break;
		memset(sc->sc_edid, 0, sizeof(sc->sc_edid));
	}

	if (sc->sc_edid[1] == 0) {
		aprint_normal_dev(sc->sc_dev, "DDC: no EDID response\n");
		return;
	}

	if (edid_parse(sc->sc_edid, &sc->sc_edid_info) == -1) {
		aprint_error_dev(sc->sc_dev,
		    "DDC: EDID parse failed (bad header or checksum)\n");
		return;
	}

	sc->sc_edid_valid = true;
	edid_print(&sc->sc_edid_info);
}

#ifdef MGAFB_PINS
/*
 * This now sort of works for PInS v1, but needs a rewrite and support 
 * for later PInS versions.
 */
static void
mgafb_read_pins(struct mgafb_softc *sc, const struct pci_attach_args *pa)
{
	bus_space_tag_t romt;
	bus_space_handle_t romh;
	bus_size_t rom_size;
	bool rom_mapped = false;	/* true if we called pci_mapreg_map */
	pcireg_t saved_rombar;
	bool rom_bar_programmed = false;
	uint32_t opt;
	pcireg_t rombar;
	uint16_t pins_off;
	uint16_t rom_magic;
	uint8_t pins_ver, pins_vcosel;
	uint16_t pins_maxdac_raw, pins_syspll;

	sc->sc_pins_valid    = false;
	sc->sc_pins_mclk_khz = 0;
	sc->sc_pins_maxdac_khz = 0;

	opt = pci_conf_read(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION);
	if ((opt & MGA_OPTION_BIOSEN) == 0) {
		aprint_error_dev(sc->sc_dev,
		    "PInS: BIOS ROM disabled (biosen=0 in OPTION 0x%08x)\n",
		    opt);
		return;
	}

	rombar = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_MAPREG_ROM);
	saved_rombar = rombar;
	aprint_verbose_dev(sc->sc_dev,
	    "PInS: ROM BAR = 0x%08x (enable=%d)\n",
	    rombar, (rombar & PCI_MAPREG_ROM_ENABLE) ? 1 : 0);

	if ((rombar & PCI_MAPREG_ROM_ADDR_MASK) == 0) {
		/*
		 * ROM BAR was not configured. Temporarily point
		 * the ROM BAR at the framebuffer aperture — the 2064W spec
		 * says BIOS EPROM has highest decode precedence when
		 * apertures overlap. 
		 */ 
		aprint_verbose_dev(sc->sc_dev,
		    "PInS: ROM BAR unassigned, borrowing FB "
		    "address 0x%08lx\n",
		    (unsigned long)sc->sc_fb_pa);
		pci_conf_write(sc->sc_pc, sc->sc_pcitag,
		    PCI_MAPREG_ROM,
		    (sc->sc_fb_pa & PCI_MAPREG_ROM_ADDR_MASK) |
		    PCI_MAPREG_ROM_ENABLE);
		rom_bar_programmed = true;

		/* Read through the existing FB mapping. */
		romt = sc->sc_fbt;
		romh = sc->sc_fbh;
		rom_size = sc->sc_fb_size;
	} else {
		/*
		 * ROM BAR has an address assigned by firmware.
		 * Map it normally.
		 */
		if (pci_mapreg_map(pa, PCI_MAPREG_ROM,
		    PCI_MAPREG_TYPE_ROM, BUS_SPACE_MAP_PREFETCHABLE,
		    &romt, &romh, NULL, &rom_size) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "PInS: cannot map expansion ROM\n");
			return;
		}
		rom_mapped = true;
	}

	/*
	 * Old Matrox ROM may (or may not) have PCIR headers that don't 
	 * match the PCI device ID or class code...
	 */
	rom_magic = bus_space_read_1(romt, romh, 0) |
	    ((uint16_t)bus_space_read_1(romt, romh, 1) << 8);
	if (rom_magic != 0xAA55) {
		aprint_error_dev(sc->sc_dev,
		    "PInS: ROM header magic 0x%04x (expected 0xAA55)\n",
		    rom_magic);
		goto out_cleanup;
	}

	if (rom_size < 0x8000) {
		aprint_normal_dev(sc->sc_dev,
		    "PInS: ROM too small for PInS (%zu bytes)\n",
		    (size_t)rom_size);
		goto out_cleanup;
	}

	pins_off  =  bus_space_read_1(romt, romh, 0x7FFC);
	pins_off |= (uint16_t)bus_space_read_1(romt, romh, 0x7FFD) << 8;

	if (pins_off < 2 || pins_off >= 0x8000) {
		aprint_normal_dev(sc->sc_dev,
		    "PInS: pointer out of range (0x%04x)\n", pins_off);
		goto out_cleanup;
	}

	aprint_verbose_dev(sc->sc_dev,
	    "PInS: pointer at 0x7FFC = 0x%04x, first 4 bytes at offset:"
	    " %02x %02x %02x %02x\n", pins_off,
	    bus_space_read_1(romt, romh, pins_off + 0),
	    bus_space_read_1(romt, romh, pins_off + 1),
	    bus_space_read_1(romt, romh, pins_off + 2),
	    bus_space_read_1(romt, romh, pins_off + 3));

	if (bus_space_read_1(romt, romh, pins_off + 0) == 0x2E &&
	    bus_space_read_1(romt, romh, pins_off + 1) == 0x41) {
		pins_ver = bus_space_read_1(romt, romh, pins_off + 5);
		aprint_verbose_dev(sc->sc_dev,
		    "PInS: version %u format at offset 0x%04x\n",
		    pins_ver, pins_off);

		pins_vcosel = bus_space_read_1(romt, romh, pins_off + 41);
		sc->sc_pins_maxdac_khz = ((uint32_t)pins_vcosel + 100) * 1000;

		pins_vcosel = bus_space_read_1(romt, romh, pins_off + 43);
		if (pins_vcosel != 0 &&
		    ((uint32_t)pins_vcosel + 100) * 1000 >= 100000) {
			sc->sc_pins_mclk_khz =
			    ((uint32_t)pins_vcosel + 100) * 1000;
		}

		sc->sc_pins_valid = true;

	} else if (bus_space_read_1(romt, romh, pins_off + 0) == 64 &&
	           bus_space_read_1(romt, romh, pins_off + 1) == 0x00) {
		aprint_normal_dev(sc->sc_dev,
		    "PInS: version 1 format at offset 0x%04x\n", pins_off);

		pins_vcosel = bus_space_read_1(romt, romh, pins_off + 22);
		pins_maxdac_raw  = bus_space_read_1(romt, romh, pins_off + 24);
		pins_maxdac_raw |=
		    (uint16_t)bus_space_read_1(romt, romh, pins_off + 25) << 8;
		pins_syspll  = bus_space_read_1(romt, romh, pins_off + 28);
		pins_syspll |=
		    (uint16_t)bus_space_read_1(romt, romh, pins_off + 29) << 8;

		switch (pins_vcosel) {
		case 0:
			sc->sc_pins_maxdac_khz = 175000;
			break;
		case 1:
			sc->sc_pins_maxdac_khz = 220000;
			break;
		default:
			/* Undefined value — use conservative maximum. */
			sc->sc_pins_maxdac_khz = pins_maxdac_raw ?
			    (uint32_t)pins_maxdac_raw * 10 : 240000;
			break;
		}

		sc->sc_pins_mclk_khz = pins_syspll ?
		    (uint32_t)pins_syspll * 10 : 50000;

		sc->sc_pins_valid = true;

	} else {
		aprint_normal_dev(sc->sc_dev,
		    "PInS: unrecognised header at offset 0x%04x "
		    "(bytes 0x%02x 0x%02x)\n", pins_off,
		    bus_space_read_1(romt, romh, pins_off + 0),
		    bus_space_read_1(romt, romh, pins_off + 1));
	}

out_cleanup:
	if (rom_mapped)
		bus_space_unmap(romt, romh, rom_size);

	/*
	 * Restore the ROM BAR.  If we borrowed the FB address, write
	 * back the original (unassigned) value.  Otherwise just clear
	 * ROM enable so the ROM stops decoding.
	 */
	if (rom_bar_programmed)
		pci_conf_write(sc->sc_pc, sc->sc_pcitag,
		    PCI_MAPREG_ROM, saved_rombar);
	else
		pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_MAPREG_ROM,
		    pci_conf_read(sc->sc_pc, sc->sc_pcitag,
		    PCI_MAPREG_ROM) & ~PCI_MAPREG_ROM_ENABLE);
}

static void
mgafb_dump_pins(struct mgafb_softc *sc)
{
	if (!sc->sc_pins_valid) {
		aprint_normal_dev
(sc->sc_dev,
		    "PInS: not available (ROM absent, unreadable, or format 2+)\n");
		return;
	}
	aprint_normal_dev(sc->sc_dev,
	    "PInS: MCLK %u kHz, max pixel clock %u kHz\n",
	    sc->sc_pins_mclk_khz, sc->sc_pins_maxdac_khz);
}
#endif /* MGAFB_PINS */

#ifndef MGAFB_NO_HW_INIT
static void
mgafb_preinit_wram(struct mgafb_softc *sc)
{
	uint32_t opt;

	aprint_verbose_dev(sc->sc_dev, "WRAM init: stabilising MEMPLLCTRL\n");

	/* Stabilise MEMPLLCTRL before toggling reset. */
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL,
	    MGA_TVP_MEMPLLCTRL_STROBEMKC4 | MGA_TVP_MEMPLLCTRL_MCLK_MCLKPLL);
	delay(200);

	/* WRAM controller reset */
	if (sc->sc_chip == MGAFB_CHIP_2164W) {
		/* Use RST.softreset */
		aprint_verbose_dev(sc->sc_dev,
		    "2164W WRAM init: asserting softreset\n");
		MGA_WRITE4(sc, MGA_RST, 1);
		delay(200);
		MGA_WRITE4(sc, MGA_RST, 0);
		aprint_verbose_dev(sc->sc_dev,
		    "2164W WRAM init: softreset deasserted\n");
		delay(200);

		/* Program rfhcnt to a safe initial value. */
		opt = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
		    MGA_PCI_OPTION);
		opt &= ~MGA_OPTION_RFHCNT_MASK;
		opt |= (8U << MGA_OPTION_RFHCNT_SHIFT) &
		    MGA_OPTION_RFHCNT_MASK;
		pci_conf_write(sc->sc_pc, sc->sc_pcitag,
		    MGA_PCI_OPTION, opt);
		delay(250);
	} else {
		/* Use M_RESET to reset the WRAM controller. */
		aprint_verbose_dev(sc->sc_dev,
		    "2064W WRAM init: asserting M_RESET\n");
		opt = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
		    MGA_PCI_OPTION);
		pci_conf_write(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION,
		    opt | MGA_OPTION_M_RESET);
		delay(250);

		/* Deassert M_RESET. */
		pci_conf_write(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION,
		    opt & ~MGA_OPTION_M_RESET);
		aprint_verbose_dev(sc->sc_dev,
		    "2064W WRAM init: M_RESET deasserted\n");
		delay(250);
	}

	/* Trigger WRAM initialisation cycle */
	aprint_verbose_dev(sc->sc_dev, "WRAM init: triggering init cycle\n");
	MGA_WRITE4(sc, MGA_MACCESS, MGA_MACCESS_WRAM_INIT);
	delay(10);

	aprint_verbose_dev(sc->sc_dev, "WRAM init: complete\n");
}

static void
mgafb_set_mclk(struct mgafb_softc *sc, int freq_khz)
{
	uint8_t pclk_n, pclk_m, pclk_p;
	uint8_t n, m, p;
	uint8_t mclk_ctl;
	int fvco_khz, rfhcnt, i;
	uint32_t opt;

	aprint_verbose_dev(sc->sc_dev, "MCLK: programming to %d kHz\n",
	    freq_khz);

	(void)freq_khz;		/* only 50000 kHz supported for now */

	n = 52;
	m = 42;
	p = 2;
	fvco_khz = 8 * MGA_TVP_REFCLK * (65 - m) / (65 - n);

	/* Save current PCLK N/M/P from TVP3026 regs. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_N);
	pclk_n = mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_M);
	pclk_m = mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_STOP);
	pclk_p = mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA);

	/* Stop PCLK. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_STOP);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0x00);

	/* Temporarily set PCLK to target MCLK frequency. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_START);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0xC0 | n);	/* N at 0x00 */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, m);		/* M at 0x01 */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0xB0 | p);	/* P at 0x02; PLL restarts */

	/* Wait for temporary PCLK to lock. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_ALL_STATUS);
	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA) &
		    MGA_TVP_PLL_LOCKED)
			break;
		delay(10);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "MCLK: timeout waiting for temporary PCLK lock\n");
	else
		aprint_verbose_dev
(sc->sc_dev,
		    "MCLK: temporary PCLK locked after %d polls\n", i);

	/* Route temp PCLK to MCLK */
	mclk_ctl = mgafb_dac_read_ind(sc, MGA_TVP_MEMPLLCTRL);
	aprint_normal_dev(sc->sc_dev,
	    "MCLK: MEMPLLCTRL was 0x%02x, routing PCLK to MCLK\n", mclk_ctl);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL,
	    mclk_ctl & 0xe7);		/* clear bits[4:3]: source=PIXPLL, strobe=0 */
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL,
	    (mclk_ctl & 0xe7) | 0x08);	/* set strobe to latch PIXPLL routing */
	delay(1000);
	aprint_verbose_dev(sc->sc_dev, "MCLK: PIXPLL routing latched\n");

	/* Stop MCLK PLL. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_MCLK_STOP);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLDATA, 0x00);
	aprint_verbose_dev(sc->sc_dev, "MCLK: PLL stopped\n");

	/* Program MCLK PLL with target N/M/P. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_MCLK_START);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLDATA, 0xC0 | n);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLDATA, m);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLDATA, 0xB0 | p);
	aprint_verbose_dev(sc->sc_dev, "MCLK: PLL programmed N=%u M=%u P=%u\n",
	    n, m, p);

	/* Wait for MCLK PLL to lock. */
	for (i = 0; i < 10000; i++) {
		mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR,
		    MGA_TVP_PLLADDR_ALL_STATUS);
		if (mgafb_dac_read_ind(sc, MGA_TVP_MEMPLLDATA) &
		    MGA_TVP_PLL_LOCKED)
			break;
		delay(10);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "MCLK: timeout waiting for MCLK PLL lock\n");
	else
		aprint_verbose_dev(sc->sc_dev,
		    "MCLK: PLL locked after %d polls\n", i);

	/* Update WRAM refresh counter. */
	rfhcnt = (fvco_khz * 333 / (10000 * (1 << p)) - 64) / 128;
	if (rfhcnt < 0)
		rfhcnt = 0;
	if (rfhcnt > 15)
		rfhcnt = 15;

	opt = pci_conf_read(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION);
	opt &= ~MGA_OPTION_RFHCNT_MASK;
	opt |= ((uint32_t)rfhcnt << MGA_OPTION_RFHCNT_SHIFT) &
	    MGA_OPTION_RFHCNT_MASK;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION, opt);
	aprint_normal_dev(sc->sc_dev,
	    "MCLK: Fvco=%d kHz Fpll=%d kHz rfhcnt=%d\n",
	    fvco_khz, fvco_khz / (1 << p), rfhcnt);

	/* Switch MCLK source to MCLK PLL. */
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL,
	    (mclk_ctl & 0xe7) | 0x10);	/* source=MCLKPLL (bit4), strobe=0 */
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL,
	    (mclk_ctl & 0xe7) | 0x18);	/* set strobe to latch MCLKPLL routing */
	delay(1000);

	/* Stop PCLK and restore its original N/M/P. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_STOP);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0x00);

	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_PCLK_START);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, pclk_n);	/* N at 0x00 */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, pclk_m);	/* M at 0x01 */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, pclk_p);	/* P at 0x02; PLL restarts */

	/* Lock check. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, MGA_TVP_PLLADDR_ALL_STATUS);
	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA) &
		    MGA_TVP_PLL_LOCKED)
			break;
		delay(10);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "MCLK: timeout waiting for PCLK relock\n");
	else
		aprint_verbose_dev
(sc->sc_dev,
		    "MCLK: programming complete\n");
}

static void
mgafb_preinit_1064sg(struct mgafb_softc *sc)
{
	int i;

	aprint_verbose_dev(sc->sc_dev,
	    "1064SG: writing OPTION register %lx\n",
	    MGA1064_OPTION_DEFAULT);
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, MGA_PCI_OPTION,
	    MGA1064_OPTION_DEFAULT);
	delay(250);

	/* System PLL with hardcoded values from xf86-video-mga... */
	aprint_verbose_dev(sc->sc_dev, "1064SG: programming system PLL\n");
	mgafb_dac_write_ind(sc, MGA_IDAC_SYS_PLL_M, 0x04);
	mgafb_dac_write_ind(sc, MGA_IDAC_SYS_PLL_N, 0x44);
	mgafb_dac_write_ind(sc, MGA_IDAC_SYS_PLL_P, 0x18);

	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_IDAC_SYS_PLL_STAT) & 0x40)
			break;
		delay(10);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "1064SG: timeout waiting for system PLL lock\n");
	else
		aprint_verbose_dev(sc->sc_dev,
		    "1064SG: system PLL locked after %d polls\n", i);
}
#endif /* MGA_NO_HW_INIT */

static void
mgafb_detect_vram(struct mgafb_softc *sc)
{
	volatile uint8_t *fb;
	bus_size_t probe_max;
	bus_size_t vram_bytes;
	bus_size_t off;
	uint8_t saved_crtcext3;
	uint32_t opt;

	if (!sc->sc_ci->ci_probe_vram) {
		sc->sc_vram_size = sc->sc_ci->ci_vram_default;
		aprint_normal_dev(sc->sc_dev,
		    "%zu MB VRAM assumed (%s)\n",
		    (size_t)(sc->sc_vram_size / (1024*1024)),
		    sc->sc_ci->ci_name);
		return;
	}

	probe_max = (bus_size_t)8*1024*1024;
	if (probe_max > sc->sc_fb_size)
		probe_max = sc->sc_fb_size;

	fb = bus_space_vaddr(sc->sc_fbt, sc->sc_fbh);
	if (fb == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "VRAM probe: bus_space_vaddr failed; assuming 2 MB\n");
		sc->sc_vram_size = 2*1024*1024;
		return;
	}

	MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, 3);
	saved_crtcext3 = MGA_READ1(sc, MGA_CRTCEXT_DATA);
	MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, 3);
	MGA_WRITE1(sc, MGA_CRTCEXT_DATA, saved_crtcext3 | MGA_CRTCEXT3_MGAMODE);

	for (off = probe_max; off > (bus_size_t)2*1024*1024;
	    off -= (bus_size_t)2*1024*1024)
		fb[off - 1] = 0xAA;

	/* Ensure probe writes have reached VRAM before read-back. */
	bus_space_barrier(sc->sc_fbt, sc->sc_fbh, 0, probe_max,
	    BUS_SPACE_BARRIER_WRITE);

	/* Cache flush. */
	MGA_WRITE1(sc, MGA_VGA_CRTC_INDEX, 0);
	delay(10);

	/* Ensure we read actual VRAM, not stale CPU cache lines. */
	bus_space_barrier(sc->sc_fbt, sc->sc_fbh, 0, probe_max,
	    BUS_SPACE_BARRIER_READ);

	vram_bytes = (bus_size_t)2*1024*1024;
	for (off = probe_max; off > (bus_size_t)2*1024*1024;
	    off -= (bus_size_t)2*1024*1024) {
		if (fb[off - 1] == 0xAA) {
			vram_bytes = off;
			break;
		}
	}

	sc->sc_vram_size = vram_bytes;

	if (sc->sc_ci->ci_has_wram)
		aprint_normal_dev(sc->sc_dev,
		    "%zu MB VRAM detected (%s WRAM bus)\n",
		    (size_t)(vram_bytes / (1024*1024)),
		    vram_bytes > (bus_size_t)2*1024*1024 ?
		    "64-bit" : "32-bit");
	else
		aprint_normal_dev(sc->sc_dev,
		    "%zu MB VRAM detected (SGRAM)\n",
		    (size_t)(vram_bytes / (1024*1024)));

	MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, 3);
	MGA_WRITE1(sc, MGA_CRTCEXT_DATA, saved_crtcext3);

	/*
	 * Set interleave bit (2064W only — OPTION bit 12).
	 * Other chips either lack this bit or use SGRAM.
	 */
	if (sc->sc_chip == MGAFB_CHIP_2064W) {
		opt = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
		    MGA_PCI_OPTION);
		opt &= ~MGA_OPTION_INTERLEAVE;
		if (vram_bytes > (bus_size_t)2*1024*1024)
			opt |= MGA_OPTION_INTERLEAVE;
		pci_conf_write(sc->sc_pc, sc->sc_pcitag,
		    MGA_PCI_OPTION, opt);
	}
}

static void
mgafb_calc_pll(int target_khz, uint8_t *n_out, uint8_t *m_out, uint8_t *p_out)
{
	int p, m, n, fvco, fpll, err;
	int best_err, best_n, best_m, best_p;

	/* 
	 * Millenium I comes with two variants of DACs - we should detect it.
	 * 175MHz DAC can do 220MHz VCO, 250MHz DAC can do 250MHz VCO. 
	 */
	const int fvco_min = 110000;
	const int fvco_max = 220000;

	/*
	 * Init 65 MHz pixel clock.
	 */
	best_err = target_khz + 1;
	best_n   = 43;
	best_m   = 40;
	best_p   = 1;

	for (p = 3; p >= 0; p--) {
		for (m = 1; m <= 62; m++) {
			for (n = 1; n <= 62; n++) {
				fvco = 8 * MGA_TVP_REFCLK * (65 - m) / (65 - n);
				if (fvco < fvco_min || fvco > fvco_max)
					continue;
				fpll = fvco >> p;
				err  = fpll - target_khz;
				if (err < 0)
					err = -err;
				if (err < best_err) {
					best_err = err;
					best_n   = n;
					best_m   = m;
					best_p   = p;
				}
			}
		}
	}

	*n_out = (uint8_t)best_n;
	*m_out = (uint8_t)best_m;
	*p_out = (uint8_t)best_p;
}

static void
mgafb_calc_pll_1064sg(int target_khz, uint8_t *m_out, uint8_t *n_out,
    uint8_t *p_out)
{
	int m, n, p, s;
	int best_m, best_n, best_p;
	int best_err;
	int fvco, fpll, err;
	int f_vco_target;
	static const int pvals[] = { 0, 1, 3, 7 };

	best_m = 1;
	best_n = 100;
	best_p = 0;
	best_err = target_khz + 1;

	/*
	 * Find P such that Fvco = target * (P+1) >= 50 MHz.
	 */
	f_vco_target = target_khz;
	for (p = 0; p <= 3; p++) {
		if (f_vco_target >= 50000)
			break;
		f_vco_target *= 2;
	}
	p = pvals[p > 3 ? 3 : p];

	for (m = 1; m <= 31; m++) {
		for (n = 100; n <= 127; n++) {
			fvco = MGA_IDAC_REFCLK * (n + 1) / (m + 1);
			if (fvco < 50000 || fvco > 220000)
				continue;
			fpll = fvco / (p + 1);
			err = fpll - target_khz;
			if (err < 0)
				err = -err;
			if (err < best_err) {
				best_err = err;
				best_m = m;
				best_n = n;
				best_p = p;
			}
		}
	}

	/* S value for loop filter bandwidth. */
	fvco = MGA_IDAC_REFCLK * (best_n + 1) / (best_m + 1);
	if (fvco < 100000)
		s = 0;
	else if (fvco < 140000)
		s = 1;
	else if (fvco < 180000)
		s = 2;
	else
		s = 3;

	*m_out = (uint8_t)(best_m & 0x1F);
	*n_out = (uint8_t)(best_n & 0x7F);
	*p_out = (uint8_t)((best_p & 0x07) | ((s & 0x03) << 3));
}

/*
 * Resolve PCI BAR assignments for control and framebuffer apertures.
 * Layout varies by chip and sometimes by PCI revision.
 */
static void
mgafb_resolve_bars(struct mgafb_softc *sc, const struct pci_attach_args *pa,
    int *regbar, int *fbbar)
{

	if (sc->sc_chip == MGAFB_CHIP_2164W ||
	    (sc->sc_chip == MGAFB_CHIP_1064SG &&
	     PCI_REVISION(pa->pa_class) >= 3)) {
		*fbbar = MILL2_BAR_FB;
		*regbar = MILL2_BAR_REG;
	} else {
		*fbbar = MILL_BAR_FB;
		*regbar = MILL_BAR_REG;
	}
}

/*
 * Compute CRTCEXT3 scale bits based on memory bus width and pixel depth.
 * 1064SG always has a 32-bit SGRAM bus (narrow).
 * 2064W with <= 2MB WRAM has a 32-bit bus (narrow).
 * 2064W with > 2MB / 2164W has a 64-bit bus (wide).
 */
static uint8_t
mgafb_crtcext3_scale(struct mgafb_softc *sc)
{
	bool is_narrow;

	is_narrow = !sc->sc_ci->ci_has_wram ||
	    sc->sc_vram_size <= (bus_size_t)2*1024*1024;

	if (sc->sc_depth == 8)
		return is_narrow ? 0x01 : 0x00;
	else
		return is_narrow ? 0x03 : 0x01;
}

static void
mgafb_calc_crtc(const struct videomode *vm, int depth, bool interleave,
    uint8_t crtc_out[25], uint8_t crtcext_out[6],
    uint8_t *misc_out, uint8_t *vsyncpol_out)
{
	int hd, hs, he, ht;
	int vd, vs, ve, vt;
	int wd;		/* logical scanline width */

	hd = (vm->hdisplay    >> 3) - 1;
	hs = (vm->hsync_start >> 3) - 1;
	he = (vm->hsync_end   >> 3) - 1;
	ht = (vm->htotal      >> 3) - 1;

	vd = vm->vdisplay  - 1;
	vs = vm->vsync_start;
	ve = vm->vsync_end;
	vt = vm->vtotal    - 2;

	/*
	 * HTOTAL workaround, round up as a precaution.
	 */
	if ((ht & 0x07) == 0x06 || (ht & 0x07) == 0x04)
		ht++;

	wd = vm->hdisplay * (depth / 8) / (interleave ? 16 : 8);

	*misc_out = 0xEF;

	if ((vm->flags & (VID_PHSYNC | VID_NHSYNC)) &&
	    (vm->flags & (VID_PVSYNC | VID_NVSYNC))) {
		*vsyncpol_out = 0;
		if (vm->flags & VID_PHSYNC)
			*vsyncpol_out |= 0x01;
		if (vm->flags & VID_PVSYNC)
			*vsyncpol_out |= 0x02;
	} else {
		if      (vm->vdisplay < 400) *vsyncpol_out = 0x01; /* +H -V */
		else if (vm->vdisplay < 480) *vsyncpol_out = 0x02; /* -H +V */
		else if (vm->vdisplay < 768) *vsyncpol_out = 0x00; /* -H -V */
		else                         *vsyncpol_out = 0x03; /* +H +V */
	}

	memset(crtc_out, 0, 25);

	crtc_out[0]  = (uint8_t)(ht - 4);		/* CR00: HTotal */
	crtc_out[1]  = (uint8_t)hd;			/* CR01: HDispEnd */
	crtc_out[2]  = (uint8_t)hd;			/* CR02: HBlankStart = HDE */
	crtc_out[3]  = (uint8_t)((ht & 0x1F) | 0x80);	/* CR03: HBlankEnd[4:0] */
	crtc_out[4]  = (uint8_t)hs;			/* CR04: HSyncStart */
	crtc_out[5]  = (uint8_t)(((ht & 0x20) << 2) |	/* CR05: HBlankEnd[5] */
	    (he & 0x1F));				/*        + HSyncEnd[4:0] */
	crtc_out[6]  = (uint8_t)(vt & 0xFF);		/* CR06: VTotal[7:0] */
	crtc_out[7]  = (uint8_t)(			/* CR07: Overflow */
	    ((vt & 0x100) >> 8) |			/*  bit0: VT[8]  */
	    ((vd & 0x100) >> 7) |			/*  bit1: VDE[8] */
	    ((vs & 0x100) >> 6) |			/*  bit2: VRS[8] */
	    ((vd & 0x100) >> 5) |			/*  bit3: VBS[8] = VDE[8] */
	    0x10                |			/*  bit4: always 1 */
	    ((vt & 0x200) >> 4) |			/*  bit5: VT[9]  */
	    ((vd & 0x200) >> 3) |			/*  bit6: VDE[9] */
	    ((vs & 0x200) >> 2));			/*  bit7: VRS[9] */
	/* crtc_out[8] = 0: CR08 PresetRowScan */
	crtc_out[9]  = (uint8_t)(((vd & 0x200) >> 4) |	/* CR09: MaxScanLine */
	    0x40);					/*  bit6: LineCompare[9] */
	/* crtc_out[10..15] = 0: cursor / start address */
	crtc_out[16] = (uint8_t)(vs & 0xFF);		/* CR10: VSyncStart[7:0] */
	crtc_out[17] = (uint8_t)((ve & 0x0F) | 0x20);	/* CR11: VSyncEnd[3:0] */
	crtc_out[18] = (uint8_t)(vd & 0xFF);		/* CR12: VDispEnd[7:0] */
	crtc_out[19] = (uint8_t)(wd & 0xFF);		/* CR13: Offset */
	/* crtc_out[20] = 0: CR14 Underline */
	crtc_out[21] = (uint8_t)(vd & 0xFF);		/* CR15: VBlankStart[7:0] */
	crtc_out[22] = (uint8_t)((vt + 1) & 0xFF);	/* CR16: VBlankEnd */
	crtc_out[23] = 0xC3;				/* CR17: byte mode */
	crtc_out[24] = 0xFF;				/* CR18: LineCompare */

	memset(crtcext_out, 0, 6);

	crtcext_out[0] = (uint8_t)((wd & 0x300) >> 4);
	crtcext_out[1] = (uint8_t)(
	    (((ht - 4) & 0x100) >> 8) |
	    ((hd & 0x100) >> 7) |
	    ((hs & 0x100) >> 6) |
	    (ht & 0x40));
	crtcext_out[2] = (uint8_t)(
	    ((vt & 0xC00) >> 10) |
	    ((vd & 0x400) >>  8) |
	    ((vd & 0xC00) >>  7) |
	    ((vs & 0xC00) >>  5));
	/* crtcext_out[3] left at 0; caller sets mgamode | scale */
}

static bool
mgafb_mode_fits(struct mgafb_softc *sc, const struct videomode *vm)
{
	uint32_t max_pclk_khz;

#ifdef MGAFB_PINS
	if (sc->sc_pins_valid)
		max_pclk_khz = sc->sc_pins_maxdac_khz;
	else
#endif
		max_pclk_khz = sc->sc_ci->ci_max_pclk;

	if ((uint32_t)vm->dot_clock > max_pclk_khz)
		return false;

	if ((bus_size_t)vm->hdisplay * (bus_size_t)vm->vdisplay *
	    (bus_size_t)(sc->sc_depth / 8) > sc->sc_vram_size)
		return false;

	return true;
}

static const struct videomode *
mgafb_pick_mode(struct mgafb_softc *sc)
{
	const struct videomode *mode, *m;
	prop_dictionary_t dict;
	uint32_t prop_w, prop_h;

	/* Start with safe default. */
	mode = pick_mode_by_ref(640, 480, 60);
	KASSERT(mode != NULL);

	/* Read firmware params. */
	dict = device_properties(sc->sc_dev);
	if (prop_dictionary_get_uint32(dict, "width",  &prop_w) &&
	    prop_dictionary_get_uint32(dict, "height", &prop_h)) {
		m = pick_mode_by_ref((int)prop_w, (int)prop_h, 60);
		if (m != NULL && mgafb_mode_fits(sc, m)) {
			aprint_verbose_dev(sc->sc_dev,
			    "mode: firmware resolution %ux%u\n",
			    prop_w, prop_h);
			mode = m;
		}
	}

	/* EDID preferred mode — highest priority. */
	if (sc->sc_edid_valid &&
	    sc->sc_edid_info.edid_preferred_mode != NULL) {
		m = sc->sc_edid_info.edid_preferred_mode;
		if (mgafb_mode_fits(sc, m)) {
			aprint_verbose_dev(sc->sc_dev,
			    "mode: EDID preferred %dx%d (%d kHz)\n",
			    m->hdisplay, m->vdisplay, m->dot_clock);
			mode = m;
		} else {
			aprint_verbose_dev(sc->sc_dev,
			    "mode: EDID preferred %dx%d (%d kHz) exceeds "
			    "hardware limits, ignoring\n",
			    m->hdisplay, m->vdisplay, m->dot_clock);
		}
	}

	return mode;
}

/*
 * TVP3026 DAC register setup for the selected pixel depth.
 */
static void
mgafb_tvp3026_setup_dac(struct mgafb_softc *sc, bool interleave)
{

	if (sc->sc_depth == 8) {
		mgafb_dac_write_ind(sc, MGA_TVP_COLORMODE, 0x06);
		mgafb_dac_write_ind(sc, MGA_TVP_PIXFMT,    0x80);
		mgafb_dac_write_ind(sc, MGA_TVP_CURCTL,
		    interleave ? 0x4C : 0x4B);
		mgafb_dac_write_ind(sc, MGA_TVP_MUXCTL,    0x25);
		mgafb_dac_write_ind(sc, MGA_TVP_LUTBYPASS, 0x0C);
		mgafb_dac_write_ind(sc, MGA_TVP_KEY_CTL,   0x00);
	} else {
		mgafb_dac_write_ind(sc, MGA_TVP_COLORMODE, 0x07);
		mgafb_dac_write_ind(sc, MGA_TVP_PIXFMT,    0x05);
		mgafb_dac_write_ind(sc, MGA_TVP_CURCTL,
		    interleave ? 0x54 : 0x53);
		mgafb_dac_write_ind(sc, MGA_TVP_MUXCTL,    0x15);
		mgafb_dac_write_ind(sc, MGA_TVP_LUTBYPASS, 0x24);
	}
}

/*
 * Program the TVP3026 PCLK PLL and loop PLL (VCLK).
 */
static void
mgafb_tvp3026_set_pclk(struct mgafb_softc *sc, int dot_clock, bool interleave)
{
	uint8_t pll_n, pll_m, pll_p;
	uint8_t lclk_n, lclk_p, lclk_q;
	uint32_t lclk_z100, bus_bytes, lclk_65mn;
	int i;

	mgafb_calc_pll(dot_clock, &pll_n, &pll_m, &pll_p);
	aprint_verbose_dev(sc->sc_dev,
	    "mode: PLL N=%u M=%u P=%u (target %d kHz)\n",
	    pll_n, pll_m, pll_p, dot_clock);

	/* Disable both PLLs before reprogramming. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR, 0x2A);
	mgafb_dac_write_ind(sc, MGA_TVP_LOOPPLLDATA, 0x00);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA,     0x00);

	/* Program PCLK shadow registers. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR,
	    MGA_TVP_PLLADDR_PCLK_START);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0xC0 | pll_n);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, pll_m);
	mgafb_dac_write_ind(sc, MGA_TVP_PLLDATA, 0xB0 | pll_p);

	/* Wait for PCLK lock. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR,
	    MGA_TVP_PLLADDR_ALL_STATUS);
	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_TVP_PLLDATA) &
		    MGA_TVP_PLL_LOCKED)
			break;
		delay(1);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "mode: timeout waiting for PCLK lock (%d kHz)\n",
		    dot_clock);
	else
		aprint_verbose_dev(sc->sc_dev,
		    "mode: PCLK locked after %d polls\n", i);

	/* Loop PLL (VCLK). */
	bus_bytes = interleave ? 8U : 4U;
	lclk_65mn = 32U * bus_bytes / (uint32_t)sc->sc_depth;
	lclk_n    = (uint8_t)(65U - lclk_65mn);
	lclk_z100 = 2750U * lclk_65mn * 1000U / (uint32_t)dot_clock;

	if (lclk_z100 <= 200) {
		lclk_p = 0; lclk_q = 0;
	} else if (lclk_z100 <= 400) {
		lclk_p = 1; lclk_q = 0;
	} else if (lclk_z100 <= 800) {
		lclk_p = 2; lclk_q = 0;
	} else if (lclk_z100 <= 1600) {
		lclk_p = 3; lclk_q = 0;
	} else {
		lclk_p = 3;
		lclk_q = (uint8_t)((lclk_z100 / 1600U) - 1U);
	}

	aprint_verbose_dev(sc->sc_dev,
	    "mode: LCLK N=%u M=61 P=%u Q=%u (z*100=%u)\n",
	    lclk_n, lclk_p, lclk_q, lclk_z100);

	/* MEMPLLCTRL Q-divider / strobe. */
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL, 0x30U | lclk_q);
	mgafb_dac_write_ind(sc, MGA_TVP_MEMPLLCTRL, 0x38U | lclk_q);

	/* Program loop PLL. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR,
	    MGA_TVP_PLLADDR_PCLK_START);
	mgafb_dac_write_ind(sc, MGA_TVP_LOOPPLLDATA, 0xC0 | lclk_n);
	mgafb_dac_write_ind(sc, MGA_TVP_LOOPPLLDATA, 61);
	mgafb_dac_write_ind(sc, MGA_TVP_LOOPPLLDATA, 0xF0 | lclk_p);

	/* Wait for LCLK lock. */
	mgafb_dac_write_ind(sc, MGA_TVP_PLLADDR,
	    MGA_TVP_PLLADDR_ALL_STATUS);
	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_TVP_LOOPPLLDATA) &
		    MGA_TVP_PLL_LOCKED)
			break;
		delay(1);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "mode: timeout waiting for LCLK lock\n");
	else
		aprint_normal_dev(sc->sc_dev,
		    "mode: LCLK locked after %d polls\n", i);
}

/*
 * 1064SG integrated DAC register setup for the selected pixel depth.
 */
static void
mgafb_idac_setup_dac(struct mgafb_softc *sc)
{

	if (sc->sc_depth == 8)
		mgafb_dac_write_ind(sc, MGA_IDAC_MUL_CTL, MGA_MULCTL_8BPP);
	else
		mgafb_dac_write_ind(sc, MGA_IDAC_MUL_CTL, MGA_MULCTL_16BPP);

	mgafb_dac_write_ind(sc, MGA_IDAC_MISC_CTL, 0x00);
	mgafb_dac_write_ind(sc, MGA_IDAC_VREF_CTL, 0x03);
}

/*
 * Program the 1064SG integrated pixel PLL (set C).
 */
static void
mgafb_idac_set_pclk(struct mgafb_softc *sc, int dot_clock)
{
	uint8_t pll_m, pll_n, pll_p;
	int i;

	mgafb_calc_pll_1064sg(dot_clock, &pll_m, &pll_n, &pll_p);
	aprint_verbose_dev(sc->sc_dev,
	    "mode: 1064SG PLL M=%u N=%u P=0x%02x (target %d kHz)\n",
	    pll_m, pll_n, pll_p, dot_clock);

	/* Disable pixel clock during reprogramming. */
	mgafb_dac_write_ind(sc, MGA_IDAC_PIX_CLK_CTL,
	    MGA_PIXCLK_DISABLE | MGA_PIXCLK_SRC_PLL);

	/* Write pixel PLL set C. */
	mgafb_dac_write_ind(sc, MGA_IDAC_PIX_PLLC_M, pll_m);
	mgafb_dac_write_ind(sc, MGA_IDAC_PIX_PLLC_N, pll_n);
	mgafb_dac_write_ind(sc, MGA_IDAC_PIX_PLLC_P, pll_p);

	/* Wait for pixel PLL lock. */
	for (i = 0; i < 10000; i++) {
		if (mgafb_dac_read_ind(sc, MGA_IDAC_PIX_PLL_STAT) & 0x40)
			break;
		delay(1);
	}
	if (i == 10000)
		aprint_error_dev(sc->sc_dev,
		    "mode: timeout waiting for pixel PLL lock (%d kHz)\n",
		    dot_clock);
	else
		aprint_verbose_dev(sc->sc_dev,
		    "mode: pixel PLL locked after %d polls\n", i);

	/* Enable pixel clock from PLL. */
	mgafb_dac_write_ind(sc, MGA_IDAC_PIX_CLK_CTL, MGA_PIXCLK_SRC_PLL);
}

static void
mgafb_set_mode(struct mgafb_softc *sc)
{
	int i;
	uint8_t locked;
	uint8_t crtc[25], crtcext[6], misc, vsyncpol;
	bool interleave;

	/*
	 * WRAM chips interleave when VRAM > 2 MB (64-bit bus).
	 * SGRAM chips always use a 32-bit bus — no interleave.
	 */
	interleave = sc->sc_ci->ci_has_wram &&
	    sc->sc_vram_size > (bus_size_t)2*1024*1024;

	/* Step 1: Sequencer async reset. */
	MGA_WRITE1(sc, MGA_VGA_SEQ_INDEX, 0x00);
	MGA_WRITE1(sc, MGA_VGA_SEQ_DATA, 0x01);
	MGA_WRITE1(sc, MGA_VGA_MISC_W, 0xEF);

	/* Step 2: CRTC timing (shared across all chips). */
	mgafb_calc_crtc(sc->sc_videomode, sc->sc_depth, interleave,
	    crtc, crtcext, &misc, &vsyncpol);

	/* Step 3: DAC setup — chip-specific. */
	if (sc->sc_ci->ci_has_tvp3026) {
		mgafb_tvp3026_setup_dac(sc, interleave);
		mgafb_dac_write_ind(sc, MGA_TVP_VSYNCPOL, vsyncpol);
	} else {
		mgafb_idac_setup_dac(sc);
	}

	/* Step 4: Pixel PLL — chip-specific. */
	if (sc->sc_ci->ci_has_tvp3026)
		mgafb_tvp3026_set_pclk(sc, sc->sc_videomode->dot_clock,
		    interleave);
	else
		mgafb_idac_set_pclk(sc, sc->sc_videomode->dot_clock);

	/* Step 5: VGA CRTC timing (shared). */
	MGA_WRITE1(sc, MGA_VGA_CRTC_INDEX, 0x11);
	locked = MGA_READ1(sc, MGA_VGA_CRTC_DATA);
	MGA_WRITE1(sc, MGA_VGA_CRTC_DATA, locked & ~0x80);	/* unlock */

	for (i = 0; i < 25; i++) {
		MGA_WRITE1(sc, MGA_VGA_CRTC_INDEX, i);
		MGA_WRITE1(sc, MGA_VGA_CRTC_DATA, crtc[i]);
	}

	MGA_WRITE1(sc, MGA_VGA_CRTC_INDEX, 0x11);
	MGA_WRITE1(sc, MGA_VGA_CRTC_DATA, crtc[0x11] | 0x80);	/* re-lock */

	/* Step 6: CRTCEXT registers. */
	crtcext[3] = MGA_CRTCEXT3_MGAMODE | mgafb_crtcext3_scale(sc);

	for (i = 0; i < 6; i++) {
		MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, i);
		MGA_WRITE1(sc, MGA_CRTCEXT_DATA, crtcext[i]);
	}

	MGA_WRITE1(sc, MGA_VGA_SEQ_INDEX, 0x00);
	MGA_WRITE1(sc, MGA_VGA_SEQ_DATA, 0x03);

	/* Step 7: Drawing engine init (shared across all chips). */
	aprint_verbose_dev(sc->sc_dev, "mode: initialising drawing engine\n");
	MGA_WRITE4(sc, MGA_MACCESS,
	    (sc->sc_depth == 8) ? MGA_PW8 : MGA_PW16);
	MGA_WRITE4(sc, MGA_PITCH,   (uint32_t)sc->sc_width);
	MGA_WRITE4(sc, MGA_PLNWT,   0xFFFFFFFF);
	MGA_WRITE4(sc, MGA_YDSTORG, 0);
	MGA_WRITE4(sc, MGA_CXBNDRY,
	    ((uint32_t)(sc->sc_width - 1) << 16) | 0);
	MGA_WRITE4(sc, MGA_YTOP, 0);
	/*
	 * YBOT must cover the full VRAM, not just the visible area.
	 * The glyph cache lives in off-screen rows (y >= sc_height);
	 * blits to/from those rows would be clipped if YBOT were set
	 * to (sc_height-1)*sc_width.
	 */
	MGA_WRITE4(sc, MGA_YBOT,
	    ((uint32_t)(sc->sc_vram_size / (bus_size_t)sc->sc_stride) - 1U) *
	    (uint32_t)sc->sc_width);

	aprint_verbose_dev(sc->sc_dev, "mode: programming complete\n");
}

static void
mgafb_load_cmap(struct mgafb_softc *sc, u_int start, u_int count)
{
	u_int i;

	mgafb_dac_write(sc, MGA_DAC_PALADDR_W, (uint8_t)start);
	for (i = start; i < start + count; i++) {
		mgafb_dac_write(sc, MGA_DAC_PALDATA, sc->sc_cmap_red[i]);
		mgafb_dac_write(sc, MGA_DAC_PALDATA, sc->sc_cmap_green[i]);
		mgafb_dac_write(sc, MGA_DAC_PALDATA, sc->sc_cmap_blue[i]);
	}
}

static void
mgafb_init_default_cmap(struct mgafb_softc *sc)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri;
	uint8_t cmap[256 * 3];
	int i, idx;

	rasops_get_cmap(ri, cmap, sizeof(cmap));

	idx = 0;
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i]   = cmap[idx++];
		sc->sc_cmap_green[i] = cmap[idx++];
		sc->sc_cmap_blue[i]  = cmap[idx++];
	}

	mgafb_load_cmap(sc, 0, 256);
}

static int
mgafb_putcmap(struct mgafb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	const uint8_t *r = cm->red, *g = cm->green, *b = cm->blue;
	uint8_t rv, gv, bv;
	u_int i;
	int error;

	if (index >= 256 || count > 256 - index)
		return EINVAL;

	for (i = 0; i < count; i++) {
		if ((error = copyin(r++, &rv, 1)) != 0 ||
		    (error = copyin(g++, &gv, 1)) != 0 ||
		    (error = copyin(b++, &bv, 1)) != 0)
			return error;
		sc->sc_cmap_red[index + i]   = rv;
		sc->sc_cmap_green[index + i] = gv;
		sc->sc_cmap_blue[index + i]  = bv;
	}

	mgafb_load_cmap(sc, index, count);
	return 0;
}

static int
mgafb_getcmap(struct mgafb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	uint8_t *r = cm->red, *g = cm->green, *b = cm->blue;
	u_int i;
	int error;

	if (index >= 256 || count > 256 - index)
		return EINVAL;

	for (i = 0; i < count; i++) {
		if ((error = copyout(&sc->sc_cmap_red[index + i],
		    r++, 1)) != 0 ||
		    (error = copyout(&sc->sc_cmap_green[index + i],
		    g++, 1)) != 0 ||
		    (error = copyout(&sc->sc_cmap_blue[index + i],
		    b++, 1)) != 0)
			return error;
	}
	return 0;
}

#ifdef MGAFB_ACCEL

static void
mgafb_wait_fifo(struct mgafb_softc *sc, int n)
{
	while ((int)(uint8_t)MGA_READ1(sc, MGA_FIFOSTATUS) < n)
		;
}

static void
mgafb_wait_idle(struct mgafb_softc *sc)
{
	while (MGA_READ4(sc, MGA_STATUS) & MGA_DWGENGSTS)
		;
}

/* Replicate a pixel colour value to fill FCOL, BCOL */
static uint32_t
mgafb_color_replicate(struct mgafb_softc *sc, uint32_t color)
{
	if (sc->sc_depth == 8) {
		color &= 0xFF;
		return color | (color << 8) | (color << 16) | (color << 24);
	} else {
		color &= 0xFFFF;
		return color | (color << 16);
	}
}

static void
mgafb_fill_rect(struct mgafb_softc *sc, int x, int y, int w, int h,
    uint32_t color)
{
	uint32_t fcol;

	fcol = mgafb_color_replicate(sc, color);

	mgafb_wait_fifo(sc, 4);
	MGA_WRITE4(sc, MGA_DWGCTL, MGA_DWGCTL_FILL);
	MGA_WRITE4(sc, MGA_FCOL,   fcol);
	MGA_WRITE4(sc, MGA_FXBNDRY,
	    ((uint32_t)(x + w - 1) << 16) | (uint32_t)(x & 0xFFFF));
	MGA_WRITE4(sc, MGA_YDSTLEN | MGA_EXEC,
	    ((uint32_t)y << 16) | (uint32_t)h);

	mgafb_wait_idle(sc);
}

static void
mgafb_blit_rect(struct mgafb_softc *sc,
    int srcx, int srcy, int dstx, int dsty, int w, int h)
{
	int pitch = sc->sc_width;
	uint32_t sgn = 0;
	int32_t ar5;
	int src_left, src_right, adj_srcy, adj_dsty;

	adj_srcy = srcy;
	adj_dsty = dsty;

	if (dsty > srcy) {
		sgn |= MGA_SGN_BLIT_UP;
		ar5 = -(int32_t)pitch;
		adj_srcy = srcy + h - 1;
		adj_dsty = dsty + h - 1;
	} else {
		ar5 = (int32_t)pitch;
	}

	if (dsty == srcy && dstx > srcx)
		sgn |= MGA_SGN_BLIT_LEFT;

	src_left  = adj_srcy * pitch + srcx;
	src_right = adj_srcy * pitch + srcx + w - 1;

	mgafb_wait_fifo(sc, 7);
	if ((srcx & 127) == (dstx & 127) && (sgn == 0)) {
		/* fast copy */
		MGA_WRITE4(sc, MGA_DWGCTL, MGA_DWGCTL_FASTCOPY);
	} else {
		MGA_WRITE4(sc, MGA_DWGCTL, MGA_DWGCTL_COPY);
		MGA_WRITE4(sc, MGA_SGN,    sgn);
	}
	MGA_WRITE4(sc, MGA_AR5,    (uint32_t)ar5);
	/* AR3 = scan start, AR0 = scan end */
	if (sgn & MGA_SGN_BLIT_LEFT) {
		MGA_WRITE4(sc, MGA_AR3, (uint32_t)src_right);
		MGA_WRITE4(sc, MGA_AR0, (uint32_t)src_left);
	} else {
		MGA_WRITE4(sc, MGA_AR3, (uint32_t)src_left);
		MGA_WRITE4(sc, MGA_AR0, (uint32_t)src_right);
	}
	MGA_WRITE4(sc, MGA_FXBNDRY,
	    ((uint32_t)(dstx + w - 1) << 16) | (uint32_t)(dstx & 0xFFFF));
	MGA_WRITE4(sc, MGA_YDSTLEN | MGA_EXEC,
	    ((uint32_t)adj_dsty << 16) | (uint32_t)h);

	mgafb_wait_idle(sc);
}

/*
 * The rop parameter is not used; glyphcache always passes gc->gc_rop
 * which we set to 0 (copy).
 */
static void
mgafb_gc_bitblt(void *cookie, int srcx, int srcy, int dstx, int dsty,
    int w, int h, int rop)
{
	mgafb_blit_rect((struct mgafb_softc *)cookie,
	    srcx, srcy, dstx, dsty, w, h);
}

/*
 * Uses ILOAD-with-Expansion to stream the raw 1bpp glyph bitmap from
 * the CPU to the drawing engine, which hardware-expands bits to FCOL
 * or BCOL as it writes to WRAM.
 */
static void
mgafb_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	struct wsdisplay_font *font = ri->ri_font;
	const uint8_t *data;
	int x, y, i, j;
	int32_t fg, bg, ul;
	uint32_t fcol, bcol, dword;
	int glyphidx, stride, dwords_per_row;
	int rv;

	rasops_unpack_attr(attr, &fg, &bg, &ul);

	x = ri->ri_xorigin + col * font->fontwidth;
	y = ri->ri_yorigin + row * font->fontheight;

	/* Unknown character: fill with background. */
	glyphidx = (int)uc - font->firstchar;
	if ((unsigned int)glyphidx >= (unsigned int)font->numchars) {
		mgafb_fill_rect(sc, x, y, font->fontwidth, font->fontheight,
		    (uint32_t)ri->ri_devcmap[bg]);
		return;
	}

	/*
	 * Try the glyph cache first.  Skip for underlined glyphs: the ILOAD
	 * renders the underline as part of the last scanline (all-foreground),
	 * so the cached pixel data would be underlined regardless of attr,
	 * corrupting non-underlined renders of the same character.
	 */
	if (!ul) {
		rv = glyphcache_try(&sc->sc_gc, (int)uc, x, y, attr);
		if (rv == GC_OK)
			return;
	} else {
		rv = GC_NOPE;
	}

	data = (const uint8_t *)font->data + glyphidx * ri->ri_fontscale;
	stride = font->stride;
	dwords_per_row = (font->fontwidth + 31) / 32;

	fcol = mgafb_color_replicate(sc, (uint32_t)ri->ri_devcmap[fg]);
	bcol = mgafb_color_replicate(sc, (uint32_t)ri->ri_devcmap[bg]);

	/* Enable pseudo-DMA BLIT WRITE */
	MGA_WRITE4(sc, MGA_OPMODE, MGA_OPMODE_DMA_BLIT_WR);

	/* Write ILOAD setup registers into the drawing FIFO. */
	mgafb_wait_fifo(sc, 8);
	MGA_WRITE4(sc, MGA_DWGCTL,  MGA_DWGCTL_ILOAD_OPAQUE);
	MGA_WRITE4(sc, MGA_FCOL,    fcol);
	MGA_WRITE4(sc, MGA_BCOL,    bcol);
	MGA_WRITE4(sc, MGA_FXBNDRY,
	    ((uint32_t)(x + font->fontwidth - 1) << 16) | (uint32_t)x);
	MGA_WRITE4(sc, MGA_AR5,     0);
	MGA_WRITE4(sc, MGA_AR3,     0);
	MGA_WRITE4(sc, MGA_AR0,     (uint32_t)(font->fontwidth - 1));
	MGA_WRITE4(sc, MGA_YDSTLEN | MGA_EXEC,
	    ((uint32_t)y << 16) | (uint32_t)font->fontheight);

	/* Stream glyph scanlines into DMAWIN. */
	for (i = 0; i < font->fontheight; i++, data += stride) {
		for (j = 0; j < dwords_per_row; j++) {
			int b = j * 4;	/* byte offset into this row */
			if (ul && i == font->fontheight - 1) {
				dword = 0xFFFFFFFF;
			} else {
				dword  = (b + 0 < stride) ?
				    (uint32_t)data[b + 0]        : 0;
				dword |= (b + 1 < stride) ?
				    (uint32_t)data[b + 1] << 8   : 0;
				dword |= (b + 2 < stride) ?
				    (uint32_t)data[b + 2] << 16  : 0;
				dword |= (b + 3 < stride) ?
				    (uint32_t)data[b + 3] << 24  : 0;
			}
			MGA_WRITE4(sc, MGA_DMAWIN, dword);
		}
	}

	/* Restore OPMODE and wait for the ILOAD to finish writing to WRAM. */
	MGA_WRITE4(sc, MGA_OPMODE, MGA_OPMODE_DMA_OFF);
	mgafb_wait_idle(sc);

	if (rv == GC_ADD)
		glyphcache_add(&sc->sc_gc, (int)uc, x, y);
}

/*
 * Alpha-blends glyph data into 16bpp RBG pixels, 
 * then stream via ILOAD BFCOL.
 * 
 * wsfontload PragmataPro_12x24.wsf
 * wsconsctl -f /dev/ttyE0 -dw font=PragmataPro
 */
static void
mgafb_putchar_aa(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	const uint8_t *data;
	int x, y, wi, he;
	int32_t fg, bg, ul;
	int glyphidx, rv;
	int fgo, bgo;
	int r0, g0, b0, r1, g1, b1;
	uint16_t bg16, fg16;
	uint32_t latch;
	int stride, pi, glyph_row, glyph_col;

	rasops_unpack_attr(attr, &fg, &bg, &ul);

	x = ri->ri_xorigin + col * font->fontwidth;
	y = ri->ri_yorigin + row * font->fontheight;
	wi = font->fontwidth;
	he = font->fontheight;

	/* seemingly  weird blitting artifacts
	if (uc == 0x20) {
		mgafb_fill_rect(sc, x, y, wi, he,
		    (uint32_t)ri->ri_devcmap[bg]);
		if (ul)
			mgafb_fill_rect(sc, x, y + he - 2, wi, 1,
			    (uint32_t)ri->ri_devcmap[fg]);
		return;
	}*/

	/* Unknown character: fill with background. */
	glyphidx = (int)uc - font->firstchar;
	if ((unsigned int)glyphidx >= (unsigned int)font->numchars) {
		mgafb_fill_rect(sc, x, y, wi, he,
		    (uint32_t)ri->ri_devcmap[bg]);
		return;
	}

	/* Glyph cache — skip for underlined characters. */
	if (!ul) {
		rv = glyphcache_try(&sc->sc_gc, (int)uc, x, y, attr);
		if (rv == GC_OK)
			return;
	} else {
		rv = GC_NOPE;
	}

	data = (const uint8_t *)font->data + glyphidx * ri->ri_fontscale;

	/*
	 * Extract 8-bit RGB components from rasops_cmap for blending.
	 * Attr encodes fg index in bits 27:24, bg index in bits 23:16.
	 */
	fgo = ((attr >> 24) & 0xf) * 3;
	bgo = ((attr >> 16) & 0xf) * 3;
	r0 = rasops_cmap[bgo];
	r1 = rasops_cmap[fgo];
	g0 = rasops_cmap[bgo + 1];
	g1 = rasops_cmap[fgo + 1];
	b0 = rasops_cmap[bgo + 2];
	b1 = rasops_cmap[fgo + 2];

	bg16 = (uint16_t)ri->ri_devcmap[bg];
	fg16 = (uint16_t)ri->ri_devcmap[fg];

	/* Set up ILOAD in full-color (BFCOL) mode. */
	MGA_WRITE4(sc, MGA_OPMODE, MGA_OPMODE_DMA_BLIT_WR);

	mgafb_wait_fifo(sc, 6);
	MGA_WRITE4(sc, MGA_DWGCTL, MGA_DWGCTL_ILOAD_COLOR);
	MGA_WRITE4(sc, MGA_FXBNDRY,
	    ((uint32_t)(x + wi - 1) << 16) | (uint32_t)x);
	MGA_WRITE4(sc, MGA_AR5, 0);
	MGA_WRITE4(sc, MGA_AR3, 0);
	MGA_WRITE4(sc, MGA_AR0, (uint32_t)(wi - 1));
	MGA_WRITE4(sc, MGA_YDSTLEN | MGA_EXEC,
	    ((uint32_t)y << 16) | (uint32_t)he);

	/*
	 * Alpha-blend and stream pixels row by row.
	 */
	stride = font->stride;
	latch = 0;
	pi = 0;

	for (glyph_row = 0; glyph_row < he; glyph_row++, data += stride) {
		for (glyph_col = 0; glyph_col < wi; glyph_col++) {
			int aval = data[glyph_col];
			uint16_t pixel;

			if (ul && glyph_row == he - 1) {
				pixel = fg16;
			} else if (aval == 0) {
				pixel = bg16;
			} else if (aval == 255) {
				pixel = fg16;
			} else {
				int r = (aval * r1 + (255 - aval) * r0) >> 8;
				int g = (aval * g1 + (255 - aval) * g0) >> 8;
				int b = (aval * b1 + (255 - aval) * b0) >> 8;
				pixel = ((r >> 3) << 11) |
				    ((g >> 2) << 5) | (b >> 3);
			}

			latch |= (uint32_t)pixel << (16 * pi);
			pi++;
			if (pi == 2) {
				MGA_WRITE4(sc, MGA_DMAWIN, latch);
				latch = 0;
				pi = 0;
			}
		}
	}
	if (pi != 0)
		MGA_WRITE4(sc, MGA_DMAWIN, latch);

	MGA_WRITE4(sc, MGA_OPMODE, MGA_OPMODE_DMA_OFF);
	mgafb_wait_idle(sc);

	if (rv == GC_ADD)
		glyphcache_add(&sc->sc_gc, (int)uc, x, y);
}

static void
mgafb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	int srcY, dstY, w, h;

	srcY = ri->ri_yorigin + srcrow * ri->ri_font->fontheight;
	dstY = ri->ri_yorigin + dstrow * ri->ri_font->fontheight;
	w    = ri->ri_emuwidth;
	h    = nrows * ri->ri_font->fontheight;

	mgafb_blit_rect(sc, ri->ri_xorigin, srcY, ri->ri_xorigin, dstY, w, h);
}

static void
mgafb_eraserows(void *cookie, int row, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	int y, w, h;
	int32_t fg, bg, ul;

	rasops_unpack_attr(attr, &fg, &bg, &ul);

	y = ri->ri_yorigin + row * ri->ri_font->fontheight;
	w = ri->ri_emuwidth;
	h = nrows * ri->ri_font->fontheight;

	mgafb_fill_rect(sc, ri->ri_xorigin, y, w, h, ri->ri_devcmap[bg]);
}

static void
mgafb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	int srcX, dstX, y, w, h;

	srcX = ri->ri_xorigin + srccol * ri->ri_font->fontwidth;
	dstX = ri->ri_xorigin + dstcol * ri->ri_font->fontwidth;
	y    = ri->ri_yorigin + row    * ri->ri_font->fontheight;
	w    = ncols * ri->ri_font->fontwidth;
	h    = ri->ri_font->fontheight;

	mgafb_blit_rect(sc, srcX, y, dstX, y, w, h);
}

static void
mgafb_erasecols(void *cookie, int row, int col, int ncols, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_vd->cookie;
	int x, y, w, h;
	int32_t fg, bg, ul;

	rasops_unpack_attr(attr, &fg, &bg, &ul);

	x = ri->ri_xorigin + col * ri->ri_font->fontwidth;
	y = ri->ri_yorigin + row * ri->ri_font->fontheight;
	w = ncols * ri->ri_font->fontwidth;
	h = ri->ri_font->fontheight;

	mgafb_fill_rect(sc, x, y, w, h, ri->ri_devcmap[bg]);
}

#endif /* MGAFB_ACCEL */

/*
 * The TVP3026 has a 64x64x2-plane hardware cursor with its own RAM,
 * color registers, and position registers, independent of the drawing
 * engine and framebuffer depth. 
 */
static void
mgafb_cursor_enable(struct mgafb_softc *sc, bool on)
{
	uint8_t v;

	v = mgafb_dac_read_ind(sc, MGA_TVP_CURCTL_IND);
	v &= ~MGA_TVP_CURCTL_CMASK;
	if (on)
		v |= MGA_TVP_CURCTL_XGA;
	mgafb_dac_write_ind(sc, MGA_TVP_CURCTL_IND, v);
	sc->sc_cursor.mc_enabled = on;
}

static void
mgafb_cursor_setpos(struct mgafb_softc *sc, int x, int y)
{
	int px, py;

	px = x + MGA_CURSOR_ORIGIN;
	py = y + MGA_CURSOR_ORIGIN;
	if (px < 0)
		px = 0;
	if (py < 0)
		py = 0;

	mgafb_dac_write(sc, MGA_DAC_CUR_X_LSB, px & 0xFF);
	mgafb_dac_write(sc, MGA_DAC_CUR_X_MSB, (px >> 8) & 0x0F);
	mgafb_dac_write(sc, MGA_DAC_CUR_Y_LSB, py & 0xFF);
	mgafb_dac_write(sc, MGA_DAC_CUR_Y_MSB, (py >> 8) & 0x0F);
}

static void
mgafb_cursor_setcmap(struct mgafb_softc *sc)
{

	/* Cursor color 0 (background) at address 1,
	 * cursor color 1 (foreground) at address 2.
	 * Address auto-increments after each blue byte. */
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_ADDR_W, 1);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_r[0]);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_g[0]);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_b[0]);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_r[1]);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_g[1]);
	mgafb_dac_write(sc, MGA_DAC_CUR_COL_DATA, sc->sc_cursor.mc_b[1]);
}

static void
mgafb_cursor_setshape(struct mgafb_softc *sc, int width, int height)
{
	uint8_t v, rowbyte;
	int row, col;

	/* Disable cursor during RAM load to prevent glitches. */
	mgafb_cursor_enable(sc, false);

	/*
	 * Clear CCR7 (use indirect control) and CCR3:CCR2 (bank 00).
	 * Then reset cursor RAM address to 0. Auto-increment wraps
	 * through all 1024 bytes (plane 0 then plane 1).
	 */
	v = mgafb_dac_read_ind(sc, MGA_TVP_CURCTL_IND);
	v &= ~(MGA_TVP_CURCTL_BMASK | MGA_TVP_CURCTL_CMASK |
	    MGA_TVP_CURCTL_CCR7);
	mgafb_dac_write_ind(sc, MGA_TVP_CURCTL_IND, v);

	mgafb_dac_write(sc, MGA_DAC_PALADDR_W, 0x00);

	/* Build a complement (XOR) block cursor using XGA mode.*/

	/* Plane 0 (image): set bits for cursor block area. */
	for (row = 0; row < MGA_CURSOR_MAX; row++) {
		for (col = 0; col < 8; col++) {
			if (row < height && col < (width + 7) / 8) {
				int bits_in_byte = width - col * 8;
				if (bits_in_byte >= 8)
					rowbyte = 0xFF;
				else
					rowbyte = (uint8_t)(0xFF <<
					    (8 - bits_in_byte));
			} else {
				rowbyte = 0x00;
			}
			mgafb_dac_write(sc, MGA_DAC_CUR_DATA, rowbyte);
		}
	}

	/* Plane 1 (mask): all 1s — makes non-cursor pixels transparent,
	 * cursor pixels complemented (XOR with screen). */
	for (row = 0; row < MGA_CURSOR_MAX; row++) {
		for (col = 0; col < 8; col++)
			mgafb_dac_write(sc, MGA_DAC_CUR_DATA, 0xFF);
	}
}

static void
mgafb_cursor_init(struct mgafb_softc *sc)
{
	uint8_t v;

	memset(&sc->sc_cursor, 0, sizeof(sc->sc_cursor));

	sc->sc_cursor.mc_r[0] = sc->sc_cursor.mc_r[1] = 0xFF;
	sc->sc_cursor.mc_g[0] = sc->sc_cursor.mc_g[1] = 0xFF;
	sc->sc_cursor.mc_b[0] = sc->sc_cursor.mc_b[1] = 0xFF;

	/* Ensure CCR7=0 so indirect CCR (reg 0x06) is in control. */
	v = mgafb_dac_read_ind(sc, MGA_TVP_CURCTL_IND);
	v &= ~(MGA_TVP_CURCTL_CCR7 | MGA_TVP_CURCTL_CMASK);
	mgafb_dac_write_ind(sc, MGA_TVP_CURCTL_IND, v);

	mgafb_cursor_setcmap(sc);
	/* Reload when font is known */
	mgafb_cursor_setshape(sc, 8, 16);
	mgafb_cursor_setpos(sc, 0, 0);
}

static void
mgafb_hw_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mgafb_softc *sc = scr->scr_cookie;

	if (on) {
		int x = col * ri->ri_font->fontwidth + ri->ri_xorigin;
		int y = row * ri->ri_font->fontheight + ri->ri_yorigin;

		ri->ri_crow = row;
		ri->ri_ccol = col;
		ri->ri_flg |= RI_CURSOR;

		mgafb_cursor_setpos(sc, x, y);
		mgafb_cursor_enable(sc, true);
	} else {
		ri->ri_flg &= ~RI_CURSOR;
		mgafb_cursor_enable(sc, false);
	}
}

static void
mgafb_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct mgafb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	wsfont_init();

	ri->ri_depth  = sc->sc_depth;
	ri->ri_width  = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg    = RI_CENTER | RI_CLEAR;
	if (sc->sc_depth == 16)
		ri->ri_flg |= RI_ENABLE_ALPHA | RI_PREFER_ALPHA;

	/* Uhh... */
#if BYTE_ORDER == BIG_ENDIAN
	if (sc->sc_depth > 8)
		ri->ri_flg |= RI_BSWAP;
#endif
	/* tested only for aa path */
	scr->scr_flags |= VCONS_LOADFONT;

	if (sc->sc_depth == 16) {
		/* 16bpp 5:6:5 */
		ri->ri_rnum = 5;  ri->ri_rpos = 11;
		ri->ri_gnum = 6;  ri->ri_gpos = 5;
		ri->ri_bnum = 5;  ri->ri_bpos = 0;
	}
	ri->ri_bits = bus_space_vaddr(sc->sc_fbt, sc->sc_fbh);

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_RESIZE
		| WSSCREEN_UNDERLINE ;
	rasops_reconfig(ri,
	    sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width  / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

#ifdef MGAFB_ACCEL
	aprint_verbose_dev(sc->sc_dev,
	    "font: %dx%d stride=%d %s\n",
	    ri->ri_font->fontwidth, ri->ri_font->fontheight,
	    ri->ri_font->stride,
	    FONT_IS_ALPHA(ri->ri_font) ? "alpha" : "bitmap");

	if (sc->sc_depth == 16 && FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = mgafb_putchar_aa;
		aprint_normal_dev(sc->sc_dev,
		    "using antialiasing\n");
	} else {
		ri->ri_ops.putchar = mgafb_putchar;
	}
	ri->ri_ops.copyrows  = mgafb_copyrows;
	ri->ri_ops.eraserows = mgafb_eraserows;
	ri->ri_ops.copycols  = mgafb_copycols;
	ri->ri_ops.erasecols = mgafb_erasecols;

#endif /* MGAFB_ACCEL */

	if (sc->sc_ci->ci_has_tvp3026) {
		ri->ri_ops.cursor = mgafb_hw_cursor;
		mgafb_cursor_setshape(sc, ri->ri_font->fontwidth,
		    ri->ri_font->fontheight);
	}
}

/* screenblank -b / -u confirmed working */
static void
mgafb_set_dpms(struct mgafb_softc *sc, int state)
{
	uint8_t seq1, crtcext1;

	sc->sc_video = state;

	MGA_WRITE1(sc, MGA_VGA_SEQ_INDEX, 0x01);
	seq1 = MGA_READ1(sc, MGA_VGA_SEQ_DATA) & ~0x20;

	MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, 0x01);
	crtcext1 = MGA_READ1(sc, MGA_CRTCEXT_DATA) & ~0x30;

	if (state == WSDISPLAYIO_VIDEO_OFF) {
		seq1 |= 0x20;		/* screen blank */
		crtcext1 |= 0x30;	/* hsync + vsync off */
	}

	MGA_WRITE1(sc, MGA_VGA_SEQ_INDEX, 0x01);
	MGA_WRITE1(sc, MGA_VGA_SEQ_DATA, seq1);

	MGA_WRITE1(sc, MGA_CRTCEXT_INDEX, 0x01);
	MGA_WRITE1(sc, MGA_CRTCEXT_DATA, crtcext1);
}

static int
mgafb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct mgafb_softc *sc = vd->cookie;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;
		{
			struct wsdisplay_fbinfo *wsfbi = data;
			wsfbi->height = ms->scr_ri.ri_height;
			wsfbi->width  = ms->scr_ri.ri_width;
			wsfbi->depth  = ms->scr_ri.ri_depth;
			wsfbi->cmsize = (sc->sc_depth == 8) ? 256 : 0;
		}
		return 0;

	case WSDISPLAYIO_PUTCMAP:
		if (sc->sc_depth != 8)
			return EINVAL;
		return mgafb_putcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_GETCMAP:
		if (sc->sc_depth != 8)
			return EINVAL;
		return mgafb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = sc->sc_video;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		mgafb_set_dpms(sc, *(int *)data);
		return 0;

	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int *)data;
			if (new_mode != sc->sc_mode) {
				sc->sc_mode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					if (sc->sc_ci->ci_has_tvp3026)
						mgafb_cursor_init(sc);
					vcons_redraw_screen(ms);
				} else {
#ifdef MGAFB_ACCEL
					mgafb_wait_idle(sc);
#endif
					if (sc->sc_ci->ci_has_tvp3026)
						mgafb_cursor_enable(sc,
						    false);
				}
			}
		}
		return 0;

	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
		}

	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_pcitag, data);
	}

	return EPASSTHROUGH;
}

static paddr_t
mgafb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct mgafb_softc *sc = vd->cookie;

	if (sc->sc_mode == WSDISPLAYIO_MODE_DUMBFB) {
		if (offset >= 0 && offset < (off_t)sc->sc_vram_size)
			return bus_space_mmap(sc->sc_fbt, sc->sc_fb_pa,
			    offset, prot,
			    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
	} else if (sc->sc_mode == WSDISPLAYIO_MODE_MAPPED) {
		if (kauth_authorize_machdep(kauth_cred_get(),
		    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0) {
			aprint_error_dev(sc->sc_dev, "mmap() rejected\n");
			return -1;
		}

		/* MGABASE2: framebuffer aperture. */
		if (offset >= (off_t)sc->sc_fb_pa &&
		    offset < (off_t)(sc->sc_fb_pa + sc->sc_fb_size))
			return bus_space_mmap(sc->sc_fbt, offset, 0, prot,
			    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);

		/* MGABASE1: control aperture / registers. */
		if (offset >= (off_t)sc->sc_reg_pa &&
		    offset < (off_t)(sc->sc_reg_pa + sc->sc_reg_size))
			return bus_space_mmap(sc->sc_regt, offset, 0, prot,
			    BUS_SPACE_MAP_LINEAR);
	}

	return -1;
}
