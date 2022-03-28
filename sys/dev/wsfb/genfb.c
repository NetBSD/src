/*	$NetBSD: genfb.c,v 1.86 2022/03/28 11:21:40 mlelstv Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: genfb.c,v 1.86 2022/03/28 11:21:40 mlelstv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/wsfb/genfbvar.h>

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#ifdef GENFB_DISABLE_TEXT
#define DISABLESPLASH (boothowto & (RB_SINGLE | RB_USERCONF | RB_ASKNAME | \
		AB_VERBOSE | AB_DEBUG) )
#endif

#ifdef _KERNEL_OPT
#include "opt_genfb.h"
#include "opt_wsfb.h"
#include "opt_rasops.h"
#endif

#ifdef GENFB_DEBUG
#define GPRINTF panic
#else
#define GPRINTF aprint_debug
#endif

#define GENFB_BRIGHTNESS_STEP 15
#define	GENFB_CHAR_WIDTH_MM 3

static int	genfb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	genfb_mmap(void *, void *, off_t, int);
static void	genfb_pollc(void *, int);

static void	genfb_init_screen(void *, struct vcons_screen *, int, long *);
static int	genfb_calc_hsize(struct genfb_softc *);
static int	genfb_calc_cols(struct genfb_softc *, struct rasops_info *);

static int	genfb_putcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static int 	genfb_getcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static int 	genfb_putpalreg(struct genfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);
static void	genfb_init_palette(struct genfb_softc *);

static void	genfb_brightness_up(device_t);
static void	genfb_brightness_down(device_t);

#if GENFB_GLYPHCACHE > 0
static int	genfb_setup_glyphcache(struct genfb_softc *, long);
static void	genfb_putchar(void *, int, int, u_int, long);
#endif

extern const u_char rasops_cmap[768];

static int genfb_cnattach_called = 0;
static int genfb_enabled = 1;

static struct genfb_softc *genfb_softc = NULL;

void
genfb_init(struct genfb_softc *sc)
{
	prop_dictionary_t dict;
	uint64_t cmap_cb, pmf_cb, mode_cb, bl_cb, br_cb, fbaddr;
	uint64_t fboffset;
	bool console;

	dict = device_properties(sc->sc_dev);
#ifdef GENFB_DEBUG
	printf("%s", prop_dictionary_externalize(dict));
#endif
	prop_dictionary_get_bool(dict, "is_console", &console);

	if (!prop_dictionary_get_uint32(dict, "width", &sc->sc_width)) {
		GPRINTF("no width property\n");
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "height", &sc->sc_height)) {
		GPRINTF("no height property\n");
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "depth", &sc->sc_depth)) {
		GPRINTF("no depth property\n");
		return;
	}

	if (!prop_dictionary_get_uint64(dict, "address", &fboffset)) {
		GPRINTF("no address property\n");
		return;
	}

	sc->sc_fboffset = (bus_addr_t)fboffset;

	sc->sc_fbaddr = NULL;
	if (prop_dictionary_get_uint64(dict, "virtual_address", &fbaddr)) {
		sc->sc_fbaddr = (void *)(uintptr_t)fbaddr;
	}

	sc->sc_shadowfb = NULL;
	if (!prop_dictionary_get_bool(dict, "enable_shadowfb",
	    &sc->sc_enable_shadowfb))
#ifdef GENFB_SHADOWFB
		sc->sc_enable_shadowfb = true;
#else
		sc->sc_enable_shadowfb = false;
#endif

	if (!prop_dictionary_get_uint32(dict, "linebytes", &sc->sc_stride))
		sc->sc_stride = (sc->sc_width * sc->sc_depth) >> 3;

	/*
	 * deal with a bug in the Raptor firmware which always sets
	 * stride = width even when depth != 8
	 */
	if (sc->sc_stride < sc->sc_width * (sc->sc_depth >> 3))
		sc->sc_stride = sc->sc_width * (sc->sc_depth >> 3);

	sc->sc_fbsize = sc->sc_height * sc->sc_stride;

	/* optional colour map callback */
	sc->sc_cmcb = NULL;
	if (prop_dictionary_get_uint64(dict, "cmap_callback", &cmap_cb)) {
		if (cmap_cb != 0)
			sc->sc_cmcb = (void *)(vaddr_t)cmap_cb;
	}

	/* optional pmf callback */
	sc->sc_pmfcb = NULL;
	if (prop_dictionary_get_uint64(dict, "pmf_callback", &pmf_cb)) {
		if (pmf_cb != 0)
			sc->sc_pmfcb = (void *)(vaddr_t)pmf_cb;
	}

	/* optional mode callback */
	sc->sc_modecb = NULL;
	if (prop_dictionary_get_uint64(dict, "mode_callback", &mode_cb)) {
		if (mode_cb != 0)
			sc->sc_modecb = (void *)(vaddr_t)mode_cb;
	}

	/* optional backlight control callback */
	sc->sc_backlight = NULL;
	if (prop_dictionary_get_uint64(dict, "backlight_callback", &bl_cb)) {
		if (bl_cb != 0) {
			sc->sc_backlight = (void *)(vaddr_t)bl_cb;
			aprint_naive_dev(sc->sc_dev,
			    "enabling backlight control\n");
		}
	}

	/* optional brightness control callback */
	sc->sc_brightness = NULL;
	if (prop_dictionary_get_uint64(dict, "brightness_callback", &br_cb)) {
		if (br_cb != 0) {
			sc->sc_brightness = (void *)(vaddr_t)br_cb;
			aprint_naive_dev(sc->sc_dev,
			    "enabling brightness control\n");
			if (console &&
			    sc->sc_brightness->gpc_upd_parameter != NULL) {
				pmf_event_register(sc->sc_dev,
				    PMFE_DISPLAY_BRIGHTNESS_UP,
				    genfb_brightness_up, TRUE);
				pmf_event_register(sc->sc_dev,
				    PMFE_DISPLAY_BRIGHTNESS_DOWN,
				    genfb_brightness_down, TRUE);
			}
		}
	}
}

int
genfb_attach(struct genfb_softc *sc, struct genfb_ops *ops)
{
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t dict;
	struct rasops_info *ri;
	paddr_t fb_phys;
	uint16_t crow;
	long defattr;
	bool console;
#ifdef SPLASHSCREEN
	int i, j;
	int error = ENXIO;
#endif

	dict = device_properties(sc->sc_dev);
	prop_dictionary_get_bool(dict, "is_console", &console);

	if (prop_dictionary_get_uint16(dict, "cursor-row", &crow) == false)
		crow = 0;
	if (prop_dictionary_get_bool(dict, "clear-screen", &sc->sc_want_clear)
	    == false)
		sc->sc_want_clear = true;

	fb_phys = (paddr_t)sc->sc_fboffset;
	if (fb_phys == 0) {
		KASSERT(sc->sc_fbaddr != NULL);
		(void)pmap_extract(pmap_kernel(), (vaddr_t)sc->sc_fbaddr,
		    &fb_phys);
	}

	aprint_verbose_dev(sc->sc_dev, "framebuffer at %p, size %dx%d, depth %d, "
	    "stride %d\n",
	    fb_phys ? (void *)(intptr_t)fb_phys : sc->sc_fbaddr,
	    sc->sc_width, sc->sc_height, sc->sc_depth, sc->sc_stride);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		  WSSCREEN_RESIZE,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	memcpy(&sc->sc_ops, ops, sizeof(struct genfb_ops));
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	if (sc->sc_modecb != NULL)
		sc->sc_modecb->gmc_setmode(sc, sc->sc_mode);

	sc->sc_accessops.ioctl = genfb_ioctl;
	sc->sc_accessops.mmap = genfb_mmap;
	sc->sc_accessops.pollc = genfb_pollc;

	if (sc->sc_enable_shadowfb) {
		sc->sc_shadowfb = kmem_alloc(sc->sc_fbsize, KM_SLEEP);
		if (sc->sc_want_clear == false)
			memcpy(sc->sc_shadowfb, sc->sc_fbaddr, sc->sc_fbsize);
		aprint_verbose_dev(sc->sc_dev,
		    "shadow framebuffer enabled, size %zu KB\n",
		    sc->sc_fbsize >> 10);
	}

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &sc->sc_accessops);
	sc->vd.init_screen = genfb_init_screen;

	/* Do not print anything between this point and the screen
	 * clear operation below.  Otherwise it will be lost. */

	ri = &sc->sc_console_screen.scr_ri;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
	    &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

#if GENFB_GLYPHCACHE > 0
	genfb_setup_glyphcache(sc, defattr);
#endif

#ifdef SPLASHSCREEN
/*
 * If system isn't going to go multiuser, or user has requested to see
 * boot text, don't render splash screen immediately
 */
	if (DISABLESPLASH)
#endif
		vcons_redraw_screen(&sc->sc_console_screen);

	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

	if (crow >= ri->ri_rows) {
		crow = 0;
		sc->sc_want_clear = 1;
	}

	if (console)
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, crow,
		    defattr);

	/* Clear the whole screen to bring it to a known state. */
	if (sc->sc_want_clear)
		(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, defattr);

#ifdef SPLASHSCREEN
	j = 0;
	for (i = 0; i < uimin(1 << sc->sc_depth, 256); i++) {
		if (i >= SPLASH_CMAP_OFFSET &&
		    i < SPLASH_CMAP_OFFSET + SPLASH_CMAP_SIZE) {
			splash_get_cmap(i,
			    &sc->sc_cmap_red[i],
			    &sc->sc_cmap_green[i],
			    &sc->sc_cmap_blue[i]);
		} else {
			sc->sc_cmap_red[i] = rasops_cmap[j];
			sc->sc_cmap_green[i] = rasops_cmap[j + 1];
			sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		}
		j += 3;
	}
	genfb_restore_palette(sc);

	sc->sc_splash.si_depth = sc->sc_depth;
	sc->sc_splash.si_bits = sc->sc_console_screen.scr_ri.ri_origbits;
	sc->sc_splash.si_hwbits = sc->sc_fbaddr;
	sc->sc_splash.si_width = sc->sc_width;
	sc->sc_splash.si_height = sc->sc_height;
	sc->sc_splash.si_stride = sc->sc_stride;
	sc->sc_splash.si_fillrect = NULL;
	if (!DISABLESPLASH) {
		error = splash_render(&sc->sc_splash,
		    SPLASH_F_CENTER|SPLASH_F_FILL);
		if (error) {
			SCREEN_ENABLE_DRAWING(&sc->sc_console_screen);
			genfb_init_palette(sc);
			vcons_replay_msgbuf(&sc->sc_console_screen);
		}
	}
#else
	genfb_init_palette(sc);
	if (console && (boothowto & (AB_SILENT|AB_QUIET)) == 0)
		vcons_replay_msgbuf(&sc->sc_console_screen);
#endif

	if (genfb_softc == NULL)
		genfb_softc = sc;

	aa.console = console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &sc->sc_accessops;
	aa.accesscookie = &sc->vd;

#ifdef GENFB_DISABLE_TEXT
	if (!DISABLESPLASH && error == 0)
		SCREEN_DISABLE_DRAWING(&sc->sc_console_screen);
#endif

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint,
	    CFARGS(.iattr = "wsemuldisplaydev"));

	return 0;
}

static int
genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct genfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;
	struct wsdisplay_param *param;
	int new_mode, error, val, ret;

	switch (cmd) {
		case WSDISPLAYIO_GINFO:
			if (ms == NULL)
				return ENODEV;
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return genfb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return genfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			new_mode = *(int *)data;

			/* notify the bus backend */
			error = 0;
			if (sc->sc_ops.genfb_ioctl)
				error = sc->sc_ops.genfb_ioctl(sc, vs,
					    cmd, data, flag, l);
			if (error && error != EPASSTHROUGH)
				return error;

			if (new_mode != sc->sc_mode) {
				sc->sc_mode = new_mode;
				if (sc->sc_modecb != NULL)
					sc->sc_modecb->gmc_setmode(sc,
					    sc->sc_mode);
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					genfb_restore_palette(sc);
					vcons_redraw_screen(ms);
				}
			}
			return 0;
		
		case WSDISPLAYIO_SSPLASH:
#if defined(SPLASHSCREEN)
			if(*(int *)data == 1) {
				SCREEN_DISABLE_DRAWING(&sc->sc_console_screen);
				splash_render(&sc->sc_splash,
						SPLASH_F_CENTER|SPLASH_F_FILL);
			} else {
				SCREEN_ENABLE_DRAWING(&sc->sc_console_screen);
				genfb_init_palette(sc);
			}
			vcons_redraw_screen(ms);
			return 0;
#else
			return ENODEV;
#endif
		case WSDISPLAYIO_GETPARAM:
			param = (struct wsdisplay_param *)data;
			switch (param->param) {
			case WSDISPLAYIO_PARAM_BRIGHTNESS:
				if (sc->sc_brightness == NULL)
					return EPASSTHROUGH;
				param->min = 0;
				param->max = 255;
				return sc->sc_brightness->gpc_get_parameter(
				    sc->sc_brightness->gpc_cookie,
				    &param->curval);
			case WSDISPLAYIO_PARAM_BACKLIGHT:
				if (sc->sc_backlight == NULL)
					return EPASSTHROUGH;
				param->min = 0;
				param->max = 1;
				return sc->sc_backlight->gpc_get_parameter(
				    sc->sc_backlight->gpc_cookie,
				    &param->curval);
			}
			return EPASSTHROUGH;

		case WSDISPLAYIO_SETPARAM:
			param = (struct wsdisplay_param *)data;
			switch (param->param) {
			case WSDISPLAYIO_PARAM_BRIGHTNESS:
				if (sc->sc_brightness == NULL)
					return EPASSTHROUGH;
				val = param->curval;
				if (val < 0) val = 0;
				if (val > 255) val = 255;
				return sc->sc_brightness->gpc_set_parameter(
				    sc->sc_brightness->gpc_cookie, val);
			case WSDISPLAYIO_PARAM_BACKLIGHT:
				if (sc->sc_backlight == NULL)
					return EPASSTHROUGH;
				val = param->curval;
				if (val < 0) val = 0;
				if (val > 1) val = 1;
				return sc->sc_backlight->gpc_set_parameter(
				    sc->sc_backlight->gpc_cookie, val);
			}
			return EPASSTHROUGH;		
	}
	ret = EPASSTHROUGH;
	if (sc->sc_ops.genfb_ioctl)
		ret = sc->sc_ops.genfb_ioctl(sc, vs, cmd, data, flag, l);
	if (ret != EPASSTHROUGH)
		return ret;
	/*
	 * XXX
	 * handle these only if there either is no ioctl() handler or it didn't
	 * know how to deal with them. This allows bus frontends to override
	 * them but still provides fallback implementations
	 */
	switch (cmd) {
		case WSDISPLAYIO_GET_EDID: {
			struct wsdisplayio_edid_info *d = data;
			return wsdisplayio_get_edid(sc->sc_dev, d);
		}
	
		case WSDISPLAYIO_GET_FBINFO: {
			struct wsdisplayio_fbinfo *fbi = data;
			return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
		}
	}
	return EPASSTHROUGH;
}

static paddr_t
genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct genfb_softc *sc = vd->cookie;

	if (sc->sc_ops.genfb_mmap)
		return sc->sc_ops.genfb_mmap(sc, vs, offset, prot);

	return -1;
}

static void
genfb_pollc(void *v, int on)
{
	struct vcons_data *vd = v;
	struct genfb_softc *sc = vd->cookie;

	if (sc == NULL)
		return;

	if (on)
		genfb_enable_polling(sc->sc_dev);
	else
		genfb_disable_polling(sc->sc_dev);
}

static void
genfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct genfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;
	int wantcols;
	bool is_bgr, is_swapped, is_10bit;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;
	if (sc->sc_want_clear)
		ri->ri_flg |= RI_FULLCLEAR;

	scr->scr_flags |= VCONS_LOADFONT;

	if (sc->sc_shadowfb != NULL) {
		ri->ri_hwbits = (char *)sc->sc_fbaddr;
		ri->ri_bits = (char *)sc->sc_shadowfb;
	} else {
		ri->ri_bits = (char *)sc->sc_fbaddr;
		scr->scr_flags |= VCONS_DONT_READ;
	}

	if (existing && sc->sc_want_clear)
		ri->ri_flg |= RI_CLEAR;

	switch (ri->ri_depth) {
	case 32:
	case 24:
		ri->ri_flg |= RI_ENABLE_ALPHA;

		is_bgr = false;
		prop_dictionary_get_bool(device_properties(sc->sc_dev),
		    "is_bgr", &is_bgr);

		is_swapped = false;
		prop_dictionary_get_bool(device_properties(sc->sc_dev),
		    "is_swapped", &is_swapped);

		is_10bit = false;
		prop_dictionary_get_bool(device_properties(sc->sc_dev),
		    "is_10bit", &is_10bit);

		const int bits = is_10bit ? 10 : 8;
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = bits;

		if (is_bgr) {
			/* someone requested BGR */
			ri->ri_rpos = bits * 0;
			ri->ri_gpos = bits * 1;
			ri->ri_bpos = bits * 2;
		} else if (is_swapped) {
			/* byte-swapped, must be 32 bpp */
			KASSERT(ri->ri_depth == 32 && bits == 8);
			ri->ri_rpos = 8;
			ri->ri_gpos = 16;
			ri->ri_bpos = 24;
		} else {
			/* assume RGB */
			ri->ri_rpos = bits * 2;
			ri->ri_gpos = bits * 1;
			ri->ri_bpos = bits * 0;
		}
		break;

	case 16:
	case 15:
		ri->ri_flg |= RI_ENABLE_ALPHA;
		break;

	case 8:
		if (sc->sc_cmcb != NULL)
			ri->ri_flg |= RI_ENABLE_ALPHA | RI_8BIT_IS_RGB;
		break;

	case 2:
		ri->ri_flg |= RI_ENABLE_ALPHA;
		break;

	default:
		break;
	}

	wantcols = genfb_calc_cols(sc, ri);

	rasops_init(ri, 0, wantcols);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		  WSSCREEN_RESIZE;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
#if GENFB_GLYPHCACHE > 0
	sc->sc_putchar = ri->ri_ops.putchar;
	ri->ri_ops.putchar = genfb_putchar;
#endif
#ifdef GENFB_DISABLE_TEXT
	if (scr == &sc->sc_console_screen && !DISABLESPLASH)
		SCREEN_DISABLE_DRAWING(&sc->sc_console_screen);
#endif
}

/* Returns the width of the display in millimeters, or 0 if not known. */
static int
genfb_calc_hsize(struct genfb_softc *sc)
{
	device_t dev = sc->sc_dev;
	prop_dictionary_t dict = device_properties(dev);
	prop_data_t edid_data;
	struct edid_info *edid;
	const char *edid_ptr;
	int hsize;

	edid_data = prop_dictionary_get(dict, "EDID");
	if (edid_data == NULL || prop_data_size(edid_data) < 128)
		return 0;

	edid = kmem_alloc(sizeof(*edid), KM_SLEEP);

	edid_ptr = prop_data_value(edid_data);
	if (edid_parse(__UNCONST(edid_ptr), edid) == 0)
		hsize = (int)edid->edid_max_hsize * 10;
	else
		hsize = 0;

	kmem_free(edid, sizeof(*edid));

	return hsize;
}

/* Return the minimum number of character columns based on DPI */
static int
genfb_calc_cols(struct genfb_softc *sc, struct rasops_info *ri)
{
	const int hsize = genfb_calc_hsize(sc);

	if (hsize != 0) {
		ri->ri_flg |= RI_PREFER_WIDEFONT;
	}

	return MAX(RASOPS_DEFAULT_WIDTH, hsize / GENFB_CHAR_WIDTH_MM);
}

static int
genfb_putcmap(struct genfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef GENFB_DEBUG
	aprint_debug("putcmap: %d %d\n",index, count);
#endif
	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		genfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
genfb_getcmap(struct genfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

void
genfb_restore_palette(struct genfb_softc *sc)
{
	int i;

	if (sc->sc_depth <= 8) {
		for (i = 0; i < (1 << sc->sc_depth); i++) {
			genfb_putpalreg(sc, i, sc->sc_cmap_red[i],
			    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
		}
	}
}

static void
genfb_init_palette(struct genfb_softc *sc)
{
	int i, j, tmp;

	if (sc->sc_depth == 8) {
		/* generate an r3g3b2 colour map */
		for (i = 0; i < 256; i++) {
			tmp = i & 0xe0;
			/*
			 * replicate bits so 0xe0 maps to a red value of 0xff
			 * in order to make white look actually white
			 */
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_red[i] = tmp;

			tmp = (i & 0x1c) << 3;
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_green[i] = tmp;

			tmp = (i & 0x03) << 6;
			tmp |= tmp >> 2;
			tmp |= tmp >> 4;
			sc->sc_cmap_blue[i] = tmp;

			genfb_putpalreg(sc, i, sc->sc_cmap_red[i],
				       sc->sc_cmap_green[i],
				       sc->sc_cmap_blue[i]);
		}
	} else {
		/* steal rasops' ANSI cmap */
		j = 0;
		for (i = 0; i < 256; i++) {
			sc->sc_cmap_red[i] = rasops_cmap[j];
			sc->sc_cmap_green[i] = rasops_cmap[j + 1];
			sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
			j += 3;
		}
	}
}

static int
genfb_putpalreg(struct genfb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{

	if (sc->sc_cmcb) {

		sc->sc_cmcb->gcc_set_mapreg(sc->sc_cmcb->gcc_cookie,
		    idx, r, g, b);
	}
	return 0;
}

void
genfb_cnattach(void)
{
	genfb_cnattach_called = 1;
}

int
genfb_cndetach(void)
{

	if (genfb_cnattach_called) {
		genfb_cnattach_called = 0;
		return 1;
	}
	return 0;
}

void
genfb_disable(void)
{
	genfb_enabled = 0;
}

int
genfb_is_console(void)
{
	return genfb_cnattach_called;
}

int
genfb_is_enabled(void)
{
	return genfb_enabled;
}

int
genfb_borrow(bus_addr_t addr, bus_space_handle_t *hdlp)
{
	struct genfb_softc *sc = genfb_softc;

	if (sc && sc->sc_ops.genfb_borrow)
		return sc->sc_ops.genfb_borrow(sc, addr, hdlp);
	return 0;
}

static void
genfb_brightness_up(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	KASSERT(sc->sc_brightness != NULL &&
		sc->sc_brightness->gpc_upd_parameter != NULL);

	(void)sc->sc_brightness->gpc_upd_parameter(
	    sc->sc_brightness->gpc_cookie, GENFB_BRIGHTNESS_STEP);
}

static void
genfb_brightness_down(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	KASSERT(sc->sc_brightness != NULL &&
		sc->sc_brightness->gpc_upd_parameter != NULL);

	(void)sc->sc_brightness->gpc_upd_parameter(
	    sc->sc_brightness->gpc_cookie, - GENFB_BRIGHTNESS_STEP);
}

void
genfb_enable_polling(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	if (sc->sc_console_screen.scr_vd) {
		SCREEN_ENABLE_DRAWING(&sc->sc_console_screen);
		vcons_hard_switch(&sc->sc_console_screen);
		vcons_enable_polling(&sc->vd);
		if (sc->sc_ops.genfb_enable_polling)
			(*sc->sc_ops.genfb_enable_polling)(sc);
	}
}

void
genfb_disable_polling(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	if (sc->sc_console_screen.scr_vd) {
		if (sc->sc_ops.genfb_disable_polling)
			(*sc->sc_ops.genfb_disable_polling)(sc);
		vcons_disable_polling(&sc->vd);
	}
}

#if GENFB_GLYPHCACHE > 0
#define GLYPHCACHESIZE ((GENFB_GLYPHCACHE) * 1024 * 1024)

static inline int
attr2idx(long attr)
{
	if ((attr & 0xf0f00ff8) != 0)
		return -1;
	
	return (((attr >> 16) & 0x0f) | ((attr >> 20) & 0xf0));
}

static int
genfb_setup_glyphcache(struct genfb_softc *sc, long defattr)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri,
			  *cri = &sc->sc_cache_ri;
	gc_bucket *b;
	int i, usedcells = 0, idx, j;

	sc->sc_cache = kmem_alloc(GLYPHCACHESIZE, KM_SLEEP);

	/*
	 * now we build a mutant rasops_info for the cache - same pixel type
	 * and such as the real fb, but only one character per line for 
	 * simplicity and locality
	 */
	memcpy(cri, ri, sizeof(struct rasops_info));
	cri->ri_ops.putchar = sc->sc_putchar;
	cri->ri_width = ri->ri_font->fontwidth;
	cri->ri_stride = ri->ri_xscale;
	cri->ri_bits = sc->sc_cache;
	cri->ri_hwbits = NULL;
	cri->ri_origbits = sc->sc_cache;
	cri->ri_cols = 1;
	cri->ri_rows = GLYPHCACHESIZE / 
	    (cri->ri_stride * cri->ri_font->fontheight);
	cri->ri_xorigin = 0;
	cri->ri_yorigin = 0;
	cri->ri_xscale = ri->ri_xscale;
	cri->ri_yscale = ri->ri_font->fontheight * ri->ri_xscale;
	
	printf("size %d %d %d\n", GLYPHCACHESIZE, ri->ri_width, ri->ri_stride);
	printf("cells: %d\n", cri->ri_rows);
	sc->sc_nbuckets = uimin(256, cri->ri_rows / 223);
	sc->sc_buckets = kmem_alloc(sizeof(gc_bucket) * sc->sc_nbuckets, KM_SLEEP);
	printf("buckets: %d\n", sc->sc_nbuckets);
	for (i = 0; i < sc->sc_nbuckets; i++) {
		b = &sc->sc_buckets[i];
		b->gb_firstcell = usedcells;
		b->gb_numcells = uimin(223, cri->ri_rows - usedcells);
		usedcells += 223;
		b->gb_usedcells = 0;
		b->gb_index = -1;
		for (j = 0; j < 223; j++) b->gb_map[j] = -1;
	}

	/* initialize the attribute map... */
	for (i = 0; i < 256; i++) {
		sc->sc_attrmap[i] = -1;
	}

	/* first bucket goes to default attr */
	idx = attr2idx(defattr);
	printf("defattr %08lx idx %x\n", defattr, idx);
	
	if (idx >= 0) {
		sc->sc_attrmap[idx] = 0;
		sc->sc_buckets[0].gb_index = idx;
	}
	
	return 0;
}

static void
genfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct genfb_softc *sc = scr->scr_cookie;
	uint8_t *src, *dst;
	gc_bucket *b;
	int i, idx, bi, cell;

	attr &= ~WSATTR_USERMASK;

	idx = attr2idx(attr);
	if (c < 33 || c > 255 || idx < 0) goto nope;

	/* look for a bucket with the right attribute */
	bi = sc->sc_attrmap[idx];
	if (bi == -1) {
		/* nope, see if there's an empty one left */
		bi = 1;
		while ((bi < sc->sc_nbuckets) && 
		       (sc->sc_buckets[bi].gb_index != -1)) {
			bi++;
		}
		if (bi < sc->sc_nbuckets) {
			/* found one -> grab it */
			sc->sc_attrmap[idx] = bi;
			b = &sc->sc_buckets[bi];
			b->gb_index = idx;
			b->gb_usedcells = 0;
			/* make sure this doesn't get evicted right away */
			b->gb_lastread = time_uptime;
		} else {
			/*
			 * still nothing
			 * steal the least recently read bucket
			 */
			time_t moo = time_uptime;
			int oldest = 1;

			for (i = 1; i < sc->sc_nbuckets; i++) {
				if (sc->sc_buckets[i].gb_lastread < moo) {
					oldest = i;
					moo = sc->sc_buckets[i].gb_lastread;
				}
			}

			/* if we end up here all buckets must be in use */
			b = &sc->sc_buckets[oldest];
			sc->sc_attrmap[b->gb_index] = -1;
			b->gb_index = idx;
			b->gb_usedcells = 0;
			sc->sc_attrmap[idx] = oldest;
			/* now scrub it */
			for (i = 0; i < 223; i++)
				b->gb_map[i] = -1;
			/* and set the time stamp */
			b->gb_lastread = time_uptime;
		}
	} else {
		/* found one */
		b = &sc->sc_buckets[bi];
	}

	/* see if there's room in the bucket */
	if (b->gb_usedcells >= b->gb_numcells) goto nope;

	cell = b->gb_map[c - 33];
	if (cell == -1) {
		if (b->gb_usedcells >= b->gb_numcells)
			goto nope;
		cell = atomic_add_int_nv(&b->gb_usedcells, 1) - 1;
		b->gb_map[c - 33] = cell;
		cell += b->gb_firstcell;
		sc->sc_putchar(&sc->sc_cache_ri, cell, 0, c, attr);
		
	} else
		cell += b->gb_firstcell;

	src = sc->sc_cache + cell * sc->sc_cache_ri.ri_yscale;
	dst = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	for (i = 0; i < ri->ri_font->fontheight; i++) {
		memcpy(dst, src, ri->ri_xscale);
		src += ri->ri_xscale;
		dst += ri->ri_stride;
	}
	b->gb_lastread = time_uptime;
	return;
nope:
	sc->sc_putchar(cookie, row, col, c, attr);
}

#endif
