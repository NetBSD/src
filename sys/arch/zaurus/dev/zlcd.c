/*	$NetBSD: zlcd.c,v 1.11.8.1 2011/06/23 14:19:51 cherry Exp $	*/
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
 *   LCD on/off switch and backlight brightness
 *   LCD panel geometry
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zlcd.c,v 1.11.8.1 2011/06/23 14:19:51 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/hpc/hpcfbio.h>

#include <machine/bus.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/zlcdvar.h>
#include <zaurus/dev/zsspvar.h>
#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/ioexpvar.h>

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
static int	lcd_param(struct pxa2x0_lcd_softc *, u_long,
		    struct wsdisplay_param *);
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

#define CURRENT_DISPLAY &sharp_zaurus_C3000

const struct lcd_panel_geometry sharp_zaurus_C3000 =
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

struct sharp_lcd_backlight {
	int	duty;		/* LZ9JG18 DAC value */
	int	cont;		/* BACKLIGHT_CONT signal */
	int	on;		/* BACKLIGHT_ON signal */
};

#define CURRENT_BACKLIGHT sharp_zaurus_C3000_bl

const struct sharp_lcd_backlight sharp_zaurus_C3000_bl[] = {
	{ 0x00, 0,  0 },	/* 0:     Off */
	{ 0x00, 0,  1 },	/* 1:      0% */
	{ 0x01, 0,  1 },	/* 2:     20% */
	{ 0x07, 0,  1 },	/* 3:     40% */
	{ 0x01, 1,  1 },	/* 4:     60% */
	{ 0x07, 1,  1 },	/* 5:     80% */
	{ 0x11, 1,  1 },	/* 6:    100% */
	{  -1, -1, -1 },	/* 7: Invalid */
};

static int	lcd_match(device_t, cfdata_t, void *);
static void	lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zlcd, sizeof(struct pxa2x0_lcd_softc),
	lcd_match, lcd_attach, NULL, NULL);

static bool	lcd_suspend(device_t, const pmf_qual_t *);
static bool	lcd_resume(device_t, const pmf_qual_t *);
static void	lcd_brightness_up(device_t);
static void	lcd_brightness_down(device_t);
static void	lcd_display_on(device_t);
static void	lcd_display_off(device_t);

static int	lcd_max_brightness(void);
static int	lcd_get_brightness(void);
static void	lcd_set_brightness(int);
static void	lcd_set_brightness_internal(int);
static int	lcd_get_backlight(void);
static void	lcd_set_backlight(int);

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

	pxa2x0_lcd_attach_sub(sc, aux, CURRENT_DISPLAY);

	aa.console = glass_console;
	aa.scrdata = &lcd_screen_list;
	aa.accessops = &lcd_accessops;
	aa.accesscookie = sc;

	(void) config_found(self, &aa, wsemuldisplaydevprint);

	/* Start with approximately 40% of full brightness. */
	lcd_set_brightness(3);

	if (!pmf_device_register(self, lcd_suspend, lcd_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    lcd_brightness_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    lcd_brightness_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_ON,
	    lcd_display_on, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_OFF,
	    lcd_display_off, true))
		aprint_error_dev(self, "couldn't register event handler\n");
}

void
lcd_cnattach(void)
{

	pxa2x0_lcd_cnattach(&lcd_std_screen, CURRENT_DISPLAY);
}

/*
 * power management
 */
static bool
lcd_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

	lcd_set_brightness(0);
	pxa2x0_lcd_suspend(sc);

	return true;
}

static bool
lcd_resume(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

	pxa2x0_lcd_resume(sc);
	lcd_set_brightness(lcd_get_brightness());

	return true;
}

static void
lcd_brightness_up(device_t dv)
{

	lcd_set_brightness(lcd_get_brightness() + 1);
}

static void
lcd_brightness_down(device_t dv)
{

	lcd_set_brightness(lcd_get_brightness() - 1);
}

static void
lcd_display_on(device_t dv)
{

	lcd_blank(0);
	lcd_set_backlight(1);
}

static void
lcd_display_off(device_t dv)
{

	lcd_set_backlight(0);
	lcd_blank(1);
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
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
		res = lcd_param(sc, cmd, (struct wsdisplay_param *)data);
		break;

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
		/* curently not implemented...  */
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

	/* Turn on LCD */
	lcd_set_brightness(lcd_get_brightness());

	return 0;
}

/*
 * wsdisplay I/O controls
 */
static int
lcd_param(struct pxa2x0_lcd_softc *sc, u_long cmd, struct wsdisplay_param *dp)
{
	int res = EINVAL;

	switch (dp->param) {
	case WSDISPLAYIO_PARAM_BACKLIGHT:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 0;
			dp->max = 1;
			dp->curval = lcd_get_backlight();
			res = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			lcd_set_backlight(dp->curval);
			res = 0;
		}
		break;

	case WSDISPLAYIO_PARAM_CONTRAST:
		/* unsupported */
		res = ENOTTY;
		break;

	case WSDISPLAYIO_PARAM_BRIGHTNESS:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 1;
			dp->max = lcd_max_brightness();
			dp->curval = lcd_get_brightness();
			res = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			lcd_set_brightness(dp->curval);
			res = 0;
		}
		break;
	}

	return res;
}

/*
 * LCD backlight
 */

static	int lcdbrightnesscurval = 1;
static	int lcdislit = 1;
static	int lcdisblank = 0;

static int
lcd_max_brightness(void)
{
	int i;

	for (i = 0; CURRENT_BACKLIGHT[i].duty != -1; i++)
		continue;
	return i - 1;
}

static int
lcd_get_brightness(void)
{

	return lcdbrightnesscurval;
}

static void
lcd_set_brightness(int newval)
{
	int maxval;

	maxval = lcd_max_brightness();
	if (newval < 0)
		newval = 0;
	else if (newval > maxval)
		newval = maxval;

	if (lcd_get_backlight() && !lcdisblank)
		lcd_set_brightness_internal(newval);

	if (newval > 0)
		lcdbrightnesscurval = newval;
}

static void
lcd_set_brightness_internal(int newval)
{
	static int curval = 1;
	int i;

	/*
	 * It appears that the C3000 backlight can draw too much power if we
	 * switch it from a low to a high brightness.  Increasing brightness
	 * in steps avoids this issue.
	 */
	if (newval > curval) {
		for (i = curval + 1; i <= newval; i++) {
			(void)zssp_ic_send(ZSSP_IC_LZ9JG18,
			    CURRENT_BACKLIGHT[i].duty);
			if (ZAURUS_ISC1000)
				ioexp_set_backlight(CURRENT_BACKLIGHT[i].on,
				    CURRENT_BACKLIGHT[i].cont);
			else
				scoop_set_backlight(CURRENT_BACKLIGHT[i].on,
				    CURRENT_BACKLIGHT[i].cont);
			delay(5000);
		}
	} else {
		(void)zssp_ic_send(ZSSP_IC_LZ9JG18,
		    CURRENT_BACKLIGHT[newval].duty);
		if (ZAURUS_ISC1000)
			ioexp_set_backlight(CURRENT_BACKLIGHT[newval].on,
			    CURRENT_BACKLIGHT[newval].cont);
		else
			scoop_set_backlight(CURRENT_BACKLIGHT[newval].on,
			    CURRENT_BACKLIGHT[newval].cont);
	}

	curval = newval;
}

static int
lcd_get_backlight(void)
{

	return lcdislit;
}

static void
lcd_set_backlight(int onoff)
{

	if (!onoff) {
		lcd_set_brightness(0);
		lcdislit = 0;
	} else {
		lcdislit = 1;
		lcd_set_brightness(lcd_get_brightness());
	}
}

void
lcd_blank(int blank)
{

	if (blank) {
		lcd_set_brightness(0);
		lcdisblank = 1;
	} else {
		lcdisblank = 0;
		lcd_set_brightness(lcd_get_brightness());
	}
}
