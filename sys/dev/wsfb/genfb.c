/*	$NetBSD: genfb.c,v 1.28.2.2 2010/10/22 07:22:22 uebayasi Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: genfb.c,v 1.28.2.2 2010/10/22 07:22:22 uebayasi Exp $");

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

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/wsfb/genfbvar.h>

#ifdef GENFB_DISABLE_TEXT
#include <sys/reboot.h>
#define DISABLESPLASH (boothowto & (RB_SINGLE | RB_USERCONF | RB_ASKNAME | \
		AB_VERBOSE | AB_DEBUG) )
#endif

#include "opt_genfb.h"
#include "opt_wsfb.h"

#ifdef GENFB_DEBUG
#define GPRINTF panic
#else
#define GPRINTF aprint_verbose
#endif

static int	genfb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	genfb_mmap(void *, void *, off_t, int);
static void	genfb_init_screen(void *, struct vcons_screen *, int, long *);

static int	genfb_putcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static int 	genfb_getcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static int 	genfb_putpalreg(struct genfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

static void	genfb_brightness_up(device_t);
static void	genfb_brightness_down(device_t);
/* set backlight level */
static void	genfb_set_backlight(struct genfb_softc *, int);
/* turn backlight on and off without messing with the level */
static void	genfb_switch_backlight(struct genfb_softc *, int);

extern const u_char rasops_cmap[768];

static int genfb_cnattach_called = 0;
static int genfb_enabled = 1;

struct wsdisplay_accessops genfb_accessops = {
	genfb_ioctl,
	genfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

static struct genfb_softc *genfb_softc = NULL;

void
genfb_init(struct genfb_softc *sc)
{
	prop_dictionary_t dict;
	uint64_t cmap_cb, pmf_cb, bl_cb;
	uint32_t fboffset;
	bool console;

	dict = device_properties(sc->sc_dev);
#ifdef GENFB_DEBUG
	printf(prop_dictionary_externalize(dict));
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

	/* XXX should be a 64bit value */
	if (!prop_dictionary_get_uint32(dict, "address", &fboffset)) {
		GPRINTF("no address property\n");
		return;
	}

	sc->sc_fboffset = fboffset;

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

	/* optional backlight control callback */
	sc->sc_backlight = NULL;
	if (prop_dictionary_get_uint64(dict, "backlight_callback", &bl_cb)) {
		if (bl_cb != 0) {
			sc->sc_backlight = (void *)(vaddr_t)bl_cb;
			sc->sc_backlight_on = 1;
			aprint_naive_dev(sc->sc_dev, 
			    "enabling backlight control\n");
			sc->sc_backlight_level = 
			    sc->sc_backlight->gpc_get_parameter(
			    sc->sc_backlight->gpc_cookie);
			if (console) {
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
	uint16_t crow;
	long defattr;
	int i, j;
	bool console;

	dict = device_properties(sc->sc_dev);
	prop_dictionary_get_bool(dict, "is_console", &console);

	if (prop_dictionary_get_uint16(dict, "cursor-row", &crow) == false)
		crow = 0;
	if (prop_dictionary_get_bool(dict, "clear-screen", &sc->sc_want_clear)
	    == false)
		sc->sc_want_clear = true;

	/* do not attach when we're not console */
	if (!console) {
		aprint_normal_dev(sc->sc_dev, "no console, unable to continue\n");
		return -1;
	}

	aprint_verbose_dev(sc->sc_dev, "framebuffer at %p, size %dx%d, depth %d, "
	    "stride %d\n",
	    sc->sc_fboffset ? (void *)(intptr_t)sc->sc_fboffset : sc->sc_fbaddr,
	    sc->sc_width, sc->sc_height, sc->sc_depth, sc->sc_stride);

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
	memcpy(&sc->sc_ops, ops, sizeof(struct genfb_ops));
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

#ifdef GENFB_SHADOWFB
	sc->sc_shadowfb = kmem_alloc(sc->sc_fbsize, KM_SLEEP);
	if (sc->sc_want_clear == false && sc->sc_shadowfb != NULL)
		memcpy(sc->sc_shadowfb, sc->sc_fbaddr, sc->sc_fbsize);
#endif

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &genfb_accessops);
	sc->vd.init_screen = genfb_init_screen;

	/* Do not print anything between this point and the screen
	 * clear operation below.  Otherwise it will be lost. */

	ri = &sc->sc_console_screen.scr_ri;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
	    &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

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
	wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, crow,
	    defattr);

	/* Clear the whole screen to bring it to a known state. */
	if (sc->sc_want_clear)
		(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, defattr);

	j = 0;
	for (i = 0; i < min(1 << sc->sc_depth, 256); i++) {
#ifndef SPLASHSCREEN
		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		genfb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1],
		    rasops_cmap[j + 2]);
		j += 3;
#else
		if(i >= SPLASH_CMAP_OFFSET &&
		    i < SPLASH_CMAP_OFFSET + SPLASH_CMAP_SIZE) {
			sc->sc_cmap_red[i] = _splash_header_data_cmap[j][0];
			sc->sc_cmap_green[i] = _splash_header_data_cmap[j][1];
			sc->sc_cmap_blue[i] = _splash_header_data_cmap[j][2];
		} else {
			sc->sc_cmap_red[i] = rasops_cmap[j];
			sc->sc_cmap_green[i] = rasops_cmap[j + 1];
			sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
			genfb_putpalreg(sc, i, rasops_cmap[j],
				rasops_cmap[j + 1],
				rasops_cmap[j + 2]);
		}
		j += 3;
#endif
	}

#ifdef SPLASHSCREEN
	sc->sc_splash.si_depth = sc->sc_depth;
	sc->sc_splash.si_bits = sc->sc_console_screen.scr_ri.ri_bits;
	sc->sc_splash.si_hwbits = sc->sc_fbaddr;
	sc->sc_splash.si_width = sc->sc_width;
	sc->sc_splash.si_height = sc->sc_height;
	sc->sc_splash.si_stride = sc->sc_stride;
	sc->sc_splash.si_fillrect = NULL;
	if (!DISABLESPLASH)
		splash_render(&sc->sc_splash, SPLASH_F_CENTER|SPLASH_F_FILL);
#ifdef SPLASHSCREEN_PROGRESS
	sc->sc_progress.sp_top = (sc->sc_height / 8) * 7;
	sc->sc_progress.sp_width = (sc->sc_width / 4) * 3;
	sc->sc_progress.sp_left = (sc->sc_width / 8) * 7;
	sc->sc_progress.sp_height = 20;
	sc->sc_progress.sp_state = -1;
	sc->sc_progress.sp_si = &sc->sc_splash;
	splash_progress_init(&sc->sc_progress);
#endif
#else
	vcons_replay_msgbuf(&sc->sc_console_screen);
#endif

	if (genfb_softc == NULL)
		genfb_softc = sc;

	aa.console = console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &genfb_accessops;
	aa.accesscookie = &sc->vd;

#ifdef GENFB_DISABLE_TEXT
	if (!DISABLESPLASH)
		SCREEN_DISABLE_DRAWING(&sc->sc_console_screen);
#endif

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);

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
	int new_mode, error;

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
			if (error)
				return error;

			if (new_mode != sc->sc_mode) {
				sc->sc_mode = new_mode;
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
#if defined(SPLASHSCREEN_PROGRESS)
				sc->sc_progress.sp_running = 1;
#endif
			} else {
				SCREEN_ENABLE_DRAWING(&sc->sc_console_screen);
#if defined(SPLASHSCREEN_PROGRESS)
				sc->sc_progress.sp_running = 0;
#endif
			}
			vcons_redraw_screen(ms);
			return 0;
#else
			return ENODEV;
#endif
		case WSDISPLAYIO_SPROGRESS:
#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
			sc->sc_progress.sp_force = 1;
			splash_progress_update(&sc->sc_progress);
			sc->sc_progress.sp_force = 0;
			vcons_redraw_screen(ms);
			return 0;
#else
			return ENODEV;
#endif
		case WSDISPLAYIO_GETPARAM:
			param = (struct wsdisplay_param *)data;
			if (sc->sc_backlight == NULL)
				return EPASSTHROUGH;
			switch (param->param) {
			case WSDISPLAYIO_PARAM_BRIGHTNESS:
				param->min = 0;
				param->max = 255;
				param->curval = sc->sc_backlight_level;
				return 0;
			case WSDISPLAYIO_PARAM_BACKLIGHT:
				param->min = 0;
				param->max = 1;
				param->curval = sc->sc_backlight_on;
				return 0;
			}
			return EPASSTHROUGH;

		case WSDISPLAYIO_SETPARAM:
			param = (struct wsdisplay_param *)data;
			if (sc->sc_backlight == NULL)
				return EPASSTHROUGH;
			switch (param->param) {
			case WSDISPLAYIO_PARAM_BRIGHTNESS:
				genfb_set_backlight(sc, param->curval);
				return 0;
			case WSDISPLAYIO_PARAM_BACKLIGHT:
				genfb_switch_backlight(sc,  param->curval);
				return 0;
			}
			return EPASSTHROUGH;
		default:
			if (sc->sc_ops.genfb_ioctl)
				return sc->sc_ops.genfb_ioctl(sc, vs, cmd,
				    data, flag, l);
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
genfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct genfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;
	if (sc->sc_want_clear)
		ri->ri_flg |= RI_FULLCLEAR;

#ifdef GENFB_SHADOWFB
	if (sc->sc_shadowfb != NULL) {

		ri->ri_hwbits = (char *)sc->sc_fbaddr;
		ri->ri_bits = (char *)sc->sc_shadowfb;
	} else
#endif
	{
		ri->ri_bits = (char *)sc->sc_fbaddr;
		scr->scr_flags |= VCONS_DONT_READ;
	}

	if (existing && sc->sc_want_clear) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	/* TODO: actually center output */
	ri->ri_hw = scr;

#ifdef GENFB_DISABLE_TEXT
	if (scr == &sc->sc_console_screen && !DISABLESPLASH)
		SCREEN_DISABLE_DRAWING(&sc->sc_console_screen);
#endif
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
	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
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

	if (index >= 255 || count > 256 || index + count > 256)
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
genfb_set_backlight(struct genfb_softc *sc, int level)
{

	KASSERT(sc->sc_backlight != NULL);

	/*
	 * should we do nothing when backlight is off, should we just store the
	 * level and use it when turning back on or should we just flip sc_bl_on
	 * and turn the backlight on?
	 * For now turn it on so a crashed screensaver can't get the user stuck
	 * with a dark screen as long as hotkeys work
	 */
	if (level > 255) level = 255;
	if (level < 0) level = 0;
	if (level == sc->sc_backlight_level)
		return;
	sc->sc_backlight_level = level;
	if (sc->sc_backlight_on == 0)
		sc->sc_backlight_on = 1;
	sc->sc_backlight->gpc_set_parameter(
	    sc->sc_backlight->gpc_cookie, level);
}

static void
genfb_switch_backlight(struct genfb_softc *sc, int on)
{
	int level;

	KASSERT(sc->sc_backlight != NULL);

	if (on == sc->sc_backlight_on)
		return;
	sc->sc_backlight_on = on;
	level = on ? sc->sc_backlight_level : 0;
	sc->sc_backlight->gpc_set_parameter(
	    sc->sc_backlight->gpc_cookie, level);
}
	

static void
genfb_brightness_up(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	genfb_set_backlight(sc, sc->sc_backlight_level + 8);
}

static void
genfb_brightness_down(device_t dev)
{
	struct genfb_softc *sc = device_private(dev);

	genfb_set_backlight(sc, sc->sc_backlight_level - 8);
}
