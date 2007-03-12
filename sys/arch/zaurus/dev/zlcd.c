/*	$NetBSD: zlcd.c,v 1.4.2.1 2007/03/12 05:51:50 rmind Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: zlcd.c,v 1.4.2.1 2007/03/12 05:51:50 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <machine/bus.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0_lcd.h>

#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/zsspvar.h>
#include <zaurus/zaurus/zaurus_var.h>

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

static int	lcd_match(struct device *, struct cfdata *, void *);
static void	lcd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(zlcd, sizeof(struct pxa2x0_lcd_softc),
	lcd_match, lcd_attach, NULL, NULL);

void	lcd_cnattach(void);
int	lcd_max_brightness(void);
int	lcd_get_brightness(void);
void	lcd_set_brightness(int);
void	lcd_set_brightness_internal(int);
int	lcd_get_backlight(void);
void	lcd_set_backlight(int);
void	lcd_blank(int);
void	lcd_power(int, void *);

static int
lcd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

static void
lcd_attach(struct device *parent, struct device *self, void *aux)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)self;
	struct wsemuldisplaydev_attach_args aa;

	pxa2x0_lcd_attach_sub(sc, aux, CURRENT_DISPLAY);

	aa.console = glass_console;
	aa.scrdata = &lcd_screen_list;
	aa.accessops = &lcd_accessops;
	aa.accesscookie = sc;

	(void) config_found(self, &aa, wsemuldisplaydevprint);

	/* Start with approximately 40% of full brightness. */
	lcd_set_brightness(3);

	(void) powerhook_establish(sc->dev.dv_xname, lcd_power, sc);
}

void
lcd_cnattach(void)
{

	pxa2x0_lcd_cnattach(&lcd_std_screen, CURRENT_DISPLAY);
}

/*
 * wsdisplay accessops overrides
 */
static int
lcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pxa2x0_lcd_softc *sc = (struct pxa2x0_lcd_softc *)v;
	int res = EINVAL;

	switch (cmd) {
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
		res = lcd_param(sc, cmd, (struct wsdisplay_param *)data);
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
lcd_param(struct pxa2x0_lcd_softc *sc, u_long cmd,
    struct wsdisplay_param *dp)
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

int
lcd_max_brightness(void)
{
	int i;

	for (i = 0; CURRENT_BACKLIGHT[i].duty != -1; i++)
		continue;
	return i - 1;
}

int
lcd_get_brightness(void)
{

	return lcdbrightnesscurval;
}

void
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

void
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
			scoop_set_backlight(CURRENT_BACKLIGHT[i].on,
			    CURRENT_BACKLIGHT[i].cont);
			delay(5000);
		}
	} else {
		(void)zssp_ic_send(ZSSP_IC_LZ9JG18,
		    CURRENT_BACKLIGHT[newval].duty);
		scoop_set_backlight(CURRENT_BACKLIGHT[newval].on,
		    CURRENT_BACKLIGHT[newval].cont);
	}

	curval = newval;
}

int
lcd_get_backlight(void)
{

	return lcdislit;
}

void
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

void
lcd_power(int why, void *v)
{

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		lcd_set_brightness(0);
		pxa2x0_lcd_power(why, v);
		break;

	case PWR_RESUME:
		pxa2x0_lcd_power(why, v);
		lcd_set_brightness(lcd_get_brightness());
		break;
	}
}
