/*	$NetBSD: zlcd.c,v 1.21 2022/05/28 10:36:23 andvar Exp $	*/
/*	$OpenBSD: zaurus_lcd.c,v 1.20 2006/06/02 20:50:14 miod Exp $	*/
/* NetBSD: lubbock_lcd.c,v 1.1 2003/08/09 19:38:53 bsh Exp */

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
 * LCD driver for Sharp Zaurus (based on the Intel Lubbock driver).
 *
 * Controlling LCD is almost completely done through PXA2X0's
 * integrated LCD controller.  Codes for it is arm/xscale/pxa2x0_lcd.c.
 *
 * Codes in this file provide platform specific things including:
 *   LCD panel geometry
 *
 * LCD on/off switch and backlight brightness are done in lcdctl.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zlcd.c,v 1.21 2022/05/28 10:36:23 andvar Exp $");

#include "lcdctl.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/hpc/hpcfbio.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/zlcdvar.h>
#if NLCDCTL > 0
#include <zaurus/dev/lcdctlvar.h>
#endif

/*
 * wsdisplay glue
 */
static struct pxa2x0_wsscreen_descr lcd_std_screen = {
	.c = {
		.name = "std",
		.textops = &pxa2x0_lcd_emulops,
		.fontwidth = 8,
		.fontheight = 16,
		.capabilities = WSSCREEN_WSCOLORS,
	},
	.depth = 16,			/* bits per pixel */
	.flags = RI_ROTATE_CW,		/* quarter clockwise rotation */
};

static const struct wsscreen_descr *lcd_scr_descr[] = {
	&lcd_std_screen.c
};

static const struct wsscreen_list lcd_screen_list = {
	.nscreens = __arraycount(lcd_scr_descr),
	.screens = lcd_scr_descr,
};

static int	lcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int	lcd_show_screen(void *, void *, int,
		    void (*)(void *, int, int), void *);

struct wsdisplay_accessops lcd_accessops = {
	lcd_ioctl,
	pxa2x0_lcd_mmap,
	pxa2x0_lcd_alloc_screen,
	pxa2x0_lcd_free_screen,
	lcd_show_screen,
	NULL,
	NULL,
	NULL,
};

const struct lcd_panel_geometry lcd_panel_geometry_c3000 =
{
	480,			/* Width */
	640,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_VSP | LCDPANEL_HSP,
	1,			/* clock divider */
	0,			/* AC bias pin freq */

	0x28,			/* horizontal sync pulse width */
	0x2e,			/* BLW */
	0x7d,			/* ELW */

	2,			/* vertical sync pulse width */
	1,			/* BFW */
	0,			/* EFW */
};

static int	lcd_match(device_t, cfdata_t, void *);
static void	lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zlcd, sizeof(struct pxa2x0_lcd_softc),
	lcd_match, lcd_attach, NULL, NULL);

static bool	lcd_suspend(device_t, const pmf_qual_t *);
static bool	lcd_resume(device_t, const pmf_qual_t *);

static int
lcd_match(device_t parent, cfdata_t cf, void *aux)
{

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000)
		return 1;
	return 0;
}

static void
lcd_attach(device_t parent, device_t self, void *aux)
{
	struct pxa2x0_lcd_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args aa;

	sc->dev = self;

	pxa2x0_lcd_attach_sub(sc, aux, &lcd_panel_geometry_c3000);

	aa.console = glass_console;
	aa.scrdata = &lcd_screen_list;
	aa.accessops = &lcd_accessops;
	aa.accesscookie = sc;

	(void) config_found(self, &aa, wsemuldisplaydevprint, CFARGS_NONE);

	if (!pmf_device_register(self, lcd_suspend, lcd_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

void
lcd_cnattach(void)
{

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000)
		pxa2x0_lcd_cnattach(&lcd_std_screen, &lcd_panel_geometry_c3000);
}

/*
 * power management
 */
static bool
lcd_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

#if NLCDCTL > 0
	lcdctl_onoff(false);
#endif
	pxa2x0_lcd_suspend(sc);

	return true;
}

static bool
lcd_resume(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

	pxa2x0_lcd_resume(sc);
#if NLCDCTL > 0
	lcdctl_onoff(true);
#endif

	return true;
}

/*
 * wsdisplay accessops overrides
 */
static int
lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)v;
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
		fbconf->hf_width = sc->geometry->panel_width;
		fbconf->hf_height = sc->geometry->panel_height;
		fbconf->hf_baseaddr = (u_long)sc->active->buf_va;
		fbconf->hf_offset = 0;
		fbconf->hf_bytes_per_line = sc->geometry->panel_width *
		    sc->active->depth / 8;
		fbconf->hf_nplanes = 1;
		fbconf->hf_bytes_per_plane = sc->geometry->panel_width *
		    sc->geometry->panel_height * sc->active->depth / 8;
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
		dspconf->hd_width = sc->geometry->panel_width;
		dspconf->hd_height = sc->geometry->panel_height;
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
	}

	if (res == EINVAL)
		res = pxa2x0_lcd_ioctl(v, vs, cmd, data, flag, l);
	return res;
}

static int
lcd_show_screen(void *v, void *cookie, int waitok,
    void (*cb_func)(void *, int, int), void *cb_arg)
{
	int error;

	error = pxa2x0_lcd_show_screen(v, cookie, waitok, cb_func, cb_arg);
	if (error)
		return (error);

#if NLCDCTL > 0
	/* Turn on LCD */
	lcdctl_onoff(true);
#endif

	return 0;
}
