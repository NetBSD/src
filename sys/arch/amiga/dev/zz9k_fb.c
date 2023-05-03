/*	$NetBSD: zz9k_fb.c,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zz9k_fb.c,v 1.1 2023/05/03 13:49:30 phx Exp $");

/* miscellaneous */
#include <sys/errno.h>			/* EPASSTHROUGH */
#include <sys/types.h>			/* size_t */
#include <sys/stdint.h>			/* uintXX_t */
#include <sys/stdbool.h>		/* bool */

/* driver(9) */
#include <sys/param.h>      		/* NODEV */
#include <sys/device.h>     		/* CFATTACH_DECL_NEW(), device_priv() */
#include <sys/errno.h>			/* EINVAL, ENODEV, EPASSTHROUGH */

/* bus_space(9) and zorro bus */
#include <sys/bus.h>			/* bus_space_xxx(), bus_space_xxx_t */
#include <sys/cpu.h>			/* kvtop() */
#include <sys/systm.h>			/* aprint_xxx() */

/* wsdisplay(9) */
#include <dev/wscons/wsconsio.h>	/* WSDISPLAYIO_XXX, wsdisplayio_xxx */
#include <dev/wscons/wsdisplayvar.h>	/* wsscreen_xxx, wsdisplay_xxx */
#include <dev/wscons/wsemulvar.h>	/* ? */
#include <dev/wscons/wsemul_vt100var.h>	/* ? */

/* rasops(9) */
/* #include <dev/wscons/wsdisplayvar.h> */
#include <dev/rasops/rasops.h>		/* rasops_unpack_attr(), rasops_info */

/* wsfont(9) */
/* #include <dev/wscons/wsconsio.h> */	/* WSDISPLAYIO_XXX, wsdisplayio_xxx */
#include <dev/wsfont/wsfont.h>		/* wsfont_init() */

/* vcons(9) */
#include <dev/wscons/wsdisplay_vconsvar.h>	/* vcons_xxx(), vcons_data,
						vcons_screen */
/* cons(9) */
#include <dev/cons.h>			/* consdev, CN_INTERNAL */

/* zz9k and amiga related */
#include <amiga/dev/kbdvar.h>		/* kbd_cnattach() */
#include <amiga/dev/zz9kvar.h>		/* zz9kbus_attach_args */
#include <amiga/dev/zz9kreg.h>		/* ZZ9000 registers */
#include "opt_zz9k_fb.h"		/* ZZFB_CONSOLE */
#include "zz9k_fb.h"			/* NZZ9K_FB */
#include "kbd.h"			/* NKBD */


/*
 * One can choose different graphics modes and color depths for the different
 * wsdisplay modes emul (console), mapped (raw) and dumbfb (e.g. X11) here.
 * Please consult zz9kreg.h for available graphics modes and color depths
 * supported by the ZZ9000.
 */
#define ZZFB_SET_CON_MODE	ZZ9K_MODE_1280x720	/* Console */
#define ZZFB_SET_CON_BPP	ZZ9K_COLOR_8BIT
#define ZZFB_SET_GFX_MODE	ZZ9K_MODE_1280x720	/* raw FB */
#define ZZFB_SET_GFX_BPP	ZZ9K_COLOR_16BIT
#define ZZFB_SET_DFB_MODE	ZZ9K_MODE_1280x720	/* X11 */
#define ZZFB_SET_DFB_BPP	ZZ9K_COLOR_16BIT

/*
 * This defines ZZ9000 scandoubler's capture mode and it is used only in case
 * the ZZ9000 does not have the early console, but also in X11 video off mode.
 * NetBSD defaults to NTSC amiga screen, so the scandoubler default is NTSC too.
 * On a custom configured kernel for a PAL screen, one should consider to change
 * the capture mode to PAL to get the best result. If the attached monitor does
 * not support a scandoubled PAL signal in 50Hz, consider using the VGA 800x600
 * capture mode which is compatible with most monitors and works fine with NTSC
 * and PAL captures at 60Hz.
 * Valid values for ZZFB_CAP_MODE: 0: NTSC, 1: PAL, 2:VGA (PAL60)
 */
#define ZZFB_CAP_MODE 0

#if   ZZFB_CAP_MODE == 0
#define ZZFB_CAPTURE_MODE	ZZ9K_MODE_720x480
#define ZZFB_DISPLAY_MODE	ZZ9K_MODE_720x480
#elif ZZFB_CAP_MODE == 1
#define ZZFB_CAPTURE_MODE	ZZ9K_MODE_720x576p50
#define ZZFB_DISPLAY_MODE	ZZ9K_MODE_720x576p50
#elif ZZFB_CAP_MODE == 2
#define ZZFB_CAPTURE_MODE	ZZ9K_MODE_800x600
#define ZZFB_DISPLAY_MODE	ZZ9K_MODE_800x600
#endif

/* The allmighty softc structure */
struct zzfb_softc {
	device_t sc_dev;
	struct bus_space_tag sc_bst;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_fbh;
	size_t sc_fbsize;

	struct vcons_screen sc_console_screen;
	struct vcons_data sc_vd;
	struct wsscreen_descr sc_defaultscreen;
	struct wsscreen_list sc_screenlist;
	const struct wsscreen_descr *sc_screens[1];
	u_int sc_wsmode;

	uint16_t sc_displaymode;
	uint16_t sc_colormode;
	uint16_t sc_width;
	uint16_t sc_height;
	uint16_t sc_bpp;
	uint16_t sc_stride;

	u_char red[ZZ9K_PALETTE_SIZE];
	u_char green[ZZ9K_PALETTE_SIZE];
	u_char blue[ZZ9K_PALETTE_SIZE];

	bool sc_isconsole;
	bool sc_isrtg;
};

static const struct {
	const char* name;
	uint16_t width;
	uint16_t height;
	uint16_t scale;
} zzfb_modes[] = {				/* Hardcoded in firmware */
	{       "1280x720p60", 1280,  720, ZZ9K_MODE_SCALE_0},
	{        "800x600p60",  800,  600, ZZ9K_MODE_SCALE_0},
	{        "640x480p60",  640,  480, ZZ9K_MODE_SCALE_0},
	{       "1024x768p60", 1024,  768, ZZ9K_MODE_SCALE_0},
	{      "1280x1024p60", 1280, 1024, ZZ9K_MODE_SCALE_0},
	{      "1920x1080p60", 1920, 1080, ZZ9K_MODE_SCALE_0},
	{        "720x576p50",  720,  576, ZZ9K_MODE_SCALE_0},	/* 50 Hz */
	{      "1920x1080p50", 1920, 1080, ZZ9K_MODE_SCALE_0},	/* 50 Hz */
	{        "720x480p60",  720,  480, ZZ9K_MODE_SCALE_0},
	{        "640x512p60",  640,  512, ZZ9K_MODE_SCALE_0},
	{      "1600x1200p60", 1600, 1200, ZZ9K_MODE_SCALE_0},
	{      "2560x1444p30", 2560, 1444, ZZ9K_MODE_SCALE_0},	/* 30 Hz */
	{ "720x576p50-NS-PAL",  720,  576, ZZ9K_MODE_SCALE_0},	/* 50 Hz */
	{ "720x480p60-NS-PAL",  720,  480, ZZ9K_MODE_SCALE_0},
	{"720x576p50-NS-NTSC",  720,  576, ZZ9K_MODE_SCALE_0},	/* 50 Hz */
	{"720x480p60-NS-NTSC",  720,  480, ZZ9K_MODE_SCALE_0},
	{        "640x400p60",  640,  400, ZZ9K_MODE_SCALE_0},
	{       "1920x800p60",  640,  400, ZZ9K_MODE_SCALE_0}
};

static const struct {
	const char* name;
	uint16_t bpp;
	uint16_t mode;
	uint16_t stride;
} zzfb_colors[] = {				/* Hardcoded in firmware */
	{      "8-bit LUT",  8, ZZ9K_MODE_COLOR_8BIT , 1},
	{  "16-bit RGB565", 16, ZZ9K_MODE_COLOR_16BIT, 2},
	{"32-bit BGRA8888", 32, ZZ9K_MODE_COLOR_32BIT, 4},
	{"16-bit ARGB1555", 15, ZZ9K_MODE_COLOR_15BIT, 2}
};

#define ZZFB_MODES_SIZE		(sizeof zzfb_modes / sizeof zzfb_modes[0])
#define ZZFB_COLORS_SIZE	(sizeof zzfb_colors / sizeof zzfb_colors[0])

/* functions to set gfx mode, palette and misc stuff to make life easier. */
static void zzfb_set_capture(struct zzfb_softc *sc, uint16_t display_mode,
    uint16_t capture_mode);
static void zzfb_set_mode(struct zzfb_softc *sc, uint16_t display_mode,
    uint16_t color_mode);
static void zzfb_wait_vblank(struct zzfb_softc *sc);
static void zzfb_init_palette(struct zzfb_softc *sc);
static void zzfb_set_palette(struct zzfb_softc *sc);
static void zzfb_clearbg(struct zzfb_softc *sc, uint8_t color_index);
static int zzfb_get_fbinfo(struct zzfb_softc *sc,
    struct wsdisplayio_fbinfo *fbi);

/* vcons_data init_screen function */
static void zzfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr);

/* accelerated raster ops functions */
static void zzfb_eraserows(void *cookie, int row, int nrows, long fillattr);
static void zzfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows);
static void zzfb_copycols(void *cookie, int row, int srccol, int dstcol,
    int ncols);
static void zzfb_erasecols(void *cookie, int row, int startcol, int ncols,
    long fillattr);

/* blitter support functions*/
static void zzfb_rectfill(struct zzfb_softc *sc, uint16_t x, uint16_t y,
    uint16_t w, uint16_t h, uint32_t color);
static void zzfb_bitblt(struct zzfb_softc *sc, uint16_t x, uint16_t y,
    uint16_t w, uint16_t h, uint16_t xs, uint16_t ys);

/* wsdisplay_accessops stuff */
static paddr_t zzfb_mmap(void *v, void *vs, off_t offset, int prot);
static int zzfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l);
static int zzfb_getcmap(struct zzfb_softc *sc, struct wsdisplay_cmap *cm);
static int zzfb_putcmap(struct zzfb_softc *sc, struct wsdisplay_cmap *cm);
struct wsdisplay_accessops zzfb_accessops =
    {zzfb_ioctl, zzfb_mmap, NULL, NULL, NULL, NULL, NULL, NULL};

/* driver(9) essentials */
static int zzfb_match(device_t parent, cfdata_t match, void *aux);
static void zzfb_attach(device_t parent, device_t self, void *aux);
CFATTACH_DECL_NEW(
    zz9k_fb, sizeof(struct zzfb_softc), zzfb_match, zzfb_attach, NULL, NULL );

#ifdef ZZFB_CONSOLE
extern bool zz9k_exists;
#endif /* ZZFB_CONSOLE */


/* If you build it, they will come... */

static int
zzfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;
	
	if (strcmp(bap->zzaa_name, "zz9k_fb") != 0) {
		return 0;
	}

	return 1;
}

static void
zzfb_attach(device_t parent, device_t self, void *aux)
{
	struct zz9kbus_attach_args *bap = aux;
	struct zzfb_softc *sc = device_private(self);
	struct zz9k_softc *psc = device_private(parent);
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args ws_aa;
	long defattr;

	sc->sc_dev = self;
	sc->sc_bst.base = bap->zzaa_base;
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;
	sc->sc_regh = psc->sc_regh;

	if (psc->sc_zsize >= (ZZ9K_FB_BASE + ZZ9K_FB_SIZE)) {
		sc->sc_fbsize = ZZ9K_FB_SIZE;
	} else {
		sc->sc_fbsize = psc->sc_zsize - ZZ9K_FB_BASE;
	}

	if (bus_space_map(sc->sc_iot, ZZ9K_FB_BASE, sc->sc_fbsize,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_fbh)) {
		aprint_error(": Failed to map MNT ZZ9000 framebuffer.\n");
		return;
	}

	zzfb_set_mode(sc, ZZFB_SET_CON_MODE, ZZFB_SET_CON_BPP);
    	zzfb_init_palette(sc);
	zzfb_clearbg(sc, WS_DEFAULT_BG);

	aprint_normal(": Framebuffer resolution: %s, "
	    "depth: %i bpp (%s)\n", zzfb_modes[sc->sc_displaymode].name,
	    sc->sc_bpp, zzfb_colors[sc->sc_colormode].name);

	aprint_debug_dev(sc->sc_dev, "[DEBUG] registers at %p/%p (pa/va), "
	    "framebuffer at %p/%p (pa/va) with %i MB\n",
	    (void *)kvtop((void *)sc->sc_regh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_regh),
	    (void *)kvtop((void *)sc->sc_fbh),
	    bus_space_vaddr(sc->sc_iot, sc->sc_fbh),
	    sc->sc_fbsize / (1024 * 1024));

	sc->sc_defaultscreen = (struct wsscreen_descr) {"default", 0, 0, NULL,
	    8, 16, WSSCREEN_WSCOLORS | WSSCREEN_HILIT, NULL};
	sc->sc_screens[0] = &sc->sc_defaultscreen;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;

	vcons_init(&sc->sc_vd, sc, &sc->sc_defaultscreen, &zzfb_accessops);
	sc->sc_vd.init_screen = zzfb_init_screen;

	ri = &sc->sc_console_screen.scr_ri;

#ifdef ZZFB_CONSOLE
	sc->sc_isconsole = true;
	vcons_init_screen(&sc->sc_vd, &sc->sc_console_screen, 1,
	    &defattr);
	sc->sc_console_screen.scr_flags = VCONS_SCREEN_IS_STATIC;
	vcons_redraw_screen(&sc->sc_console_screen);
		
	sc->sc_defaultscreen.textops = &ri->ri_ops;
	sc->sc_defaultscreen.capabilities = ri->ri_caps;
	sc->sc_defaultscreen.nrows = ri->ri_rows;
	sc->sc_defaultscreen.ncols = ri->ri_cols;
		
	wsdisplay_cnattach(&sc->sc_defaultscreen, ri, 0, 0, defattr);
	vcons_replay_msgbuf(&sc->sc_console_screen);
#else
	sc->sc_isconsole = false;
	if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
		vcons_init_screen(&sc->sc_vd, &sc->sc_console_screen, 1,
		    &defattr);
	} else {
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}
	zzfb_set_capture(sc, ZZFB_DISPLAY_MODE, ZZFB_CAPTURE_MODE);
	aprint_normal_dev (sc->sc_dev, "Scandoubler capture: %s, "
	    "display: %s in %s.\n",
	    zzfb_modes[ZZFB_CAPTURE_MODE].name,
	    zzfb_modes[ZZFB_DISPLAY_MODE].name,
	    zzfb_colors[ZZ9K_COLOR_32BIT].name);
#endif /* ZZFB_CONSOLE */

	ws_aa.console = sc->sc_isconsole;
	ws_aa.scrdata = &sc->sc_screenlist;
	ws_aa.accessops = &zzfb_accessops;
	ws_aa.accesscookie = &sc->sc_vd;

	config_found(sc->sc_dev, &ws_aa, wsemuldisplaydevprint, CFARGS_NONE);
}

static void
zzfb_init_palette(struct zzfb_softc *sc)
{
	for (int index = 0; index < ZZ9K_PALETTE_SIZE; index++) {
		sc->red[index] = rasops_cmap[index * 3 + 0];
		sc->green[index] = rasops_cmap[index * 3 + 1];
		sc->blue[index] = rasops_cmap[index * 3 + 2];
	}

	zzfb_set_palette(sc);
}

static void
zzfb_set_palette(struct zzfb_softc *sc)
{
	uint32_t palette;
	uint8_t rVal;
	uint8_t gVal;
	uint8_t bVal;

	for (int index = 0; index < ZZ9K_PALETTE_SIZE; index++) {
		rVal = sc->red[index];
		gVal = sc->green[index];
		bVal = sc->blue[index];
		palette = ((index << 24) | (rVal << 16) | (gVal << 8) | bVal);
		ZZREG_W(ZZ9K_VIDEO_CTRL_DATA_HI, palette >> 16);
		ZZREG_W(ZZ9K_VIDEO_CTRL_DATA_LO, palette & 0xFFFF);
		ZZREG_W(ZZ9K_VIDEO_CTRL_OP, ZZ9K_OP_PALETTE);
		ZZREG_W(ZZ9K_VIDEO_CTRL_OP, ZZ9K_OP_NOP);
	}
}

static void
zzfb_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct zzfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	scr->scr_flags = VCONS_SCREEN_IS_STATIC;

	wsfont_init();

	ri->ri_bits = (u_char *)bus_space_vaddr(sc->sc_iot, sc->sc_fbh);
	ri->ri_depth = sc->sc_bpp;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = 0;

	if (ri->ri_depth == 32) {	/* adjust for BGRA8888 */
		ri->ri_rnum = 8;	/* for other depths default is OK */
		ri->ri_gnum = 8;
		ri->ri_bnum = 8;
		ri->ri_rpos = 8;	/* skip over alpha channel */
		ri->ri_gpos = 8 + ri->ri_rnum;
		ri->ri_bpos = 8 + ri->ri_rnum + ri->ri_gnum;
	}

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);
	ri->ri_hw = scr;

	ri->ri_ops.eraserows = zzfb_eraserows;
	ri->ri_ops.copyrows = zzfb_copyrows;
	ri->ri_ops.erasecols = zzfb_erasecols;
	ri->ri_ops.copycols = zzfb_copycols;
}

static void
zzfb_set_capture(struct zzfb_softc *sc, uint16_t display_mode,
    uint16_t capture_mode)
{
	uint16_t panPtrHi;
	uint16_t panPtrLo;
	uint16_t new_mode;

	switch (display_mode) {
	case ZZ9K_MODE_720x480:		/* NTSC */
		panPtrHi = ZZ9K_CAPTURE_PAN_NTSC >> 16;
		panPtrLo = ZZ9K_CAPTURE_PAN_NTSC & 0xFFFF;
		break;
	case ZZ9K_MODE_720x576p50:	/* PAL */
		panPtrHi = ZZ9K_CAPTURE_PAN_PAL >> 16;
		panPtrLo = ZZ9K_CAPTURE_PAN_PAL & 0xFFFF;
		break;
	case ZZ9K_MODE_800x600:		/* VGA */
		panPtrHi = ZZ9K_CAPTURE_PAN_VGA >> 16;
		panPtrLo = ZZ9K_CAPTURE_PAN_VGA & 0xFFFF;
		break;
	default:
		aprint_error_dev(sc->sc_dev, "Unsupported scandoubler "
		    "capture and display mode combination.\n");
		return;
	}
	new_mode = ZZ9K_MODE_SCALE_2 | ZZ9K_MODE_COLOR_32BIT | display_mode;
	zzfb_wait_vblank(sc);
	ZZREG_W(ZZ9K_VIDEOCAP_VMODE, capture_mode);
	ZZREG_W(ZZ9K_BLITTER_USER1, ZZ9K_FEATURE_NONSTANDARD_VSYNC);
	ZZREG_W(ZZ9K_SET_FEATURE, 0x0000);
	ZZREG_W(ZZ9K_VIDEO_CAPTURE_MODE, ZZ9K_CAPTURE_ON);
	ZZREG_W(ZZ9K_BLITTER_X1, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_Y1, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_X2, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_COLORMODE, ZZ9K_COLOR_32BIT);
	ZZREG_W(ZZ9K_PAN_PTR_HI, panPtrHi);
	ZZREG_W(ZZ9K_PAN_PTR_LO, panPtrLo);
	ZZREG_W(ZZ9K_MODE, new_mode);
	sc->sc_isrtg = false;
}

void
zzfb_set_mode(struct zzfb_softc *sc, uint16_t display_mode,
    uint16_t color_mode)
{
	uint16_t new_mode;

	if ((display_mode < 0) || (display_mode >= ZZFB_MODES_SIZE))
		display_mode = ZZ9K_MODE_1280x720;

	sc->sc_width	= zzfb_modes[display_mode].width;
	sc->sc_height	= zzfb_modes[display_mode].height;
	new_mode	= zzfb_modes[display_mode].scale;
	
	if ((color_mode < 0) || (color_mode >= ZZFB_COLORS_SIZE))
		color_mode = ZZ9K_COLOR_8BIT;
		
	if ((color_mode == ZZ9K_COLOR_32BIT) && (sc->sc_width > 1920))
		color_mode = ZZ9K_COLOR_16BIT;

	sc->sc_bpp    = zzfb_colors[color_mode].bpp;
	sc->sc_stride = sc->sc_width * zzfb_colors[color_mode].stride;
	new_mode      = new_mode | zzfb_colors[color_mode].mode | display_mode;

	sc->sc_displaymode = display_mode;
	sc->sc_colormode   = color_mode;

	zzfb_wait_vblank(sc);
	ZZREG_W(ZZ9K_VIDEO_CAPTURE_MODE, ZZ9K_CAPTURE_OFF);
	ZZREG_W(ZZ9K_BLITTER_X1, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_Y1, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_X2, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_COLORMODE, color_mode);
	ZZREG_W(ZZ9K_BLITTER_SRC_HI, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_SRC_LO, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_DST_HI, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_DST_LO, 0x0000);
	ZZREG_W(ZZ9K_BLITTER_SPLIT_POS, 0x0000);
	ZZREG_W(ZZ9K_PAN_PTR_HI, 0x0000);
	ZZREG_W(ZZ9K_PAN_PTR_LO, 0x0000);
	ZZREG_W(ZZ9K_MODE, new_mode);
	sc->sc_isrtg = true;
}

static void
zzfb_wait_vblank(struct zzfb_softc *sc)
{
	uint16_t vb_status = ZZREG_R(ZZ9K_VIDEO_BLANK_STATUS);
	while (vb_status != 0)
		vb_status = ZZREG_R(ZZ9K_VIDEO_BLANK_STATUS);
	while (vb_status == 0)
		vb_status = ZZREG_R(ZZ9K_VIDEO_BLANK_STATUS);
}

static void
zzfb_clearbg(struct zzfb_softc *sc, uint8_t color_index)
{
	if (color_index >= ZZ9K_PALETTE_SIZE)
		color_index = 0;
	
	uint32_t palette = 0;
	uint8_t rVal = rasops_cmap[color_index * 3 + 0];
	uint8_t gVal = rasops_cmap[color_index * 3 + 1];
	uint8_t bVal = rasops_cmap[color_index * 3 + 2];

	switch (sc->sc_colormode) {
	case ZZ9K_COLOR_32BIT:	/* BGRA8888 */
		palette = ((bVal << 24) | (gVal << 16) | (rVal << 8) | 0xFF);
		break;
	case ZZ9K_COLOR_16BIT: /* RGB565 at high word, don't ask why. */
		palette = (((rVal & 0xF8) << 8) |
			   ((gVal & 0xFC) << 3) |
			   ((bVal >> 3) & 0x1F) ) << 16;
		break;
	case ZZ9K_COLOR_15BIT: /* ARGB1555 at high word, don't ask why. */
		palette = ((0x8000) |
			   ((rVal & 0xF8) << 7) |
			   ((gVal & 0xF8) << 2) |
			   ((bVal >> 3) & 0x1F) ) << 16;
		break;
	case ZZ9K_COLOR_8BIT: /* 256 LUT */
	default:
		palette = color_index;
		break;
	}

	zzfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height, palette);
}

static void
zzfb_rectfill(struct zzfb_softc *sc, uint16_t x, uint16_t y,
    uint16_t w, uint16_t h, uint32_t color)
{
	ZZREG_W(ZZ9K_BLITTER_X1, x);
	ZZREG_W(ZZ9K_BLITTER_Y1, y);
	ZZREG_W(ZZ9K_BLITTER_X2, w);
	ZZREG_W(ZZ9K_BLITTER_Y2, h);
	ZZREG_W(ZZ9K_BLITTER_ROW_PITCH, sc->sc_stride >> 2);
	ZZREG_W(ZZ9K_BLITTER_COLORMODE, sc->sc_colormode);
	ZZREG_W(ZZ9K_BLITTER_RGB_HI, color >> 16);
	ZZREG_W(ZZ9K_BLITTER_RGB_LO, color & 0xFFFF);
	ZZREG_W(ZZ9K_BLITTER_OP_FILLRECT, 0x00FF);
}

static void
zzfb_bitblt(struct zzfb_softc *sc, uint16_t x, uint16_t y, uint16_t w,
    uint16_t h, uint16_t xs, uint16_t ys)
{
	ZZREG_W(ZZ9K_BLITTER_X1, x);
	ZZREG_W(ZZ9K_BLITTER_Y1, y);
	ZZREG_W(ZZ9K_BLITTER_X2, w);
	ZZREG_W(ZZ9K_BLITTER_Y2, h);
	ZZREG_W(ZZ9K_BLITTER_X3, xs);
	ZZREG_W(ZZ9K_BLITTER_Y3, ys);
	ZZREG_W(ZZ9K_BLITTER_ROW_PITCH, sc->sc_stride >> 2);
	ZZREG_W(ZZ9K_BLITTER_COLORMODE, (0xFF << 8) | sc->sc_colormode);
	ZZREG_W(ZZ9K_BLITTER_OP_COPYRECT, ZZ9K_OPT_REGULAR);
}

static void
zzfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct zzfb_softc *sc = scr->scr_cookie;
	int x, y, w, h, ys;

	if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		y = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		w = ri->ri_emuwidth;
		h = ri->ri_font->fontheight * nrows;
		zzfb_bitblt(sc, x, y, w, h, x, ys);
	}
}

static void
zzfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct zzfb_softc *sc = scr->scr_cookie;
	int x, y, w, h, fg, bg, ul;

	if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		w = ri->ri_emuwidth;
		h = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);
		zzfb_rectfill(sc, x, y, w, h, ri->ri_devcmap[bg]);
	}
}

static void
zzfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct zzfb_softc *sc = scr->scr_cookie;
	int x, y, w, h, xs;

	if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		x = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		w = ri->ri_font->fontwidth * ncols;
		h = ri->ri_font->fontheight;
		zzfb_bitblt(sc, x, y, w, h, xs, y);
	}
}

static void
zzfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct zzfb_softc *sc = scr->scr_cookie;
	int x, y, w, h, fg, bg, ul;

	if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		w = ri->ri_font->fontwidth * ncols;
		h = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);
		zzfb_rectfill(sc, x, y, w, h, ri->ri_devcmap[bg & 0xf]);
	}
}

static int
zzfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	int retval = 0;
	u_int new_wsmode;
	struct vcons_data *vd = v;
	struct zzfb_softc *sc = vd->cookie;
	struct vcons_screen *scr = vd->active;
	struct wsdisplayio_bus_id *busid;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_UNKNOWN;
		break;
	case WSDISPLAYIO_GET_FBINFO:
		retval = zzfb_get_fbinfo(sc, (struct wsdisplayio_fbinfo *)data);
		break;
	case WSDISPLAYIO_GINFO:
		((struct wsdisplay_fbinfo *)data)->width  = sc->sc_width;
		((struct wsdisplay_fbinfo *)data)->height = sc->sc_height;
		((struct wsdisplay_fbinfo *)data)->depth  = sc->sc_bpp;
		((struct wsdisplay_fbinfo *)data)->cmsize = ZZ9K_PALETTE_SIZE;
		break;
	case WSDISPLAYIO_GETCMAP:
		retval = zzfb_getcmap(sc, (struct wsdisplay_cmap *)data);
		break;
	case WSDISPLAYIO_PUTCMAP:
		retval = zzfb_putcmap(sc, (struct wsdisplay_cmap *)data);
		break;
	case WSDISPLAYIO_GVIDEO:
		*(int *)data = (sc->sc_isrtg == true) ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		break;
	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON) {
			zzfb_set_mode(sc, sc->sc_displaymode, sc->sc_colormode);
		} else {
			zzfb_set_capture(sc, ZZFB_DISPLAY_MODE,
			    ZZFB_CAPTURE_MODE);
		}
		break;	
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		break;
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		retval = EPASSTHROUGH;
		break;	
	case WSDISPLAYIO_GMODE:
		*(u_int *)data = sc->sc_wsmode;
		break;	
	case WSDISPLAYIO_SMODE:
		new_wsmode = *(u_int *)data;
		if (new_wsmode != sc->sc_wsmode) {
			switch (new_wsmode) {
			case WSDISPLAYIO_MODE_EMUL:
				zzfb_set_mode(sc,
				    ZZFB_SET_CON_MODE, ZZFB_SET_CON_BPP);
				zzfb_init_palette(sc);
				zzfb_clearbg(sc, WS_DEFAULT_BG);
				vcons_redraw_screen(scr);
				sc->sc_wsmode = new_wsmode;
				break;
			case WSDISPLAYIO_MODE_MAPPED:
				zzfb_set_mode(sc,
				    ZZFB_SET_GFX_MODE, ZZFB_SET_GFX_BPP);
				    zzfb_clearbg(sc, WSCOL_BLACK);
				sc->sc_wsmode = new_wsmode;
				break;
			case WSDISPLAYIO_MODE_DUMBFB:
				zzfb_set_mode(sc,
				    ZZFB_SET_DFB_MODE, ZZFB_SET_DFB_BPP);
				    zzfb_clearbg(sc, WSCOL_BLACK);
				sc->sc_wsmode = new_wsmode;
				break;
			default:
				retval = EINVAL;
				break;
			}
		} else {
			retval = EINVAL;
		}
		break;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;
		break;
	case WSDISPLAYIO_GMSGATTRS:
	case WSDISPLAYIO_SMSGATTRS:
	case WSDISPLAYIO_GBORDER:
	case WSDISPLAYIO_SBORDER:
	case WSDISPLAYIO_GETWSCHAR:
	case WSDISPLAYIO_PUTWSCHAR:
	case WSDISPLAYIO_SSPLASH:
	case WSDISPLAYIO_GET_EDID:
	case WSDISPLAYIO_SETVERSION:	
	default:
		retval = EPASSTHROUGH;
		break;
	}

	return retval;
}

static paddr_t
zzfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct zzfb_softc *sc = vd->cookie;

	if ( (offset >= 0) && (offset < sc->sc_fbsize) ) {
		return bus_space_mmap( sc->sc_iot,
		    (bus_addr_t)kvtop((void *)sc->sc_fbh), offset, prot,
		    BUS_SPACE_MAP_LINEAR);
	} else {
		return -1;
	}
}

static int
zzfb_get_fbinfo(struct zzfb_softc *sc, struct wsdisplayio_fbinfo *fbi)
{
	uint32_t bpA, bpR, bpG, bpB;

	switch (sc->sc_bpp) {
	case 8:
		bpA = 0; bpR = 0; bpG = 0; bpB = 0;
		break;
	case 15:
		bpA = 1; bpR = 5; bpG = 5; bpB = 5;
		break;
	case 16:
		bpA = 0; bpR = 5; bpG = 6; bpB = 5;
		break;
	case 32:
		bpA = 8; bpR = 8; bpG = 8; bpB = 8;
		break;
	default:
		return EINVAL;
	}

	fbi->fbi_flags		= 0;
	fbi->fbi_fboffset	= 0;
	fbi->fbi_fbsize		= sc->sc_stride * sc->sc_height;
	fbi->fbi_width 		= sc->sc_width;
	fbi->fbi_height		= sc->sc_height;
	fbi->fbi_stride		= sc->sc_stride;
	fbi->fbi_bitsperpixel	= (sc->sc_bpp == 15) ? 16 : sc->sc_bpp;

	switch (sc->sc_bpp) {
	case 8:
		fbi->fbi_pixeltype			    = WSFB_CI;	
		fbi->fbi_subtype.fbi_cmapinfo.cmap_entries  = ZZ9K_PALETTE_SIZE;
		return 0;
	case 15: /* ZZ9000 uses ARGB1555 format for 15 bpp */
	case 16: /* ZZ9000 uses RGB565 format for 16 bpp */
		fbi->fbi_subtype.fbi_rgbmasks.alpha_offset	= bpB+bpG+bpR;
		fbi->fbi_subtype.fbi_rgbmasks.red_offset	= bpB+bpG;
		fbi->fbi_subtype.fbi_rgbmasks.green_offset	= bpB;
		fbi->fbi_subtype.fbi_rgbmasks.blue_offset	= 0;
		break;
	case 32: /* ZZ9000 uses BGRA8888 format for 32 bpp */
		fbi->fbi_subtype.fbi_rgbmasks.alpha_offset	= 0;
		fbi->fbi_subtype.fbi_rgbmasks.red_offset	= bpA;
		fbi->fbi_subtype.fbi_rgbmasks.green_offset	= bpA+bpR;
		fbi->fbi_subtype.fbi_rgbmasks.blue_offset	= bpA+bpR+bpG;
		break;
	default:
		return EINVAL;
	}

	fbi->fbi_pixeltype				= WSFB_RGB;
	fbi->fbi_subtype.fbi_rgbmasks.alpha_size	= bpA;
	fbi->fbi_subtype.fbi_rgbmasks.red_size		= bpR;
	fbi->fbi_subtype.fbi_rgbmasks.green_size	= bpG;
	fbi->fbi_subtype.fbi_rgbmasks.blue_size		= bpB;

	return 0;
}

static int
zzfb_getcmap(struct zzfb_softc *sc, struct wsdisplay_cmap *cm)
{
	int retval = 0;
	u_int index = cm->index;
	u_int count = cm->count;

	if (index >= ZZ9K_PALETTE_SIZE || index + count > ZZ9K_PALETTE_SIZE)
		return EINVAL;

	retval = copyout(&sc->red[index], cm->red, count);
	if (retval != 0)
		return retval;
	
	retval = copyout(&sc->green[index], cm->green, count);
	if (retval != 0)
		return retval;
	
	retval = copyout(&sc->blue[index], cm->blue, count);
	if (retval != 0)
		return retval;

	return retval;
}

static int
zzfb_putcmap(struct zzfb_softc *sc, struct wsdisplay_cmap *cm)
{
	int retval = 0;
	u_int index = cm->index;
	u_int count = cm->count;
	
	if (index >= ZZ9K_PALETTE_SIZE || index + count > ZZ9K_PALETTE_SIZE)
		return EINVAL;

	retval = copyin(cm->red, &sc->red[index], count);
	if (retval != 0)
		return retval;

	retval = copyin(cm->green, &sc->green[index], count);
	if (retval != 0)
		return retval;

	retval = copyin(cm->blue, &sc->blue[index], count);
	if (retval != 0)
		return retval;
	
	zzfb_set_palette(sc);
	return retval;
}

/*
 * Early console handling, associated with amiga/conf.c file which holds a
 * table of all console devices. The below functions ensures that ZZ9000 becomes
 * wsdisplay0 and wskbd0 gets attached to it.
 */

/* early console handling */
void zzfb_cnprobe(struct consdev *cd);
void zzfb_cninit(struct consdev *cd);
void zzfb_cnpollc(dev_t cd, int on);
void zzfb_cnputc(dev_t cd, int ch);
int zzfb_cngetc(dev_t cd);

void 
zzfb_cnprobe(struct consdev *cd)
{
#ifdef ZZFB_CONSOLE
	if (zz9k_exists == true) {
		cd->cn_pri = CN_INTERNAL;
	} else {
		cd->cn_pri = CN_DEAD;
	}
	cd->cn_dev = NODEV;
#endif /* ZZFB_CONSOLE */
}

void
zzfb_cninit(struct consdev *cd)
{
#if defined ZZFB_CONSOLE && NKBD > 0
	/* tell kbd device it is used as console keyboard */
	if (zz9k_exists == true)
		kbd_cnattach();
#endif /* ZZFB_CONSOLE && NKBD > 0 */
}

void
zzfb_cnpollc(dev_t cd, int on)
{
}

void
zzfb_cnputc(dev_t cd, int ch)
{
}

int
zzfb_cngetc(dev_t cd)
{
	return 0;
}
