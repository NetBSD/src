/* $NetBSD: w100lcd.c,v 1.6 2022/05/28 10:36:23 andvar Exp $ */
/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LCD driver for Sharp Zaurus SL-C7x0/860, based on zlcd.c.
 *
 * Controlling LCD is done in w100.c.
 *
 * Codes in this file provide platform specific things including:
 *   LCD panel geometry
 *
 * LCD on/off switch and backlight brightness are done in lcdctl.c.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: w100lcd.c,v 1.6 2022/05/28 10:36:23 andvar Exp $");

#include "lcdctl.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/hpc/hpcfbio.h>

#include <arm/xscale/pxa2x0var.h>
#include <zaurus/dev/w100var.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/w100lcdvar.h>
#if NLCDCTL > 0
#include <zaurus/dev/lcdctlvar.h>
#endif

/*
 * wsdisplay glue
 */
static struct w100_wsscreen_descr w100lcd_std_screen = {
	.c = {
		.name = "std",
		.textops = &w100_emulops,
		.fontwidth = 8,
		.fontheight = 16,
		.capabilities = WSSCREEN_WSCOLORS,
	},
	.depth = 16,			/* bits per pixel */
	.flags = 0,
};

static const struct wsscreen_descr *w100lcd_scr_descr[] = {
	&w100lcd_std_screen.c
};

static const struct wsscreen_list w100lcd_screen_list = {
	.nscreens = __arraycount(w100lcd_scr_descr),
	.screens = w100lcd_scr_descr,
};

static int	w100lcd_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static int	w100lcd_show_screen(void *, void *, int,
		    void (*)(void *, int, int), void *);

struct wsdisplay_accessops w100lcd_accessops = {
	w100lcd_ioctl,
	w100_mmap,
	w100_alloc_screen,
	w100_free_screen,
	w100lcd_show_screen,
	NULL,
	NULL,
	NULL,
};

const struct w100_panel_geometry lcd_panel_geometry_c700 =
{
	480,			/* Width */
	640,			/* Height */
	W100_PANEL_ROTATE_CW,   /* Rotate */
};

static int	w100lcd_match(device_t, cfdata_t, void *);
static void	w100lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(w100lcd, sizeof(struct w100_softc),
    w100lcd_match, w100lcd_attach, NULL, NULL);

static bool	w100lcd_suspend(device_t, const pmf_qual_t *);
static bool	w100lcd_resume(device_t, const pmf_qual_t *);

static int
w100lcd_match(device_t parent, cfdata_t cf, void *aux)
{

	if (ZAURUS_ISC860)
		return 1;
	return 0;
}

static void
w100lcd_attach(device_t parent, device_t self, void *aux)
{
	struct w100_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = (struct pxaip_attach_args *)aux;
	struct wsemuldisplaydev_attach_args aa;

	sc->dev = self;

	w100_attach_subr(sc, pxa->pxa_iot, &lcd_panel_geometry_c700);

	aa.console = glass_console;
	aa.scrdata = &w100lcd_screen_list;
	aa.accessops = &w100lcd_accessops;
	aa.accesscookie = sc;

	(void)config_found(self, &aa, wsemuldisplaydevprint, CFARGS_NONE);

	if (!pmf_device_register(self, w100lcd_suspend, w100lcd_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

void
w100lcd_cnattach(void)
{

	if (ZAURUS_ISC860)
		w100_cnattach(&w100lcd_std_screen, &lcd_panel_geometry_c700);
}

/*
 * power management
 */
static bool
w100lcd_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct w100_softc *sc = device_private(dv);

#if NLCDCTL > 0
	lcdctl_onoff(false);
#endif
	w100_suspend(sc);

	return true;
}

static bool
w100lcd_resume(device_t dv, const pmf_qual_t *qual)
{
	struct w100_softc *sc = device_private(dv);

	w100_resume(sc);
#if NLCDCTL > 0
	lcdctl_onoff(true);
#endif

	return true;
}

/*
 * wsdisplay accessops overrides
 */
static int
w100lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct w100_softc *sc = (struct w100_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	int res = EINVAL;

	switch (cmd) {
#if NLCDCTL > 0
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
		res = lcdctl_param(cmd, (struct wsdisplay_param *)data);
		break;
#endif
	case HPCFBIO_GCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			break;
		}

		fbconf->hf_conf_index = 0;
		fbconf->hf_nconfs = 1;
		fbconf->hf_class = HPCFB_CLASS_RGBCOLOR;
		strlcpy(fbconf->hf_name, "Sharp Zaurus frame buffer",
		    sizeof(fbconf->hf_name));
		strlcpy(fbconf->hf_conf_name, "default",
		    sizeof(fbconf->hf_conf_name));
		fbconf->hf_width = sc->display_width;
		fbconf->hf_height = sc->display_height;
		fbconf->hf_baseaddr = (u_long)sc->active->buf_va;
		fbconf->hf_offset = 0;
		fbconf->hf_bytes_per_line = sc->display_width *
		    sc->active->depth / 8;
		fbconf->hf_nplanes = 1;
		fbconf->hf_bytes_per_plane = sc->display_width *
		    sc->display_height * sc->active->depth / 8;
		fbconf->hf_pack_width = sc->active->depth;
		fbconf->hf_pixels_per_pack = 1;
		fbconf->hf_pixel_width = sc->active->depth;
		fbconf->hf_access_flags = (0
					   | HPCFB_ACCESS_BYTE
					   | HPCFB_ACCESS_WORD
					   | HPCFB_ACCESS_DWORD);
		fbconf->hf_order_flags = 0;
		fbconf->hf_reg_offset = 0;

		fbconf->hf_class_data_length = sizeof(struct hf_rgb_tag);
		fbconf->hf_u.hf_rgb.hf_flags = 0;
		fbconf->hf_u.hf_rgb.hf_red_width = 5;
		fbconf->hf_u.hf_rgb.hf_red_shift = 11;
		fbconf->hf_u.hf_rgb.hf_green_width = 6;
		fbconf->hf_u.hf_rgb.hf_green_shift = 5;
		fbconf->hf_u.hf_rgb.hf_blue_width = 5;
		fbconf->hf_u.hf_rgb.hf_blue_shift = 0;
		fbconf->hf_u.hf_rgb.hf_alpha_width = 0;
		fbconf->hf_u.hf_rgb.hf_alpha_shift = 0;

		fbconf->hf_ext_size = 0;
		fbconf->hf_ext_data = NULL;

		res = 0;
		break;

	case HPCFBIO_SCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			break;
		}
		/* nothing to do because we have only one configuration */
		res = 0;
		break;

	case HPCFBIO_GDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		     dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
		     dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			break;
		}

		dspconf->hd_unit_index = 0;
		dspconf->hd_nunits = 1;
		dspconf->hd_class = HPCFB_DSP_CLASS_COLORLCD;
		strlcpy(dspconf->hd_name, "Sharp Zaurus LCD",
		    sizeof(dspconf->hd_name));
		dspconf->hd_op_flags = 0;
		dspconf->hd_conf_index = 0;
		dspconf->hd_nconfs = 1;
		strlcpy(dspconf->hd_conf_name, "default",
		    sizeof(dspconf->hd_conf_name));
		dspconf->hd_width = sc->display_width;
		dspconf->hd_height = sc->display_height;
		dspconf->hd_xdpi = HPCFB_DSP_DPI_UNKNOWN;
		dspconf->hd_ydpi = HPCFB_DSP_DPI_UNKNOWN;

		res = 0;
		break;

	case HPCFBIO_SDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		     dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
		     dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			break;
		}
		/*
		 * nothing to do
		 * because we have only one unit and one configuration
		 */
		res = 0;
		break;

	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		/* currently not implemented...  */
		break;

	default:
		break;
	}

	if (res == EINVAL)
		res = w100_ioctl(v, vs, cmd, data, flag, l);

	return res;
}

static int
w100lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb_func)(void *, int, int), void *cb_arg)
{
	int error;

	error = w100_show_screen(v, cookie, waitok, cb_func, cb_arg);
	if (error)
		return (error);

	/* Turn on LCD */
#if NLCDCTL > 0
	lcdctl_onoff(true);
#endif

	return 0;
}
