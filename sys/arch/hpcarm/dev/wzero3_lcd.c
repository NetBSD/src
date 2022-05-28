/*	$NetBSD: wzero3_lcd.c,v 1.8 2022/05/28 10:36:22 andvar Exp $	*/

/*-
 * Copyright (C) 2008, 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_lcd.c,v 1.8 2022/05/28 10:36:22 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/pmf.h>
#include <sys/bus.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/hpc/hpcfbio.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

/*
 * wsdisplay glue
 */
static struct pxa2x0_wsscreen_descr wzero3lcd_std_screen = {
	.c = {
		.name = "std",
		.textops = &pxa2x0_lcd_emulops,
		.fontwidth = 8,
		.fontheight = 16,
		.capabilities = WSSCREEN_WSCOLORS,
	},
	.depth = 16,			/* bits per pixel */
	.flags = 0,
};

static const struct wsscreen_descr *wzero3lcd_scr_descr[] = {
	&wzero3lcd_std_screen.c
};

static const struct wsscreen_list wzero3lcd_screen_list = {
	.nscreens = __arraycount(wzero3lcd_scr_descr),
	.screens = wzero3lcd_scr_descr,
};

static int wzero3lcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int wzero3lcd_param(struct pxa2x0_lcd_softc *, u_long, struct wsdisplay_param *);
static int wzero3lcd_show_screen(void *, void *, int, void (*)(void *, int, int), void *);

static struct wsdisplay_accessops wzero3lcd_accessops = {
	wzero3lcd_ioctl,
	pxa2x0_lcd_mmap,
	pxa2x0_lcd_alloc_screen,
	pxa2x0_lcd_free_screen,
	wzero3lcd_show_screen,
	NULL,
	NULL,
	NULL,
};

/* WS003SH or WS004SH */
static const struct lcd_panel_geometry sharp_ws003sh = {
	480,			/* Width */
	640,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_VSP | LCDPANEL_HSP,
	1,			/* clock divider */
	0,			/* AC bias pin freq */

	0x14,			/* horizontal sync pulse width */
	0x4e,			/* BLW */
	0x46,			/* ELW */

	0,			/* vertical sync pulse width */
	2,			/* BFW */
	5,			/* EFW */

	0,			/* PCDDIV */
};

/* WS007SH */
static const struct lcd_panel_geometry sharp_ws007sh = {
	480,			/* Width */
	640,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_VSP | LCDPANEL_HSP | LCDPANEL_PCP | LCDPANEL_OEP,
	3,			/* clock divider */
	0,			/* AC bias pin freq */

	0x27,			/* horizontal sync pulse width */
	0x68,			/* BLW */
	0x5b,			/* ELW */

	0,			/* vertical sync pulse width */
	2,			/* BFW */
	5,			/* EFW */

	1,			/* PCDDIV */
};

/* WS011SH */
static const struct lcd_panel_geometry sharp_ws011sh = {
	480,			/* Width */
	800,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_VSP | LCDPANEL_HSP | LCDPANEL_PCP,
	1,			/* clock divider */
	0,			/* AC bias pin freq */

	0x0a,			/* horizontal sync pulse width */
	0x0c,			/* BLW */
	0x5e,			/* ELW */

	0,			/* vertical sync pulse width */
	2,			/* BFW */
	1,			/* EFW */

	0,			/* PCDDIV */
};

/* WS020SH */
static const struct lcd_panel_geometry sharp_ws020sh = {
	480,			/* Width */
	800,			/* Height */
	0,			/* No extra lines */

	LCDPANEL_ACTIVE | LCDPANEL_VSP | LCDPANEL_HSP | LCDPANEL_PCP,
	1,			/* clock divider */
	0,			/* AC bias pin freq */

	0x0a,			/* horizontal sync pulse width */
	0x0c,			/* BLW */
	0x5e,			/* ELW */

	0,			/* vertical sync pulse width */
	2,			/* BFW */
	1,			/* EFW */

	0,			/* PCDDIV */
};

static int	wzero3lcd_match(device_t, cfdata_t, void *);
static void	wzero3lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3lcd, sizeof(struct pxa2x0_lcd_softc),
	wzero3lcd_match, wzero3lcd_attach, NULL, NULL);

static const struct lcd_panel_geometry *wzero3lcd_lookup(void);
void wzero3lcd_cnattach(void);
static bool wzero3lcd_suspend(device_t dv, const pmf_qual_t *);
static bool wzero3lcd_resume(device_t dv, const pmf_qual_t *);

/* default: quarter counter clockwise rotation */
int screen_rotate = 270;

static const struct lcd_panel_geometry *
wzero3lcd_lookup(void)
{

	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS003SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS004SH))
		return &sharp_ws003sh;
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS007SH))
		return &sharp_ws007sh;
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH))
		return &sharp_ws011sh;
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS020SH))
		return &sharp_ws020sh;
	return NULL;
}

static int
wzero3lcd_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "lcd") != 0)
		return 0;
	if (wzero3lcd_lookup() == NULL)
		return 0;
	return 1;
}

static void
wzero3lcd_attach(device_t parent, device_t self, void *aux)
{
	struct pxa2x0_lcd_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args aa;
	const struct lcd_panel_geometry *panel;

	sc->dev = self;

	panel = wzero3lcd_lookup();
	if (panel == NULL) {
		aprint_error(": unknown model\n");
		return;
	}

	if ((platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS007SH))
	 || (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH))
	 || (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS020SH)))
		sc->flags |= FLAG_NOUSE_ACBIAS;

	wzero3lcd_std_screen.flags &= ~(RI_ROTATE_MASK);
	switch (screen_rotate) {
	default:
		break;

	case 270:	/* quarter counter clockwise rotation */
		wzero3lcd_std_screen.flags |= RI_ROTATE_CCW;
		break;
	}
	pxa2x0_lcd_attach_sub(sc, aux, panel);

	aa.console = (bootinfo->bi_cnuse != BI_CNUSE_SERIAL);
	aa.scrdata = &wzero3lcd_screen_list;
	aa.accessops = &wzero3lcd_accessops;
	aa.accesscookie = sc;

	(void) config_found(self, &aa, wsemuldisplaydevprint, CFARGS_NONE);

	if (!pmf_device_register(sc->dev, wzero3lcd_suspend, wzero3lcd_resume))
		aprint_error_dev(sc->dev, "couldn't establish power handler\n");
}

void
wzero3lcd_cnattach(void)
{
	const struct lcd_panel_geometry *panel;

	panel = wzero3lcd_lookup();
	if (panel == NULL)
		return;

	pxa2x0_lcd_cnattach(&wzero3lcd_std_screen, panel);
}

/*
 * Power management
 */
static bool
wzero3lcd_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

	pxa2x0_lcd_suspend(sc);

	return true;
}

static bool
wzero3lcd_resume(device_t dv, const pmf_qual_t *qual)
{
	struct pxa2x0_lcd_softc *sc = device_private(dv);

	pxa2x0_lcd_resume(sc);

	return true;
}

/*
 * wsdisplay accessops overrides
 */
static int
wzero3lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	int res = EINVAL;

	switch (cmd) {
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
		res = wzero3lcd_param(sc, cmd, (struct wsdisplay_param *)data);
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
		strlcpy(fbconf->hf_name, "Sharp W-ZERO3 frame buffer",
		    sizeof(fbconf->hf_name));
		strlcpy(fbconf->hf_conf_name, "LCD",
		    sizeof(fbconf->hf_conf_name));
		fbconf->hf_baseaddr = (u_long)sc->active->buf_va;
		fbconf->hf_width = sc->geometry->panel_width;
		fbconf->hf_height = sc->geometry->panel_height;
		fbconf->hf_offset = 0;
		fbconf->hf_bytes_per_line = fbconf->hf_width *
		    sc->active->depth / 8;
		fbconf->hf_nplanes = 1;
		fbconf->hf_bytes_per_plane = fbconf->hf_width *
		    fbconf->hf_height * sc->active->depth / 8;
		fbconf->hf_pack_width = sc->active->depth;
		fbconf->hf_pixels_per_pack = 1;
		fbconf->hf_pixel_width = sc->active->depth;
		fbconf->hf_access_flags = (HPCFB_ACCESS_STATIC
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
		strlcpy(dspconf->hd_name, "PXA2x0 Internal LCD controller",
		    sizeof(dspconf->hd_name));
		dspconf->hd_op_flags = 0;
		dspconf->hd_conf_index = 0;
		dspconf->hd_nconfs = 1;
		strlcpy(dspconf->hd_conf_name, "LCD",
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
wzero3lcd_show_screen(void *v, void *cookie, int waitok, void (*cb_func)(void *, int, int), void *cb_arg)
{
	int error;

	error = pxa2x0_lcd_show_screen(v, cookie, waitok, cb_func, cb_arg);
	if (error)
		return (error);

	return 0;
}

/*
 * wsdisplay I/O controls
 */
static int
wzero3lcd_param(struct pxa2x0_lcd_softc *sc, u_long cmd, struct wsdisplay_param *dp)
{
	int res = EINVAL;

	switch (dp->param) {
	case WSDISPLAYIO_PARAM_BACKLIGHT:
		/* unsupported */
		DPRINTF(("%s: ioctl(WSDISPLAYIO_PARAM_BACKLIGHT) isn't supported\n", device_xname(sc->dev)));
		res = ENOTTY;
		break;

	case WSDISPLAYIO_PARAM_CONTRAST:
		DPRINTF(("%s: ioctl(WSDISPLAYIO_PARAM_CONTRAST) isn't supported\n", device_xname(sc->dev)));
		/* unsupported */
		res = ENOTTY;
		break;

	case WSDISPLAYIO_PARAM_BRIGHTNESS:
		DPRINTF(("%s: ioctl(WSDISPLAYIO_PARAM_BRIGHTNESS) isn't supported\n", device_xname(sc->dev)));
		/* unsupported */
		res = ENOTTY;
	}

	return res;
}
