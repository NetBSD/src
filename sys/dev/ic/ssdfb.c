/* $NetBSD: ssdfb.c,v 1.3 2019/03/17 04:03:17 tnn Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
__KERNEL_RCSID(0, "$NetBSD: ssdfb.c,v 1.3 2019/03/17 04:03:17 tnn Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_device.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/ic/ssdfbvar.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

/* userland interface */
static int	ssdfb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	ssdfb_mmap(void *, void *, off_t, int);

/* wscons screen management */
static int	ssdfb_alloc_screen(void *, const struct wsscreen_descr *,
				   void **, int *, int *, long *);
static void	ssdfb_free_screen(void *, void *);
static int	ssdfb_show_screen(void *, void *, int,
				  void (*cb) (void *, int, int), void *);

/* rasops hooks */
static void	ssdfb_putchar(void *, int, int, u_int, long);
static void	ssdfb_copycols(void *, int, int, int, int);
static void	ssdfb_erasecols(void *, int, int, int, long);
static void	ssdfb_copyrows(void *, int, int, int);
static void	ssdfb_eraserows(void *, int, int, long);
static void	ssdfb_cursor(void *, int, int, int);

/* hardware interface */
static int	ssdfb_init(struct ssdfb_softc *);
static int	ssdfb_set_contrast(struct ssdfb_softc *, uint8_t, bool);
static int	ssdfb_set_display_on(struct ssdfb_softc *, bool, bool);
static int	ssdfb_set_mode(struct ssdfb_softc *, u_int);

/* frame buffer damage tracking and synchronization */
static void	ssdfb_udv_attach(struct ssdfb_softc *sc);
static bool	ssdfb_is_modified(struct ssdfb_softc *sc);
static bool	ssdfb_clear_modify(struct ssdfb_softc *sc);
static void	ssdfb_damage(struct ssdfb_softc *);
static void	ssdfb_thread(void *);
static void	ssdfb_set_usepoll(struct ssdfb_softc *, bool);
static int	ssdfb_sync(struct ssdfb_softc *, bool);
static uint64_t	ssdfb_transpose_block_1bpp(uint8_t *, size_t);
static uint64_t	ssdfb_transpose_block_8bpp(uint8_t *, size_t);

/* misc helpers */
static const struct ssdfb_product *
		ssdfb_lookup_product(ssdfb_product_id_t);
static int	ssdfb_pick_font(int *, struct wsdisplay_font **);
static void	ssdfb_clear_screen(struct ssdfb_softc *);
#if defined(DDB)
static void	ssdfb_ddb_trap_callback(int);
#endif

static const char *ssdfb_controller_names[] = {
	[SSDFB_CONTROLLER_UNKNOWN] =	"unknown",
	[SSDFB_CONTROLLER_SSD1306] =	"Solomon Systech SSD1306",
	[SSDFB_CONTROLLER_SH1106] =	"Sino Wealth SH1106"
};

/*
 * Display module assemblies supported by this driver.
 */
static const struct ssdfb_product ssdfb_products[] = {
	{
		.p_product_id =		SSDFB_PRODUCT_SSD1306_GENERIC,
		.p_controller_id =	SSDFB_CONTROLLER_SSD1306,
		.p_name =		"generic",
		.p_width =		128,
		.p_height =		64,
		.p_panel_shift =	0,
		.p_fosc =		0x8,
		.p_fosc_div =		0,
		.p_precharge =		0x1,
		.p_discharge =		0xf,
		.p_compin_cfg =		SSDFB_COM_PINS_A1_MASK
					| SSDFB_COM_PINS_ALTERNATIVE_MASK,
		.p_vcomh_deselect_level = SSD1306_VCOMH_DESELECT_LEVEL_0_77_VCC,
		.p_default_contrast =	0x7f,
		.p_multiplex_ratio =	0x3f,
		.p_chargepump_cmd =	SSD1306_CMD_SET_CHARGE_PUMP,
		.p_chargepump_arg =	SSD1306_CHARGE_PUMP_ENABLE
	},
	{
		.p_product_id =		SSDFB_PRODUCT_SH1106_GENERIC,
		.p_controller_id =	SSDFB_CONTROLLER_SH1106,
		.p_name =		"generic",
		.p_width =		128,
		.p_height =		64,
		.p_panel_shift =	2,
		.p_fosc =		0x5,
		.p_fosc_div =		0,
		.p_precharge =		0x2,
		.p_discharge =		0x2,
		.p_compin_cfg =		SSDFB_COM_PINS_A1_MASK
					| SSDFB_COM_PINS_ALTERNATIVE_MASK,
		.p_vcomh_deselect_level = SH1106_VCOMH_DESELECT_LEVEL_DEFAULT,
		.p_default_contrast =	0x80,
		.p_multiplex_ratio =	0x3f,
		.p_chargepump_cmd =	SH1106_CMD_SET_CHARGE_PUMP_7V4,
		.p_chargepump_arg =	SSDFB_CMD_NOP
	},
	{
		.p_product_id =		SSDFB_PRODUCT_ADAFRUIT_938,
		.p_controller_id =	SSDFB_CONTROLLER_SSD1306,
		.p_name =		"Adafruit Industries, LLC product 938",
		.p_width =		128,
		.p_height =		64,
		.p_panel_shift =	0,
		.p_fosc =		0x8,
		.p_fosc_div =		0,
		.p_precharge =		0x1,
		.p_discharge =		0xf,
		.p_compin_cfg =		0x12,
		.p_vcomh_deselect_level = 0x40,
		.p_default_contrast =	0x8f,
		.p_multiplex_ratio =	0x3f,
		.p_chargepump_cmd =	SSD1306_CMD_SET_CHARGE_PUMP,
		.p_chargepump_arg =	SSD1306_CHARGE_PUMP_ENABLE
	},
	{
		.p_product_id =		SSDFB_PRODUCT_ADAFRUIT_931,
		.p_controller_id =	SSDFB_CONTROLLER_SSD1306,
		.p_name =		"Adafruit Industries, LLC product 931",
		.p_width =		128,
		.p_height =		32,
		.p_panel_shift =	0,
		.p_fosc =		0x8,
		.p_fosc_div =		0,
		.p_precharge =		0x1,
		.p_discharge =		0xf,
		.p_compin_cfg =		0x2,
		.p_vcomh_deselect_level = 0x40,
		.p_default_contrast =	0x8f,
		.p_multiplex_ratio =	0x1f,
		.p_chargepump_cmd =	SSD1306_CMD_SET_CHARGE_PUMP,
		.p_chargepump_arg =	SSD1306_CHARGE_PUMP_ENABLE
	}
};

static const struct wsdisplay_accessops ssdfb_accessops = {
	.ioctl =	ssdfb_ioctl,
	.mmap =		ssdfb_mmap,
	.alloc_screen =	ssdfb_alloc_screen,
	.free_screen =	ssdfb_free_screen,
	.show_screen =	ssdfb_show_screen
};

#define SSDFB_CMD1(c) do { cmd[0] = (c); error = sc->sc_cmd(sc->sc_cookie, cmd, 1, usepoll); } while(0)
#define SSDFB_CMD2(c, a) do { cmd[0] = (c); cmd[1] = (a); error = sc->sc_cmd(sc->sc_cookie, cmd, 2, usepoll); } while(0)

void
ssdfb_attach(struct ssdfb_softc *sc, int flags)
{
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri = &sc->sc_ri;
	int error = 0;
	long defattr;
	const struct ssdfb_product *p;

	p = ssdfb_lookup_product(flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK);
	if (p == NULL) {
		aprint_error(": unknown display assembly\n");
		return;
	}
	sc->sc_p = p;

	aprint_naive("\n");
	aprint_normal(": %s (%s)\n",
	    ssdfb_controller_names[p->p_controller_id],
	    p->p_name);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_is_console = flags & SSDFB_ATTACH_FLAG_CONSOLE ? true : false;
	sc->sc_inverse = flags & SSDFB_ATTACH_FLAG_INVERSE ? true : false;
	sc->sc_upsidedown = flags & SSDFB_ATTACH_FLAG_UPSIDEDOWN ? true : false;
	sc->sc_backoff = 1;
	sc->sc_contrast = sc->sc_p->p_default_contrast;
	sc->sc_gddram_len = sc->sc_p->p_width * sc->sc_p->p_height / 8;
	sc->sc_gddram = kmem_alloc(sc->sc_gddram_len, KM_SLEEP);
	if (sc->sc_gddram == NULL)
		goto out;

	aprint_normal_dev(sc->sc_dev, "%dx%d%s\n", sc->sc_p->p_width,
	    sc->sc_p->p_height, sc->sc_is_console ? ", console" : "");

	/*
	 * Initialize rasops. The native depth is 1-bit monochrome and we
	 * support this in text emul mode via rasops1. But modern Xorg
	 * userland has many rendering glitches when running with 1-bit depth
	 * so to better support this use case we instead declare ourselves as
	 * an 8-bit display with a two entry constant color map.
	 */
	error = ssdfb_pick_font(&sc->sc_fontcookie, &sc->sc_font);
	if (error) {
		aprint_error_dev(sc->sc_dev, "no font\n");
		goto out;
	}
	ri->ri_depth =	8;
	ri->ri_font =	sc->sc_font;
	ri->ri_width =	sc->sc_p->p_width;
	ri->ri_height =	sc->sc_p->p_height;
	ri->ri_stride =	ri->ri_width * ri->ri_depth / 8;
	ri->ri_hw =	sc;
	ri->ri_flg =	RI_FULLCLEAR;
	sc->sc_ri_bits_len = round_page(ri->ri_stride * ri->ri_height);
	ri->ri_bits	= (u_char *)uvm_km_alloc(kernel_map, sc->sc_ri_bits_len,
						 0, UVM_KMF_WIRED);
	if (ri->ri_bits == NULL)
		goto out;

	error = rasops_init(ri,
	    sc->sc_p->p_height / sc->sc_font->fontheight,
	    sc->sc_p->p_width  / sc->sc_font->fontwidth);
	if (error)
		goto out;

	ri->ri_caps &= ~WSSCREEN_WSCOLORS;

	/*
	 * Save original emul ops & insert our damage notification hooks.
	 */
	sc->sc_orig_riops =	ri->ri_ops;
	ri->ri_ops.putchar =	ssdfb_putchar;
	ri->ri_ops.copycols =	ssdfb_copycols;
	ri->ri_ops.erasecols =	ssdfb_erasecols;
	ri->ri_ops.copyrows =	ssdfb_copyrows;
	ri->ri_ops.eraserows =	ssdfb_eraserows;
	ri->ri_ops.cursor =	ssdfb_cursor;

	/*
	 * Set up the screen.
	 */
	sc->sc_screen_descr = (struct wsscreen_descr){
		.name =		"default",
		.ncols =	ri->ri_cols,
		.nrows =	ri->ri_rows,
		.textops =	&ri->ri_ops,
		.fontwidth =	ri->ri_font->fontwidth,
		.fontheight =	ri->ri_font->fontheight,
		.capabilities =	ri->ri_caps
	};
	sc->sc_screens[0] = &sc->sc_screen_descr;
	sc->sc_screenlist = (struct wsscreen_list){
		.nscreens =	1,
		.screens =	sc->sc_screens
	};

	/*
	 * Initialize hardware.
	 */
	error = ssdfb_init(sc);
	if (error)
		goto out;

	if (sc->sc_is_console)
		ssdfb_set_usepoll(sc, true);

	mutex_init(&sc->sc_cond_mtx, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cond, "ssdfb");
	error = kthread_create(PRI_SOFTCLOCK, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN,
	    NULL, ssdfb_thread, sc, &sc->sc_thread, "%s",
	    device_xname(sc->sc_dev));
	if (error) {
		cv_destroy(&sc->sc_cond);
		mutex_destroy(&sc->sc_cond_mtx);
		goto out;
	}

	/*
	 * Attach wsdisplay.
	 */
	if (sc->sc_is_console) {
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
		wsdisplay_cnattach(&sc->sc_screen_descr, ri, 0, 0, defattr);
#if defined(DDB)
		db_trap_callback = ssdfb_ddb_trap_callback;
#endif
	}
	aa = (struct wsemuldisplaydev_attach_args){
		.console =	sc->sc_is_console,
		.scrdata =	&sc->sc_screenlist,
		.accessops =	&ssdfb_accessops,
		.accesscookie =	sc
	};
	sc->sc_wsdisplay =
	    config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);

	return;
out:
	aprint_error_dev(sc->sc_dev, "attach failed: %d\n", error);
	if (sc->sc_gddram != NULL)
		kmem_free(sc->sc_gddram, sc->sc_gddram_len);
	if (ri->ri_bits != NULL)
		uvm_km_free(kernel_map, (vaddr_t)ri->ri_bits, sc->sc_ri_bits_len,
		    UVM_KMF_WIRED);
	if (sc->sc_fontcookie > 0)
		(void) wsfont_unlock(sc->sc_fontcookie);
}

int
ssdfb_detach(struct ssdfb_softc *sc)
{
	mutex_enter(&sc->sc_cond_mtx);
	sc->sc_detaching = true;
	cv_broadcast(&sc->sc_cond);
	mutex_exit(&sc->sc_cond_mtx);
	kthread_join(sc->sc_thread);

	if (sc->sc_uobj != NULL) {
		mutex_enter(sc->sc_uobj->vmobjlock);
		sc->sc_uobj->uo_refs--;
		mutex_exit(sc->sc_uobj->vmobjlock);
	}
	config_detach(sc->sc_wsdisplay, DETACH_FORCE);

	cv_destroy(&sc->sc_cond);
	mutex_destroy(&sc->sc_cond_mtx);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_ri.ri_bits, sc->sc_ri_bits_len,
	    UVM_KMF_WIRED);
	kmem_free(sc->sc_gddram, sc->sc_gddram_len);
	(void) wsfont_unlock(sc->sc_fontcookie);
	return 0;
}

static int
ssdfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ssdfb_softc *sc = v;
	struct wsdisplay_param *wdp;
	struct wsdisplay_cmap *wc;
	u_char cmap[] = {0, 0xff};
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		 *(u_int *)data = WSDISPLAY_TYPE_SSDFB;
		return 0;
	case WSDISPLAYIO_GINFO:
		*(struct wsdisplay_fbinfo *)data = (struct wsdisplay_fbinfo){
			.width =	sc->sc_ri.ri_width,
			.height =	sc->sc_ri.ri_height,
			.depth =	sc->sc_ri.ri_depth,
			.cmsize =	2
		};
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		return wsdisplayio_get_fbinfo(&sc->sc_ri,
			(struct wsdisplayio_fbinfo *)data);
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_ri.ri_stride;
		return 0;
	case WSDISPLAYIO_GETPARAM:
		wdp = (struct wsdisplay_param *)data;
		if (wdp->param != WSDISPLAYIO_PARAM_CONTRAST)
			return EINVAL;
		wdp->min = 0;
		wdp->max = 0xff;
		wdp->curval = sc->sc_contrast;
		return 0;
	case WSDISPLAYIO_SETPARAM:
		wdp = (struct wsdisplay_param *)data;
		if (wdp->param != WSDISPLAYIO_PARAM_CONTRAST)
			return EINVAL;
		if (wdp->curval < 0 || wdp->curval > 0xff)
			return EINVAL;
		return ssdfb_set_contrast(sc, wdp->curval, sc->sc_usepoll);
	case WSDISPLAYIO_GMODE:
		*(u_int *)data = sc->sc_mode;
		return 0;
	case WSDISPLAYIO_SMODE:
		return ssdfb_set_mode(sc, *(u_int *)data);
	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_display_on
			? WSDISPLAYIO_VIDEO_ON
			: WSDISPLAYIO_VIDEO_OFF;
		return 0;
	case WSDISPLAYIO_SVIDEO:
		switch (*(u_int *)data) {
		case WSDISPLAYIO_VIDEO_ON:
		case WSDISPLAYIO_VIDEO_OFF:
			break;
		default:
			return EINVAL;
		}
		return ssdfb_set_display_on(sc,
		    *(u_int *)data == WSDISPLAYIO_VIDEO_ON ? true : false,
		    sc->sc_usepoll);
#if 0 /* don't let userland mess with polling yet */
	case WSDISPLAYIO_SET_POLLING:
		switch (*(u_int *)data) {
		case 0:
		case 1:
			break;
		default:
			return EINVAL;
		}
		mutex_enter(&sc->sc_cond_mtx);
		ssdfb_set_usepoll(sc, *(u_int *)data ? true : false);
		cv_broadcast(&sc->sc_cond);
		mutex_exit(&sc->sc_cond_mtx);
		return 0;
#endif
	case WSDISPLAYIO_GETCMAP:
		wc = (struct wsdisplay_cmap *)data;
		if (wc->index >= __arraycount(cmap) ||
		    wc->count >  __arraycount(cmap) - wc->index)
			return EINVAL;
		error = copyout(&cmap[wc->index], wc->red, wc->count);
		if (error)
			return error;
		error = copyout(&cmap[wc->index], wc->green, wc->count);
		if (error)
			return error;
		error = copyout(&cmap[wc->index], wc->blue, wc->count);
		return error;
	case WSDISPLAYIO_PUTCMAP:
		return ENODEV;
	}

	return EPASSTHROUGH;
}

static paddr_t
ssdfb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct ssdfb_softc *sc = (struct ssdfb_softc *)v;
	struct rasops_info *ri = &sc->sc_ri;
	vaddr_t va_base = (vaddr_t)ri->ri_bits;
	paddr_t pa;

	if (off < 0 || off >= sc->sc_ri_bits_len || (off & PAGE_MASK) != 0)
		return -1;

	if (!pmap_extract(pmap_kernel(), va_base + off, &pa))
		return -1;

	return atop(pa);
}

static int
ssdfb_alloc_screen(void *v, const struct wsscreen_descr *descr, void **cookiep,
		   int *curxp, int *curyp, long *attrp)
{
	struct ssdfb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_ri;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	ri->ri_ops.allocattr(ri, 0, 0, 0, attrp);
	*cookiep = &sc->sc_ri;
	*curxp = 0;
	*curyp = 0;
	sc->sc_nscreens++;

	return 0;
}

static void
ssdfb_free_screen(void *v, void *cookie)
{
	struct ssdfb_softc *sc = v;

	if (sc->sc_is_console)
		panic("ssdfb_free_screen: is console");

	sc->sc_nscreens--;
}

static int
ssdfb_show_screen(void *v, void *cookie, int waitok,
		  void (*cb) (void *, int, int), void *cb_arg)
{
	return 0;
}

static void
ssdfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.putchar(cookie, row, col, c, attr);
	ssdfb_damage(sc);
}

static void
ssdfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.copycols(cookie, row, srccol, dstcol, ncols);
	ssdfb_damage(sc);
}

static void
ssdfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.erasecols(cookie, row, startcol, ncols, fillattr);
	ssdfb_damage(sc);
}

static void
ssdfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.copyrows(cookie, srcrow, dstrow, nrows);
	ssdfb_damage(sc);
}

static void
ssdfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.eraserows(cookie, row, nrows, fillattr);
	ssdfb_damage(sc);
}

static void
ssdfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct ssdfb_softc *sc = ri->ri_hw;

	sc->sc_orig_riops.cursor(cookie, on, row, col);
	ssdfb_damage(sc);
}

static int
ssdfb_init(struct ssdfb_softc *sc)
{
	int error;
	uint8_t cmd[2];
	bool usepoll = true;

	/*
	 * Enter sleep.
	 */
	SSDFB_CMD1(SSDFB_CMD_SET_DISPLAY_OFF);
	if (error)
		return error;
	SSDFB_CMD1(SSDFB_CMD_DEACTIVATE_SCROLL);
	if (error)
		return error;
	SSDFB_CMD1(SSDFB_CMD_ENTIRE_DISPLAY_OFF);
	if (error)
		return error;

	/*
	 * Configure physical display panel layout.
	 */
	SSDFB_CMD2(SSDFB_CMD_SET_MULTIPLEX_RATIO, sc->sc_p->p_multiplex_ratio);
	if (error)
		return error;
	SSDFB_CMD2(SSDFB_CMD_SET_DISPLAY_OFFSET, 0);
	if (error)
		return error;
	SSDFB_CMD1(SSDFB_CMD_SET_DISPLAY_START_LINE_BASE + 0x00);
	if (error)
		return error;
	SSDFB_CMD2(SSDFB_CMD_SET_COM_PINS_HARDWARE_CFG, sc->sc_p->p_compin_cfg);
	if (error)
		return error;
	if (sc->sc_upsidedown) {
		SSDFB_CMD1(SSDFB_CMD_SET_SEGMENT_REMAP_REVERSE);
		if (error)
			return error;
		SSDFB_CMD1(SSDFB_CMD_SET_COM_OUTPUT_DIRECTION_REMAP);
		if (error)
			return error;
	} else {
		SSDFB_CMD1(SSDFB_CMD_SET_SEGMENT_REMAP_NORMAL);
		if (error)
			return error;
		SSDFB_CMD1(SSDFB_CMD_SET_COM_OUTPUT_DIRECTION_NORMAL);
		if (error)
			return error;
	}
	SSDFB_CMD1(SSDFB_CMD_SET_NORMAL_DISPLAY + (uint8_t)sc->sc_inverse);
	if (error)
		return error;

	/*
	 * Configure timing characteristics.
	 */
	SSDFB_CMD2(SSDFB_CMD_SET_DISPLAY_CLOCK_RATIO,
	    ((sc->sc_p->p_fosc << SSDFB_DISPLAY_CLOCK_OSCILLATOR_SHIFT) &
	     SSDFB_DISPLAY_CLOCK_OSCILLATOR_MASK) |
	    ((sc->sc_p->p_fosc_div << SSDFB_DISPLAY_CLOCK_DIVIDER_SHIFT) &
	     SSDFB_DISPLAY_CLOCK_DIVIDER_MASK));
	if (error)
		return error;
	SSDFB_CMD2(SSDFB_CMD_SET_CONTRAST_CONTROL, sc->sc_contrast);
	if (error)
		return error;
	SSDFB_CMD2(SSDFB_CMD_SET_PRECHARGE_PERIOD,
	    ((sc->sc_p->p_precharge << SSDFB_PRECHARGE_SHIFT) &
	     SSDFB_PRECHARGE_MASK) |
	    ((sc->sc_p->p_discharge << SSDFB_DISCHARGE_SHIFT) &
	     SSDFB_DISCHARGE_MASK));
	if (error)
		return error;
	SSDFB_CMD2(SSDFB_CMD_SET_VCOMH_DESELECT_LEVEL,
	    sc->sc_p->p_vcomh_deselect_level);
	if (error)
		return error;

	/*
	 * Start charge pump.
	 */
	SSDFB_CMD2(sc->sc_p->p_chargepump_cmd, sc->sc_p->p_chargepump_arg);
	if (error)
		return error;

	if (sc->sc_p->p_controller_id == SSDFB_CONTROLLER_SH1106) {
		SSDFB_CMD2(SH1106_CMD_SET_DC_DC, SH1106_DC_DC_ON);
		if (error)
			return error;
	}

	ssdfb_clear_screen(sc);
	error = ssdfb_sync(sc, usepoll);
	if (error)
		return error;
	error = ssdfb_set_display_on(sc, true, usepoll);

	return error;
}

static int
ssdfb_set_contrast(struct ssdfb_softc *sc, uint8_t value, bool usepoll)
{
	uint8_t cmd[2];
	int error;

	sc->sc_contrast = value;
	SSDFB_CMD2(SSDFB_CMD_SET_CONTRAST_CONTROL, value);

	return error;
}

static int
ssdfb_set_display_on(struct ssdfb_softc *sc, bool value, bool usepoll)
{
	uint8_t cmd[1];
	int error;
	sc->sc_display_on = value;

	SSDFB_CMD1(value ? SSDFB_CMD_SET_DISPLAY_ON : SSDFB_CMD_SET_DISPLAY_OFF);

	return error;
}

static int
ssdfb_set_mode(struct ssdfb_softc *sc, u_int mode)
{
	switch (mode) {
	case WSDISPLAYIO_MODE_EMUL:
	case WSDISPLAYIO_MODE_DUMBFB:
		break;
	default:
		return EINVAL;
	}
	if (mode == sc->sc_mode)
		return 0;
	mutex_enter(&sc->sc_cond_mtx);
	sc->sc_mode = mode;
	cv_broadcast(&sc->sc_cond);
	mutex_exit(&sc->sc_cond_mtx);
	ssdfb_clear_screen(sc);
	ssdfb_damage(sc);

	return 0;
}

static void
ssdfb_damage(struct ssdfb_softc *sc)
{
	int s;

	if (sc->sc_usepoll) {
		(void) ssdfb_sync(sc, true);
	} else {
		/*
		 * kernel code isn't permitted to call us via kprintf at
		 * splhigh. In case misbehaving code calls us anyway we can't
		 * safely take the mutex so we skip the damage notification.
		 */
		if (sc->sc_is_console) {
			s = splhigh();
			splx(s);
			if (s == IPL_HIGH)
				return;
		}
		mutex_enter(&sc->sc_cond_mtx);
		sc->sc_modified = true;
		cv_broadcast(&sc->sc_cond);
		mutex_exit(&sc->sc_cond_mtx);
	}
}

static void
ssdfb_udv_attach(struct ssdfb_softc *sc)
{
	extern const struct cdevsw wsdisplay_cdevsw;
	dev_t dev;
#define WSDISPLAYMINOR(unit, screen)	(((unit) << 8) | (screen))
	dev = makedev(cdevsw_lookup_major(&wsdisplay_cdevsw),
	    WSDISPLAYMINOR(device_unit(sc->sc_wsdisplay), 0));
	sc->sc_uobj = udv_attach(dev, VM_PROT_READ|VM_PROT_WRITE, 0,
	    sc->sc_ri_bits_len);
}

static bool
ssdfb_is_modified(struct ssdfb_softc *sc)
{
	vaddr_t va, va_end;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
		return sc->sc_modified;

	if (sc->sc_uobj == NULL)
		return false;

	va = (vaddr_t)sc->sc_ri.ri_bits;
	va_end = va + sc->sc_ri_bits_len;
	while (va < va_end) {
		if (pmap_is_modified(uvm_pageratop(va)))
			return true;
		va += PAGE_SIZE;
	}

	return false;
}

static bool
ssdfb_clear_modify(struct ssdfb_softc *sc)
{
	vaddr_t va, va_end;
	bool ret;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		ret = sc->sc_modified;
		sc->sc_modified = false;
		return ret;
	}

	if (sc->sc_uobj == NULL)
		return false;

	va = (vaddr_t)sc->sc_ri.ri_bits;
	va_end = va + sc->sc_ri_bits_len;
	ret = false;
	while (va < va_end) {
		if (pmap_clear_modify(uvm_pageratop(va)))
			ret = true;
		va += PAGE_SIZE;
	}

	return ret;
}

static void
ssdfb_thread(void *arg)
{
	struct ssdfb_softc *sc = (struct ssdfb_softc *)arg;
	int error;

	mutex_enter(&sc->sc_cond_mtx);

	if (sc->sc_usepoll)
		ssdfb_set_usepoll(sc, false);

	while(!sc->sc_detaching) {
		if (sc->sc_mode == WSDISPLAYIO_MODE_DUMBFB &&
		    sc->sc_uobj == NULL) {
			mutex_exit(&sc->sc_cond_mtx);
			ssdfb_udv_attach(sc);
			mutex_enter(&sc->sc_cond_mtx);
		}
		if (!ssdfb_is_modified(sc)) {
			if (cv_timedwait(&sc->sc_cond, &sc->sc_cond_mtx,
			    sc->sc_mode == WSDISPLAYIO_MODE_EMUL
			    ? 0 : sc->sc_backoff) == EWOULDBLOCK
			    && sc->sc_backoff < mstohz(200)) {
				sc->sc_backoff <<= 1;
			}
			continue;
		}
		sc->sc_backoff = 1;
		(void) ssdfb_clear_modify(sc);
		if (sc->sc_usepoll)
			continue;
		mutex_exit(&sc->sc_cond_mtx);
		error = ssdfb_sync(sc, false);
		if (error)
			device_printf(sc->sc_dev, "ssdfb_sync: error %d\n",
			    error);
		mutex_enter(&sc->sc_cond_mtx);
	}

	mutex_exit(&sc->sc_cond_mtx);
}

static void
ssdfb_set_usepoll(struct ssdfb_softc *sc, bool enable)
{
	sc->sc_usepoll = enable;
}

static int
ssdfb_sync(struct ssdfb_softc *sc, bool usepoll)
{
	struct rasops_info *ri = &sc->sc_ri;
	int block_size = 8;
	int ri_block_stride = ri->ri_stride * block_size;
	int height_in_blocks = sc->sc_p->p_height / block_size;
	int width_in_blocks  = sc->sc_p->p_width / block_size;
	int ri_block_step = block_size * ri->ri_depth / 8;
	int x, y;
	union ssdfb_block *blockp;
	uint64_t raw_block;
	uint8_t *src;
	int x1, x2, y1, y2;

	/*
	 * Transfer rasops bitmap into gddram shadow buffer while keeping track
	 * of the bounding box of the dirty region we scribbled over.
	 */
	x1 = width_in_blocks;
	x2 = -1;
	y1 = height_in_blocks;
	y2 = -1;
	for (y = 0; y < height_in_blocks; y++) {
		src = &ri->ri_bits[y*ri_block_stride];
		blockp = &sc->sc_gddram[y * width_in_blocks];
		for (x = 0; x < width_in_blocks; x++) {
			raw_block = (ri->ri_depth == 1)
			   ? ssdfb_transpose_block_1bpp(src, ri->ri_stride)
			   : ssdfb_transpose_block_8bpp(src, ri->ri_stride);
			if (raw_block != blockp->raw) {
				blockp->raw = raw_block;
				if (x1 > x)
					x1 = x;
				if (x2 < x)
					x2 = x;
				if (y1 > y)
					y1 = y;
				if (y2 < y)
					y2 = y;
			}
			src += ri_block_step;
			blockp++;
		}
	}
	if (x2 != -1)
		return sc->sc_transfer_rect(sc->sc_cookie,
		    x1 * block_size + sc->sc_p->p_panel_shift,
		    (x2 + 1) * block_size - 1 + sc->sc_p->p_panel_shift,
		    y1,
		    y2,
		    &sc->sc_gddram[y1 * width_in_blocks + x1].col[0],
		    sc->sc_p->p_width,
		    usepoll);

	return 0;
}

static uint64_t
ssdfb_transpose_block_1bpp(uint8_t *src, size_t src_stride)
{
	uint64_t x = 0;
	uint64_t t;
	int i;

	/*
	 * collect the 8x8 block.
	 */
	for (i = 0; i < 8; i++) {
		x >>= 8;
		x |= (uint64_t)src[i * src_stride] << 56;
	}

	/*
	 * Transpose it into gddram layout.
	 * Post-transpose bswap is the same as pre-transpose bit order reversal.
	 * We do this to match rasops1 bit order.
	 */
	t = (x ^ (x >> 28)) & 0x00000000F0F0F0F0ULL;
	x = x ^ t ^ (t << 28);
	t = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCULL;
	x = x ^ t ^ (t << 14);
	t = (x ^ (x >>  7)) & 0x00AA00AA00AA00AAULL;
	x = x ^ t ^ (t <<  7);
	x = bswap64(x);

	return htole64(x);
}

static uint64_t
ssdfb_transpose_block_8bpp(uint8_t *src, size_t src_stride)
{
	uint64_t x = 0;
	int m, n;

	for (m = 0; m < 8; m++) {
		for (n = 0; n < 8; n++) {
			x >>= 1;
			x |= src[n * src_stride + m] ? (1ULL << 63) : 0;
		}
	}

	return htole64(x);
}

static const struct ssdfb_product *
ssdfb_lookup_product(ssdfb_product_id_t id)
{
	int i;

	for (i = 0; i < __arraycount(ssdfb_products); i++) {
		if (ssdfb_products[i].p_product_id == id)
			return &ssdfb_products[i];
	}

	return NULL;
}

static int
ssdfb_pick_font(int *cookiep, struct wsdisplay_font **fontp)
{
	int error;
	int c;
	struct wsdisplay_font *f;
	int i;
	uint8_t d[4][2] = {{5, 8}, {8, 8}, {8, 10} ,{8, 16}};

	/*
	 * Try to find fonts in order of inreasing size.
	 */
	wsfont_init();
	for(i = 0; i < __arraycount(d); i++) {
		c = wsfont_find(NULL, d[i][0], d[i][1], 0,
		    WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R,
		    WSFONT_FIND_BITMAP);
		if (c > 0)
			break;
	}
	if (c <= 0)
		return ENOENT;
	error = wsfont_lock(c, &f);
	if (error)
		return error;
	*cookiep = c;
	*fontp = f;

	return 0;
}

static void
ssdfb_clear_screen(struct ssdfb_softc *sc)
{
	struct rasops_info *ri = &sc->sc_ri;

	memset(sc->sc_gddram, 0xff, sc->sc_gddram_len);
	memset(ri->ri_bits, 0, sc->sc_ri_bits_len);
}

#if defined(DDB)
static void
ssdfb_ddb_trap_callback(int enable)
{
	extern struct cfdriver ssdfb_cd;
	struct ssdfb_softc *sc;
	int i;

	for (i = 0; i < ssdfb_cd.cd_ndevs; i++) {
		sc = device_lookup_private(&ssdfb_cd, i);
		if (sc != NULL && sc->sc_is_console) {
			ssdfb_set_usepoll(sc, (bool)enable);
		}
        }
}
#endif
